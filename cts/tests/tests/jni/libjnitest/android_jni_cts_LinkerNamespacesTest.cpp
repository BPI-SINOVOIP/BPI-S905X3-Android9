/*
 * Copyright (C) 2016 The Android Open Source Project
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

/*
 * Tests accessibility of platform native libraries
 */

#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <jni.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

#include <android-base/strings.h>
#include <nativehelper/JNIHelp.h>
#include <nativehelper/ScopedLocalRef.h>
#include <nativehelper/ScopedUtfChars.h>

#if defined(__LP64__)
static const std::string kSystemLibraryPath = "/system/lib64";
static const std::string kVendorLibraryPath = "/vendor/lib64";
static const std::string kProductLibraryPath = "/product/lib64";
#else
static const std::string kSystemLibraryPath = "/system/lib";
static const std::string kVendorLibraryPath = "/vendor/lib";
static const std::string kProductLibraryPath = "/product/lib";
#endif

// This is not the complete list - just a small subset
// of the libraries that should reside in /system/lib
// for app-compatibility reasons.
// (in addition to kSystemPublicLibraries)
static std::vector<std::string> kSystemLibraries = {
    "libart.so",
    "libandroid_runtime.so",
    "libbinder.so",
    "libcrypto.so",
    "libcutils.so",
    "libexpat.so",
    "libgui.so",
    "libmedia.so",
    "libnativehelper.so",
    "libstagefright.so",
    "libsqlite.so",
    "libui.so",
    "libutils.so",
    "libvorbisidec.so",
  };

static bool is_directory(const char* path) {
  struct stat sb;
  if (stat(path, &sb) != -1) {
    return S_ISDIR(sb.st_mode);
  }

  return false;
}

static bool not_accessible(const std::string& err) {
  return err.find("dlopen failed: library \"") == 0 &&
         err.find("is not accessible for the namespace \"classloader-namespace\"") != std::string::npos;
}

static bool not_found(const std::string& err) {
  return err.find("dlopen failed: library \"") == 0 &&
         err.find("\" not found") != std::string::npos;
}

static bool wrong_arch(const std::string& library, const std::string& err) {
  // https://issuetracker.google.com/37428428
  // It's okay to not be able to load a library because it's for another
  // architecture (typically on an x86 device, when we come across an arm library).
  return err.find("dlopen failed: \"" + library + "\" has unexpected e_machine: ") == 0;
}

static bool is_library_on_path(const std::unordered_set<std::string>& library_search_paths,
                               const std::string& baselib,
                               const std::string& path) {
  std::string tail = '/' + baselib;
  if (!android::base::EndsWith(path, tail)) return false;
  return library_search_paths.count(path.substr(0, path.size() - tail.size())) > 0;
}

static bool check_lib(const std::string& path,
                      const std::unordered_set<std::string>& library_search_paths,
                      const std::unordered_set<std::string>& libraries,
                      std::vector<std::string>* errors) {
  std::unique_ptr<void, decltype(&dlclose)> handle(dlopen(path.c_str(), RTLD_NOW), dlclose);

  // The current restrictions on public libraries:
  //  - It must exist only in the top level directory of "library_search_paths".
  //  - No library with the same name can be found in a sub directory.
  //  - Each public library does not contain any directory components.

  // Check if this library should be considered a public library.
  std::string baselib = basename(path.c_str());
  if (libraries.find(baselib) != libraries.end() &&
      is_library_on_path(library_search_paths, baselib, path)) {
    if (handle.get() == nullptr) {
      errors->push_back("The library \"" + path +
                        "\" is a public library but it cannot be loaded: " + dlerror());
      return false;
    }
  } else if (handle.get() != nullptr) {
    errors->push_back("The library \"" + path + "\" is not a public library but it loaded.");
    return false;
  } else { // (handle == nullptr && !shouldBeAccessible(path))
    // Check the error message
    std::string err = dlerror();
    if (!not_accessible(err) && !not_found(err) && !wrong_arch(path, err)) {
      errors->push_back("unexpected dlerror: " + err);
      return false;
    }
  }
  return true;
}

static bool check_path(const std::string& library_path,
                       const std::unordered_set<std::string>& library_search_paths,
                       const std::unordered_set<std::string>& libraries,
                       std::vector<std::string>* errors) {
  bool success = true;
  std::queue<std::string> dirs;
  dirs.push(library_path);
  while (!dirs.empty()) {
    std::string dir = dirs.front();
    dirs.pop();

    std::unique_ptr<DIR, decltype(&closedir)> dirp(opendir(dir.c_str()), closedir);
    if (dirp == nullptr) {
      errors->push_back("Failed to open " + dir + ": " + strerror(errno));
      success = false;
      continue;
    }

    dirent* dp;
    while ((dp = readdir(dirp.get())) != nullptr) {
      // skip "." and ".."
      if (strcmp(".", dp->d_name) == 0 || strcmp("..", dp->d_name) == 0) {
        continue;
      }

      std::string path = dir + "/" + dp->d_name;
      if (is_directory(path.c_str())) {
        dirs.push(path);
      } else if (!check_lib(path, library_search_paths, libraries, errors)) {
        success = false;
      }
    }
  }

  return success;
}

static bool jobject_array_to_set(JNIEnv* env,
                                 jobjectArray java_libraries_array,
                                 std::unordered_set<std::string>* libraries,
                                 std::string* error_msg) {
  error_msg->clear();
  size_t size = env->GetArrayLength(java_libraries_array);
  bool success = true;
  for (size_t i = 0; i<size; ++i) {
    ScopedLocalRef<jstring> java_soname(
        env, (jstring) env->GetObjectArrayElement(java_libraries_array, i));
    std::string soname(ScopedUtfChars(env, java_soname.get()).c_str());

    // Verify that the name doesn't contain any directory components.
    if (soname.rfind('/') != std::string::npos) {
      *error_msg += "\n---Illegal value, no directories allowed: " + soname;
      continue;
    }

    // Check to see if the string ends in " 32" or " 64" to indicate the
    // library is only public for one bitness.
    size_t space_pos = soname.rfind(' ');
    if (space_pos != std::string::npos) {
      std::string type = soname.substr(space_pos + 1);
      if (type != "32" && type != "64") {
        *error_msg += "\n---Illegal value at end of line (only 32 or 64 allowed): " + soname;
        success = false;
        continue;
      }
#if defined(__LP64__)
      if (type == "32") {
        // Skip this, it's a 32 bit only public library.
        continue;
      }
#else
      if (type == "64") {
        // Skip this, it's a 64 bit only public library.
        continue;
      }
#endif
      soname.resize(space_pos);
    }

    libraries->insert(soname);
  }

  return success;
}

// This is not public function but only known way to get search path of the default namespace.
extern "C" void android_get_LD_LIBRARY_PATH(char*, size_t) __attribute__((__weak__));

extern "C" JNIEXPORT jstring JNICALL
    Java_android_jni_cts_LinkerNamespacesHelper_runAccessibilityTestImpl(
        JNIEnv* env,
        jclass clazz __attribute__((unused)),
        jobjectArray java_system_public_libraries,
        jobjectArray java_vendor_public_libraries,
        jobjectArray java_product_public_libraries) {
  bool success = true;
  std::vector<std::string> errors;
  std::string error_msg;
  std::unordered_set<std::string> vendor_public_libraries;
  if (!jobject_array_to_set(env, java_vendor_public_libraries, &vendor_public_libraries,
                            &error_msg)) {
    success = false;
    errors.push_back("Errors in vendor public library file:" + error_msg);
  }

  std::unordered_set<std::string> system_public_libraries;
  if (!jobject_array_to_set(env, java_system_public_libraries, &system_public_libraries,
                            &error_msg)) {
    success = false;
    errors.push_back("Errors in system public library file:" + error_msg);
  }

  std::unordered_set<std::string> product_public_libraries;
  if (!jobject_array_to_set(env, java_product_public_libraries, &product_public_libraries,
                            &error_msg)) {
    success = false;
    errors.push_back("Errors in product public library file:" + error_msg);
  }

  // Check the system libraries.

  // Check current search path and add the rest of search path configured for
  // the default namepsace.
  char default_search_paths[PATH_MAX];
  android_get_LD_LIBRARY_PATH(default_search_paths, sizeof(default_search_paths));

  std::vector<std::string> library_search_paths = android::base::Split(default_search_paths, ":");

  // Remove everything pointing outside of /system/lib*
  std::unordered_set<std::string> system_library_search_paths;

  for (const auto& path : library_search_paths) {
    if (android::base::StartsWith(path, "/system/lib")) {
      system_library_search_paths.insert(path);
    }
  }

  // This path should be tested too - this is because apps may rely on some
  // libraries being available in /system/${LIB}/
  system_library_search_paths.insert(kSystemLibraryPath);

  if (!check_path(kSystemLibraryPath, system_library_search_paths, system_public_libraries, &errors)) {
    success = false;
  }

  // Check that the mandatory system libraries are present - the grey list
  for (const auto& name : kSystemLibraries) {
    std::string library = kSystemLibraryPath + "/" + name;
    void* handle = dlopen(library.c_str(), RTLD_NOW);
    if (handle == nullptr) {
      std::string err = dlerror();
      // The libraries should be present and produce specific dlerror when inaccessible.
      if (!not_accessible(err)) {
          errors.push_back("Mandatory system library \"" + library + "\" failed to load with unexpected error: " + err);
          success = false;
      }
    } else {
      dlclose(handle);
    }
  }

  // Check the product libraries, if /product/lib exists.
  if (is_directory(kProductLibraryPath.c_str())) {
    if (!check_path(kProductLibraryPath, { kProductLibraryPath }, product_public_libraries, &errors)) {
      success = false;
    }
  }

  // Check the vendor libraries.
  if (!check_path(kVendorLibraryPath, { kVendorLibraryPath }, vendor_public_libraries, &errors)) {
    success = false;
  }

  if (!success) {
    std::string error_str;
    for (const auto& line : errors) {
      error_str += line + '\n';
    }
    return env->NewStringUTF(error_str.c_str());
  }

  return nullptr;
}

