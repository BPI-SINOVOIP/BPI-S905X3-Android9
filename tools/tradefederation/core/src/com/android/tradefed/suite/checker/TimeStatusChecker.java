/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.tradefed.suite.checker;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.suite.checker.StatusCheckerResult.CheckStatus;
import com.android.tradefed.util.TimeUtil;

import java.util.Date;

/** Status checker to ensure that the device and host time are kept in sync. */
public class TimeStatusChecker implements ISystemStatusChecker {

    @Override
    public StatusCheckerResult postExecutionCheck(ITestDevice device)
            throws DeviceNotAvailableException {
        long difference = device.getDeviceTimeOffset(new Date());
        if (difference > 5000L) {
            String message =
                    String.format(
                            "Found a difference of '%s' between the host and device clock, "
                                    + "resetting the device time.",
                            TimeUtil.formatElapsedTime(difference));
            CLog.w(message);
            device.setDate(new Date());
            StatusCheckerResult result = new StatusCheckerResult(CheckStatus.FAILED);
            result.setErrorMessage(message);
            return result;
        }
        return new StatusCheckerResult(CheckStatus.SUCCESS);
    }
}
