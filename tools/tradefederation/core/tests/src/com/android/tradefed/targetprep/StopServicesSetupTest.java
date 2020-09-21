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

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import junit.framework.TestCase;

import org.easymock.EasyMock;

/**
 * Unit tests for {@link StopServicesSetup}
 */
public class StopServicesSetupTest extends TestCase {

    private StopServicesSetup mPreparer = null;
    private ITestDevice mMockDevice = null;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockDevice = EasyMock.createStrictMock(ITestDevice.class);
        mPreparer = new StopServicesSetup();
    }

    /**
     * Test that the framework is stopped in the default case.
     */
    public void testNoop() throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.executeShellCommand("stop")).andReturn(null);

        EasyMock.replay(mMockDevice);
        mPreparer.setUp(mMockDevice, null);
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test that stopping the framework can be overwritten.
     */
    public void testNoStopFramework() throws DeviceNotAvailableException {
        mPreparer.setStopFramework(false);

        EasyMock.replay(mMockDevice);
        mPreparer.setUp(mMockDevice, null);
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test that additional services are stopped if specified.
     */
    public void testStopServices() throws DeviceNotAvailableException {
        mPreparer.addService("service1");
        mPreparer.addService("service2");

        EasyMock.expect(mMockDevice.executeShellCommand("stop")).andReturn(null);
        EasyMock.expect(mMockDevice.executeShellCommand("stop service1")).andReturn(null);
        EasyMock.expect(mMockDevice.executeShellCommand("stop service2")).andReturn(null);

        EasyMock.replay(mMockDevice);
        mPreparer.setUp(mMockDevice, null);
        EasyMock.verify(mMockDevice);
    }
}

