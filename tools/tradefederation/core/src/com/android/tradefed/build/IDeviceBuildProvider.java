/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.tradefed.build;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

/**
 * A {@link IBuildProvider} that uses information from a {@link ITestDevice} to retrieve a build.
 * <p/>
 * The typical use case for this interface is a build provider that fetches different kinds of
 * builds based on the device type. It is not recommended to perform actions in a BuildProvider that
 * modify a device's state.
 * <p/>
 * Implementing this interface will cause TF framework to call the {@link #getBuild(ITestDevice)}
 * method instead of {@link IBuildProvider#getBuild()}.
 */
public interface IDeviceBuildProvider extends IBuildProvider {

    /**
     * Retrieve the data for build under test
     *
     * @param device the {@link ITestDevice} allocated for test
     * @return the {@link IBuildInfo} for build under test or <code>null</code> if no build is
     *         available for testing
     * @throws BuildRetrievalError if build info failed to be retrieved due to an unexpected error
     * @throws DeviceNotAvailableException if device became unavailable for testing
     */
    public IBuildInfo getBuild(ITestDevice device) throws BuildRetrievalError,
            DeviceNotAvailableException;

}
