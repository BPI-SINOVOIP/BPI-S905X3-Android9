/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.tradefed.device;

import com.android.ddmlib.IDevice.DeviceState;

/**
 * A more fully featured representation of device state than {@link DeviceState}.
 * <p/>
 * Logically this should extend  {@link DeviceState} to just add the FASTBOOT and NOT_AVAILABLE
 * states, but extending enums is not allowed.
 */
public enum TestDeviceState {
    FASTBOOT,
    ONLINE,
    RECOVERY,
    NOT_AVAILABLE;

    /**
     * Converts from {@link TestDeviceState} to {@link DeviceState}
     * @return the {@link DeviceState} or <code>null</code>
     */
    DeviceState getDdmsState() {
        switch (this) {
            case ONLINE:
                return DeviceState.ONLINE;
            case RECOVERY:
                return DeviceState.RECOVERY;
            default:
                return null;
        }
    }

    /**
     * Returns the {@link TestDeviceState} corresponding to the {@link DeviceState}.
     */
    static TestDeviceState getStateByDdms(DeviceState ddmsState) {
        if (ddmsState == null) {
            return TestDeviceState.NOT_AVAILABLE;
        }

        switch (ddmsState) {
            case ONLINE:
                return TestDeviceState.ONLINE;
            case OFFLINE:
                return TestDeviceState.NOT_AVAILABLE;
            case RECOVERY:
                return TestDeviceState.RECOVERY;
            case UNAUTHORIZED:
                return TestDeviceState.NOT_AVAILABLE;
            case BOOTLOADER:
                return TestDeviceState.FASTBOOT;
            default:
                return TestDeviceState.NOT_AVAILABLE;
        }
    }
}
