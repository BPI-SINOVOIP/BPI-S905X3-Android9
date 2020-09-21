/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <string>

#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/inet_diag.h>
#include <linux/sock_diag.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include <cutils/qtaguid.h>

#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include "bpf/BpfMap.h"
#include "bpf/BpfUtils.h"

using namespace android::bpf;

using android::base::unique_fd;
using android::netdutils::status::ok;
using android::netdutils::StatusOr;

namespace android {
namespace bpf {

// Use the upper limit of uid to avoid conflict with real app uids. We can't use UID_MAX because
// it's -1, which is INVALID_UID.
constexpr uid_t TEST_UID = UID_MAX - 1;
constexpr uint32_t TEST_TAG = 42;
constexpr int TEST_COUNTERSET = 1;
constexpr int DEFAULT_COUNTERSET = 0;

class BpfBasicTest : public testing::Test {
  protected:
    BpfBasicTest() {}
};

TEST_F(BpfBasicTest, TestCgroupMounted) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    ASSERT_EQ(0, access(CGROUP_ROOT_PATH, R_OK));
    ASSERT_EQ(0, access("/dev/cg2_bpf/cgroup.controllers", R_OK));
}

TEST_F(BpfBasicTest, TestTrafficControllerSetUp) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    ASSERT_EQ(0, access(BPF_EGRESS_PROG_PATH, R_OK));
    ASSERT_EQ(0, access(BPF_INGRESS_PROG_PATH, R_OK));
    ASSERT_EQ(0, access(XT_BPF_INGRESS_PROG_PATH, R_OK));
    ASSERT_EQ(0, access(XT_BPF_EGRESS_PROG_PATH, R_OK));
    ASSERT_EQ(0, access(COOKIE_TAG_MAP_PATH, R_OK));
    ASSERT_EQ(0, access(UID_COUNTERSET_MAP_PATH, R_OK));
    ASSERT_EQ(0, access(UID_STATS_MAP_PATH, R_OK));
    ASSERT_EQ(0, access(TAG_STATS_MAP_PATH, R_OK));
    ASSERT_EQ(0, access(IFACE_INDEX_NAME_MAP_PATH, R_OK));
    ASSERT_EQ(0, access(IFACE_STATS_MAP_PATH, R_OK));
    ASSERT_EQ(0, access(DOZABLE_UID_MAP_PATH, R_OK));
    ASSERT_EQ(0, access(STANDBY_UID_MAP_PATH, R_OK));
    ASSERT_EQ(0, access(POWERSAVE_UID_MAP_PATH, R_OK));
}

TEST_F(BpfBasicTest, TestTagSocket) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    BpfMap<uint64_t, UidTag> cookieTagMap(mapRetrieve(COOKIE_TAG_MAP_PATH, 0));
    ASSERT_LE(0, cookieTagMap.getMap());
    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    ASSERT_LE(0, sock);
    uint64_t cookie = getSocketCookie(sock);
    ASSERT_NE(NONEXISTENT_COOKIE, cookie);
    ASSERT_EQ(0, qtaguid_tagSocket(sock, TEST_TAG, TEST_UID));
    StatusOr<UidTag> tagResult = cookieTagMap.readValue(cookie);
    ASSERT_TRUE(isOk(tagResult));
    ASSERT_EQ(TEST_UID, tagResult.value().uid);
    ASSERT_EQ(TEST_TAG, tagResult.value().tag);
    ASSERT_EQ(0, qtaguid_untagSocket(sock));
    tagResult = cookieTagMap.readValue(cookie);
    ASSERT_FALSE(isOk(tagResult));
    ASSERT_EQ(ENOENT, tagResult.status().code());
}

TEST_F(BpfBasicTest, TestCloseSocketWithoutUntag) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    BpfMap<uint64_t, UidTag> cookieTagMap(mapRetrieve(COOKIE_TAG_MAP_PATH, 0));
    ASSERT_LE(0, cookieTagMap.getMap());
    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    ASSERT_LE(0, sock);
    uint64_t cookie = getSocketCookie(sock);
    ASSERT_NE(NONEXISTENT_COOKIE, cookie);
    ASSERT_EQ(0, qtaguid_tagSocket(sock, TEST_TAG, TEST_UID));
    StatusOr<UidTag> tagResult = cookieTagMap.readValue(cookie);
    ASSERT_TRUE(isOk(tagResult));
    ASSERT_EQ(TEST_UID, tagResult.value().uid);
    ASSERT_EQ(TEST_TAG, tagResult.value().tag);
    ASSERT_EQ(0, close(sock));
    // Check map periodically until sk destroy handler have done its job.
    for (int i = 0; i < 10; i++) {
        tagResult = cookieTagMap.readValue(cookie);
        if (!isOk(tagResult)) {
            ASSERT_EQ(ENOENT, tagResult.status().code());
            return;
        }
        usleep(50);
    }
    FAIL() << "socket tag still exist after 500ms";
}

TEST_F(BpfBasicTest, TestChangeCounterSet) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    BpfMap<uint32_t, uint8_t> uidCounterSetMap(mapRetrieve(UID_COUNTERSET_MAP_PATH, 0));
    ASSERT_LE(0, uidCounterSetMap.getMap());
    ASSERT_EQ(0, qtaguid_setCounterSet(TEST_COUNTERSET, TEST_UID));
    uid_t uid = TEST_UID;
    StatusOr<uint8_t> counterSetResult = uidCounterSetMap.readValue(uid);
    ASSERT_TRUE(isOk(counterSetResult));
    ASSERT_EQ(TEST_COUNTERSET, counterSetResult.value());
    ASSERT_EQ(0, qtaguid_setCounterSet(DEFAULT_COUNTERSET, TEST_UID));
    counterSetResult = uidCounterSetMap.readValue(uid);
    ASSERT_FALSE(isOk(counterSetResult));
    ASSERT_EQ(ENOENT, counterSetResult.status().code());
}

TEST_F(BpfBasicTest, TestDeleteTagData) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    BpfMap<StatsKey, StatsValue> uidStatsMap(mapRetrieve(UID_STATS_MAP_PATH, 0));
    ASSERT_LE(0, uidStatsMap.getMap());
    BpfMap<StatsKey, StatsValue> tagStatsMap(mapRetrieve(TAG_STATS_MAP_PATH, 0));
    ASSERT_LE(0, tagStatsMap.getMap());
    BpfMap<uint32_t, StatsValue> appUidStatsMap(mapRetrieve(APP_UID_STATS_MAP_PATH, 0));
    ASSERT_LE(0, appUidStatsMap.getMap());

    StatsKey key = {.uid = TEST_UID, .tag = TEST_TAG, .counterSet = TEST_COUNTERSET,
                    .ifaceIndex = 1};
    StatsValue statsMapValue = {.rxPackets = 1, .rxBytes = 100};
    EXPECT_TRUE(isOk(tagStatsMap.writeValue(key, statsMapValue, BPF_ANY)));
    key.tag = 0;
    EXPECT_TRUE(isOk(uidStatsMap.writeValue(key, statsMapValue, BPF_ANY)));
    EXPECT_TRUE(isOk(appUidStatsMap.writeValue(TEST_UID, statsMapValue, BPF_ANY)));
    ASSERT_EQ(0, qtaguid_deleteTagData(0, TEST_UID));
    StatusOr<StatsValue> statsResult = uidStatsMap.readValue(key);
    ASSERT_FALSE(isOk(statsResult));
    ASSERT_EQ(ENOENT, statsResult.status().code());
    statsResult = appUidStatsMap.readValue(TEST_UID);
    ASSERT_FALSE(isOk(statsResult));
    ASSERT_EQ(ENOENT, statsResult.status().code());
    key.tag = TEST_TAG;
    statsResult = tagStatsMap.readValue(key);
    ASSERT_FALSE(isOk(statsResult));
    ASSERT_EQ(ENOENT, statsResult.status().code());
}

}
}
