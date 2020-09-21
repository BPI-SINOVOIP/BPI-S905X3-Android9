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
package com.android.tradefed.suite.checker;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestLoggerReceiver;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.suite.checker.StatusCheckerResult.CheckStatus;
import com.android.tradefed.util.StreamUtil;

/** Status checker for left over activities running at the end of a module. */
public class ActivityStatusChecker implements ISystemStatusChecker, ITestLoggerReceiver {

    private ITestLogger mLogger;

    @Override
    public StatusCheckerResult postExecutionCheck(ITestDevice device)
            throws DeviceNotAvailableException {
        return isFrontActivityLauncher(device);
    }

    private StatusCheckerResult isFrontActivityLauncher(ITestDevice device)
            throws DeviceNotAvailableException {
        StatusCheckerResult result = new StatusCheckerResult();
        String output =
                device.executeShellCommand(
                        "dumpsys window windows | grep -E 'mCurrentFocus|mFocusedApp'");
        CLog.d("dumpsys window windows: %s", output);
        if (output.contains("Launcher")) {
            result.setStatus(CheckStatus.SUCCESS);
            return result;
        } else {
            InputStreamSource screen = device.getScreenshot("JPEG");
            try {
                mLogger.testLog("status_checker_front_activity", LogDataType.JPEG, screen);
            } finally {
                StreamUtil.cancel(screen);
            }
            // TODO: Add a step to return to home page, or refresh the device (reboot?)
            result.setStatus(CheckStatus.FAILED);
            result.setErrorMessage("Launcher activity is not in front.");
            return result;
        }
    }

    @Override
    public void setTestLogger(ITestLogger testLogger) {
        mLogger = testLogger;
    }
}
