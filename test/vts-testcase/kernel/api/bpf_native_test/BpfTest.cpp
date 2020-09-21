/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless requied by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#define LOG_TAG "BpfTest"
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/utsname.h>

#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <gtest/gtest.h>
#include <utils/Log.h>

#include "bpf/BpfUtils.h"

using namespace android::bpf;

namespace android {

TEST(BpfTest, bpfMapPinTest) {
  const char* bpfMapPath = "/sys/fs/bpf/testMap";
  if (!hasBpfSupport()) {
    GTEST_LOG_(INFO) << "skipped since kernel version is not compatible.\n";
    return;
  }
  int ret = access(bpfMapPath, F_OK);
  if (!ret) {
    ASSERT_EQ(0, remove(bpfMapPath));
  } else {
    ASSERT_EQ(errno, ENOENT);
  }

  android::base::unique_fd mapfd(createMap(BPF_MAP_TYPE_HASH, sizeof(uint32_t),
                                           sizeof(uint32_t), 10,
                                           BPF_F_NO_PREALLOC));
  ASSERT_LT(0, mapfd) << "create map failed with error: " << strerror(errno);
  ASSERT_EQ(0, mapPin(mapfd, bpfMapPath))
      << "pin map failed with error: " << strerror(errno);
  ASSERT_EQ(0, access(bpfMapPath, F_OK));
  ASSERT_EQ(0, remove(bpfMapPath));
}

}  // namespace android
