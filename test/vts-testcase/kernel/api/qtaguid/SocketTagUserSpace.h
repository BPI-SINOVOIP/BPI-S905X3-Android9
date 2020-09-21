/*
 * Copyright (C) 2017 The Android Open Source Project
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
 *
 */

#ifndef _VTS_SYSTEM_QTAGUID_SOCKET_TAG_USER_SPACE_
#define _VTS_SYSTEM_QTAGUID_SOCKET_TAG_USER_SPACE_

#include <stdint.h>
#include <stdlib.h>

namespace android {

// Class created to store socket file description and setup socket
class SockInfo {
 public:
  SockInfo() : fd(-1) {}
  // set up the socket and verify it by trying to tag it
  int setup(int tag);
  // helper function check if the socket is tagged correctly
  bool checkTag(uint64_t acct_tag, uid_t uid);
  // helper function check if the socket traffic is properly recorded.
  bool checkStats(uint64_t acct_tag, uid_t uid, int counterSet,
                  uint32_t *stats_result);
  // socket file description
  int fd;
};

}  // namespace android

#endif  // _VTS_SYSTEM_QTAGUID_SOCKET_TAG_USER_SPACE_
