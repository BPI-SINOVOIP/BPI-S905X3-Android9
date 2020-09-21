/*
 *
 * Copyright 2018, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SYSTEM_EXTRAS_PERFPROFD_DROPBOX_DROPBOX_H_
#define SYSTEM_EXTRAS_PERFPROFD_DROPBOX_DROPBOX_H_

#include <string>

#include "perfprofd_record-fwd.h"

namespace android {
namespace perfprofd {
namespace dropbox {

bool SendToDropbox(android::perfprofd::PerfprofdRecord* profile,
                   const std::string& temp_directory,
                   std::string* error_msg);

}  // namespace dropbox
}  // namespace perfprofd
}  // namespace android

#endif  // SYSTEM_EXTRAS_PERFPROFD_DROPBOX_DROPBOX_H_
