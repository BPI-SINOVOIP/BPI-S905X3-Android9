/*
 * Copyright 2018 The Android Open Source Project
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
package com.android.bluetooth.hfp;

/**
 * A blob of data representing AG's device state in response to an AT+CIND command from HF
 */
class HeadsetDeviceState extends HeadsetMessageObject {
    /**
     * Service availability indicator
     *
     * 0 - no service, no home/roam network is available
     * 1 - presence of service, home/roam network available
     */
    int mService;
    /**
     * Roaming status indicator
     *
     * 0 - roaming is not active
     * 1 - roaming is active
     */
    int mRoam;
    /**
     * Signal strength indicator, value ranges from 0 to 5
     */
    int mSignal;
    /**
     * Battery charge indicator from AG, value ranges from 0 to 5
     */
    int mBatteryCharge;

    HeadsetDeviceState(int service, int roam, int signal, int batteryCharge) {
        mService = service;
        mRoam = roam;
        mSignal = signal;
        mBatteryCharge = batteryCharge;
    }

    @Override
    public void buildString(StringBuilder builder) {
        if (builder == null) {
            return;
        }
        builder.append(this.getClass().getSimpleName())
                .append("[hasCellularService=")
                .append(mService)
                .append(", isRoaming=")
                .append(mRoam)
                .append(", signalStrength")
                .append(mSignal)
                .append(", batteryCharge=")
                .append(mBatteryCharge)
                .append("]");
    }
}
