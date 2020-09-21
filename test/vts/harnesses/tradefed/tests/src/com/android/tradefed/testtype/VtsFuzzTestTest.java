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
package com.android.tradefed.testtype;

import com.android.ddmlib.IShellOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ITestLifeCycleReceiver;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.util.concurrent.TimeUnit;


/**
 * Unit tests for {@link VtsFuzzTest}.
 */
public class VtsFuzzTestTest extends TestCase {
    private ITestInvocationListener mMockInvocationListener = null;
    private IShellOutputReceiver mMockReceiver = null;
    private ITestDevice mMockITestDevice = null;
    private VtsFuzzTest mTest;

    /**
     * Helper to initialize the various EasyMocks we'll need.
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockInvocationListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockReceiver = EasyMock.createMock(IShellOutputReceiver.class);
        mMockITestDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockITestDevice.getSerialNumber()).andStubReturn("serial");
        mTest = new VtsFuzzTest() {
            @Override
            IShellOutputReceiver createResultParser(
                    String runName, ITestLifeCycleReceiver listener) {
                return mMockReceiver;
            }
        };
    }

    /**
     * Helper that replays all mocks.
     */
    private void replayMocks() {
      EasyMock.replay(mMockInvocationListener, mMockITestDevice, mMockReceiver);
    }

    /**
     * Helper that verifies all mocks.
     */
    private void verifyMocks() {
      EasyMock.verify(mMockInvocationListener, mMockITestDevice, mMockReceiver);
    }

    /**
     * Test the run method with a normal input.
     */
    public void testRunNormalInput() throws DeviceNotAvailableException {
        final String fuzzerBinaryPath = VtsFuzzTest.DEFAULT_FUZZER_BINARY_PATH;

        mTest.setDevice(mMockITestDevice);
        EasyMock.expect(mMockITestDevice.doesFileExist(fuzzerBinaryPath)).andReturn(true);
        mMockITestDevice.executeShellCommand(
            EasyMock.contains(fuzzerBinaryPath),
            EasyMock.same(mMockReceiver),
            EasyMock.anyLong(),
            (TimeUnit)EasyMock.anyObject(),
            EasyMock.anyInt());

        replayMocks();

        mTest.setTargetClass("hal_conventional");
        mTest.setTargetType("gps");
        mTest.setTargetVersion(1.0f);
        mTest.setTargetComponentPath("libunittest_foo.so");
        mTest.run(mMockInvocationListener);
        verifyMocks();
    }

    /**
     * Test the run method with abnormal input data.
     */
    public void testRunAbnormalInput() throws DeviceNotAvailableException {
        final String fuzzerBinaryPath = VtsFuzzTest.DEFAULT_FUZZER_BINARY_PATH;

        mTest.setDevice(mMockITestDevice);
        EasyMock.expect(mMockITestDevice.doesFileExist(fuzzerBinaryPath)).andReturn(true);

        replayMocks();

        try {
            mTest.run(mMockInvocationListener);
        } catch (IllegalArgumentException e) {
            // expected
        }

        verifyMocks();
    }

    /**
     * Test the run method without any device.
     */
    public void testRunDevice() throws DeviceNotAvailableException {
        mTest.setDevice(null);

        replayMocks();

        try {
            mTest.run(mMockInvocationListener);
        } catch (IllegalArgumentException e) {
            // expected
        }

        verifyMocks();
    }
}
