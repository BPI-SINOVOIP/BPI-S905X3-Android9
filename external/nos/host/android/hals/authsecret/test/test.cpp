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

#include <gtest/gtest.h>

#include <application.h>
#include <app_nugget.h>
#include <nos/MockNuggetClient.h>

#include <AuthSecret.h>

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Return;

using ::android::hardware::hidl_vec;
using ::android::hardware::authsecret::AuthSecret;

using ::nos::MockNuggetClient;

uint8_t SECRET_STRING[] = "this_password_is_fake";
// sha256( "citadel_update" | 0 | secret )
const uint8_t EXPECTED_PASSWORD[NUGGET_UPDATE_PASSWORD_LEN] = {
    0x31, 0x96, 0xb6, 0x46, 0x49, 0x90, 0x17, 0xc2,
    0x08, 0x80, 0xa6, 0x4d, 0x3f, 0x2d, 0x50, 0x9a,
    0x3e, 0x9a, 0x25, 0xaa, 0xd7, 0xcc, 0x48, 0x5d,
    0xd0, 0x0b, 0x8c, 0xf3, 0xcc, 0x8f, 0xfe, 0x41,
};
// First 4 bytes of sha1 of the expectedPassword
constexpr uint32_t EXPECTED_PASSWORD_CHECKSUM = 0xdca3b96d;

// This is the Citadel update password if one hasn't been set
const uint8_t DEFUALT_PASSWORD[NUGGET_UPDATE_PASSWORD_LEN] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};
constexpr uint32_t DEFUALT_PASSWORD_CHECKSUM = 0xffffffff; // uninitialized flash

// primaryUserCredential

MATCHER_P2(OldPasswordEq, pwd, checksum, "") {
    const nugget_app_change_update_password* const msg
            = reinterpret_cast<const nugget_app_change_update_password*>(arg.data());
    return memcmp(msg->old_password.password, pwd, NUGGET_UPDATE_PASSWORD_LEN) == 0
           && msg->old_password.digest == checksum;
}

MATCHER_P2(NewPasswordEq, pwd, checksum, "") {
    const nugget_app_change_update_password* const msg
            = reinterpret_cast<const nugget_app_change_update_password*>(arg.data());
    return memcmp(msg->new_password.password, pwd, NUGGET_UPDATE_PASSWORD_LEN) == 0
            && msg->new_password.digest == checksum;
}

TEST(AuthSecretHalTest, changePasswordFromDefault) {
    MockNuggetClient client;
    AuthSecret hal{client};

    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_CHANGE_UPDATE_PASSWORD,
                                OldPasswordEq(DEFUALT_PASSWORD, DEFUALT_PASSWORD_CHECKSUM), _))
            .WillOnce(Return(APP_SUCCESS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_ENABLE_UPDATE, _, _))
            .WillOnce(Return(APP_SUCCESS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT, _, _))
            .WillOnce(Return(APP_SUCCESS));

    hidl_vec<uint8_t> secret;
    secret.setToExternal(SECRET_STRING, sizeof(SECRET_STRING) - 1);
    hal.primaryUserCredential(secret);
}

TEST(AuthSecretHalTest, changePasswordToDerived) {
    MockNuggetClient client;
    AuthSecret hal{client};

    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_CHANGE_UPDATE_PASSWORD,
                                NewPasswordEq(EXPECTED_PASSWORD, EXPECTED_PASSWORD_CHECKSUM), _))
            .WillOnce(Return(APP_SUCCESS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_ENABLE_UPDATE, _, _))
            .WillOnce(Return(APP_SUCCESS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT, _, _))
            .WillOnce(Return(APP_SUCCESS));

    hidl_vec<uint8_t> secret;
    secret.setToExternal(SECRET_STRING, sizeof(SECRET_STRING) - 1);
    hal.primaryUserCredential(secret);
}

TEST(AuthSecretHalTest, whenPasswordAlreadySetAlsoTryUpdateAndReboot) {
    MockNuggetClient client;
    AuthSecret hal{client};

    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_CHANGE_UPDATE_PASSWORD, _, _))
            .WillOnce(Return(APP_ERROR_BOGUS_ARGS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_ENABLE_UPDATE, _, _))
            .WillOnce(Return(APP_SUCCESS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT, _, _))
            .WillOnce(Return(APP_SUCCESS));

    hidl_vec<uint8_t> secret(19);
    hal.primaryUserCredential(secret);
}

MATCHER_P2(UpdatePasswordEq, pwd, checksum, "") {
    const nugget_app_enable_update* const msg
            = reinterpret_cast<const nugget_app_enable_update*>(arg.data());
    return memcmp(msg->password.password, pwd, NUGGET_UPDATE_PASSWORD_LEN) == 0
            && msg->password.digest == checksum;
}

MATCHER_P(WhichHeaderEq, headers, "") {
    const nugget_app_enable_update* const msg
            = reinterpret_cast<const nugget_app_enable_update*>(arg.data());
    return msg->which_headers == headers;
}

TEST(AuthSecretHalTest, attemptUpdateWithDerivedPassword) {
    MockNuggetClient client;
    AuthSecret hal{client};

    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_CHANGE_UPDATE_PASSWORD, _, _))
            .WillOnce(Return(APP_ERROR_BOGUS_ARGS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_ENABLE_UPDATE,
                                UpdatePasswordEq(EXPECTED_PASSWORD, EXPECTED_PASSWORD_CHECKSUM), _))
            .WillOnce(Return(APP_SUCCESS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT, _, _))
            .WillOnce(Return(APP_SUCCESS));

    hidl_vec<uint8_t> secret;
    secret.setToExternal(SECRET_STRING, sizeof(SECRET_STRING) - 1);
    hal.primaryUserCredential(secret);
}

TEST(AuthSecretHalTest, updateRwAndRoTogether) {
    MockNuggetClient client;
    AuthSecret hal{client};

    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_CHANGE_UPDATE_PASSWORD, _, _))
            .WillOnce(Return(APP_ERROR_BOGUS_ARGS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_ENABLE_UPDATE,
                                WhichHeaderEq(NUGGET_ENABLE_HEADER_RW | NUGGET_ENABLE_HEADER_RO),
                                _)).WillOnce(Return(APP_SUCCESS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT,
                                _, _)).WillOnce(Return(APP_SUCCESS));

    hidl_vec<uint8_t> secret(47);
    hal.primaryUserCredential(secret);
}

TEST(AuthSecretHalTest, citadelDoesNotLikePasswordSoDoNotReboot) {
    MockNuggetClient client;
    AuthSecret hal{client};

    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_CHANGE_UPDATE_PASSWORD, _, _))
            .WillOnce(Return(APP_ERROR_BOGUS_ARGS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_ENABLE_UPDATE, _, _))
            .WillOnce(Return(APP_ERROR_BOGUS_ARGS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT, _, _)).Times(0);

    hidl_vec<uint8_t> secret(20);
    hal.primaryUserCredential(secret);
    // Nothing should happen, in particular it shouldn't crash
}

TEST(AuthSecretHalTest, updateHitAnErrorSoDoNotReboot) {
    MockNuggetClient client;
    AuthSecret hal{client};

    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_CHANGE_UPDATE_PASSWORD, _, _))
            .WillOnce(Return(APP_ERROR_BOGUS_ARGS));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_ENABLE_UPDATE, _, _))
            .WillOnce(Return(APP_ERROR_INTERNAL));
    EXPECT_CALL(client, CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT, _, _)).Times(0);

    hidl_vec<uint8_t> secret(32);
    hal.primaryUserCredential(secret);
    // Nothing should happen, in particular it shouldn't crash
}
