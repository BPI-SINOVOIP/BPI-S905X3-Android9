/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_RUNTIME_COMMON_RUNTIME_TEST_H_
#define ART_RUNTIME_COMMON_RUNTIME_TEST_H_

#include <gtest/gtest.h>
#include <jni.h>

#include <string>

#include <android-base/logging.h>

#include "arch/instruction_set.h"
#include "base/mutex.h"
#include "base/os.h"
#include "base/unix_file/fd_file.h"
#include "dex/art_dex_file_loader.h"
#include "dex/compact_dex_level.h"
#include "globals.h"
// TODO: Add inl file and avoid including inl.
#include "obj_ptr-inl.h"
#include "scoped_thread_state_change-inl.h"

namespace art {

using LogSeverity = android::base::LogSeverity;
using ScopedLogSeverity = android::base::ScopedLogSeverity;

// OBJ pointer helpers to avoid needing .Decode everywhere.
#define EXPECT_OBJ_PTR_EQ(a, b) EXPECT_EQ(MakeObjPtr(a).Ptr(), MakeObjPtr(b).Ptr());
#define ASSERT_OBJ_PTR_EQ(a, b) ASSERT_EQ(MakeObjPtr(a).Ptr(), MakeObjPtr(b).Ptr());
#define EXPECT_OBJ_PTR_NE(a, b) EXPECT_NE(MakeObjPtr(a).Ptr(), MakeObjPtr(b).Ptr());
#define ASSERT_OBJ_PTR_NE(a, b) ASSERT_NE(MakeObjPtr(a).Ptr(), MakeObjPtr(b).Ptr());

class ClassLinker;
class CompilerCallbacks;
class DexFile;
class JavaVMExt;
class Runtime;
typedef std::vector<std::pair<std::string, const void*>> RuntimeOptions;
class Thread;
class VariableSizedHandleScope;

class ScratchFile {
 public:
  ScratchFile();

  explicit ScratchFile(const std::string& filename);

  ScratchFile(const ScratchFile& other, const char* suffix);

  ScratchFile(ScratchFile&& other);

  ScratchFile& operator=(ScratchFile&& other);

  explicit ScratchFile(File* file);

  ~ScratchFile();

  const std::string& GetFilename() const {
    return filename_;
  }

  File* GetFile() const {
    return file_.get();
  }

  int GetFd() const;

  void Close();
  void Unlink();

 private:
  std::string filename_;
  std::unique_ptr<File> file_;
};

class CommonRuntimeTestImpl {
 public:
  CommonRuntimeTestImpl();
  virtual ~CommonRuntimeTestImpl();
  static void SetUpAndroidRoot();

  // Note: setting up ANDROID_DATA may create a temporary directory. If this is used in a
  // non-derived class, be sure to also call the corresponding tear-down below.
  static void SetUpAndroidData(std::string& android_data);

  static void TearDownAndroidData(const std::string& android_data, bool fail_on_error);

  // Gets the paths of the libcore dex files.
  static std::vector<std::string> GetLibCoreDexFileNames();

  // Returns bin directory which contains host's prebuild tools.
  static std::string GetAndroidHostToolsDir();

  // Returns bin directory which contains target's prebuild tools.
  static std::string GetAndroidTargetToolsDir(InstructionSet isa);

  // Retuerns the filename for a test dex (i.e. XandY or ManyMethods).
  std::string GetTestDexFileName(const char* name) const;

  // A helper function to fill the heap.
  static void FillHeap(Thread* self,
                       ClassLinker* class_linker,
                       VariableSizedHandleScope* handle_scope)
      REQUIRES_SHARED(Locks::mutator_lock_);
  // A helper to set up a small heap (4M) to make FillHeap faster.
  static void SetUpRuntimeOptionsForFillHeap(RuntimeOptions *options);

  template <typename Mutator>
  bool MutateDexFile(File* output_dex, const std::string& input_jar, const Mutator& mutator) {
    std::vector<std::unique_ptr<const DexFile>> dex_files;
    std::string error_msg;
    const ArtDexFileLoader dex_file_loader;
    CHECK(dex_file_loader.Open(input_jar.c_str(),
                               input_jar.c_str(),
                               /*verify*/ true,
                               /*verify_checksum*/ true,
                               &error_msg,
                               &dex_files)) << error_msg;
    EXPECT_EQ(dex_files.size(), 1u) << "Only one input dex is supported";
    const std::unique_ptr<const DexFile>& dex = dex_files[0];
    CHECK(dex->EnableWrite()) << "Failed to enable write";
    DexFile* dex_file = const_cast<DexFile*>(dex.get());
    mutator(dex_file);
    const_cast<DexFile::Header&>(dex_file->GetHeader()).checksum_ = dex_file->CalculateChecksum();
    if (!output_dex->WriteFully(dex->Begin(), dex->Size())) {
      return false;
    }
    if (output_dex->Flush() != 0) {
      PLOG(FATAL) << "Could not flush the output file.";
    }
    return true;
  }

 protected:
  // Allow subclases such as CommonCompilerTest to add extra options.
  virtual void SetUpRuntimeOptions(RuntimeOptions* options ATTRIBUTE_UNUSED) {}

  // Called before the runtime is created.
  virtual void PreRuntimeCreate() {}

  // Called after the runtime is created.
  virtual void PostRuntimeCreate() {}

  static bool IsHost() {
    return !kIsTargetBuild;
  }

  // File location to core.art, e.g. $ANDROID_HOST_OUT/system/framework/core.art
  static std::string GetCoreArtLocation();

  // File location to core.oat, e.g. $ANDROID_HOST_OUT/system/framework/core.oat
  static std::string GetCoreOatLocation();

  std::unique_ptr<const DexFile> LoadExpectSingleDexFile(const char* location);

  void ClearDirectory(const char* dirpath, bool recursive = true);

  std::string GetTestAndroidRoot();

  std::vector<std::unique_ptr<const DexFile>> OpenTestDexFiles(const char* name);

  std::unique_ptr<const DexFile> OpenTestDexFile(const char* name);

  // Loads the test dex file identified by the given dex_name into a PathClassLoader.
  // Returns the created class loader.
  jobject LoadDex(const char* dex_name) REQUIRES_SHARED(Locks::mutator_lock_);
  // Loads the test dex file identified by the given first_dex_name and second_dex_name
  // into a PathClassLoader. Returns the created class loader.
  jobject LoadMultiDex(const char* first_dex_name, const char* second_dex_name)
      REQUIRES_SHARED(Locks::mutator_lock_);

  jobject LoadDexInPathClassLoader(const std::string& dex_name, jobject parent_loader);
  jobject LoadDexInDelegateLastClassLoader(const std::string& dex_name, jobject parent_loader);
  jobject LoadDexInWellKnownClassLoader(const std::string& dex_name,
                                        jclass loader_class,
                                        jobject parent_loader);

  std::string android_data_;
  std::string dalvik_cache_;

  std::unique_ptr<Runtime> runtime_;

  // The class_linker_, java_lang_dex_file_, and boot_class_path_ are all
  // owned by the runtime.
  ClassLinker* class_linker_;
  const DexFile* java_lang_dex_file_;
  std::vector<const DexFile*> boot_class_path_;

  // Get the dex files from a PathClassLoader or DelegateLastClassLoader.
  // This only looks into the current class loader and does not recurse into the parents.
  std::vector<const DexFile*> GetDexFiles(jobject jclass_loader);
  std::vector<const DexFile*> GetDexFiles(ScopedObjectAccess& soa,
                                          Handle<mirror::ClassLoader> class_loader)
    REQUIRES_SHARED(Locks::mutator_lock_);

  // Get the first dex file from a PathClassLoader. Will abort if it is null.
  const DexFile* GetFirstDexFile(jobject jclass_loader);

  std::unique_ptr<CompilerCallbacks> callbacks_;

  virtual void SetUp();

  virtual void TearDown();

  // Called to finish up runtime creation and filling test fields. By default runs root
  // initializers, initialize well-known classes, and creates the heap thread pool.
  virtual void FinalizeSetup();

  // Creates the class path string for the given dex files (the list of dex file locations
  // separated by ':').
  std::string CreateClassPath(
      const std::vector<std::unique_ptr<const DexFile>>& dex_files);
  // Same as CreateClassPath but add the dex file checksum after each location. The separator
  // is '*'.
  std::string CreateClassPathWithChecksums(
      const std::vector<std::unique_ptr<const DexFile>>& dex_files);

 private:
  static std::string GetCoreFileLocation(const char* suffix);

  std::vector<std::unique_ptr<const DexFile>> loaded_dex_files_;
};

template <typename TestType>
class CommonRuntimeTestBase : public TestType, public CommonRuntimeTestImpl {
 public:
  CommonRuntimeTestBase() {}
  virtual ~CommonRuntimeTestBase() {}

 protected:
  virtual void SetUp() OVERRIDE {
    CommonRuntimeTestImpl::SetUp();
  }

  virtual void TearDown() OVERRIDE {
    CommonRuntimeTestImpl::TearDown();
  }
};

using CommonRuntimeTest = CommonRuntimeTestBase<testing::Test>;

template <typename Param>
using CommonRuntimeTestWithParam = CommonRuntimeTestBase<testing::TestWithParam<Param>>;

// Sets a CheckJni abort hook to catch failures. Note that this will cause CheckJNI to carry on
// rather than aborting, so be careful!
class CheckJniAbortCatcher {
 public:
  CheckJniAbortCatcher();

  ~CheckJniAbortCatcher();

  void Check(const std::string& expected_text);
  void Check(const char* expected_text);

 private:
  static void Hook(void* data, const std::string& reason);

  JavaVMExt* const vm_;
  std::string actual_;

  DISALLOW_COPY_AND_ASSIGN(CheckJniAbortCatcher);
};

#define TEST_DISABLED_FOR_TARGET() \
  if (kIsTargetBuild) { \
    printf("WARNING: TEST DISABLED FOR TARGET\n"); \
    return; \
  }

#define TEST_DISABLED_FOR_MIPS() \
  if (kRuntimeISA == InstructionSet::kMips) { \
    printf("WARNING: TEST DISABLED FOR MIPS\n"); \
    return; \
  }

#define TEST_DISABLED_FOR_X86() \
  if (kRuntimeISA == InstructionSet::kX86) { \
    printf("WARNING: TEST DISABLED FOR X86\n"); \
    return; \
  }

#define TEST_DISABLED_FOR_STRING_COMPRESSION() \
  if (mirror::kUseStringCompression) { \
    printf("WARNING: TEST DISABLED FOR STRING COMPRESSION\n"); \
    return; \
  }

#define TEST_DISABLED_WITHOUT_BAKER_READ_BARRIERS() \
  if (!kEmitCompilerReadBarrier || !kUseBakerReadBarrier) { \
    printf("WARNING: TEST DISABLED FOR GC WITHOUT BAKER READ BARRIER\n"); \
    return; \
  }

#define TEST_DISABLED_FOR_NON_STATIC_HOST_BUILDS() \
  if (!kHostStaticBuildEnabled) { \
    printf("WARNING: TEST DISABLED FOR NON-STATIC HOST BUILDS\n"); \
    return; \
  }

#define TEST_DISABLED_FOR_MEMORY_TOOL() \
  if (RUNNING_ON_MEMORY_TOOL > 0) { \
    printf("WARNING: TEST DISABLED FOR MEMORY TOOL\n"); \
    return; \
  }

#define TEST_DISABLED_FOR_MEMORY_TOOL_VALGRIND() \
  if (RUNNING_ON_MEMORY_TOOL > 0 && kMemoryToolIsValgrind) { \
    printf("WARNING: TEST DISABLED FOR MEMORY TOOL VALGRIND\n"); \
    return; \
  }

#define TEST_DISABLED_FOR_MEMORY_TOOL_ASAN() \
  if (RUNNING_ON_MEMORY_TOOL > 0 && !kMemoryToolIsValgrind) { \
    printf("WARNING: TEST DISABLED FOR MEMORY TOOL ASAN\n"); \
    return; \
  }

#define TEST_DISABLED_FOR_COMPACT_DEX() \
  if (kDefaultCompactDexLevel != CompactDexLevel::kCompactDexLevelNone) { \
    printf("WARNING: TEST DISABLED FOR COMPACT DEX\n"); \
    return; \
  }

#define TEST_DISABLED_FOR_HEAP_POISONING() \
  if (kPoisonHeapReferences) { \
    printf("WARNING: TEST DISABLED FOR HEAP POISONING\n"); \
    return; \
  }
}  // namespace art

#endif  // ART_RUNTIME_COMMON_RUNTIME_TEST_H_
