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
package com.android.tradefed.testtype;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.result.ITestInvocationListener;

import junit.framework.Test;

/**
 * A test that reports results directly to a {@link ITestInvocationListener}.
 * <p/>
 * This has the following benefits over a JUnit {@link Test}
 * <ul>
 * <li> easier to report the results of a test that has been run remotely on an Android device, as
 * the results of a remote test don't need to be unnecessarily marshalled and unmarshalled from
 * {@link Test} objects.
 * </li>
 * <li> supports reporting test metrics</li>
 * </ul>
 */
public interface IRemoteTest {

    /**
     * Runs the tests, and reports result to the listener.
     *
     * @param listener the {@link ITestInvocationListener} of test results
     * @throws DeviceNotAvailableException
     */
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException;
}
