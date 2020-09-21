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

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

#include <android-base/logging.h>
#include "VtsCompilerUtils.h"
#include "code_gen/CodeGenBase.h"

using namespace std;

// To generate both header and source files,
//   Usage: vtsc -mDRIVER | -mPROFILER <.vts input file path> \
//          <header output dir> <C/C++ source output file path>
// To generate only a header file,
//   Usage: vtsc -mDRIVER | -mPROFILER -tHEADER -b<base path> \
//          <.vts input file or dir path> <header output file or dir path>
// To generate only a source file,
//   Usage: vtsc -mDRIVER | -mPROFILER -tSOURCE -b<base path> \
//          <.vts input file or dir path> \
//          <C/C++ source output file or dir path>
// where <base path> is a base path of where .vts input file or dir is
// stored but should be excluded when computing the package path of generated
// source or header output file(s).

int main(int argc, char* argv[]) {
#ifdef VTS_DEBUG
  cout << "Android VTS Compiler (AVTSC)" << endl;
#endif
  int opt_count = 0;
  android::vts::VtsCompileMode mode = android::vts::kDriver;
  android::vts::VtsCompileFileType type = android::vts::VtsCompileFileType::kBoth;
  string vts_base_dir;
  for (int i = 0; i < argc; i++) {
#ifdef VTS_DEBUG
    cout << "- args[" << i << "] " << argv[i] << endl;
#endif
    if (argv[i] && strlen(argv[i]) > 1 && argv[i][0] == '-') {
      opt_count++;
      if (argv[i][1] == 'm') {
        if (!strcmp(&argv[i][2], "PROFILER")) {
          mode = android::vts::kProfiler;
#ifdef VTS_DEBUG
          cout << "- mode: PROFILER" << endl;
#endif
        } else if (!strcmp(&argv[i][2], "FUZZER")) {
          mode = android::vts::kFuzzer;
#ifdef VTS_DEBUG
          cout << "- mode: FUZZER" << endl;
#endif
        }
      }
      if (argv[i][1] == 't') {
        if (!strcmp(&argv[i][2], "HEADER")) {
          type = android::vts::kHeader;
#ifdef VTS_DEBUG
          cout << "- type: HEADER" << endl;
#endif
        } else if (!strcmp(&argv[i][2], "SOURCE")) {
          type = android::vts::kSource;
#ifdef VTS_DEBUG
          cout << "- type: SOURCE" << endl;
#endif
        }
      }
      if (argv[i][1] == 'b') {
        vts_base_dir = &argv[i][2];
#ifdef VTS_DEBUG
        cout << "- VTS base dir: " << vts_base_dir << endl;
#endif
      }
    }
  }
  if (argc < 5) {
    cerr << "argc " << argc << " < 5" << endl;
    return -1;
  }
  switch (type) {
    case android::vts::kBoth:
      android::vts::Translate(
          mode, argv[opt_count + 1], argv[opt_count + 2], argv[opt_count + 3]);
      break;
    case android::vts::kHeader:
    case android::vts::kSource: {
      struct stat s;
      bool is_dir = false;
      if (vts_base_dir.length() > 0) {
        if (chdir(vts_base_dir.c_str())) {
          cerr << __func__ << " can't chdir to " << vts_base_dir << endl;
          exit(-1);
        }
      }
      if (stat(argv[opt_count + 1], &s) == 0) {
        if (s.st_mode & S_IFDIR) {
          is_dir = true;
        }
      }
      if (!is_dir) {
        android::vts::TranslateToFile(
            mode, argv[opt_count + 1], argv[opt_count + 2], type);
      } else {
        DIR* input_dir;
        struct dirent* ent;
        if ((input_dir = opendir(argv[opt_count + 1])) != NULL) {
          // argv[opt_count + 2] should be a directory. if that dir does not exist,
          // that dir is created as part of the translation operation.
          while ((ent = readdir(input_dir)) != NULL) {
            if (!strncmp(&ent->d_name[strlen(ent->d_name)-4], ".vts", 4)) {
              string src_file = android::vts::RemoveBaseDir(
                  android::vts::PathJoin(
                      argv[opt_count + 1], ent->d_name), vts_base_dir);
              string dst_file = android::vts::RemoveBaseDir(
                  android::vts::PathJoin(
                      argv[opt_count + 2], ent->d_name), vts_base_dir);
              if (type == android::vts::kHeader) {
                dst_file = android::vts::PathJoin(dst_file.c_str(), ".h");
              } else {
                dst_file = android::vts::PathJoin(dst_file.c_str(), ".cpp");
              }
#ifdef VTS_DEBUG
              cout << ent->d_name << endl;
              cout << "<- " << src_file.c_str() << endl;
              cout << "-> " << dst_file.c_str() << endl;
#endif
              android::vts::TranslateToFile(
                  mode, src_file.c_str(), dst_file.c_str(), type);
            }
          }
          closedir(input_dir);
        } else {
          cerr << __func__ << " can't open the given input dir, "
               << argv[opt_count + 1] << "." << endl;
          exit(-1);
        }
      }
      break;
    }
  }
  return 0;
}
