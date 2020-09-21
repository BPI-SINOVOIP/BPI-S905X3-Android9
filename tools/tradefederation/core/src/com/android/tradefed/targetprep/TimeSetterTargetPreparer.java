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

package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import java.util.Date;
import java.util.concurrent.TimeUnit;

/**
 * Target preparer to restore the correct time to the device on cleanup. This allows tests to modify
 * the time however they like since the original time will be restored, making sure the state of the
 * device isn't changed at the end.
 *
 * <p>Can also optionally set the time on setup. The time restored on cleanup will be the time set
 * before this target preparer ran.
 */
@OptionClass(alias = "time-setter")
public class TimeSetterTargetPreparer extends BaseTargetPreparer implements ITargetCleaner {
    @Option(
        name = "time",
        description = "Time to set (epoch time in milliseconds).",
        mandatory = true
    )
    private Long mTimeToSet;

    private long mStartNanoTime, mDeviceStartTimeMillis;

    // Exposed for testing.
    long getNanoTime() {
        return System.nanoTime();
    }

    private void setDeviceTime(ITestDevice device, long time) throws DeviceNotAvailableException {
        device.setDate(new Date(time));
    }

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        if (mTimeToSet == null) {
            return;
        }
        mStartNanoTime = getNanoTime();
        mDeviceStartTimeMillis = device.getDeviceDate();
        setDeviceTime(device, mTimeToSet);
    }

    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (mTimeToSet == null) {
            return;
        }
        long elapsedNanos = getNanoTime() - mStartNanoTime;
        long newTime = mDeviceStartTimeMillis + TimeUnit.NANOSECONDS.toMillis(elapsedNanos);
        setDeviceTime(device, newTime);
    }
}
