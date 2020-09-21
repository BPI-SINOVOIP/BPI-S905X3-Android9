/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * TrafficControllerTest.cpp - unit tests for TrafficController.cpp
 */

#include <string>
#include <vector>

#include <fcntl.h>
#include <inttypes.h>
#include <linux/inet_diag.h>
#include <linux/sock_diag.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include <netdutils/MockSyscalls.h>
#include "netdutils/Status.h"
#include "netdutils/StatusOr.h"

#include "FirewallController.h"
#include "TrafficController.h"
#include "bpf/BpfUtils.h"

using namespace android::bpf;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::Test;

namespace android {
namespace net {

using base::unique_fd;
using netdutils::isOk;
using netdutils::Status;
using netdutils::status::ok;
using netdutils::StatusOr;

constexpr int TEST_MAP_SIZE = 10;
constexpr uid_t TEST_UID = 10086;
constexpr uid_t TEST_UID2 = 54321;
constexpr uid_t TEST_UID3 = 98765;
constexpr uint32_t TEST_TAG = 42;
constexpr uint32_t TEST_COUNTERSET = 1;
constexpr uint32_t DEFAULT_COUNTERSET = 0;

class TrafficControllerTest : public ::testing::Test {
  protected:
    TrafficControllerTest() {}
    TrafficController mTc;
    BpfMap<uint64_t, UidTag> mFakeCookieTagMap;
    BpfMap<uint32_t, uint8_t> mFakeUidCounterSetMap;
    BpfMap<uint32_t, StatsValue> mFakeAppUidStatsMap;
    BpfMap<StatsKey, StatsValue> mFakeUidStatsMap;
    BpfMap<StatsKey, StatsValue> mFakeTagStatsMap;
    BpfMap<uint32_t, uint8_t> mFakeDozableUidMap;
    BpfMap<uint32_t, uint8_t> mFakeStandbyUidMap;
    BpfMap<uint32_t, uint8_t> mFakePowerSaveUidMap;

    void SetUp() {
        std::lock_guard<std::mutex> ownerGuard(mTc.mOwnerMatchMutex);
        SKIP_IF_BPF_NOT_SUPPORTED;

        mFakeCookieTagMap.reset(createMap(BPF_MAP_TYPE_HASH, sizeof(uint64_t),
                                          sizeof(struct UidTag), TEST_MAP_SIZE, 0));
        ASSERT_LE(0, mFakeCookieTagMap.getMap());

        mFakeUidCounterSetMap.reset(
            createMap(BPF_MAP_TYPE_HASH, sizeof(uint32_t), sizeof(uint8_t), TEST_MAP_SIZE, 0));
        ASSERT_LE(0, mFakeUidCounterSetMap.getMap());

        mFakeAppUidStatsMap.reset(createMap(BPF_MAP_TYPE_HASH, sizeof(uint32_t),
                                            sizeof(struct StatsValue), TEST_MAP_SIZE, 0));
        ASSERT_LE(0, mFakeAppUidStatsMap.getMap());

        mFakeUidStatsMap.reset(createMap(BPF_MAP_TYPE_HASH, sizeof(struct StatsKey),
                                         sizeof(struct StatsValue), TEST_MAP_SIZE, 0));
        ASSERT_LE(0, mFakeUidStatsMap.getMap());

        mFakeTagStatsMap.reset(createMap(BPF_MAP_TYPE_HASH, sizeof(struct StatsKey),
                                         sizeof(struct StatsValue), TEST_MAP_SIZE, 0));
        ASSERT_LE(0, mFakeTagStatsMap.getMap());

        mFakeDozableUidMap.reset(
            createMap(BPF_MAP_TYPE_HASH, sizeof(uint32_t), sizeof(uint8_t), TEST_MAP_SIZE, 0));
        ASSERT_LE(0, mFakeDozableUidMap.getMap());

        mFakeStandbyUidMap.reset(
            createMap(BPF_MAP_TYPE_HASH, sizeof(uint32_t), sizeof(uint8_t), TEST_MAP_SIZE, 0));
        ASSERT_LE(0, mFakeStandbyUidMap.getMap());

        mFakePowerSaveUidMap.reset(
            createMap(BPF_MAP_TYPE_HASH, sizeof(uint32_t), sizeof(uint8_t), TEST_MAP_SIZE, 0));
        ASSERT_LE(0, mFakePowerSaveUidMap.getMap());
        // Make sure trafficController use the eBPF code path.
        mTc.ebpfSupported = true;

        mTc.mCookieTagMap.reset(mFakeCookieTagMap.getMap());
        mTc.mUidCounterSetMap.reset(mFakeUidCounterSetMap.getMap());
        mTc.mAppUidStatsMap.reset(mFakeAppUidStatsMap.getMap());
        mTc.mUidStatsMap.reset(mFakeUidStatsMap.getMap());
        mTc.mTagStatsMap.reset(mFakeTagStatsMap.getMap());
        mTc.mDozableUidMap.reset(mFakeDozableUidMap.getMap());
        mTc.mStandbyUidMap.reset(mFakeStandbyUidMap.getMap());
        mTc.mPowerSaveUidMap.reset(mFakePowerSaveUidMap.getMap());
    }

    int setUpSocketAndTag(int protocol, uint64_t* cookie, uint32_t tag, uid_t uid) {
        int sock = socket(protocol, SOCK_STREAM, 0);
        EXPECT_LE(0, sock);
        *cookie = getSocketCookie(sock);
        EXPECT_NE(NONEXISTENT_COOKIE, *cookie);
        EXPECT_EQ(0, mTc.tagSocket(sock, tag, uid));
        return sock;
    }

    void expectUidTag(uint64_t cookie, uid_t uid, uint32_t tag) {
        StatusOr<UidTag> tagResult = mFakeCookieTagMap.readValue(cookie);
        EXPECT_TRUE(isOk(tagResult));
        EXPECT_EQ(uid, tagResult.value().uid);
        EXPECT_EQ(tag, tagResult.value().tag);
    }

    void expectNoTag(uint64_t cookie) { EXPECT_FALSE(isOk(mFakeCookieTagMap.readValue(cookie))); }

    void expectTagMapEmpty() { EXPECT_FALSE(isOk(mFakeCookieTagMap.getFirstKey())); }

    void populateFakeStats(uint64_t cookie, uint32_t uid, uint32_t tag, StatsKey* key) {
        UidTag cookieMapkey = {.uid = (uint32_t)uid, .tag = tag};
        EXPECT_TRUE(isOk(mFakeCookieTagMap.writeValue(cookie, cookieMapkey, BPF_ANY)));
        *key = {.uid = uid, .tag = tag, .counterSet = TEST_COUNTERSET, .ifaceIndex = 1};
        StatsValue statsMapValue = {.rxPackets = 1, .rxBytes = 100};
        uint8_t counterSet = TEST_COUNTERSET;
        EXPECT_TRUE(isOk(mFakeUidCounterSetMap.writeValue(uid, counterSet, BPF_ANY)));
        EXPECT_TRUE(isOk(mFakeTagStatsMap.writeValue(*key, statsMapValue, BPF_ANY)));
        key->tag = 0;
        EXPECT_TRUE(isOk(mFakeUidStatsMap.writeValue(*key, statsMapValue, BPF_ANY)));
        EXPECT_TRUE(isOk(mFakeAppUidStatsMap.writeValue(uid, statsMapValue, BPF_ANY)));
        // put tag information back to statsKey
        key->tag = tag;
    }

    void checkUidOwnerRuleForChain(ChildChain chain, BpfMap<uint32_t, uint8_t>& targetMap) {
        uint32_t uid = TEST_UID;
        EXPECT_EQ(0, mTc.changeUidOwnerRule(chain, uid, DENY, BLACKLIST));
        StatusOr<uint8_t> value = targetMap.readValue(uid);
        EXPECT_TRUE(isOk(value));
        EXPECT_EQ(BPF_DROP, value.value());

        uid = TEST_UID2;
        EXPECT_EQ(0, mTc.changeUidOwnerRule(chain, uid, ALLOW, WHITELIST));
        value = targetMap.readValue(uid);
        EXPECT_TRUE(isOk(value));
        EXPECT_EQ(BPF_PASS, value.value());

        EXPECT_EQ(0, mTc.changeUidOwnerRule(chain, uid, DENY, WHITELIST));
        value = targetMap.readValue(uid);
        EXPECT_FALSE(isOk(value));
        EXPECT_EQ(ENOENT, value.status().code());

        uid = TEST_UID;
        EXPECT_EQ(0, mTc.changeUidOwnerRule(chain, uid, ALLOW, BLACKLIST));
        value = targetMap.readValue(uid);
        EXPECT_FALSE(isOk(value));
        EXPECT_EQ(ENOENT, value.status().code());

        uid = TEST_UID3;
        EXPECT_EQ(-ENOENT, mTc.changeUidOwnerRule(chain, uid, ALLOW, BLACKLIST));
        value = targetMap.readValue(uid);
        EXPECT_FALSE(isOk(value));
        EXPECT_EQ(ENOENT, value.status().code());
    }

    void checkEachUidValue(const std::vector<int32_t>& uids, const uint8_t expectValue,
                           BpfMap<uint32_t, uint8_t>& targetMap) {
        for (uint32_t uid : uids) {
            StatusOr<uint8_t> value = targetMap.readValue(uid);
            EXPECT_TRUE(isOk(value));
            EXPECT_EQ(expectValue, value.value());
        }
        std::set<uint32_t> uidSet(uids.begin(), uids.end());
        const auto checkNoOtherUid = [&uidSet](const int32_t& key,
                                               const BpfMap<uint32_t, uint8_t>&) {
            EXPECT_NE(uidSet.end(), uidSet.find(key));
            return netdutils::status::ok;
        };
        EXPECT_TRUE(isOk(targetMap.iterate(checkNoOtherUid)));
    }

    void checkUidMapReplace(const std::string& name, const std::vector<int32_t>& uids,
                            BpfMap<uint32_t, uint8_t>& targetMap) {
        bool isWhitelist = true;
        EXPECT_EQ(0, mTc.replaceUidOwnerMap(name, isWhitelist, uids));
        checkEachUidValue(uids, BPF_PASS, targetMap);

        isWhitelist = false;
        EXPECT_EQ(0, mTc.replaceUidOwnerMap(name, isWhitelist, uids));
        checkEachUidValue(uids, BPF_DROP, targetMap);
    }

    void TearDown() {
        std::lock_guard<std::mutex> ownerGuard(mTc.mOwnerMatchMutex);
        mFakeCookieTagMap.reset();
        mFakeUidCounterSetMap.reset();
        mFakeAppUidStatsMap.reset();
        mFakeUidStatsMap.reset();
        mFakeTagStatsMap.reset();
        mTc.mDozableUidMap.reset();
        mTc.mStandbyUidMap.reset();
        mTc.mPowerSaveUidMap.reset();
    }
};

TEST_F(TrafficControllerTest, TestTagSocketV4) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    uint64_t sockCookie;
    int v4socket = setUpSocketAndTag(AF_INET, &sockCookie, TEST_TAG, TEST_UID);
    expectUidTag(sockCookie, TEST_UID, TEST_TAG);
    ASSERT_EQ(0, mTc.untagSocket(v4socket));
    expectNoTag(sockCookie);
    expectTagMapEmpty();
}

TEST_F(TrafficControllerTest, TestReTagSocket) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    uint64_t sockCookie;
    int v4socket = setUpSocketAndTag(AF_INET, &sockCookie, TEST_TAG, TEST_UID);
    expectUidTag(sockCookie, TEST_UID, TEST_TAG);
    ASSERT_EQ(0, mTc.tagSocket(v4socket, TEST_TAG + 1, TEST_UID + 1));
    expectUidTag(sockCookie, TEST_UID + 1, TEST_TAG + 1);
}

TEST_F(TrafficControllerTest, TestTagTwoSockets) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    uint64_t sockCookie1;
    uint64_t sockCookie2;
    int v4socket1 = setUpSocketAndTag(AF_INET, &sockCookie1, TEST_TAG, TEST_UID);
    setUpSocketAndTag(AF_INET, &sockCookie2, TEST_TAG, TEST_UID);
    expectUidTag(sockCookie1, TEST_UID, TEST_TAG);
    expectUidTag(sockCookie2, TEST_UID, TEST_TAG);
    ASSERT_EQ(0, mTc.untagSocket(v4socket1));
    expectNoTag(sockCookie1);
    expectUidTag(sockCookie2, TEST_UID, TEST_TAG);
    ASSERT_FALSE(isOk(mFakeCookieTagMap.getNextKey(sockCookie2)));
}

TEST_F(TrafficControllerTest, TestTagSocketV6) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    uint64_t sockCookie;
    int v6socket = setUpSocketAndTag(AF_INET6, &sockCookie, TEST_TAG, TEST_UID);
    expectUidTag(sockCookie, TEST_UID, TEST_TAG);
    ASSERT_EQ(0, mTc.untagSocket(v6socket));
    expectNoTag(sockCookie);
    expectTagMapEmpty();
}

TEST_F(TrafficControllerTest, TestTagInvalidSocket) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    int invalidSocket = -1;
    ASSERT_GT(0, mTc.tagSocket(invalidSocket, TEST_TAG, TEST_UID));
    expectTagMapEmpty();
}

TEST_F(TrafficControllerTest, TestUntagInvalidSocket) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    int invalidSocket = -1;
    ASSERT_GT(0, mTc.untagSocket(invalidSocket));
    int v4socket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GT(0, mTc.untagSocket(v4socket));
    expectTagMapEmpty();
}

TEST_F(TrafficControllerTest, TestSetCounterSet) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    ASSERT_EQ(0, mTc.setCounterSet(TEST_COUNTERSET, TEST_UID));
    uid_t uid = TEST_UID;
    StatusOr<uint8_t> counterSetResult = mFakeUidCounterSetMap.readValue(uid);
    ASSERT_TRUE(isOk(counterSetResult));
    ASSERT_EQ(TEST_COUNTERSET, counterSetResult.value());
    ASSERT_EQ(0, mTc.setCounterSet(DEFAULT_COUNTERSET, TEST_UID));
    ASSERT_FALSE(isOk(mFakeUidCounterSetMap.readValue(uid)));
    ASSERT_FALSE(isOk(mFakeUidCounterSetMap.getFirstKey()));
}

TEST_F(TrafficControllerTest, TestSetInvalidCounterSet) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    ASSERT_GT(0, mTc.setCounterSet(OVERFLOW_COUNTERSET, TEST_UID));
    uid_t uid = TEST_UID;
    ASSERT_FALSE(isOk(mFakeUidCounterSetMap.readValue(uid)));
    ASSERT_FALSE(isOk(mFakeUidCounterSetMap.getFirstKey()));
}

TEST_F(TrafficControllerTest, TestDeleteTagData) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    uint64_t cookie = 1;
    uid_t uid = TEST_UID;
    uint32_t tag = TEST_TAG;
    StatsKey tagStatsMapKey;
    populateFakeStats(cookie, uid, tag, &tagStatsMapKey);
    ASSERT_EQ(0, mTc.deleteTagData(TEST_TAG, TEST_UID));
    ASSERT_FALSE(isOk(mFakeCookieTagMap.readValue(cookie)));
    StatusOr<uint8_t> counterSetResult = mFakeUidCounterSetMap.readValue(uid);
    ASSERT_TRUE(isOk(counterSetResult));
    ASSERT_EQ(TEST_COUNTERSET, counterSetResult.value());
    ASSERT_FALSE(isOk(mFakeTagStatsMap.readValue(tagStatsMapKey)));
    tagStatsMapKey.tag = 0;
    StatusOr<StatsValue> statsMapResult = mFakeUidStatsMap.readValue(tagStatsMapKey);
    ASSERT_TRUE(isOk(statsMapResult));
    ASSERT_EQ((uint64_t)1, statsMapResult.value().rxPackets);
    ASSERT_EQ((uint64_t)100, statsMapResult.value().rxBytes);
    auto appStatsResult = mFakeAppUidStatsMap.readValue(TEST_UID);
    ASSERT_TRUE(isOk(appStatsResult));
    ASSERT_EQ((uint64_t)1, appStatsResult.value().rxPackets);
    ASSERT_EQ((uint64_t)100, appStatsResult.value().rxBytes);
}

TEST_F(TrafficControllerTest, TestDeleteAllUidData) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    uint64_t cookie = 1;
    uid_t uid = TEST_UID;
    uint32_t tag = TEST_TAG;
    StatsKey tagStatsMapKey;
    populateFakeStats(cookie, uid, tag, &tagStatsMapKey);
    ASSERT_EQ(0, mTc.deleteTagData(0, TEST_UID));
    ASSERT_FALSE(isOk(mFakeCookieTagMap.readValue(cookie)));
    ASSERT_FALSE(isOk(mFakeUidCounterSetMap.readValue(uid)));
    ASSERT_FALSE(isOk(mFakeTagStatsMap.readValue(tagStatsMapKey)));
    tagStatsMapKey.tag = 0;
    ASSERT_FALSE(isOk(mFakeUidStatsMap.readValue(tagStatsMapKey)));
    ASSERT_FALSE(isOk(mFakeAppUidStatsMap.readValue(TEST_UID)));
}

TEST_F(TrafficControllerTest, TestDeleteDataWithTwoTags) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    uint64_t cookie1 = 1;
    uint64_t cookie2 = 2;
    uid_t uid = TEST_UID;
    uint32_t tag1 = TEST_TAG;
    uint32_t tag2 = TEST_TAG + 1;
    StatsKey tagStatsMapKey1;
    StatsKey tagStatsMapKey2;
    populateFakeStats(cookie1, uid, tag1, &tagStatsMapKey1);
    populateFakeStats(cookie2, uid, tag2, &tagStatsMapKey2);
    ASSERT_EQ(0, mTc.deleteTagData(TEST_TAG, TEST_UID));
    ASSERT_FALSE(isOk(mFakeCookieTagMap.readValue(cookie1)));
    StatusOr<UidTag> cookieMapResult = mFakeCookieTagMap.readValue(cookie2);
    ASSERT_TRUE(isOk(cookieMapResult));
    ASSERT_EQ(TEST_UID, cookieMapResult.value().uid);
    ASSERT_EQ(TEST_TAG + 1, cookieMapResult.value().tag);
    StatusOr<uint8_t> counterSetResult = mFakeUidCounterSetMap.readValue(uid);
    ASSERT_TRUE(isOk(counterSetResult));
    ASSERT_EQ(TEST_COUNTERSET, counterSetResult.value());
    ASSERT_FALSE(isOk(mFakeTagStatsMap.readValue(tagStatsMapKey1)));
    StatusOr<StatsValue> statsMapResult = mFakeTagStatsMap.readValue(tagStatsMapKey2);
    ASSERT_TRUE(isOk(statsMapResult));
    ASSERT_EQ((uint64_t)1, statsMapResult.value().rxPackets);
    ASSERT_EQ((uint64_t)100, statsMapResult.value().rxBytes);
}

TEST_F(TrafficControllerTest, TestDeleteDataWithTwoUids) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    uint64_t cookie1 = 1;
    uint64_t cookie2 = 2;
    uid_t uid1 = TEST_UID;
    uid_t uid2 = TEST_UID + 1;
    uint32_t tag = TEST_TAG;
    StatsKey tagStatsMapKey1;
    StatsKey tagStatsMapKey2;
    populateFakeStats(cookie1, uid1, tag, &tagStatsMapKey1);
    populateFakeStats(cookie2, uid2, tag, &tagStatsMapKey2);

    // Delete the stats of one of the uid. Check if it is properly collected by
    // removedStats.
    ASSERT_EQ(0, mTc.deleteTagData(0, uid2));
    ASSERT_FALSE(isOk(mFakeCookieTagMap.readValue(cookie2)));
    StatusOr<uint8_t> counterSetResult = mFakeUidCounterSetMap.readValue(uid1);
    ASSERT_TRUE(isOk(counterSetResult));
    ASSERT_EQ(TEST_COUNTERSET, counterSetResult.value());
    ASSERT_FALSE(isOk(mFakeUidCounterSetMap.readValue(uid2)));
    ASSERT_FALSE(isOk(mFakeTagStatsMap.readValue(tagStatsMapKey2)));
    tagStatsMapKey2.tag = 0;
    ASSERT_FALSE(isOk(mFakeUidStatsMap.readValue(tagStatsMapKey2)));
    ASSERT_FALSE(isOk(mFakeAppUidStatsMap.readValue(uid2)));
    tagStatsMapKey1.tag = 0;
    StatusOr<StatsValue> statsMapResult = mFakeUidStatsMap.readValue(tagStatsMapKey1);
    ASSERT_TRUE(isOk(statsMapResult));
    ASSERT_EQ((uint64_t)1, statsMapResult.value().rxPackets);
    ASSERT_EQ((uint64_t)100, statsMapResult.value().rxBytes);
    auto appStatsResult = mFakeAppUidStatsMap.readValue(uid1);
    ASSERT_TRUE(isOk(appStatsResult));
    ASSERT_EQ((uint64_t)1, appStatsResult.value().rxPackets);
    ASSERT_EQ((uint64_t)100, appStatsResult.value().rxBytes);

    // Delete the stats of the other uid.
    ASSERT_EQ(0, mTc.deleteTagData(0, uid1));
    ASSERT_FALSE(isOk(mFakeUidStatsMap.readValue(tagStatsMapKey1)));
    ASSERT_FALSE(isOk(mFakeAppUidStatsMap.readValue(uid1)));
}

TEST_F(TrafficControllerTest, TestUpdateOwnerMapEntry) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    uint32_t uid = TEST_UID;
    ASSERT_TRUE(isOk(mTc.updateOwnerMapEntry(mFakeDozableUidMap, uid, DENY, BLACKLIST)));
    StatusOr<uint8_t> value = mFakeDozableUidMap.readValue(uid);
    ASSERT_TRUE(isOk(value));
    ASSERT_EQ(BPF_DROP, value.value());

    uid = TEST_UID2;
    ASSERT_TRUE(isOk(mTc.updateOwnerMapEntry(mFakeDozableUidMap, uid, ALLOW, WHITELIST)));
    value = mFakeDozableUidMap.readValue(uid);
    ASSERT_TRUE(isOk(value));
    ASSERT_EQ(BPF_PASS, value.value());

    ASSERT_TRUE(isOk(mTc.updateOwnerMapEntry(mFakeDozableUidMap, uid, DENY, WHITELIST)));
    ASSERT_FALSE(isOk(mFakeDozableUidMap.readValue(uid)));

    uid = TEST_UID;
    ASSERT_TRUE(isOk(mTc.updateOwnerMapEntry(mFakeDozableUidMap, uid, ALLOW, BLACKLIST)));
    ASSERT_FALSE(isOk(mFakeDozableUidMap.readValue(uid)));

    uid = TEST_UID3;
    ASSERT_FALSE(isOk(mTc.updateOwnerMapEntry(mFakeDozableUidMap, uid, ALLOW, BLACKLIST)));
    ASSERT_FALSE(isOk(mFakeDozableUidMap.readValue(uid)));
}

TEST_F(TrafficControllerTest, TestChangeUidOwnerRule) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    checkUidOwnerRuleForChain(DOZABLE, mFakeDozableUidMap);
    checkUidOwnerRuleForChain(STANDBY, mFakeStandbyUidMap);
    checkUidOwnerRuleForChain(POWERSAVE, mFakePowerSaveUidMap);
    ASSERT_EQ(-EINVAL, mTc.changeUidOwnerRule(NONE, TEST_UID, ALLOW, WHITELIST));
    ASSERT_EQ(-EINVAL, mTc.changeUidOwnerRule(INVALID_CHAIN, TEST_UID, ALLOW, WHITELIST));
}

TEST_F(TrafficControllerTest, TestReplaceUidOwnerMap) {
    SKIP_IF_BPF_NOT_SUPPORTED;

    std::vector<int32_t> uids = {TEST_UID, TEST_UID2, TEST_UID3};
    checkUidMapReplace("fw_dozable", uids, mFakeDozableUidMap);
    checkUidMapReplace("fw_standby", uids, mFakeStandbyUidMap);
    checkUidMapReplace("fw_powersave", uids, mFakePowerSaveUidMap);
    ASSERT_EQ(-EINVAL, mTc.replaceUidOwnerMap("unknow", true, uids));
}

}  // namespace net
}  // namespace android
