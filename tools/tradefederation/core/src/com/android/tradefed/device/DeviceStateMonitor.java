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

import com.android.ddmlib.AdbCommandRejectedException;
import com.android.ddmlib.CollectingOutputReceiver;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.ShellCommandUnresponsiveException;
import com.android.ddmlib.TimeoutException;
import com.android.tradefed.log.LogUtil.CLog;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

/**
 * Helper class for monitoring the state of a {@link IDevice}.
 */
public class DeviceStateMonitor extends NativeDeviceStateMonitor {

    public DeviceStateMonitor(IDeviceManager mgr, IDevice device, boolean fastbootEnabled) {
        super(mgr, device, fastbootEnabled);
    }

    /**
     * Waits for the device package manager to be responsive.
     *
     * @param waitTime time in ms to wait before giving up
     * @return <code>true</code> if package manage becomes responsive before waitTime expires.
     * <code>false</code> otherwise
     */
    protected boolean waitForPmResponsive(final long waitTime) {
        CLog.i("Waiting %d ms for device %s package manager",
                waitTime, getSerialNumber());
        long startTime = System.currentTimeMillis();
        int counter = 1;
        while (System.currentTimeMillis() - startTime < waitTime) {
            final CollectingOutputReceiver receiver = createOutputReceiver();
            final String cmd = "pm path android";
            try {
                getIDevice().executeShellCommand(cmd, receiver, MAX_OP_TIME, TimeUnit.MILLISECONDS);
                String output = receiver.getOutput();
                CLog.v("%s returned %s", cmd, output);
                if (output.contains("package:")) {
                    return true;
                }
            } catch (IOException e) {
                CLog.i("%s on device %s failed: %s", cmd, getSerialNumber(), e.getMessage());
            } catch (TimeoutException e) {
                CLog.i("%s on device %s failed: timeout", cmd, getSerialNumber());
            } catch (AdbCommandRejectedException e) {
                CLog.i("%s on device %s failed: %s", cmd, getSerialNumber(), e.getMessage());
            } catch (ShellCommandUnresponsiveException e) {
                CLog.i("%s on device %s failed: %s", cmd, getSerialNumber(), e.getMessage());
            }
            getRunUtil().sleep(Math.min(getCheckPollTime() * counter, MAX_CHECK_POLL_TIME));
            counter++;
        }
        CLog.w("Device %s package manager is unresponsive", getSerialNumber());
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected boolean postOnlineCheck(final long waitTime) {
        long startTime = System.currentTimeMillis();
        if (!waitForPmResponsive(waitTime)) {
            return false;
        }
        long elapsedTime = System.currentTimeMillis() - startTime;
        return super.postOnlineCheck(waitTime - elapsedTime);
    }
}
