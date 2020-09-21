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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;

/** Unit tests for {@link InstrumentationSerialTest}. */
@RunWith(JUnit4.class)
public class InstrumentationSerialTestTest {

    /** The {@link InstrumentationSerialTest} under test, with all dependencies mocked out */
    private InstrumentationSerialTest mInstrumentationSerialTest;

    // The mock objects.
    private ITestDevice mMockTestDevice;
    private ITestInvocationListener mMockListener;

    @Before
    public void setUp() throws Exception {
        mMockTestDevice = EasyMock.createMock(ITestDevice.class);
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);

        EasyMock.expect(mMockTestDevice.getSerialNumber()).andStubReturn("serial");
    }

    /** Test normal run scenario with a single test. */
    @Test
    public void testRun() throws DeviceNotAvailableException, ConfigurationException {
        final String packageName = "com.foo";
        final TestDescription test = new TestDescription("FooTest", "testFoo");
        final Collection<TestDescription> testList = new ArrayList<TestDescription>(1);
        testList.add(test);
        final InstrumentationTest mockITest =
                new InstrumentationTest() {
                    @Override
                    public void run(ITestInvocationListener listener) {
                        listener.testRunStarted(packageName, 1);
                        listener.testStarted(test);
                        listener.testEnded(test, 10l, new HashMap<String, Metric>());
                        listener.testRunEnded(0, new HashMap<String, Metric>());
                    }
                };

        // mock out InstrumentationTest that will be used to create InstrumentationSerialTest
        mockITest.setDevice(mMockTestDevice);
        mockITest.setPackageName(packageName);

        mInstrumentationSerialTest =
                new InstrumentationSerialTest(mockITest, testList) {
                    @Override
                    InstrumentationTest createInstrumentationTest(
                            InstrumentationTest instrumentationTest) throws ConfigurationException {
                        return mockITest;
                    }
                };
        mMockListener.testRunStarted(packageName, 1);
        mMockListener.testStarted(EasyMock.eq(test), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test), EasyMock.eq(10l), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(0, new HashMap<String, Metric>());

        EasyMock.replay(mMockListener, mMockTestDevice);
        mInstrumentationSerialTest.run(mMockListener);
        assertEquals(mMockTestDevice, mockITest.getDevice());
        assertEquals(test.getClassName(), mockITest.getClassName());
        assertEquals(test.getTestName(), mockITest.getMethodName());
        EasyMock.verify(mMockListener, mMockTestDevice);
    }

    /**
     * Test {@link InstrumentationSerialTest#run} when the test run fails without executing the
     * test.
     */
    @Test
    public void testRun_runFailure() throws DeviceNotAvailableException, ConfigurationException {
        final String packageName = "com.foo";
        final TestDescription test = new TestDescription("FooTest", "testFoo");
        final String runFailureMsg = "run failed";
        final Collection<TestDescription> testList = new ArrayList<TestDescription>(1);
        testList.add(test);
        final InstrumentationTest mockITest =
                new InstrumentationTest() {
                    @Override
                    public void run(ITestInvocationListener listener) {
                        listener.testRunStarted(packageName, 1);
                        listener.testRunFailed(runFailureMsg);
                        listener.testRunEnded(0, new HashMap<String, Metric>());
                    }
                };
        // mock out InstrumentationTest that will be used to create InstrumentationSerialTest
        mockITest.setDevice(mMockTestDevice);
        mockITest.setPackageName(packageName);
        mInstrumentationSerialTest = new InstrumentationSerialTest(mockITest, testList) {
            @Override
            InstrumentationTest createInstrumentationTest(InstrumentationTest instrumentationTest)
                    throws ConfigurationException {
                return mockITest;
            }
        };
        // expect two attempts, plus 1 additional run to mark the test as failed
        int expectedAttempts = InstrumentationSerialTest.FAILED_RUN_TEST_ATTEMPTS+1;
        mMockListener.testRunStarted(packageName, 1);
        EasyMock.expectLastCall().times(expectedAttempts);
        mMockListener.testRunFailed(runFailureMsg);
        EasyMock.expectLastCall().times(expectedAttempts);
        mMockListener.testRunEnded(0, new HashMap<String, Metric>());
        EasyMock.expectLastCall().times(expectedAttempts - 1);
        mMockListener.testRunEnded(0, new HashMap<String, Metric>());

        // now expect test to be marked as failed
        mMockListener.testStarted(EasyMock.eq(test));
        mMockListener.testFailed(EasyMock.eq(test),
                EasyMock.contains(runFailureMsg));
        mMockListener.testEnded(EasyMock.eq(test), EasyMock.eq(new HashMap<String, Metric>()));

        EasyMock.replay(mMockListener, mMockTestDevice);
        mInstrumentationSerialTest.run(mMockListener);
        assertEquals(mMockTestDevice, mockITest.getDevice());
        assertEquals(test.getClassName(), mockITest.getClassName());
        assertEquals(test.getTestName(), mockITest.getMethodName());
        EasyMock.verify(mMockListener, mMockTestDevice);
    }

    /** Test that IllegalArgumentException is thrown when attempting run without setting device. */
    @Test
    public void testRun_noDevice() throws DeviceNotAvailableException, ConfigurationException {
        mInstrumentationSerialTest =
                new InstrumentationSerialTest(
                        new InstrumentationTest(), new ArrayList<TestDescription>());
        EasyMock.replay(mMockListener);
        try {
            mInstrumentationSerialTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }
}
