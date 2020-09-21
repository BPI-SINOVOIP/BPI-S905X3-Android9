/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

/**
 * Recovers a device by re-establishing a TCP connection via the adb server on
 * the host.
 */
public class ReconnectingRecovery implements IDeviceRecovery {

    private static final int ADB_TIMEOUT = 2 * 60 * 1000;
    private static final int CONNECTION_ATTEMPTS = 5;

    /**
     * {@inheritDoc}
     */
    @Override
    public void recoverDevice(IDeviceStateMonitor monitor, boolean recoverUntilOnline)
            throws DeviceNotAvailableException {
        String serial = monitor.getSerialNumber();

        // disconnect - many versions of adb client have stale TCP connection
        // status
        getRunUtil().runTimedCmd(ADB_TIMEOUT, "adb", "disconnect", serial);

        // try to reconnect
        int attempt = 1;
        do {
            CLog.i("Trying to reconnect with device " + serial + " / attempt " + attempt);
            getRunUtil().runTimedCmd(ADB_TIMEOUT, "adb", "connect", serial);
        } while (monitor.waitForDeviceOnline() == null && ++attempt <= CONNECTION_ATTEMPTS);

        String errMsg = "Could not recover device " + serial + " after " + --attempt + " attempts";

        // occasionally device is erroneously reported as online - double check
        // that we can shell into device
        if (!monitor.waitForDeviceShell(10 * 1000)) {
            throw new DeviceUnresponsiveException(errMsg, serial);
        }

        if (!recoverUntilOnline) {
            if (monitor.waitForDeviceAvailable() == null) {
                throw new DeviceUnresponsiveException(errMsg, serial);
            }
        }

        CLog.v("Successfully reconnected with device " + serial);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void recoverDeviceBootloader(IDeviceStateMonitor monitor)
            throws DeviceNotAvailableException {
        throw new java.lang.UnsupportedOperationException(
                "This implementation can't recover a device in bootloader mode.");
    }

    /**
     * {@inheritDoc}
     * <p>
     * This implementation assumes devices in recovery mode can't be talked to
     * at all, so it will try to recover a device and leave it in fully booted
     * mode.
     */
    @Override
    public void recoverDeviceRecovery(IDeviceStateMonitor monitor)
            throws DeviceNotAvailableException {
        recoverDevice(monitor, false);
    }

    /**
     * Get the {@link RunUtil} instance to use.
     * <p/>
     * Exposed for unit testing.
     */
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }
}
