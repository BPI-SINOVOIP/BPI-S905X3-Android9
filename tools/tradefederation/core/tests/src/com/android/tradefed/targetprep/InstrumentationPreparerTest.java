/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.tradefed.build.DeviceBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.device.DeviceAllocationState;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.InstrumentationTest;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.util.HashMap;

/**
 * Unit tests for {@link InstrumentationPreparer}.
 */
public class InstrumentationPreparerTest extends TestCase {

    private InstrumentationPreparer mInstrumentationPreparer;
    private ITestDevice mMockDevice;
    private IDeviceBuildInfo mMockBuildInfo;
    private InstrumentationTest mMockITest;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn("foo").anyTimes();
        mMockBuildInfo = new DeviceBuildInfo("0","");
    }

    @Override
    public void tearDown() throws Exception {
        super.tearDown();
    }

    public void testRun() throws Exception {
        final String packageName = "packageName";
        final TestDescription test = new TestDescription("FooTest", "testFoo");
        mMockITest =
                new InstrumentationTest() {
                    @Override
                    public void run(ITestInvocationListener listener) {
                        listener.testRunStarted(packageName, 1);
                        listener.testStarted(test);
                        listener.testEnded(test, new HashMap<String, Metric>());
                        listener.testRunEnded(0, new HashMap<String, Metric>());
                    }
                };
        mInstrumentationPreparer = new InstrumentationPreparer() {
            @Override
            InstrumentationTest createInstrumentationTest() {
                return mMockITest;
            }
        };
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andReturn(new DeviceDescriptor("SERIAL",
                false, DeviceAllocationState.Available, "unknown", "unknown", "unknown", "unknown",
                "unknown"));
        EasyMock.replay(mMockDevice);
        mInstrumentationPreparer.setUp(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockDevice);
    }

    public void testRun_testFailed() throws Exception {
        final String packageName = "packageName";
        final TestDescription test = new TestDescription("FooTest", "testFoo");
        mMockITest =
                new InstrumentationTest() {
                    @Override
                    public void run(ITestInvocationListener listener) {
                        listener.testRunStarted(packageName, 1);
                        listener.testStarted(test);
                        listener.testFailed(test, null);
                        listener.testEnded(test, new HashMap<String, Metric>());
                        listener.testRunEnded(0, new HashMap<String, Metric>());
                    }
                };
        mInstrumentationPreparer = new InstrumentationPreparer() {
            @Override
            InstrumentationTest createInstrumentationTest() {
                return mMockITest;
            }
        };
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andStubReturn(new DeviceDescriptor(
                "SERIAL", false, DeviceAllocationState.Available, "unknown", "unknown", "unknown",
                "unknown", "unknown"));
        EasyMock.replay(mMockDevice);
        try {
            mInstrumentationPreparer.setUp(mMockDevice, mMockBuildInfo);
            fail("BuildError not thrown");
        } catch(final BuildError e) {
            assertTrue("The exception message does not contain failed test names",
                    e.getMessage().contains(test.toString()));
            // expected
        }
        EasyMock.verify(mMockDevice);
    }
}
