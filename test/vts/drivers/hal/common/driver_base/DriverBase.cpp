/*
 * Copyright 2016 The Android Open Source Project
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
#define LOG_TAG "VtsHalDriverBase"

#include "driver_base/DriverBase.h"

#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <vector>

#include <android-base/logging.h>

#include "GcdaParser.h"
#include "component_loader/DllLoader.h"
#include "test/vts/proto/ComponentSpecificationMessage.pb.h"
#include "utils/InterfaceSpecUtil.h"

using namespace std;
using namespace android;

#define USE_GCOV 1

namespace android {
namespace vts {

const string default_gcov_output_basepath = "/data/misc/gcov";

static void RemoveDir(char* path) {
  struct dirent* entry = NULL;
  DIR* dir = opendir(path);

  while ((entry = readdir(dir)) != NULL) {
    DIR* sub_dir = NULL;
    FILE* file = NULL;
    char abs_path[4096] = {0};

    if (*(entry->d_name) != '.') {
      sprintf(abs_path, "%s/%s", path, entry->d_name);
      if ((sub_dir = opendir(abs_path)) != NULL) {
        closedir(sub_dir);
        RemoveDir(abs_path);
      } else if ((file = fopen(abs_path, "r")) != NULL) {
        fclose(file);
        remove(abs_path);
      }
    }
  }
  remove(path);
}

DriverBase::DriverBase(int target_class)
    : device_(NULL),
      hmi_(NULL),
      target_dll_path_(NULL),
      target_class_(target_class),
      component_filename_(NULL),
      gcov_output_basepath_(NULL) {}

DriverBase::~DriverBase() { free(component_filename_); }

void wfn() { LOG(DEBUG) << "debug"; }

void ffn() { LOG(DEBUG) << "debug"; }

bool DriverBase::LoadTargetComponent(const char* target_dll_path) {
  if (target_dll_path && target_dll_path_ &&
      !strcmp(target_dll_path, target_dll_path_)) {
    LOG(DEBUG) << "Skip loading " << target_dll_path;
    return true;
  }

  if (!target_loader_.Load(target_dll_path)) return false;
  target_dll_path_ = (char*)malloc(strlen(target_dll_path) + 1);
  strcpy(target_dll_path_, target_dll_path);
  LOG(DEBUG) << "Loaded the target";
  if (target_class_ == HAL_LEGACY) return true;
  LOG(DEBUG) << "Loaded a non-legacy HAL file.";

  if (target_dll_path_) {
    LOG(DEBUG) << "Target DLL path " << target_dll_path_;
    string target_path(target_dll_path_);

    size_t offset = target_path.rfind("/", target_path.length());
    if (offset != string::npos) {
      string filename =
          target_path.substr(offset + 1, target_path.length() - offset);
      filename = filename.substr(0, filename.length() - 3 /* for .so */);
      component_filename_ = (char*)malloc(filename.length() + 1);
      strcpy(component_filename_, filename.c_str());
      LOG(DEBUG) << "Module file name: " << component_filename_;
    }
    LOG(DEBUG) << "Target_dll_path " << target_dll_path_;
  }

#if USE_GCOV
  LOG(DEBUG) << "gcov init " << target_loader_.GcovInit(wfn, ffn);
#endif
  return true;
}

bool DriverBase::Fuzz(vts::ComponentSpecificationMessage* message,
                      void** result) {
  LOG(DEBUG) << "Fuzzing target component: "
             << "class " << message->component_class() << " type "
             << message->component_type() << " version "
             << message->component_type_version();

  string function_name_prefix = GetFunctionNamePrefix(*message);
  function_name_prefix_ = function_name_prefix.c_str();
  for (vts::FunctionSpecificationMessage func_msg :
       *message->mutable_interface()->mutable_api()) {
    Fuzz(&func_msg, result, "");
  }
  return true;
}

void DriverBase::FunctionCallBegin() {
  char product_path[4096];
  char product[128];
  char module_basepath[4096];

  snprintf(product_path, 4096, "%s/%s", default_gcov_output_basepath.c_str(),
           "proc/self/cwd/out/target/product");
  DIR* srcdir = opendir(product_path);
  if (!srcdir) {
    LOG(WARNING) << "Couldn't open " << product_path;
    return;
  }

  int dir_count = 0;
  struct dirent* dent;
  while ((dent = readdir(srcdir)) != NULL) {
    struct stat st;
    if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0) {
      continue;
    }
    if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0) {
      LOG(ERROR) << "Error " << dent->d_name;
      continue;
    }
    if (S_ISDIR(st.st_mode)) {
      LOG(DEBUG) << "dir " << dent->d_name;
      strcpy(product, dent->d_name);
      dir_count++;
    }
  }
  closedir(srcdir);
  if (dir_count != 1) {
    LOG(ERROR) << "More than one product dir found.";
    return;
  }

  int n = snprintf(module_basepath, 4096, "%s/%s/obj/SHARED_LIBRARIES",
                   product_path, product);
  if (n <= 0 || n >= 4096) {
    LOG(ERROR) << "Couln't get module_basepath";
    return;
  }
  srcdir = opendir(module_basepath);
  if (!srcdir) {
    LOG(ERROR) << "Couln't open " << module_basepath;
    return;
  }

  if (component_filename_ != NULL) {
    dir_count = 0;
    string target = string(component_filename_) + "_intermediates";
    bool hit = false;
    while ((dent = readdir(srcdir)) != NULL) {
      struct stat st;
      if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0) {
        continue;
      }
      if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0) {
        LOG(ERROR) << "Error " << dent->d_name;
        continue;
      }
      if (S_ISDIR(st.st_mode)) {
        LOG(DEBUG) << "module_basepath " << string(dent->d_name);
        if (string(dent->d_name) == target) {
          LOG(DEBUG) << "hit";
          hit = true;
        }
        dir_count++;
      }
    }
    if (hit) {
      free(gcov_output_basepath_);
      gcov_output_basepath_ =
          (char*)malloc(strlen(module_basepath) + target.length() + 2);
      if (!gcov_output_basepath_) {
        LOG(ERROR) << "Couldn't alloc memory";
        return;
      }
      sprintf(gcov_output_basepath_, "%s/%s", module_basepath, target.c_str());
      RemoveDir(gcov_output_basepath_);
    }
  } else {
    LOG(ERROR) << "component_filename_ is NULL";
  }
  // TODO: check how it works when there already is a file.
  closedir(srcdir);
}

bool DriverBase::ReadGcdaFile(const string& basepath, const string& filename,
                              FunctionSpecificationMessage* msg) {
#if VTS_GCOV_DEBUG
  LOG(DEBUG) << "file = " << dent->d_name;
#endif
  if (string(filename).rfind(".gcda") != string::npos) {
    string buffer = basepath + "/" + filename;
    vector<unsigned> processed_data =
        android::vts::GcdaRawCoverageParser(buffer.c_str()).Parse();
    for (const auto& data : processed_data) {
      msg->mutable_processed_coverage_data()->Add(data);
    }

    FILE* gcda_file = fopen(buffer.c_str(), "rb");
    if (!gcda_file) {
      LOG(ERROR) << "Unable to open a gcda file. " << buffer;
    } else {
      LOG(DEBUG) << "Opened a gcda file. " << buffer;
      fseek(gcda_file, 0, SEEK_END);
      long gcda_file_size = ftell(gcda_file);
#if VTS_GCOV_DEBUG
      LOG(DEBUG) << "File size " << gcda_file_size << " bytes";
#endif
      fseek(gcda_file, 0, SEEK_SET);

      char* gcda_file_buffer = (char*)malloc(gcda_file_size + 1);
      if (!gcda_file_buffer) {
        LOG(ERROR) << "Unable to allocate memory to read a gcda file.";
      } else {
        if (fread(gcda_file_buffer, gcda_file_size, 1, gcda_file) != 1) {
          LOG(ERROR) << "File read error";
        } else {
#if VTS_GCOV_DEBUG
          LOG(DEBUG) << "GCDA field populated.";
#endif
          gcda_file_buffer[gcda_file_size] = '\0';
          NativeCodeCoverageRawDataMessage* raw_msg =
              msg->mutable_raw_coverage_data()->Add();
          raw_msg->set_file_path(filename.c_str());
          string gcda_file_buffer_string(gcda_file_buffer, gcda_file_size);
          raw_msg->set_gcda(gcda_file_buffer_string);
          free(gcda_file_buffer);
        }
      }
      fclose(gcda_file);
    }
#if USE_GCOV_DEBUG
    if (result) {
      for (unsigned int index = 0; index < result->size(); index++) {
        LOG(DEBUG) << result->at(index);
      }
    }
#endif
    return true;
  }
  return false;
}

bool DriverBase::ScanAllGcdaFiles(const string& basepath,
                                  FunctionSpecificationMessage* msg) {
  DIR* srcdir = opendir(basepath.c_str());
  if (!srcdir) {
    LOG(ERROR) << "Couln't open " << basepath;
    return false;
  }

  struct dirent* dent;
  while ((dent = readdir(srcdir)) != NULL) {
#if VTS_GCOV_DEBUG
    LOG(DEBUG) << "readdir(" << basepath << ") for " << dent->d_name;
#endif
    struct stat st;
    if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0) {
      continue;
    }
    if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0) {
      LOG(ERROR) << "error " << dent->d_name;
      continue;
    }
    if (S_ISDIR(st.st_mode)) {
      ScanAllGcdaFiles(basepath + "/" + dent->d_name, msg);
    } else {
      ReadGcdaFile(basepath, dent->d_name, msg);
    }
  }
#if VTS_GCOV_DEBUG
  LOG(DEBUG) << "closedir(" << srcdir << ")";
#endif
  closedir(srcdir);
  return true;
}

bool DriverBase::FunctionCallEnd(FunctionSpecificationMessage* msg) {
#if USE_GCOV
  target_loader_.GcovFlush();
  // find the file.
  if (!gcov_output_basepath_) {
    LOG(WARNING) << "No gcov basepath set";
    return ScanAllGcdaFiles(default_gcov_output_basepath, msg);
  }
  DIR* srcdir = opendir(gcov_output_basepath_);
  if (!srcdir) {
    LOG(WARNING) << "Couln't open " << gcov_output_basepath_;
    return false;
  }

  struct dirent* dent;
  while ((dent = readdir(srcdir)) != NULL) {
    LOG(DEBUG) << "readdir(" << srcdir << ") for " << dent->d_name;

    struct stat st;
    if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0) {
      continue;
    }
    if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0) {
      LOG(ERROR) << "error " << dent->d_name;
      continue;
    }
    if (!S_ISDIR(st.st_mode) &&
        ReadGcdaFile(gcov_output_basepath_, dent->d_name, msg)) {
      break;
    }
  }
  LOG(DEBUG) << "closedir(" << srcdir << ")";
  closedir(srcdir);
#endif
  return true;
}

}  // namespace vts
}  // namespace android
