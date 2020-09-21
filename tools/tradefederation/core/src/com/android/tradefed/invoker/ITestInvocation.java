/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.tradefed.invoker;

import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.result.ITestInvocationListener;

/**
 * Handles one TradeFederation test invocation.
 */
public interface ITestInvocation {

    /**
     * Perform the test invocation.
     *
     * @param metadata the {@link IInvocationContext} to perform tests.
     * @param config the {@link IConfiguration} of this test run.
     * @param rescheduler the {@link IRescheduler}, for rescheduling portions of the invocation for
     *        execution on another resource(s)
     * @param extraListeners {@link ITestInvocationListener}s to notify, in addition to those in
     *        <var>config</var>
     * @throws DeviceNotAvailableException if communication with device was lost
     * @throws Throwable
     */
    public void invoke(IInvocationContext metadata, IConfiguration config,
            IRescheduler rescheduler, ITestInvocationListener... extraListeners)
            throws DeviceNotAvailableException, Throwable;

    /** Notify the {@link TestInvocation} that TradeFed has been requested to stop. */
    public default void notifyInvocationStopped() {}
}
