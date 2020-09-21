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

#ifndef __VTS_DRIVER_FILE_UTIL_H_
#define __VTS_DRIVER_FILE_UTIL_H_

#include <string>

using namespace std;

namespace android {
namespace vts {

/*
 * Read file content into a string
 */
extern string ReadFile(const string& file_path);

/*
 * Get the directory path out from file path.
 * This method does not check for error at this time.
 */
extern string GetDirFromFilePath(const string& file_path_absolute);

}  // namespace vts
}  // namespace android

#endif  // __VTS_DRIVER_FILE_UTIL_H_
