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

#include <MockWeaver.client.h>
#include <Weaver.h>

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::Pointwise;
using ::testing::Return;
using ::testing::SetArgPointee;

using ::android::hardware::hidl_vec;
using ::android::hardware::weaver::Weaver;
using ::android::hardware::weaver::V1_0::WeaverConfig;
using ::android::hardware::weaver::V1_0::WeaverReadResponse;
using ::android::hardware::weaver::V1_0::WeaverReadStatus;
using ::android::hardware::weaver::V1_0::WeaverStatus;

using ::nugget::app::weaver::MockWeaver;
using ::nugget::app::weaver::GetConfigResponse;
using ::nugget::app::weaver::WriteResponse;
using ::nugget::app::weaver::ReadResponse;

// GetConfig

TEST(WeaverHalTest, getConfigReturnsValuesFromApp) {
    MockWeaver mockService;

    GetConfigResponse response;
    response.set_number_of_slots(57);
    response.set_key_size(467);
    response.set_value_size(9);
    EXPECT_CALL(mockService, GetConfig(_, _))
            .WillOnce(DoAll(SetArgPointee<1>(response), Return(APP_SUCCESS)));

    Weaver hal{mockService};
    hal.getConfig([&](WeaverStatus status, WeaverConfig config) {
        EXPECT_THAT(status, Eq(WeaverStatus::OK));
        EXPECT_THAT(config.slots, Eq(response.number_of_slots()));
        EXPECT_THAT(config.keySize, Eq(response.key_size()));
        EXPECT_THAT(config.valueSize, Eq(response.value_size()));
    });
}

TEST(WeaverHalTest, getConfigFailsOnAppFailure) {
    MockWeaver mockService;

    EXPECT_CALL(mockService, GetConfig(_, _)).WillOnce(Return(APP_ERROR_INTERNAL));

    Weaver hal{mockService};
    hal.getConfig([](WeaverStatus status, WeaverConfig config) {
        EXPECT_THAT(status, Eq(WeaverStatus::FAILED));
        (void) config;
    });
}

TEST(WeaverHalTest, getConfigFailsOnRpcFailure) {
    MockWeaver mockService;

    EXPECT_CALL(mockService, GetConfig(_, _)).WillOnce(Return(APP_ERROR_RPC));

    Weaver hal{mockService};
    hal.getConfig([](WeaverStatus status, WeaverConfig config) {
        EXPECT_THAT(status, Eq(WeaverStatus::FAILED));
        (void) config;
    });
}

// Write

MATCHER_P3(WriteRequestFor, slot, key, value, "") {
    return arg.slot() == slot
            && std::equal(std::begin(arg.key()), std::end(arg.key()),
                          std::begin(key), std::end(key))
            && std::equal(std::begin(arg.value()), std::end(arg.value()),
                          std::begin(value), std::end(value));
}

TEST(WeaverHalTest, writeForwardsArgumentsToApp) {
    MockWeaver mockService;

    constexpr uint32_t slot = 93;
    const hidl_vec<uint8_t> key{1, 2, 3, 4, 5};
    const hidl_vec<uint8_t> value{9, 8, 7, 6};

    EXPECT_CALL(mockService, Write(WriteRequestFor(slot, key, value), _))
            .WillOnce(Return(APP_SUCCESS));

    Weaver hal{mockService};
    EXPECT_THAT(hal.write(slot, key, value), Eq(WeaverStatus::OK));
}

TEST(WeaverHalTest, writeSucceedsIfAppSucceeds) {
    MockWeaver mockService;

    EXPECT_CALL(mockService, Write(_, _)).WillOnce(Return(APP_SUCCESS));

    Weaver hal{mockService};
    EXPECT_THAT(hal.write(0, hidl_vec<uint8_t>{}, hidl_vec<uint8_t>{}), Eq(WeaverStatus::OK));
}

TEST(WeaverHalTest, writeFailsOnAppFailure) {
    MockWeaver mockService;

    EXPECT_CALL(mockService, Write(_, _)).WillOnce(Return(APP_ERROR_INTERNAL));

    Weaver hal{mockService};
    EXPECT_THAT(hal.write(0, hidl_vec<uint8_t>{}, hidl_vec<uint8_t>{}), Eq(WeaverStatus::FAILED));
}

TEST(WeaverHalTest, writeFailsOnRpcFailure) {
    MockWeaver mockService;

    EXPECT_CALL(mockService, Write(_, _)).WillOnce(Return(APP_ERROR_RPC));

    Weaver hal{mockService};
    EXPECT_THAT(hal.write(0, hidl_vec<uint8_t>{}, hidl_vec<uint8_t>{}), Eq(WeaverStatus::FAILED));
}

// Read

TEST(WeaverHalTest, readSucceedsIfAppSucceeds) {
    MockWeaver mockService;

    ReadResponse response;
    response.set_error(ReadResponse::NONE);
    response.set_value("turn me into bytes");
    EXPECT_CALL(mockService, Read(_, _))
            .WillOnce(DoAll(SetArgPointee<1>(response), Return(APP_SUCCESS)));

    Weaver hal{mockService};
    hal.read(0, hidl_vec<uint8_t>{}, [&](WeaverReadStatus status, WeaverReadResponse r) {
        EXPECT_THAT(status, Eq(WeaverReadStatus::OK));
        EXPECT_THAT(r.timeout, Eq(0u));
        EXPECT_THAT(static_cast<std::vector<uint8_t>>(r.value), Pointwise(Eq(), response.value()));
    });
}

TEST(WeaverHalTest, readPropagatesWrongKey) {
    MockWeaver mockService;

    ReadResponse response;
    response.set_error(ReadResponse::WRONG_KEY);
    response.set_throttle_msec(5649);
    EXPECT_CALL(mockService, Read(_, _))
            .WillOnce(DoAll(SetArgPointee<1>(response), Return(APP_SUCCESS)));

    Weaver hal{mockService};
    hal.read(0, hidl_vec<uint8_t>{}, [&](WeaverReadStatus status, WeaverReadResponse r) {
        EXPECT_THAT(status, Eq(WeaverReadStatus::INCORRECT_KEY));
        EXPECT_THAT(r.timeout, Eq(response.throttle_msec()));
        EXPECT_THAT(r.value.size(), Eq(0u));
    });
}

TEST(WeaverHalTest, readPropagatesThrottle) {
    MockWeaver mockService;

    ReadResponse response;
    response.set_error(ReadResponse::THROTTLE);
    response.set_throttle_msec(946);
    EXPECT_CALL(mockService, Read(_, _))
            .WillOnce(DoAll(SetArgPointee<1>(response), Return(APP_SUCCESS)));

    Weaver hal{mockService};
    hal.read(0, hidl_vec<uint8_t>{}, [&](WeaverReadStatus status, WeaverReadResponse r) {
        EXPECT_THAT(status, Eq(WeaverReadStatus::THROTTLE));
        EXPECT_THAT(r.timeout, Eq(response.throttle_msec()));
        EXPECT_THAT(r.value.size(), Eq(0u));
    });
}

TEST(WeaverHalTest, readFailsOnInvalidErrorCode) {
    MockWeaver mockService;

    ReadResponse response;
    response.set_error(static_cast<::nugget::app::weaver::ReadResponse_Error>(9999));
    EXPECT_CALL(mockService, Read(_, _))
            .WillOnce(DoAll(SetArgPointee<1>(response), Return(APP_SUCCESS)));

    Weaver hal{mockService};
    hal.read(0, hidl_vec<uint8_t>{}, [](WeaverReadStatus status, WeaverReadResponse r) {
        EXPECT_THAT(status, Eq(WeaverReadStatus::FAILED));
        EXPECT_THAT(r.timeout, Eq(0u));
        EXPECT_THAT(r.value.size(), Eq(0u));
    });
}

TEST(WeaverHalTest, readFailsOnAppFailure) {
    MockWeaver mockService;

    EXPECT_CALL(mockService, Read(_, _)).WillOnce(Return(APP_ERROR_INTERNAL));

    Weaver hal{mockService};
    hal.read(0, hidl_vec<uint8_t>{}, [](WeaverReadStatus status, WeaverReadResponse r) {
        EXPECT_THAT(status, Eq(WeaverReadStatus::FAILED));
        EXPECT_THAT(r.timeout, Eq(0u));
        EXPECT_THAT(r.value.size(), Eq(0u));
    });
}

TEST(WeaverHalTest, readFailsOnRpcFailure) {
    MockWeaver mockService;

    EXPECT_CALL(mockService, Read(_, _)).WillOnce(Return(APP_ERROR_RPC));

    Weaver hal{mockService};
    hal.read(0, hidl_vec<uint8_t>{}, [](WeaverReadStatus status, WeaverReadResponse r) {
        EXPECT_THAT(status, Eq(WeaverReadStatus::FAILED));
        EXPECT_THAT(r.timeout, Eq(0u));
        EXPECT_THAT(r.value.size(), Eq(0u));
    });
}
