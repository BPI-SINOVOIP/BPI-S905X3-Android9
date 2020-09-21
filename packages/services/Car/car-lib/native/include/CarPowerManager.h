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

#ifndef CAR_POWER_MANAGER
#define CAR_POWER_MANAGER

#include <binder/Status.h>
#include <utils/RefBase.h>

#include "android/car/ICar.h"
#include "android/car/hardware/power/BnCarPowerStateListener.h"
#include "android/car/hardware/power/ICarPower.h"

using android::binder::Status;

namespace android {
namespace car {
namespace hardware {
namespace power {


class CarPowerManager : public RefBase {
public:
    // Enumeration of possible boot reasons
    //  NOTE:  The entries in this enum must match the ones in located CarPowerManager java class:
    //      packages/services/Car/car-lib/src/android/car/hardware/power/CarPowerManager.java
    enum class BootReason {
        kUserPowerOn = 1,
        kDoorUnlock = 2,
        kTimer = 3,
        kDoorOpen = 4,
        kRemoteStart = 5,

        kFirst = kUserPowerOn,
        kLast = kRemoteStart,
    };

    // Enumeration of state change events
    //  NOTE:  The entries in this enum must match the ones in CarPowerStateListener located in
    //      packages/services/Car/car-lib/src/android/car/hardware/power/CarPowerManager.java
    enum class State {
        kShutdownCancelled = 0,
        kShutdownEnter = 1,
        kSuspendEnter = 2,
        kSuspendExit = 3,

        kFirst = kShutdownCancelled,
        kLast = kSuspendExit,
    };

    using Listener = std::function<void(State)>;

    CarPowerManager() = default;
    virtual ~CarPowerManager() = default;

    // Removes the listener and turns off callbacks
    //  Returns 0 on success
    int clearListener();

    // Returns the boot reason, defined in kBootReason*
    //  Returns 0 on success
    int getBootReason(BootReason *bootReason);

    // Request device to shutdown in lieu of suspend at the next opportunity
    //  Returns 0 on success
    int requestShutdownOnNextSuspend();

    // Set the callback function.  This will execute in the binder thread.
    //  Returns 0 on success
    int setListener(Listener listener);

private:
    class CarPowerStateListener final : public BnCarPowerStateListener {
    public:
        explicit CarPowerStateListener(CarPowerManager* parent) : mParent(parent) {};

        Status onStateChanged(int state, int token) override {
            sp<CarPowerManager> parent = mParent.promote();
            if ((parent != nullptr) && (parent->mListener != nullptr)) {
                if ((state >= static_cast<int>(State::kFirst)) &&
                    (state <= static_cast<int>(State::kLast))) {
                    parent->mListener(static_cast<State>(state));
                } else {
                    ALOGE("onStateChanged unknown state received = %d", state);
                }
            }

            if (token != 0) {
                // Call finished() method to let CPMS know that we're ready to suspend/shutdown.
                parent->mICarPower->finished(parent->mListenerToService, token);
            }
            return binder::Status::ok();
        };

    private:
        wp<CarPowerManager> mParent;
    };

    bool connectToCarService();

    sp<ICarPower> mICarPower;
    bool mIsConnected;
    Listener mListener;
    sp<CarPowerStateListener> mListenerToService;
};

} // namespace power
} // namespace hardware
} // namespace car
} // namespace android

#endif // CAR_POWER_MANAGER

