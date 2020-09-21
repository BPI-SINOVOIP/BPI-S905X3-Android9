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
package com.android.tradefed.invoker.sandbox;

import com.android.tradefed.build.BuildRetrievalError;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.IRescheduler;
import com.android.tradefed.invoker.InvocationExecution;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;

/**
 * Special sandbox execution of the invocation: This is the InvocationExection for when we are
 * inside the sandbox running the command. The build should already be available in the context.
 */
public class SandboxedInvocationExecution extends InvocationExecution {

    /** {@inheritDoc} */
    @Override
    public boolean fetchBuild(
            IInvocationContext context,
            IConfiguration config,
            IRescheduler rescheduler,
            ITestInvocationListener listener)
            throws DeviceNotAvailableException, BuildRetrievalError {
        // If the invocation is currently sandboxed, builds have already been downloaded.
        CLog.d("Skipping download in the sandbox.");
        if (!config.getConfigurationDescription().shouldUseSandbox()) {
            throw new RuntimeException(
                    "We should only skip download if we are a sandbox. Something went very wrong.");
        }
        return true;
    }

    /** {@inheritDoc} */
    @Override
    public boolean resetBuildAndReschedule(
            Throwable exception,
            ITestInvocationListener listener,
            IConfiguration config,
            IInvocationContext context) {
        if (!config.getConfigurationDescription().shouldUseSandbox()) {
            throw new RuntimeException(
                    "We should only skip resetAndReschedule if we are a sandbox. "
                            + "Something went very wrong.");
        }
        // If we are sandboxed, build reset and reschedule should happen on the parents.
        return false;
    }
}
