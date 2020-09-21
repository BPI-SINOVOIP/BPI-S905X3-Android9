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

#include "AuthSecret.h"

#include <android-base/logging.h>

#include <openssl/sha.h>

#include <application.h>
#include <app_nugget.h>
#include <nos/debug.h>

#include <AuthSecret.h>

namespace android {
namespace hardware {
namespace authsecret {

// libhidl
using ::android::hardware::Void;

namespace {

/** The digest is the first 4 bytes of the sha1 of the password */
uint32_t CalculatePasswordDigest(const nugget_app_password& password) {
    uint8_t passwordSha1[SHA_DIGEST_LENGTH];
    SHA_CTX c;
    SHA1_Init(&c);
    SHA1_Update(&c, &password.password, NUGGET_UPDATE_PASSWORD_LEN);
    SHA1_Final(passwordSha1, &c);

    uint32_t digest;
    memcpy(&digest, passwordSha1, sizeof(digest));
    return digest;
}

/*
 * Derive the update password from the secret:
 *     update_password = sha256( CITADEL_UPDATE_KEY | 0 | secret )
 *
 * Password must point to NUGGET_UPDATE_PASSWORD_LEN bytes.
 *
 * The password is derived in case the secret needs to be used for another
 * purpose. Deriving different values for each subsystem adds a layer of
 * security.
 */
nugget_app_password DeriveCitadelUpdatePassword(const hidl_vec<uint8_t>& secret) {
    nugget_app_password password;

    static_assert(SHA256_DIGEST_LENGTH == NUGGET_UPDATE_PASSWORD_LEN,
                  "Hash output does not match update password length");
    const uint8_t CITADEL_UPDATE_KEY[] = "citadel_update";
    SHA256_CTX c;
    SHA256_Init(&c);
    SHA256_Update(&c, CITADEL_UPDATE_KEY, sizeof(CITADEL_UPDATE_KEY));
    SHA256_Update(&c, secret.data(), secret.size());
    SHA256_Final(password.password, &c);
    password.digest = CalculatePasswordDigest(password);
    return password;
}

/**
 * The first time this is called, Citadel won't have its update password set so
 * always try and enroll it. If the update password is already set, this will
 * faily gracefully.
 */
void TryEnrollCitadelUpdatePassword(NuggetClientInterface& client,
                                    const nugget_app_password& password) {
    std::vector<uint8_t> buffer(sizeof(nugget_app_change_update_password));
    nugget_app_change_update_password* const msg
            = reinterpret_cast<nugget_app_change_update_password*>(buffer.data());

    msg->new_password = password;
    // Default password is uninitialized flash i.e. all 0xff
    memset(&msg->old_password.password, 0xff, NUGGET_UPDATE_PASSWORD_LEN);
    msg->old_password.digest = 0xffffffff;

    const uint32_t appStatus = client.CallApp(APP_ID_NUGGET,
                                              NUGGET_PARAM_CHANGE_UPDATE_PASSWORD,
                                              buffer, nullptr);
    if (appStatus == APP_ERROR_BOGUS_ARGS) {
        LOG(VERBOSE) << "Citadel update password already installed";
        return;
    }
    if (appStatus != APP_SUCCESS) {
        LOG(ERROR) << "Citadel change update password failed: "
                   << ::nos::StatusCodeString(appStatus) << " (" << appStatus << ")";
        return;
    }

    LOG(INFO) << "Citadel update password installed";
}

/** Request a hard reboot of Citadel. */
void RebootCitadel(NuggetClientInterface& client) {
    std::vector<uint8_t> ignored = {1};         // older images require this
    const uint32_t appStatus = client.CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT, ignored, nullptr);
    if (appStatus != APP_SUCCESS) {
        LOG(ERROR) << "Citadel failed to reboot: " << ::nos::StatusCodeString(appStatus)
                   << " (" << appStatus << ")";
    }
}

/**
 * Try to enable a Citadel update with the update password. If this fails,
 * something has gone wrong somewhere...
 */
void TryEnablingCitadelUpdate(NuggetClientInterface& client, const nugget_app_password& password) {
    std::vector<uint8_t> buffer(sizeof(nugget_app_enable_update));
    nugget_app_enable_update* const msg
            = reinterpret_cast<nugget_app_enable_update*>(buffer.data());

    msg->password = password;
    msg->which_headers = NUGGET_ENABLE_HEADER_RW | NUGGET_ENABLE_HEADER_RO;
    const uint32_t appStatus = client.CallApp(APP_ID_NUGGET,
                                              NUGGET_PARAM_ENABLE_UPDATE,
                                              buffer, &buffer);
    if (appStatus == APP_ERROR_BOGUS_ARGS) {
        LOG(ERROR) << "Incorrect Citadel update password";
        return;
    }
    if (appStatus != APP_SUCCESS) {
        LOG(ERROR) << "Citadel enable update failed: " << ::nos::StatusCodeString(appStatus)
                   << " (" << appStatus << ")";
        return;
    }

    // If the header[s] changed, reboot for the update to take effect
    // Old firmware doesn't have a reply but still needs to be updated
    if (buffer.empty() || buffer[0]) {
      LOG(INFO) << "Update password enabled a new image; rebooting Citadel";
      RebootCitadel(client);
    }
}

} // namespace

// Methods from ::android::hardware::authsecret::V1_0::IAuthSecret follow.
Return<void> AuthSecret::primaryUserCredential(const hidl_vec<uint8_t>& secret) {
    LOG(VERBOSE) << "Running AuthSecret::primaryUserCredential";

    if (secret.size() < 16) {
        LOG(WARNING) << "The secret is shorter than 16 bytes";
    }

    const nugget_app_password password = DeriveCitadelUpdatePassword(secret);
    TryEnrollCitadelUpdatePassword(_client, password);
    TryEnablingCitadelUpdate(_client, password);

    return Void();
}

} // namespace authsecret
} // namespace hardware
} // namespace android
