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

import com.android.ddmlib.IShellOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IFileEntry;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;

import junit.framework.TestCase;

import org.easymock.Capture;
import org.easymock.EasyMock;

import java.util.HashMap;
import java.util.concurrent.TimeUnit;

/**
 * Unit tests for {@link NativeStressTest}.
 */
public class NativeStressTestTest extends TestCase {

    private static final String RUN_NAME = "run-name";
    private ITestInvocationListener mMockListener;
    private Capture<HashMap<String, Metric>> mCapturedMetricMap;
    private NativeStressTest mNativeTest;
    private ITestDevice mMockDevice;
    private IFileEntry mMockStressFile;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mNativeTest = new NativeStressTest();
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
        mCapturedMetricMap = new Capture<HashMap<String, Metric>>();
        // expect this call
        mMockListener.testRunStarted(RUN_NAME, 0);
        mMockListener.testRunEnded(EasyMock.anyLong(), EasyMock.capture(mCapturedMetricMap));
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockStressFile = EasyMock.createNiceMock(IFileEntry.class);
        EasyMock.expect(mMockDevice.getFileEntry((String)EasyMock.anyObject())).andReturn(
                mMockStressFile);
        EasyMock.expect(mMockStressFile.isDirectory()).andReturn(Boolean.FALSE);
        EasyMock.expect(mMockStressFile.getName()).andStubReturn(RUN_NAME);
        EasyMock.expect(mMockStressFile.getFullEscapedPath()).andStubReturn(RUN_NAME);

        mNativeTest.setDevice(mMockDevice);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn("serial");
        EasyMock.expect(mMockDevice.executeShellCommand(EasyMock.contains("chmod"))).andReturn("");
    }

    /**
     * Test a run where --iterations has not been specified.
     */
    public void testRun_missingIterations() throws DeviceNotAvailableException {
        try {
            mNativeTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test a run with default values.
     */
    public void testRun() throws DeviceNotAvailableException {
        mNativeTest.setNumIterations(100);
        mMockDevice.executeShellCommand(EasyMock.contains("-s 0 -e 99"), (IShellOutputReceiver)
                EasyMock.anyObject(), EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(),
                EasyMock.anyInt());
        replayMocks();
        mNativeTest.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test a stress test execution with two runs.
     */
    public void testRun_twoRuns() throws DeviceNotAvailableException {
        mNativeTest.setNumIterations(100);
        mNativeTest.setNumRuns(2);
        mMockDevice.executeShellCommand(EasyMock.contains("-s 0 -e 99"), (IShellOutputReceiver)
                EasyMock.anyObject(), EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(),
                EasyMock.anyInt());
        mMockDevice.executeShellCommand(EasyMock.contains("-s 100 -e 199"), (IShellOutputReceiver)
                EasyMock.anyObject(), EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(),
                EasyMock.anyInt());

        replayMocks();
        mNativeTest.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test that stress test results are still reported even if device becomes not available
     */
    public void testRun_deviceNotAvailable() throws DeviceNotAvailableException {
        mNativeTest.setNumIterations(100);
        mMockDevice.executeShellCommand(EasyMock.contains("-s 0 -e 99"), (IShellOutputReceiver)
                EasyMock.anyObject(), EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(),
                EasyMock.anyInt());
        EasyMock.expectLastCall().andThrow(new DeviceNotAvailableException());

        replayMocks();
        try {
            mNativeTest.run(mMockListener);
            fail("DeviceNotAvailableException not thrown");
        } catch (DeviceNotAvailableException e) {
            // expected
        }
        verifyMocks();
    }

    private void replayMocks() {
        EasyMock.replay(mMockListener, mMockDevice, mMockStressFile);
    }

    private void verifyMocks() {
        EasyMock.verify(mMockListener, mMockDevice);
    }
}
