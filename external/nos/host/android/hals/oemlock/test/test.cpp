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

#include <android-base/endian.h>

#include <MockAvb.client.h>
#include <OemLock.h>

#include <avb.h>

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Return;
using ::testing::SetArgPointee;

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::oemlock::OemLock;
using ::android::hardware::oemlock::OemLockStatus;
using ::android::hardware::oemlock::OemLockSecureStatus;

using ::nugget::app::avb::MockAvb;
using ::nugget::app::avb::CarrierUnlock;
using ::nugget::app::avb::CarrierUnlockRequest;
using ::nugget::app::avb::GetLockRequest;
using ::nugget::app::avb::GetLockResponse;
using ::nugget::app::avb::LockIndex;
using ::nugget::app::avb::SetDeviceLockRequest;

namespace {

/**
 * The signature from the server is a concatenation of a few values. This is a
 * helper to do the concatenation.
 */
hidl_vec<uint8_t> makeSignature(uint64_t version, uint64_t nonce,
                                const std::vector<uint8_t>& token) {
    const uint64_t token_offset = sizeof(uint64_t) * 2;
    hidl_vec<uint8_t> signature(token_offset + token.size());

    // Little-endian in the first 8 bytes
    const uint64_t version_le = htole64(version);
    memcpy(signature.data(), &version_le, sizeof(uint64_t));

    // Little-endian in the second 8 bytes
    const uint64_t nonce_le = htole64(nonce);
    memcpy(signature.data() + sizeof(uint64_t), &nonce_le, sizeof(uint64_t));

    // Remaining data is the token
    memcpy(signature.data() + token_offset, token.data(), token.size());
    return signature;
}

} // namespace

// getName

TEST(OemLockHalTest, getNameReports01) {
    MockAvb mockService;

    // This doesn't need to go to the AVB app

    OemLock hal{mockService};
    hal.getName([](OemLockStatus status, const hidl_string& name) {
        ASSERT_THAT(status, Eq(OemLockStatus::OK));
        EXPECT_THAT(name, Eq("01"));
    });
}

// setOemUnlockAllowedByCarrier

MATCHER_P(CarrierUnlockEq, msg, "") {
    const auto& argToken = arg.token();
    const auto& msgToken = msg.token();
    return argToken.version() == msgToken.version()
        && argToken.nonce() == msgToken.nonce()
        && argToken.signature() == msgToken.signature();
}

TEST(OemLockHalTest, setOemUnlockAllowedByCarrierSendsUnlockSignatureToAvb) {
    constexpr uint64_t version = 93;
    constexpr uint64_t nonce = 3486438654;
    const std::vector<uint8_t> token {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
        0, 2, 4, 6, 8, 1, 3, 5, 7, 9,
    };

    MockAvb mockService;

    auto unlock = std::make_unique<CarrierUnlock>();
    unlock->set_version(version);
    unlock->set_nonce(nonce);
    unlock->set_signature(token.data(), token.size());
    CarrierUnlockRequest request;
    request.set_allocated_token(unlock.release());
    EXPECT_CALL(mockService, CarrierUnlock(CarrierUnlockEq(request), _))
            .WillOnce(Return(APP_SUCCESS));

    OemLock hal{mockService};
    ASSERT_THAT(hal.setOemUnlockAllowedByCarrier(true, makeSignature(version, nonce, token)),
                Eq(OemLockSecureStatus::OK));
}

TEST(OemLockHalTest, setOemUnlockAllowedByCarrierInvalidSignatureIfUnauthorizedByApp) {
    MockAvb mockService;

    EXPECT_CALL(mockService, CarrierUnlock(_, _)).WillOnce(Return(APP_ERROR_AVB_AUTHORIZATION));

    OemLock hal{mockService};
    ASSERT_THAT(hal.setOemUnlockAllowedByCarrier(true, makeSignature(1, 2, {3, 4, 5})),
                Eq(OemLockSecureStatus::INVALID_SIGNATURE));
}

TEST(OemLockHalTest, setOemUnlockAllowedByCarrierFailsOnAvbError) {
    MockAvb mockService;

    EXPECT_CALL(mockService, CarrierUnlock(_, _)).WillOnce(Return(APP_ERROR_INTERNAL));

    OemLock hal{mockService};
    ASSERT_THAT(hal.setOemUnlockAllowedByCarrier(true, makeSignature(1, 2, {3, 4, 5})),
                Eq(OemLockSecureStatus::FAILED));
}

TEST(OemLockHalTest, setOemUnlockAllowedByCarrierShortSignatureIsInvalid) {
    MockAvb mockService;
    OemLock hal{mockService};
    ASSERT_THAT(hal.setOemUnlockAllowedByCarrier(true, hidl_vec<uint8_t>{}),
                Eq(OemLockSecureStatus::INVALID_SIGNATURE));
}

TEST(OemLockHalTest, setOemUnlockAllowedByCarrierFailsToDisallow) {
    MockAvb mockService;
    OemLock hal{mockService};
    ASSERT_THAT(hal.setOemUnlockAllowedByCarrier(false, hidl_vec<uint8_t>{}),
                Eq(OemLockSecureStatus::FAILED));
}

// isOemUnlockAllowedByCarrier

MATCHER_P(GetLockEq, msg, "") {
    return arg.lock() == msg.lock();
}

TEST(OemLockHalTest, isOemUnlockAllowedByCarrierChecksCarrierLock) {
    MockAvb mockService;

    GetLockRequest request;
    request.set_lock(LockIndex::CARRIER);
    EXPECT_CALL(mockService, GetLock(GetLockEq(request), _)).WillOnce(Return(APP_ERROR_RPC));

    OemLock hal{mockService};
    hal.isOemUnlockAllowedByCarrier([](OemLockStatus status, bool allowed) {
        ASSERT_THAT(status, Eq(OemLockStatus::FAILED));
        (void) allowed;
    });
}

TEST(OemLockHalTest, isOemUnlockAllowedByCarrierWhenAllowed) {
    MockAvb mockService;

    GetLockResponse response;
    response.set_locked(0);
    EXPECT_CALL(mockService, GetLock(_, _))
            .WillOnce(DoAll(SetArgPointee<1>(response), Return(APP_SUCCESS)));

    OemLock hal{mockService};
    hal.isOemUnlockAllowedByCarrier([](OemLockStatus status, bool allowed) {
        ASSERT_THAT(status, Eq(OemLockStatus::OK));
        ASSERT_TRUE(allowed);
    });
}

TEST(OemLockHalTest, isOemUnlockAllowedByCarrierWhenNotAllowed) {
    MockAvb mockService;

    GetLockResponse response;
    response.set_locked(1);
    EXPECT_CALL(mockService, GetLock(_, _))
            .WillOnce(DoAll(SetArgPointee<1>(response), Return(APP_SUCCESS)));

    OemLock hal{mockService};
    hal.isOemUnlockAllowedByCarrier([](OemLockStatus status, bool allowed) {
        ASSERT_THAT(status, Eq(OemLockStatus::OK));
        ASSERT_FALSE(allowed);
    });
}

TEST(OemLockHalTest, isOemUnlockAllowedByCarrierFailsonAvbError) {
    MockAvb mockService;

    EXPECT_CALL(mockService, GetLock(_, _)).WillOnce(Return(APP_ERROR_INTERNAL));

    OemLock hal{mockService};
    hal.isOemUnlockAllowedByCarrier([](OemLockStatus status, bool allowed) {
        ASSERT_THAT(status, Eq(OemLockStatus::FAILED));
        (void) allowed;
    });
}

// setOemUnlockAllowedByDevice

MATCHER_P(SetDeviceLockEq, msg, "") {
    return arg.locked() == msg.locked();
}

TEST(OemLockHalTest, setOemUnlockAllowedByDevice) {
    MockAvb mockService;

    SetDeviceLockRequest request;
    request.set_locked(0);
    EXPECT_CALL(mockService, SetDeviceLock(SetDeviceLockEq(request), _))
            .WillOnce(Return(APP_SUCCESS));

    OemLock hal{mockService};
    ASSERT_THAT(hal.setOemUnlockAllowedByDevice(true), Eq(OemLockStatus::OK));
}

TEST(OemLockHalTest, setOemUnlockNotAllowedByDevice) {
    MockAvb mockService;

    SetDeviceLockRequest request;
    request.set_locked(1);
    EXPECT_CALL(mockService, SetDeviceLock(SetDeviceLockEq(request), _))
            .WillOnce(Return(APP_SUCCESS));

    OemLock hal{mockService};
    ASSERT_THAT(hal.setOemUnlockAllowedByDevice(false), Eq(OemLockStatus::OK));
}

TEST(OemLockHalTest, setOemUnlockAllowedByDeviceFailsOnAppError) {
    MockAvb mockService;

    EXPECT_CALL(mockService, SetDeviceLock(_, _)).WillOnce(Return(APP_ERROR_INTERNAL));

    OemLock hal{mockService};
    ASSERT_THAT(hal.setOemUnlockAllowedByDevice(true), Eq(OemLockStatus::FAILED));
}

// isOemUnlockAllowedByDevice

TEST(OemLockHalTest, isOemUnlockAllowedByDeviceChecksDeviceLock) {
    MockAvb mockService;

    GetLockRequest request;
    request.set_lock(LockIndex::DEVICE);
    EXPECT_CALL(mockService, GetLock(GetLockEq(request), _)).WillOnce(Return(APP_ERROR_RPC));

    OemLock hal{mockService};
    hal.isOemUnlockAllowedByDevice([](OemLockStatus status, bool allowed) {
        ASSERT_THAT(status, Eq(OemLockStatus::FAILED));
        (void) allowed;
    });
}

TEST(OemLockHalTest, isOemUnlockAllowedByDeviceWhenAllowed) {
    MockAvb mockService;

    GetLockResponse response;
    response.set_locked(0);
    EXPECT_CALL(mockService, GetLock(_, _))
            .WillOnce(DoAll(SetArgPointee<1>(response), Return(APP_SUCCESS)));

    OemLock hal{mockService};
    hal.isOemUnlockAllowedByDevice([](OemLockStatus status, bool allowed) {
        ASSERT_THAT(status, Eq(OemLockStatus::OK));
        ASSERT_TRUE(allowed);
    });
}

TEST(OemLockHalTest, isOemUnlockAllowedByDeviceWhenNotAllowed) {
    MockAvb mockService;

    GetLockResponse response;
    response.set_locked(1);
    EXPECT_CALL(mockService, GetLock(_, _))
            .WillOnce(DoAll(SetArgPointee<1>(response), Return(APP_SUCCESS)));

    OemLock hal{mockService};
    hal.isOemUnlockAllowedByDevice([](OemLockStatus status, bool allowed) {
        ASSERT_THAT(status, Eq(OemLockStatus::OK));
        ASSERT_FALSE(allowed);
    });
}

TEST(OemLockHalTest, isOemUnlockAllowedByDeviceFailsonAvbError) {
    MockAvb mockService;

    EXPECT_CALL(mockService, GetLock(_, _)).WillOnce(Return(APP_ERROR_INTERNAL));

    OemLock hal{mockService};
    hal.isOemUnlockAllowedByDevice([](OemLockStatus status, bool allowed) {
        ASSERT_THAT(status, Eq(OemLockStatus::FAILED));
        (void) allowed;
    });
}

// carrierUnlockFromSignature

TEST(OemLockHalTest, carrierUnlockFromSignatureParsesFields) {
    constexpr uint64_t version = 0x1234567890abcdef;
    constexpr uint64_t nonce = 0x24680ace13579bdf;
    const std::vector<uint8_t> token {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
        0, 2, 4, 6, 8, 1, 3, 5, 7, 9,
    };

    const auto signature = makeSignature(version, nonce, token);

    CarrierUnlock unlock;
    ASSERT_TRUE(OemLock::carrierUnlockFromSignature(signature, &unlock));
    EXPECT_THAT(unlock.version(), Eq(version));
    EXPECT_THAT(unlock.nonce(), Eq(nonce));
    EXPECT_THAT(unlock.signature().size(), Eq(token.size()));
    EXPECT_THAT(unlock.signature(), ElementsAreArray(token));
}

TEST(OemLockHalTest, carrierUnlockFromSignatureCrashesOnNullptr) {
    hidl_vec<uint8_t> signature(sizeof(uint64_t) * 2);
    ASSERT_DEATH(OemLock::carrierUnlockFromSignature(signature, nullptr), "");
}

TEST(OemLockHalTest, carrierUnlockFromSignatureFailsOnShortSignature) {
    hidl_vec<uint8_t> signature((sizeof(uint64_t) * 2) - 1);
    CarrierUnlock unlock;
    ASSERT_FALSE(OemLock::carrierUnlockFromSignature(signature, &unlock));
}
