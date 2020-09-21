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
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.suite.checker.StatusCheckerResult.CheckStatus;

/** Checks if system server appears to be running out of FDs. */
public class SystemServerFileDescriptorChecker implements ISystemStatusChecker {

    /** Process will fail to allocate beyond 1024, so heuristic considers 900 a bad state */
    private static final int MAX_EXPECTED_FDS = 900;
    private static final String BUILD_TYPE_PROP = "ro.build.type";
    private static final String USER_BUILD = "user";

    private String mBuildType = null;

    @Override
    public StatusCheckerResult preExecutionCheck(ITestDevice device)
            throws DeviceNotAvailableException {
        if (mBuildType == null) {
            // build type not initialized yet, check on device
            mBuildType = device.getProperty(BUILD_TYPE_PROP);
        }
        return new StatusCheckerResult(CheckStatus.SUCCESS);
    }

    /** {@inheritDoc} */
    @Override
    public StatusCheckerResult postExecutionCheck(ITestDevice device)
            throws DeviceNotAvailableException {
        if (USER_BUILD.equals(mBuildType)) {
            CLog.d("Skipping system_server fd check on user builds.");
            return new StatusCheckerResult(CheckStatus.SUCCESS);
        }
        Integer pid = getIntegerFromCommand(device, "pidof system_server");
        if (pid == null) {
            CLog.d("Unable to find system_server pid.");
            return new StatusCheckerResult(CheckStatus.SUCCESS);
        }

        Integer fds = getIntegerFromCommand(device, "su root ls /proc/" + pid + "/fd | wc -w");
        if (fds == null) {
            CLog.d("Unable to query system_server fd count.");
            return new StatusCheckerResult(CheckStatus.SUCCESS);
        }

        if (fds > MAX_EXPECTED_FDS) {
            StatusCheckerResult result = new StatusCheckerResult(CheckStatus.FAILED);
            String message = String.format("FDs currently allocated in system server %s", fds);
            CLog.w(message);
            result.setErrorMessage(message);
            return result;
        }
        return new StatusCheckerResult(CheckStatus.SUCCESS);
    }

    private static Integer getIntegerFromCommand(ITestDevice device, String command)
            throws DeviceNotAvailableException {
        String output = device.executeShellCommand(command);
        if (output == null) {
            CLog.w("no shell output for command: " + command);
            return null;
        }
        output = output.trim();
        try {
            return Integer.parseInt(output);
        } catch (NumberFormatException e) {
            CLog.w("unable to parse result of '" + command + "' : " + output);
            return null;
        }
    }
}
