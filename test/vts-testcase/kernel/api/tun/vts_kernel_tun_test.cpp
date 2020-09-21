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
 */

#include <iostream>
#include <string>

#include <fcntl.h>

#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <gtest/gtest.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

using std::cout;
using std::endl;
using std::string;
using android::base::StringPrintf;
using android::base::unique_fd;

static const short kTunModes[] = {
  IFF_TUN,
  IFF_TAP,
};

class VtsKernelTunTest : public ::testing::TestWithParam<short> {
 public:
  virtual void SetUp() override {
    tun_device_ = "/dev/tun";
    uint32_t num = arc4random_uniform(1000);
    ifr_name_ = StringPrintf("tun%d", num);
  }

  // Used to initialize tun device.
  int TunInit(short flags);

  string tun_device_;
  string ifr_name_;
  unique_fd fd_;
};

int VtsKernelTunTest::TunInit(short mode) {
  struct ifreq ifr = {
    .ifr_flags = mode,
  };
  strncpy(ifr.ifr_name, ifr_name_.c_str(), IFNAMSIZ);
  int fd = open(tun_device_.c_str(), O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    return -1;
  }
  if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
    close(fd);
    return -1;
  }
  return fd;
}

// Test opening and closing of a tun interface.
TEST_P(VtsKernelTunTest, OpenAndClose) {
  fd_ = unique_fd(TunInit(GetParam()));
  ASSERT_TRUE(fd_ >= 0);
}

// Test basic read functionality of a tuen interface.
TEST_P(VtsKernelTunTest, BasicRead) {
  fd_ = unique_fd(TunInit(GetParam()));
  ASSERT_TRUE(fd_ >= 0);

  uint8_t test_output;
  // Expect no packets available on this interface.
  ASSERT_TRUE(read(fd_, &test_output, 1) < 0);
}

INSTANTIATE_TEST_CASE_P(Basic, VtsKernelTunTest,
                        ::testing::ValuesIn(kTunModes));

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
