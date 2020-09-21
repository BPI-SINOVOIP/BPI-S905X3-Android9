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

/**
 * Interface for recovering a device that has gone offline.
 */
public interface IDeviceRecovery {

    /**
     * Attempt to recover the given device that can no longer be communicated with.
     * <p/>
     * Method should block and only return when device is in requested state.
     *
     * @param monitor the {@link IDeviceStateMonitor} to use.
     * @param recoverUntilOnline if true, method should return as soon as device is online on adb.
     *            If false, method should block until device is fully available for testing (ie
     *            {@link IDeviceStateMonitor#waitForDeviceAvailable()} succeeds.
     * @throws DeviceNotAvailableException if device could not be recovered
     */
    public void recoverDevice(IDeviceStateMonitor monitor, boolean recoverUntilOnline)
            throws DeviceNotAvailableException;

    /**
     * Attempt to recover the given unresponsive device in recovery mode.
     *
     * @param monitor the {@link IDeviceStateMonitor} to use.
     * @throws DeviceNotAvailableException if device could not be recovered
     */
    public void recoverDeviceRecovery(IDeviceStateMonitor monitor)
            throws DeviceNotAvailableException;

    /**
     * Attempt to recover the given unresponsive device in bootloader mode.
     *
     * @param monitor the {@link IDeviceStateMonitor} to use.
     * @throws DeviceNotAvailableException if device could not be recovered
     */
    public void recoverDeviceBootloader(IDeviceStateMonitor monitor)
            throws DeviceNotAvailableException;

    /**
     * Sets the path to the fastboot binary to be used.
     *
     * @param fastbootPath a {@link String} defining the path to the fastboot binary.
     */
    public default void setFastbootPath(String fastbootPath) {
        // empty by default for implementation that do not require fastboot.
    }
}
