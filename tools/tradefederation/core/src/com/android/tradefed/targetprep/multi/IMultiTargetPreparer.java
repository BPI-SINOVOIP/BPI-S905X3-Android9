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
package com.android.tradefed.targetprep.multi;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.IDisableable;

/**
 * Prepares the test environment for several devices together. Only use for a setup that requires
 * multiple devices, otherwise use the regular {@link ITargetPreparer} on each device.
 *
 * <p>Note that multiple {@link IMultiTargetPreparer}s can be specified in a configuration. It is
 * recommended that each IMultiTargetPreparer clearly document its expected environment pre-setup
 * and post-setUp.
 */
public interface IMultiTargetPreparer extends IDisableable {

    /**
     * Perform the targets setup for testing.
     *
     * @param context the {@link IInvocationContext} describing the invocation, devices, builds.
     * @throws TargetSetupError if fatal error occurred setting up environment
     * @throws DeviceNotAvailableException if device became unresponsive
     */
    public void setUp(IInvocationContext context) throws TargetSetupError,
            BuildError, DeviceNotAvailableException;


    /**
     * Perform the targets cleanup/teardown after testing.
     *
     * @param context the {@link IInvocationContext} describing the invocation, devices, builds.
     * @param e if the invocation ended with an exception, this will be the exception that was
     *        caught at the Invocation level.  Otherwise, will be <code>null</code>.
     * @throws DeviceNotAvailableException if device became unresponsive
     */
    public default void tearDown(IInvocationContext context, Throwable e)
            throws DeviceNotAvailableException {
        // default do nothing.
    }
}