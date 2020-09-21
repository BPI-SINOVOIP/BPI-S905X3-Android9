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

#include "VtsDriverFileUtil.h"

#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

namespace android {
namespace vts {

string ReadFile(const string& file_path) {
  ifstream ifs(file_path);
  string content((istreambuf_iterator<char>(ifs)),
                 (istreambuf_iterator<char>()));
  return content;
}

string GetDirFromFilePath(const string& str) {
  size_t found = str.find_last_of("/\\");
  if (found == string::npos) {
    return ".";
  }

  return str.substr(0, found);
}

}  // namespace vts
}  //
