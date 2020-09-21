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

package com.android.tradefed.util;

import com.android.ddmlib.AdbCommandRejectedException;
import com.android.ddmlib.TimeoutException;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IManagedTestDevice;
import com.android.tradefed.device.TestDeviceState;
import com.android.tradefed.log.LogUtil.CLog;

import java.io.IOException;

public class DeviceRecoveryModeUtil {

    /**
     * Boots a device in recovery mode back into the main OS.
     *
     * @param device the {@link IManagedTestDevice} to boot
     * @param timeoutMs how long to wait for the device to be in the recovery state
     * @throws DeviceNotAvailableException
     */
    public static void bootOutOfRecovery(IManagedTestDevice device, long timeoutMs)
            throws DeviceNotAvailableException {
        bootOutOfRecovery(device, timeoutMs, 0);
    }

    /**
     * Boots a device in recovery mode back into the main OS. Can optionally wait after the
     * RECOVERY state begins, as it may not be immediately possible to send adb commands when
     * recovery starts.
     *
     * @param managedDevice the {@link IManagedTestDevice} to boot
     * @param timeoutMs how long to wait for the device to be in the recovery state
     * @param bufferMs the number of ms to wait after the device enters the recovery state
     * @throws DeviceNotAvailableException
     */
    public static void bootOutOfRecovery(IManagedTestDevice managedDevice,
            long timeoutMs, long bufferMs)
            throws DeviceNotAvailableException {
        if (managedDevice.getDeviceState().equals(TestDeviceState.RECOVERY)) {
            CLog.i("Rebooting to exit recovery");
            if (bufferMs > 0) {
                CLog.i("Pausing for %d ms while recovery loads", bufferMs);
                RunUtil.getDefault().sleep(bufferMs);
            }
            try {
                // we don't want to enable root until the reboot is fully finished and
                // the device is available, or it may get stuck in recovery and time out
                managedDevice.getIDevice().reboot(null);
                managedDevice.waitForDeviceAvailable(timeoutMs);
                managedDevice.postBootSetup();
            } catch (TimeoutException | AdbCommandRejectedException | IOException e) {
                CLog.e("Failed to reboot, trying last-ditch recovery");
                CLog.e(e);
                managedDevice.recoverDevice();
            }
        }
    }
}
