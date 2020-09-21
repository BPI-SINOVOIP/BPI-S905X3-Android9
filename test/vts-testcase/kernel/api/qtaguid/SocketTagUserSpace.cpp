/*
 * Copyright (C) 2017 The Android Open Source Project
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

/*
 * This socket tagging test is to ensure that the
 * netfilter/xt_qtaguid kernel module somewhat behaves as expected
 * with respect to tagging sockets.
 */
#define LOG_TAG "SocketTagUsrSpaceTest"
#include "SocketTagUserSpace.h"
#include <arpa/inet.h>
#include <assert.h>
#include <cutils/qtaguid.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <string>

#include <fstream>

#include <android-base/stringprintf.h>
#include <gtest/gtest.h>
#include <qtaguid/qtaguid.h>
#include <utils/Log.h>

static const int kMaxCounterSet = 2;

namespace android {

#define SKIP_IF_QTAGUID_NOT_SUPPORTED()                                        \
  do {                                                                         \
    bool hasQtaguidSupport;                                                    \
    EXPECT_EQ(checkKernelSupport(&hasQtaguidSupport), 0)                       \
        << "kernel support check failed";                                      \
    if (!hasQtaguidSupport) {                                                  \
      GTEST_LOG_(INFO) << "skipped since kernel version is not compatible.\n"; \
      return;                                                                  \
    }                                                                          \
  } while (0);
/* A helper program to generate some traffic between two socket. */
int server_download(SockInfo sock_server, SockInfo sock_client) {
  struct sockaddr_in server, client;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_family = AF_INET;
  server.sin_port = htons(8765);
  if (bind(sock_server.fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
    std::cerr << "bind failed" << std::endl;
    return -1;
  }
  listen(sock_server.fd, 3);
  int sock_addr_length;
  sock_addr_length = sizeof(struct sockaddr_in);
  if (connect(sock_client.fd, (struct sockaddr *)&server, sizeof(server)), 0) {
    return -1;
  }
  int new_socket;
  new_socket = accept(sock_server.fd, (struct sockaddr *)&client,
                      reinterpret_cast<socklen_t *>(&sock_addr_length));
  if (new_socket < 0) {
    return -1;
  }
  int packet_Count = 1024;
  char byte_buffer[1024];
  snprintf(byte_buffer, sizeof(byte_buffer), "%d", packet_Count);
  send(sock_client.fd, "start", 5, 0);
  if (recv(new_socket, byte_buffer, strlen(byte_buffer), 0) < 0) {
    close(new_socket);
    return -1;
  }
  memset(byte_buffer, 'x', 1023);
  byte_buffer[1023] = '\0';
  if (send(new_socket, byte_buffer, strlen(byte_buffer), 0) < 0) {
    close(new_socket);
    return -1;
  }
  EXPECT_GE(recv(sock_client.fd, byte_buffer, 1024, 0), 0);
  close(new_socket);
  return 0;
}

int checkKernelSupport(bool *qtaguidSupport) {
  int ret;
  struct utsname buf;
  int kernel_version_major;
  int kernel_version_minor;

  ret = uname(&buf);
  if (ret) {
    ret = -errno;
    std::cout << "Get system information failed: %s\n"
              << strerror(errno) << std::endl;
    return ret;
  }
  char dummy;
  ret = sscanf(buf.release, "%d.%d%c", &kernel_version_major,
               &kernel_version_minor, &dummy);
  // For device running kernel 4.9 or above and running Android P, it should use
  // the eBPF cgroup filter to monitoring networking stats instead. So it may
  // not have xt_qtaguid module on device. But for devices that still have
  // xt_qtaguid, this test is still useful to make sure it behaves properly.
  // b/30950746
  if (ret >= 2 && ((kernel_version_major == 4 && kernel_version_minor >= 9) ||
                   (kernel_version_major > 4))) {
    *qtaguidSupport = (access("/dev/xt_qtaguid", F_OK) != -1);
  } else {
    *qtaguidSupport = true;
  }
  return 0;
}

/* socket setup, initial the socket and try to validate the socket. */
int SockInfo::setup(int tag) {
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    std::cout << "socket creation failed: %s" << strerror(errno) << std::endl;
    return -1;
  }
  if (legacy_tagSocket(fd, tag, getuid()) < 0) {
    std::cout << "socket setup: failed to tag" << std::endl;
    close(fd);
    return -1;
  }

  if (legacy_untagSocket(fd) < 0) {
    std::cout << "socket setup: Unexpected results" << std::endl;
    close(fd);
    return -1;
  }
  return 0;
}

/* Check if the socket is properly tagged by read through the proc file.*/
bool SockInfo::checkTag(uint64_t acct_tag, uid_t uid) {

  std::ifstream fctrl("/proc/net/xt_qtaguid/ctrl", std::fstream::in);
  if (!fctrl.is_open()) {
    std::cout << "qtaguid ctrl open failed!" << std::endl;
  }

  uint64_t full_tag = (acct_tag << 32) | uid;
  std::string buff =
      android::base::StringPrintf(" tag=0x%" PRIx64 " (uid=%u)", full_tag, uid);

  std::string ctrl_data;
  std::size_t pos = std::string::npos;
  while (std::getline(fctrl, ctrl_data)) {
    pos = ctrl_data.find(buff);
    if (pos != std::string::npos) break;
  }
  return pos != std::string::npos;
}

/*
 * Check if the socket traffic statistics is properly recorded by reading the
 * corresponding proc file.
 */
bool SockInfo::checkStats(uint64_t acct_tag, uid_t uid, int counterSet,
                          uint32_t *stats_result) {
  uint64_t kTag = (uint64_t)acct_tag << 32;

  std::ifstream fstats("/proc/net/xt_qtaguid/stats", std::fstream::in);
  if (!fstats.is_open()) {
    std::cout << "qtaguid ctrl open failed!" << std::endl;
  }
  std::string buff =
      android::base::StringPrintf("0x%" PRIx64 " %u %d", kTag, uid, counterSet);
  std::string stats_data;
  std::size_t pos = std::string::npos;
  while (std::getline(fstats, stats_data)) {
    pos = stats_data.find(buff);
    if (pos != std::string::npos) {
      std::string match_data = stats_data.substr(pos);
      sscanf(match_data.c_str(), "0x%" PRIx64 " %u %d %d %d", &kTag, &uid,
             &counterSet, stats_result, stats_result + 1);
      return pos != std::string::npos;
    }
  }
  return pos != std::string::npos;
}

class SocketTagUsrSpaceTest : public ::testing::Test {
 protected:
  uint32_t stats_result_[2];
  SockInfo sock_0;
  SockInfo sock_1;
  uid_t fake_uid;
  uid_t fake_uid2;
  uid_t inet_uid;
  uid_t my_uid;
  pid_t my_pid;
  int valid_tag1;
  int valid_tag2;
  uint64_t max_uint_tag;

  virtual void SetUp() {
    my_uid = getuid();
    my_pid = getpid();
    srand48(my_pid * my_uid);
    // Adjust fake UIDs and tags so that multiple instances can run
    // in parallel.
    fake_uid = rand() & 0x7FFFFFFF;
    fake_uid2 = rand() & 0x7FFFFFFF;
    inet_uid = 1024;
    valid_tag1 = (my_pid << 12) | (rand());
    valid_tag2 = (my_pid << 12) | (rand());
    max_uint_tag = 0xffffffff00000000llu;
    max_uint_tag = 1llu << 63 | (((uint64_t)my_pid << 48) ^ max_uint_tag);
    // Check the node /dev/xt_qtaguid exist before start.
    struct stat nodeStat;
    EXPECT_GE(stat("/dev/xt_qtaguid", &nodeStat), 0)
        << "fail to check /dev/xt_qtaguid";
    // We want to clean up any previous faulty test runs.
    EXPECT_GE(legacy_deleteTagData(0, fake_uid), 0)
        << "Failed to delete fake_uid";
    EXPECT_GE(legacy_deleteTagData(0, fake_uid2), 0)
        << "Failed to delete fake_uid2";
    EXPECT_GE(legacy_deleteTagData(0, my_uid), 0) << "Failed to delete my_uid";
    EXPECT_GE(legacy_deleteTagData(0, inet_uid), 0)
        << "Failed to delete inet_uid";
    ASSERT_FALSE(sock_0.setup(valid_tag1)) << "socket0 setup failed";
    ASSERT_FALSE(sock_1.setup(valid_tag1)) << "socket1 setup failed";
  }

  virtual void TearDown() {
    if (sock_0.fd >= 0) {
      close(sock_0.fd);
    }
    if (sock_1.fd >= 0) {
      close(sock_1.fd);
    }
  }
};

/* Tag to a invalid socket fd, should fail */
TEST_F(SocketTagUsrSpaceTest, invalidSockfdFail) {
  EXPECT_LT(legacy_tagSocket(-1, valid_tag1, my_uid), 0)
      << "Invalid socketfd case 1, should fail.";
}

/* Check the stats of a invalid socket, should fail. */
TEST_F(SocketTagUsrSpaceTest, CheckStatsInvalidSocketFail) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  memset(stats_result_, 0, 2);
  EXPECT_FALSE(sock_0.checkStats(valid_tag1, fake_uid, 0, stats_result_))
      << "No stats should be here";
}

/* Untag invalid socket fd, should fail */
TEST_F(SocketTagUsrSpaceTest, UntagInvalidSocketFail) {
  EXPECT_LT(legacy_untagSocket(-1), 0) << "invalid socket fd, should fail";
  EXPECT_LT(legacy_untagSocket(sock_0.fd), 0)
      << "no tags on sock0, should fail";
}

/*
 * Set the counter to a number larger then max counter avalaible
 * should fail
 */
TEST_F(SocketTagUsrSpaceTest, CounterSetNumExceedFail) {
  int wrongCounterNum = kMaxCounterSet + 1;
  EXPECT_LT(legacy_setCounterSet(wrongCounterNum, my_uid), 0)
      << "Invalid counter set number, should fail.";
}

/* Tag without valid uid, should be tagged with my_uid */
TEST_F(SocketTagUsrSpaceTest, NoUidTag) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag1, 0), 0)
      << "tag failed without uid";
  EXPECT_TRUE(sock_0.checkTag(valid_tag1, my_uid)) << "Tag not found";
}

/*
 * Tag without tag and uid number, should be tagged with tag 0 and
 * my_uid
 */
TEST_F(SocketTagUsrSpaceTest, NoTagNoUid) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  EXPECT_GE(legacy_tagSocket(sock_0.fd, 0, 0), 0)
      << "no tag and uid infomation";
  ASSERT_TRUE(sock_0.checkTag(0, my_uid)) << "Tag not found";
}

/* Untag from a tagged socket */
TEST_F(SocketTagUsrSpaceTest, ValidUntag) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag1, my_uid), 0);
  EXPECT_TRUE(sock_0.checkTag(valid_tag1, my_uid)) << "Tag not found";
  EXPECT_GE(legacy_untagSocket(sock_0.fd), 0);
  EXPECT_FALSE(sock_0.checkTag(valid_tag1, my_uid)) << "Tag should be removed";
}

/* Tag a socket for the first time */
TEST_F(SocketTagUsrSpaceTest, ValidFirsttag) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag2, fake_uid), 0);
  EXPECT_TRUE(sock_0.checkTag(valid_tag2, fake_uid)) << "Tag not found.";
}

/* ReTag a already tagged socket with the same tag and uid */
TEST_F(SocketTagUsrSpaceTest, ValidReTag) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag2, fake_uid), 0);
  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag2, fake_uid), 0);
  EXPECT_TRUE(sock_0.checkTag(valid_tag2, fake_uid)) << "Tag not found.";
}

/*
 * Retag a already tagged socket with the same uid but different tag
 * Should keep the second one and untag the original one
 */
TEST_F(SocketTagUsrSpaceTest, ValidReTagWithAcctTagChange) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag2, fake_uid), 0);
  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag1, fake_uid), 0);
  EXPECT_TRUE(sock_0.checkTag(valid_tag1, fake_uid)) << "Tag not found.";
  EXPECT_FALSE(sock_0.checkTag(valid_tag2, fake_uid))
      << "Tag should not be here";
}

/*
 * Retag a already tagged socket with different uid and different tag.
 * Should keep both
 */
TEST_F(SocketTagUsrSpaceTest, ReTagWithUidChange) {
  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag2, fake_uid), 0);
  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag1, fake_uid2), 0);
}

/*
 * Retag a already tagged socket with different uid but the same tag.
 * The original one should be replaced by the new one.
 */
TEST_F(SocketTagUsrSpaceTest, ReTagWithUidChange2) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag2, fake_uid), 0);
  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag2, fake_uid2), 0);
  EXPECT_TRUE(sock_0.checkTag(valid_tag2, fake_uid2)) << "Tag not found.";
  EXPECT_FALSE(sock_0.checkTag(valid_tag2, fake_uid))
      << "Tag should not be here";
}

/* Tag two sockets with two uids and two tags. */
TEST_F(SocketTagUsrSpaceTest, TagAnotherSocket) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  EXPECT_GE(legacy_tagSocket(sock_0.fd, max_uint_tag, my_uid), 0);
  EXPECT_GE(legacy_tagSocket(sock_1.fd, valid_tag1, fake_uid2), 0);
  EXPECT_TRUE(sock_1.checkTag(valid_tag1, fake_uid2)) << "Tag not found.";
  EXPECT_GE(legacy_untagSocket(sock_0.fd), 0);
  EXPECT_FALSE(sock_0.checkTag(max_uint_tag, fake_uid))
      << "Tag should not be there";
  EXPECT_TRUE(sock_1.checkTag(valid_tag1, fake_uid2)) << "Tag not found";
  EXPECT_GE(legacy_untagSocket(sock_1.fd), 0);
  EXPECT_FALSE(sock_1.checkTag(valid_tag1, fake_uid2))
      << "Tag should not be there";
}

/* Tag two sockets with the same uid but different acct_tags. */
TEST_F(SocketTagUsrSpaceTest, SameUidTwoSocket) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag1, my_uid), 0);
  EXPECT_GE(legacy_tagSocket(sock_1.fd, valid_tag2, my_uid), 0);
  EXPECT_TRUE(sock_1.checkTag(valid_tag2, my_uid)) << "Tag not found.";
  EXPECT_TRUE(sock_0.checkTag(valid_tag1, my_uid)) << "Tag not found.";
  EXPECT_GE(legacy_untagSocket(sock_0.fd), 0);
  EXPECT_FALSE(sock_0.checkTag(valid_tag1, my_uid))
      << "Tag should not be there";
  EXPECT_TRUE(sock_1.checkTag(valid_tag2, my_uid)) << "Tag not found";
  EXPECT_GE(legacy_untagSocket(sock_1.fd), 0);
  EXPECT_FALSE(sock_1.checkTag(valid_tag2, my_uid))
      << "Tag should not be there";
}

/* Tag two sockets with the same acct_tag but different uids */
TEST_F(SocketTagUsrSpaceTest, SameTagTwoSocket) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag1, fake_uid), 0);
  EXPECT_GE(legacy_tagSocket(sock_1.fd, valid_tag1, fake_uid2), 0);
  EXPECT_TRUE(sock_1.checkTag(valid_tag1, fake_uid)) << "Tag not found.";
  EXPECT_TRUE(sock_0.checkTag(valid_tag1, fake_uid2)) << "Tag not found.";
  EXPECT_GE(legacy_untagSocket(sock_0.fd), 0);
  EXPECT_FALSE(sock_0.checkTag(valid_tag1, fake_uid))
      << "Tag should not be there";
  EXPECT_TRUE(sock_1.checkTag(valid_tag1, fake_uid2)) << "Tag not found";
  EXPECT_GE(legacy_untagSocket(sock_1.fd), 0);
  EXPECT_FALSE(sock_1.checkTag(valid_tag1, fake_uid2))
      << "Tag should not be there";
}

/* Tag a closed socket, should fail. */
TEST_F(SocketTagUsrSpaceTest, TagInvalidSocketFail) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  close(sock_0.fd);
  EXPECT_LT(legacy_tagSocket(sock_0.fd, valid_tag1, my_uid), 0);
  EXPECT_FALSE(sock_0.checkTag(valid_tag1, my_uid))
      << "Tag should not be there";
}

/* untag from a closed socket, should fail. */
TEST_F(SocketTagUsrSpaceTest, UntagClosedSocketFail) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  EXPECT_GE(legacy_tagSocket(sock_1.fd, valid_tag1, my_uid), 0);
  EXPECT_TRUE(sock_1.checkTag(valid_tag1, my_uid));
  close(sock_1.fd);
  EXPECT_LT(legacy_untagSocket(sock_1.fd), 0) << "no tag attached, should fail";
}

/*
 * try to connect with google.com in order to generate some
 * tranffic through the socket, the traffic statistics should
 * be stored in the stats file and will be returned.
 */
TEST_F(SocketTagUsrSpaceTest, dataTransmitTest) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  memset(stats_result_, 0, 2);
  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag1, inet_uid), 0);
  EXPECT_TRUE(sock_0.checkTag(valid_tag1, inet_uid)) << "tag not found";
  EXPECT_GE(server_download(sock_0, sock_1), 0);
  EXPECT_GE(legacy_untagSocket(sock_0.fd), 0);
  close(sock_0.fd);
  EXPECT_TRUE(sock_0.checkStats(valid_tag1, inet_uid, 0, stats_result_))
      << "failed to retreive data";
  EXPECT_GT(*stats_result_, (uint32_t)0) << "no stats found for this socket";
  EXPECT_GT(*(stats_result_ + 1), (uint32_t)0)
      << "no stats stored for this socket";
}

/* Generate some traffic first and then delete the
 * tag and uid from stats. All the stats related should
 * be deleted. checkStats() should return false.
 */
TEST_F(SocketTagUsrSpaceTest, dataStatsDeleteTest) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  memset(stats_result_, 0, 2);
  EXPECT_GE(legacy_tagSocket(sock_0.fd, valid_tag1, fake_uid), 0);
  EXPECT_TRUE(sock_0.checkTag(valid_tag1, fake_uid)) << "tag not found";
  EXPECT_GE(server_download(sock_0, sock_1), 0);
  EXPECT_GE(legacy_untagSocket(sock_0.fd), 0);
  EXPECT_TRUE(sock_0.checkStats(valid_tag1, fake_uid, 0, stats_result_))
      << "failed to retreive data";
  EXPECT_GT(*stats_result_, (uint32_t)0) << "no stats found for this socket";
  EXPECT_GT(*(stats_result_ + 1), (uint32_t)0)
      << "no stats stored for this socket";
  EXPECT_GE(legacy_deleteTagData(0, fake_uid), 0)
      << "Failed to delete fake_uid";
  EXPECT_FALSE(sock_0.checkStats(valid_tag1, fake_uid, 0, stats_result_))
      << "NO DATA should be stored";
}

/*
 * try to store the traffic stats in the secound counter
 * insdead of the first. All the stats should be stored
 * in the secound counter.
 */
TEST_F(SocketTagUsrSpaceTest, CounterSetTest) {
  SKIP_IF_QTAGUID_NOT_SUPPORTED();

  memset(stats_result_, 0, 2);
  EXPECT_GE(legacy_tagSocket(sock_1.fd, valid_tag1, inet_uid), 0);
  EXPECT_GE(legacy_setCounterSet(1, inet_uid), 0);
  EXPECT_TRUE(sock_1.checkTag(valid_tag1, inet_uid)) << "tag not found";
  EXPECT_GE(server_download(sock_0, sock_1), 0);
  EXPECT_GE(legacy_untagSocket(sock_1.fd), 0);
  close(sock_1.fd);
  EXPECT_TRUE(sock_1.checkStats(valid_tag1, inet_uid, 1, stats_result_))
      << "failed to retreive data";
  uint32_t packet_count = 1;
  uint32_t total_byte = 1024;
  EXPECT_GT(*stats_result_, total_byte) << "no stats found for this socket";
  EXPECT_GT(*(stats_result_ + 1), packet_count)
      << "wrong stats stored for this socket";
  uint32_t stats_foreground[2] = {0, 0};
  EXPECT_TRUE(sock_0.checkStats(valid_tag1, inet_uid, 0, stats_foreground))
      << "fail to retrieve data";
  EXPECT_LE(*stats_foreground, (uint32_t)0) << "stats data is not zero";
}
}  // namespace android
