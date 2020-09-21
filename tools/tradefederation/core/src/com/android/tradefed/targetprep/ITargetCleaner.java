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
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

/**
 * Cleans up the target device after the test run has finished.
 * <p/>
 * For example, removes software, collects metrics, remove temporary files etc.
 * <p/>
 * Note that multiple {@link ITargetCleaner}s can be specified in a configuration.
 */
public interface ITargetCleaner extends ITargetPreparer {

    /**
     * Perform the target cleanup/teardown after testing.
     *
     * @param device the {@link ITestDevice} to prepare.
     * @param buildInfo data about the build under test.
     * @param e if the invocation ended with an exception, this will be the exception that was
     *        caught at the Invocation level.  Otherwise, will be <code>null</code>.
     * @throws DeviceNotAvailableException if device became unresponsive
     */
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException;
}
