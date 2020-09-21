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
 * An object that represents AG indicator enable states
 */
public class HeadsetAgIndicatorEnableState extends HeadsetMessageObject {
    public boolean service;
    public boolean roam;
    public boolean signal;
    public boolean battery;

    HeadsetAgIndicatorEnableState(boolean newService, boolean newRoam, boolean newSignal,
            boolean newBattery) {
        service = newService;
        roam = newRoam;
        signal = newSignal;
        battery = newBattery;
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof HeadsetAgIndicatorEnableState)) {
            return false;
        }
        HeadsetAgIndicatorEnableState other = (HeadsetAgIndicatorEnableState) obj;
        return service == other.service && roam == other.roam && signal == other.signal
                && battery == other.battery;
    }

    @Override
    public int hashCode() {
        int result = 0;
        if (service) result += 1;
        if (roam) result += 2;
        if (signal) result += 4;
        if (battery) result += 8;
        return result;
    }

    @Override
    public void buildString(StringBuilder builder) {
        if (builder == null) {
            return;
        }
        builder.append(this.getClass().getSimpleName())
                .append("[service=")
                .append(service)
                .append(", roam=")
                .append(roam)
                .append(", signal=")
                .append(signal)
                .append(", battery=")
                .append(battery)
                .append("]");
    }
}
