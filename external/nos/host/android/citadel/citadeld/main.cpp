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

#include <limits>
#include <thread>
#include <mutex>

#include <android-base/logging.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include <nos/device.h>
#include <nos/NuggetClient.h>

#include <android/hardware/citadel/BnCitadeld.h>

using ::android::OK;
using ::android::defaultServiceManager;
using ::android::sp;
using ::android::status_t;
using ::android::IPCThreadState;
using ::android::IServiceManager;
using ::android::ProcessState;

using ::android::binder::Status;

using ::nos::NuggetClient;

using ::android::hardware::citadel::BnCitadeld;
using ::android::hardware::citadel::ICitadeld;

namespace {

class CitadelProxy : public BnCitadeld {
public:
    CitadelProxy(NuggetClient& client) : _client{client} {}
    ~CitadelProxy() override = default;

    Status callApp(const int32_t _appId, const int32_t _arg, const std::vector<uint8_t>& request,
                   std::vector<uint8_t>* const response, int32_t* const _aidl_return) override {
        // AIDL doesn't support integers less than 32-bit so validate it before casting
        if (_appId < 0 || _appId > kMaxAppId) {
            LOG(ERROR) << "App ID " << _appId << " is outside the app ID range";
            return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        }
        if (_arg < 0 || _arg > std::numeric_limits<uint16_t>::max()) {
            LOG(ERROR) << "Argument " << _arg << " is outside the unsigned 16-bit range";
            return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        }

        const uint8_t appId = static_cast<uint32_t>(_appId);
        const uint16_t arg =  static_cast<uint16_t>(_arg);
        uint32_t* const appStatus = reinterpret_cast<uint32_t*>(_aidl_return);

        // Make the call to the app while holding the lock for that app
        std::unique_lock<std::mutex> lock(_appLocks[appId]);
        *appStatus = _client.CallApp(appId, arg, request, response);
        return Status::ok();
    }

    Status reset(bool* const _aidl_return) override {
        // This doesn't use the transport API to talk to any app so doesn't need
        // to hold any app locks.
        const nos_device& device = *_client.Device();
        *_aidl_return = (device.ops.reset(device.ctx) == 0);
        return Status::ok();
    }

private:
    static constexpr auto kMaxAppId = std::numeric_limits<uint8_t>::max();

    NuggetClient& _client;
    std::mutex _appLocks[kMaxAppId + 1];
};

[[noreturn]] void CitadelEventDispatcher(const nos_device& device) {
    LOG(INFO) << "Event dispatcher startup.";
    while(1) {
        if (device.ops.wait_for_interrupt(device.ctx, -1) > 0) {
            LOG(INFO) << "Citadel has dispatched an event";
        } else {
            LOG(INFO) << "Citadel did something unexpected";
        }

        // This is a placeholder for the message handling that gives citadel a
        // chance to deassert CTLD_AP_IRQ, so this doesn't spam the logs.
        // TODO(b/62713383) Replace this with the code to contact citadel.
        sleep(1);
    }
}

} // namespace

int main() {
    LOG(INFO) << "Starting citadeld";

    // Connect to Citadel
    NuggetClient citadel;
    citadel.Open();
    if (!citadel.IsOpen()) {
        LOG(FATAL) << "Failed to open Citadel client";
    }

    // Citadel HALs will communicate with this daemon via /dev/vndbinder as this
    // is vendor code
    ProcessState::initWithDriver("/dev/vndbinder");

    sp<CitadelProxy> proxy = new CitadelProxy(citadel);
    const status_t status = defaultServiceManager()->addService(ICitadeld::descriptor, proxy);
    if (status != OK) {
        LOG(FATAL) << "Failed to register citadeld as a service (status " << status << ")";
    }

    // Handle interrupts triggered by Citadel and dispatch any events to
    // registered listeners.
    std::thread event_dispatcher(CitadelEventDispatcher, *citadel.Device());

    // Start handling binder requests with multiple threads
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
