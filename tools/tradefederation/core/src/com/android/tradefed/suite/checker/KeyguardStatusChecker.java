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

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.suite.checker.StatusCheckerResult.CheckStatus;
import com.android.tradefed.util.KeyguardControllerState;

/** Checks the keyguard status after module execution. */
public class KeyguardStatusChecker implements ISystemStatusChecker {

    private boolean mIsKeyguardShowing;
    private boolean mIsKeyguardOccluded;

    /** {@inheritDoc} */
    @Override
    public StatusCheckerResult postExecutionCheck(ITestDevice device)
            throws DeviceNotAvailableException {
        StatusCheckerResult result = new StatusCheckerResult(CheckStatus.SUCCESS);
        // We assume keyguard was dismissed before the module.
        KeyguardControllerState ksc = device.getKeyguardState();
        if (ksc == null) {
            // for compatibility
            CLog.logAndDisplay(
                    LogLevel.DEBUG,
                    "KeyguardControllerState is not supported by the "
                            + "device. Skipping System Checker.");
            return result;
        }
        mIsKeyguardShowing = ksc.isKeyguardShowing();
        mIsKeyguardOccluded = ksc.isKeyguardOccluded();
        if (mIsKeyguardShowing || mIsKeyguardOccluded) {
            String message =
                    String.format(
                            "SystemChecker - post-execution:\n" + "\tKeyguard on: %s, occluded: %s",
                            mIsKeyguardShowing, mIsKeyguardOccluded);
            CLog.logAndDisplay(LogLevel.WARN, message);
            // best effort to dismiss keyguard: will not work if a secured keyguard was left around
            CLog.w("Also attempting to dismiss keyguard.");
            device.disableKeyguard();
            result.setStatus(CheckStatus.FAILED);
            result.setErrorMessage(message);
            return result;
        }
        return result;
    }
}
