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

package com.android.cts.net;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil;
import com.android.tradefed.targetprep.ITargetCleaner;
import com.android.tradefed.targetprep.ITargetPreparer;

public class NetPolicyTestsPreparer implements ITargetPreparer, ITargetCleaner {
    private final static String KEY_PAROLE_DURATION = "parole_duration";
    private final static int DESIRED_PAROLE_DURATION = 0;
    private final static String KEY_STABLE_CHARGING_THRESHOLD = "stable_charging_threshold";
    private final static int DESIRED_STABLE_CHARGING_THRESHOLD = 0;

    private ITestDevice mDevice;
    private String mOriginalAppIdleConsts;

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws DeviceNotAvailableException {
        mDevice = device;
        mOriginalAppIdleConsts = getAppIdleConstants();
        setAppIdleConstants(KEY_PAROLE_DURATION + "=" + DESIRED_PAROLE_DURATION + ","
                + KEY_STABLE_CHARGING_THRESHOLD + "=" + DESIRED_STABLE_CHARGING_THRESHOLD);
        LogUtil.CLog.d("Original app_idle_constants: " + mOriginalAppIdleConsts);
    }

    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable throwable)
            throws DeviceNotAvailableException {
        setAppIdleConstants(mOriginalAppIdleConsts);
    }

    private void setAppIdleConstants(String appIdleConstants) throws DeviceNotAvailableException {
        executeCmd("settings put global app_idle_constants " + appIdleConstants);
    }

    private String getAppIdleConstants() throws DeviceNotAvailableException {
        return executeCmd("settings get global app_idle_constants");
    }

    private String executeCmd(String cmd) throws DeviceNotAvailableException {
        final String output = mDevice.executeShellCommand(cmd).trim();
        LogUtil.CLog.d("Output for '%s': %s", cmd, output);
        return output;
    }
}
