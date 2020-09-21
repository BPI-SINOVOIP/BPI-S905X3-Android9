/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

/**
 * A {@link ITargetPreparer} that waits for datetime to be set on device
 *
 * <p>Optionally this preparer can force a {@link TargetSetupError} if datetime is not set within
 * timeout, or force host datetime onto device,
 */
@OptionClass(alias = "wait-for-datetime")
public class WaitForDeviceDatetimePreparer extends BaseTargetPreparer {

    // 30s to wait for device datetime
    private static final long DATETIME_WAIT_TIMEOUT = 30 * 1000;
    // poll every 5s when waiting correct device datetime
    private static final long DATETIME_CHECK_INTERVAL = 5 * 1000;
    // allow 10s of margin for datetime difference between host/device
    private static final long DATETIME_MARGIN = 10;

    @Option(name = "force-datetime", description = "Force sync host datetime to device if device "
            + "fails to set datetime automatically.")
    private boolean mForceDatetime = false;

    @Option(name = "datetime-wait-timeout",
            description = "Timeout in ms to wait for correct datetime on device.")
    private long mDatetimeWaitTimeout = DATETIME_WAIT_TIMEOUT;

    @Option(name = "force-setup-error",
            description = "Throw an TargetSetupError if correct datetime was not set. "
                    + "Only meaningful if \"force-datetime\" is not used.")
    private boolean mForceSetupError = false;

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        if (!waitForDeviceDatetime(device, mForceDatetime)) {
            if (mForceSetupError) {
                throw new TargetSetupError("datetime on device is incorrect after wait timeout",
                        device.getDeviceDescriptor());
            } else {
                CLog.w("datetime on device is incorrect after wait timeout.");
            }
        }
    }

    /**
     * Sets the timeout for waiting on valid device datetime
     */
    public void setDatetimeWaitTimeout(long datetimeWaitTimeout) {
        mDatetimeWaitTimeout = datetimeWaitTimeout;
    }

    /**
     * Sets the if datetime should be forced from host to device
     */
    public void setForceDatetime(boolean forceDatetime) {
        mForceDatetime = forceDatetime;
    }

    /**
     * Sets the boolean for forcing a {@link TargetSetupError} if the datetime is not set correctly.
     */
    public void setForceSetupError(boolean forceSetupError) {
        mForceSetupError = forceSetupError;
    }

    /**
     * Waits for a correct datetime on device, optionally force host datetime onto device
     * @param forceDatetime
     * @return <code>true</code> if datetime is correct or forced, <code>false</code> otherwise
     */
    boolean waitForDeviceDatetime(ITestDevice device, boolean forceDatetime)
            throws DeviceNotAvailableException {
        return waitForDeviceDatetime(device, forceDatetime,
                mDatetimeWaitTimeout, DATETIME_CHECK_INTERVAL);
    }

    /**
     * Waits for a correct datetime on device, optionally force host datetime onto device
     * @param forceDatetime
     * @param datetimeWaitTimeout
     * @param datetimeCheckInterval
     * @return <code>true</code> if datetime is correct or forced, <code>false</code> otherwise
     */
    boolean waitForDeviceDatetime(ITestDevice device, boolean forceDatetime,
            long datetimeWaitTimeout, long datetimeCheckInterval)
            throws DeviceNotAvailableException {
        long start = System.currentTimeMillis();
        while ((System.currentTimeMillis() - start) < datetimeWaitTimeout) {
            long datetime = getDeviceDatetimeEpoch(device);
            long now = System.currentTimeMillis() / 1000;
            if (datetime == -1) {
                if (forceDatetime) {
                    throw new UnsupportedOperationException(
                            "unexpected return from \"date\" command on device");
                } else {
                    return false;
                }
            }
            if ((Math.abs(now - datetime) < DATETIME_MARGIN)) {
                return true;
            }
            getRunUtil().sleep(datetimeCheckInterval);
        }
        if (forceDatetime) {
            device.setDate(null);
            return true;
        } else {
            return false;
        }
    }

    /**
     * Retrieve device datetime in epoch format
     * @param device
     * @return datetime on device in epoch format, -1 if failed
     */
    long getDeviceDatetimeEpoch(ITestDevice device) throws DeviceNotAvailableException {
        String datetime = device.executeShellCommand("date '+%s'").trim();
        try {
            return Long.parseLong(datetime);
        } catch (NumberFormatException nfe) {
            CLog.v("returned datetime from device is not a number: '%s'", datetime);
            return -1;
        }
    }

    /**
     * @return the {@link IRunUtil} to use
     */
    protected IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }
}
