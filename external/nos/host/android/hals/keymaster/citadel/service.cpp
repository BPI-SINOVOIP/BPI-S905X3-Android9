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

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/StrongPointer.h>

#include <application.h>
#include <nos/AppClient.h>
#include <nos/CitadeldProxyClient.h>
#include <nos/debug.h>

#include <KeymasterDevice.h>
#include <Keymaster.client.h>

#ifdef ENABLE_QCOM_OTF_PROVISIONING
#include <KeymasterKeyProvision.h>
#endif
#include <nugget/app/keymaster/keymaster.pb.h>
#include "../proto_utils.h"

using ::android::OK;
using ::android::sp;
using ::android::status_t;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;

using ::android::hardware::keymaster::KeymasterDevice;
using ::android::hardware::keymaster::translate_error_code;

using ::nos::CitadeldProxyClient;
using ::nos::AppClient;
using ::nugget::app::keymaster::ProvisionPresharedSecretRequest;
using ::nugget::app::keymaster::ProvisionPresharedSecretResponse;
using ::nugget::app::keymaster::PresharedSecretStatus;
using ErrorCodeNos = ::nugget::app::keymaster::ErrorCode;
using ::android::hardware::keymaster::V4_0::ErrorCode;

using KeymasterClient = ::nugget::app::keymaster::Keymaster;

#ifdef ENABLE_QCOM_OTF_PROVISIONING
// TODO(ngm): move this code into the HAL implementation.
static bool alreadyProvisioned(KeymasterClient *keymasterClient) {
    ProvisionPresharedSecretRequest request;
    ProvisionPresharedSecretResponse response;
    request.set_get_status(true);

    const uint32_t status = keymasterClient->ProvisionPresharedSecret(
        request, &response);
    if (status != APP_SUCCESS) {
        LOG(ERROR) << "KM ProvisionPresharedSecret() failed with status: "
                   << ::nos::StatusCodeString(status);
        return false;
    }
    const ErrorCode error_code = translate_error_code(response.error_code());
    if (error_code != ErrorCode::OK) {
        LOG(ERROR) << "ProvisionPresharedSecret() response error code: "
                   << toString(error_code);
        return false;
    }
    if (response.status() == PresharedSecretStatus::ALREADY_SET) {
        LOG(INFO) << "Preshared key previously established";
        return true;
    }

    return false;
}

// TODO(ngm): move this code into the HAL implementation.
static bool maybeProvision(KeymasterClient *keymasterClient) {
    if (alreadyProvisioned(keymasterClient)) {
        return true;
    }

    // Attempt to provision the preshared-secret.
    keymasterdevice::KeymasterKeyProvision qc_km_provisioning;
    int result = qc_km_provisioning.KeyMasterProvisionInit();
    if (result) {
        LOG(ERROR) << "KeyMasterProvisionInit error: " << result;
        return false;
    }
    std::vector<uint8_t> preshared_secret(32);
    result = qc_km_provisioning.GetPreSharedSecret(
        preshared_secret.data(), preshared_secret.size());
    if (result != KM_ERROR_OK) {
        LOG(ERROR) << "GetPreSharedSecret error: " << result;
        return false;
    }

    ProvisionPresharedSecretRequest request;
    ProvisionPresharedSecretResponse response;
    request.set_get_status(false);
    request.set_preshared_secret(preshared_secret.data(),
                                 preshared_secret.size());
    const uint32_t status = keymasterClient->ProvisionPresharedSecret(
            request, &response);
    if (status != APP_SUCCESS) {
        LOG(ERROR) << "KM ProvisionPresharedSecret() failed with status: "
                   << ::nos::StatusCodeString(status);
        return false;
    }
    if (response.error_code() != ErrorCodeNos::OK) {
        LOG(ERROR) << "KM ProvisionPresharedSecret() failed with code: "
                   << response.error_code();
        return false;
    }

    return true;
}
#else
static bool maybeProvision(KeymasterClient *) {
    return true;
}
#endif

int main() {
    LOG(INFO) << "Keymaster HAL service starting";

    // Connect to citadeld
    CitadeldProxyClient citadeldProxy;
    citadeldProxy.Open();
    if (!citadeldProxy.IsOpen()) {
        LOG(FATAL) << "Failed to open citadeld client";
    }

    // This thread will become the only thread of the daemon
    constexpr bool thisThreadWillJoinPool = true;
    configureRpcThreadpool(1, thisThreadWillJoinPool);

    // Start the HAL service
    KeymasterClient keymasterClient{citadeldProxy};
    sp<KeymasterDevice> keymaster = new KeymasterDevice{keymasterClient};

    if (maybeProvision(&keymasterClient)) {
      const status_t status = keymaster->registerAsService("strongbox");
      if (status != OK) {
        LOG(FATAL) << "Failed to register Keymaster as a service (status: "
                   << status << ")";
      }
    } else {
        LOG(ERROR) << "Skipping Keymaster registration, as provisioning failed";
        // Leave the service running to avoid being restarted.
    }

    joinRpcThreadpool();
    return -1; // Should never be reached
}
