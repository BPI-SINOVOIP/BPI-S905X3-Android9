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

#define LOG_TAG "CarPowerManagerNative"

#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Log.h>

#include "CarPowerManager.h"

namespace android {
namespace car {
namespace hardware {
namespace power {

// Public functions
int CarPowerManager::clearListener() {
    int retVal = -1;

    if (mIsConnected && (mListenerToService != nullptr)) {
        mICarPower->unregisterListener(mListenerToService);
        mListenerToService = nullptr;
        retVal = 0;
    }
    return retVal;
}

int CarPowerManager::getBootReason(BootReason *bootReason) {
    int retVal = -1;

    if ((bootReason != nullptr) && connectToCarService()) {
        int reason = -1;
        mICarPower->getBootReason(&reason);

        if ((reason >= static_cast<int>(BootReason::kFirst)) &&
            (reason <= static_cast<int>(BootReason::kLast))) {
            *bootReason = static_cast<BootReason>(reason);
            retVal = 0;
        } else {
            ALOGE("Received unknown bootReason = %d", reason);
        }
    }
    return retVal;
}

int CarPowerManager::requestShutdownOnNextSuspend() {
    int retVal = -1;

    if (connectToCarService()) {
        mICarPower->requestShutdownOnNextSuspend();
        retVal = 0;
    }
    return retVal;
}

int CarPowerManager::setListener(Listener listener) {
    int retVal = -1;

    if (connectToCarService()) {
        if (mListenerToService == nullptr) {
            mListenerToService = new CarPowerStateListener(this);
            mICarPower->registerListener(mListenerToService);
        }

        // Set the listener
        mListener = listener;
        retVal = 0;
    }
    return retVal;
}


// Private functions
bool CarPowerManager::connectToCarService() {
    if (mIsConnected) {
        // Service is already connected
        return true;
    }

    const String16 ICarName("car_service");
    const String16 ICarPowerName("power");

    ALOGI("Connecting to CarService" LOG_TAG);

    // Get ICar
    sp<IServiceManager> serviceManager = defaultServiceManager();
    if (serviceManager == nullptr) {
        ALOGE("Cannot get defaultServiceManager");
        return(false);
    }

    sp<IBinder> binder = (serviceManager->getService(ICarName));
    if (binder == nullptr) {
        ALOGE("Cannot get ICar");
        return false;
    }

    // Get ICarPower
    sp<ICar> iCar = interface_cast<ICar>(binder);
    if (iCar == nullptr) {
        ALOGW("car service unavailable");
        return false;
    }

    iCar->getCarService(ICarPowerName, &binder);
    if (binder == nullptr) {
        ALOGE("Cannot get ICarPower");
        return false;
    }

    mICarPower = interface_cast<ICarPower>(binder);
    if (mICarPower == nullptr) {
        ALOGW("car power management service unavailable");
        return false;
    }
    mIsConnected = true;
    return true;
}


} // namespace power
} // namespace hardware
} // namespace car
} // namespace android

