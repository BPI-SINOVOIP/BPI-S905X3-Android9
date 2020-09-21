/*
 * Copyright (C) 2016 The Android Open Source Project
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
import com.android.tradefed.suite.checker.StatusCheckerResult.CheckStatus;

/**
 * An checker that performs checks on system status and returns a boolean to indicate if the system
 * is in an expected state. Such check maybe performed either prior to or after a module execution.
 *
 * <p>Note: the checker must be reentrant: meaning that the same instance will be called multiple
 * times for each module executed, so it should not leave a state so as to interfere with the checks
 * to be performed for the following modules.
 *
 * <p>The return {@link StatusCheckerResult} describing the results. May have an error message set
 * in case of failure.
 */
public interface ISystemStatusChecker {

    /**
     * Check system condition before test module execution. Subclass should override this method if
     * a check is desirable here. Implementation must return a <code>boolean</code> value to
     * indicate if the status check has passed or failed.
     *
     * <p>It's strongly recommended that system status be checked <strong>after</strong> module
     * execution, and this method may be used for the purpose of caching certain system state prior
     * to module execution.
     *
     * @param device The {@link ITestDevice} on which to run the checks.
     * @return result of system status check
     * @throws DeviceNotAvailableException
     */
    public default StatusCheckerResult preExecutionCheck(ITestDevice device)
            throws DeviceNotAvailableException {
        return new StatusCheckerResult(CheckStatus.SUCCESS);
    }

    /**
     * Check system condition after test module execution. Subclass should override this method if a
     * check is desirable here. Implementation must return a <code>boolean</code> value to indicate
     * if the status check has passed or failed.
     *
     * @param device The {@link ITestDevice} on which to run the checks.
     * @return result of system status check
     * @throws DeviceNotAvailableException
     */
    public default StatusCheckerResult postExecutionCheck(ITestDevice device)
            throws DeviceNotAvailableException {
        return new StatusCheckerResult(CheckStatus.SUCCESS);
    }
}
