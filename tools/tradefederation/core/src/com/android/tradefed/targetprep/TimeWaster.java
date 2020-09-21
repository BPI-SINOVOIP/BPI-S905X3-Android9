/*
 * Copyright (C) 2012 The Android Open Source Project
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

/** A simple target preparer to waste time and potentially restart the device. */
@OptionClass(alias = "time-waster")
public class TimeWaster extends BaseTargetPreparer {
    @Option(name = "delay", description = "Time to delay, in msecs", mandatory = true)
    private long mDelayMsecs = 0;

    @Option(name = "reboot-before-sleep", description = "Whether to reboot the device before " +
            "sleeping")
    private boolean mRebootBeforeSleep = false;

    @Option(name = "reboot-after-sleep", description = "Whether to reboot the device after " +
            "sleeping")
    private boolean mRebootAfterSleep = false;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            DeviceNotAvailableException {
        if (mRebootBeforeSleep) {
            CLog.d("Pre-sleep device reboot on %s", device.getSerialNumber());
            device.reboot();
        }

        CLog.d("Sleeping %d msecs on device %s", mDelayMsecs, device.getSerialNumber());
        getRunUtil().sleep(mDelayMsecs);

        if (mRebootAfterSleep) {
            CLog.d("Post-sleep device reboot on %s", device.getSerialNumber());
            device.reboot();
        }
    }

    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }
}
