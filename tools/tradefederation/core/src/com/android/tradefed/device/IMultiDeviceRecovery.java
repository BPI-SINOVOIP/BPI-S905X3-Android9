/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.tradefed.config.GlobalConfiguration;

import java.util.List;

/**
 * Interface for recovering multiple offline devices. There are some device recovery methods which
 * can affect multiple devices (ex) restarting adb, resetting usb, ...). We can implement those
 * recovery methods through this interface. Once the implementation is configured through
 * {@link GlobalConfiguration}, {@link #recoverDevices(List)} will be called
 * periodically from {@link DeviceManager}.
 */
public interface IMultiDeviceRecovery {

    /**
     * Recovers offline devices on host.
     *
     * @param managedDevices a list of {@link ITestDevice}s.
     */
    void recoverDevices(List<IManagedTestDevice> managedDevices);

    /**
     * Sets the path to the fastboot binary to be used.
     */
    @SuppressWarnings("unused")
    public default void setFastbootPath(String fastbootPath) {
        // empty by default for implementation that do not require fastboot.
    }
}
