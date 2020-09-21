/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.tradefed.invoker.shard;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.metric.IMetricCollector;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.StubTest;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mockito;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.CountDownLatch;

/** Unit tests for {@link TestsPoolPoller}. */
@RunWith(JUnit4.class)
public class TestsPoolPollerTest {

    private ITestInvocationListener mListener;
    private ITestDevice mDevice;
    private List<IMetricCollector> mMetricCollectors;

    @Before
    public void setUp() {
        mListener = Mockito.mock(ITestInvocationListener.class);
        mDevice = Mockito.mock(ITestDevice.class);
        Mockito.doReturn("serial").when(mDevice).getSerialNumber();
        mMetricCollectors = new ArrayList<>();
    }

    /**
     * Tests that {@link TestsPoolPoller#poll()} returns a {@link IRemoteTest} from the pool or null
     * when the pool is empty.
     */
    @Test
    public void testMultiPolling() {
        int numTests = 5;
        List<IRemoteTest> testsList = new ArrayList<>();
        for (int i = 0; i < numTests; i++) {
            testsList.add(new StubTest());
        }
        CountDownLatch tracker = new CountDownLatch(2);
        TestsPoolPoller poller1 = new TestsPoolPoller(testsList, tracker);
        TestsPoolPoller poller2 = new TestsPoolPoller(testsList, tracker);
        // initial size
        assertEquals(numTests, testsList.size());
        assertNotNull(poller1.poll());
        assertEquals(numTests - 1, testsList.size());
        assertNotNull(poller2.poll());
        assertEquals(numTests - 2, testsList.size());
        assertNotNull(poller1.poll());
        assertNotNull(poller1.poll());
        assertNotNull(poller2.poll());
        assertTrue(testsList.isEmpty());
        // once empty poller returns null
        assertNull(poller1.poll());
        assertNull(poller2.poll());
    }

    /**
     * Tests that {@link TestsPoolPoller#run(ITestInvocationListener)} is properly running and
     * redirecting the invocation callbacks.
     */
    @Test
    public void testPollingRun() throws Exception {
        int numTests = 5;
        List<IRemoteTest> testsList = new ArrayList<>();
        for (int i = 0; i < numTests; i++) {
            IRemoteTest test = new StubTest();
            OptionSetter setter = new OptionSetter(test);
            setter.setOptionValue("run-a-test", "true");
            testsList.add(test);
        }
        CountDownLatch tracker = new CountDownLatch(1);
        TestsPoolPoller poller = new TestsPoolPoller(testsList, tracker);
        poller.setMetricCollectors(mMetricCollectors);
        poller.run(mListener);
        Mockito.verify(mListener, Mockito.times(numTests))
                .testRunStarted(Mockito.anyString(), Mockito.anyInt());
        Mockito.verify(mListener, Mockito.times(numTests))
                .testRunEnded(Mockito.anyLong(), (HashMap<String, Metric>) Mockito.any());
        assertEquals(0, tracker.getCount());
    }

    /**
     * Tests that {@link TestsPoolPoller#run(ITestInvocationListener)} will continue to run tests
     * even if one of them throws a {@link RuntimeException}.
     */
    @Test
    public void testRun_runtimeException() throws Exception {
        List<IRemoteTest> testsList = new ArrayList<>();
        // Add one bad test first that will throw an exception.
        IRemoteTest badTest = new StubTest();
        OptionSetter setter = new OptionSetter(badTest);
        setter.setOptionValue("test-throw-runtime", "true");
        testsList.add(badTest);
        // Add tests that can run
        int numTests = 5;
        for (int i = 0; i < numTests; i++) {
            IRemoteTest test = new StubTest();
            OptionSetter s = new OptionSetter(test);
            s.setOptionValue("run-a-test", "true");
            testsList.add(test);
        }
        CountDownLatch tracker = new CountDownLatch(1);
        TestsPoolPoller poller = new TestsPoolPoller(testsList, tracker);
        poller.setMetricCollectors(mMetricCollectors);
        poller.run(mListener);
        Mockito.verify(mListener, Mockito.times(numTests))
                .testRunStarted(Mockito.anyString(), Mockito.anyInt());
        Mockito.verify(mListener, Mockito.times(numTests))
                .testRunEnded(Mockito.anyLong(), (HashMap<String, Metric>) Mockito.any());
        assertEquals(0, tracker.getCount());
    }

    /**
     * Tests that {@link TestsPoolPoller#run(ITestInvocationListener)} will continue to run tests
     * even if one of them throws a {@link DeviceUnresponsiveException}.
     */
    @Test
    public void testRun_deviceUnresponsive() throws Exception {
        List<IRemoteTest> testsList = new ArrayList<>();
        // Add one bad test first that will throw an exception.
        IRemoteTest badTest = new StubTest();
        OptionSetter setter = new OptionSetter(badTest);
        setter.setOptionValue("test-throw-unresponsive", "true");
        testsList.add(badTest);
        // Add tests that can run
        int numTests = 5;
        for (int i = 0; i < numTests; i++) {
            IRemoteTest test = new StubTest();
            OptionSetter s = new OptionSetter(test);
            s.setOptionValue("run-a-test", "true");
            testsList.add(test);
        }
        CountDownLatch tracker = new CountDownLatch(1);
        TestsPoolPoller poller = new TestsPoolPoller(testsList, tracker);
        poller.setMetricCollectors(mMetricCollectors);
        poller.run(mListener);
        Mockito.verify(mListener, Mockito.times(numTests))
                .testRunStarted(Mockito.anyString(), Mockito.anyInt());
        Mockito.verify(mListener, Mockito.times(numTests))
                .testRunEnded(Mockito.anyLong(), (HashMap<String, Metric>) Mockito.any());
        assertEquals(0, tracker.getCount());
    }

    /**
     * Tests that {@link TestsPoolPoller#run(ITestInvocationListener)} will stop to run tests if one
     * of them throws a {@link DeviceNotAvailableException}.
     */
    @Test
    public void testRun_dnae() throws Exception {
        List<IRemoteTest> testsList = new ArrayList<>();
        // Add one bad test first that will throw an exception.
        IRemoteTest badTest = new StubTest();
        OptionSetter setter = new OptionSetter(badTest);
        setter.setOptionValue("test-throw-not-available", "true");
        testsList.add(badTest);
        // Add tests that can run
        int numTests = 5;
        for (int i = 0; i < numTests; i++) {
            IRemoteTest test = new StubTest();
            OptionSetter s = new OptionSetter(test);
            s.setOptionValue("run-a-test", "true");
            testsList.add(test);
        }
        CountDownLatch tracker = new CountDownLatch(1);
        TestsPoolPoller poller = new TestsPoolPoller(testsList, tracker);
        poller.setMetricCollectors(mMetricCollectors);
        poller.setDevice(mDevice);
        try {
            poller.run(mListener);
            fail("Should have thrown an exception.");
        } catch (DeviceNotAvailableException expected) {
            // expected
        }
        // We expect no callbacks on these, poller should stop early.
        Mockito.verify(mListener, Mockito.times(0))
                .testRunStarted(Mockito.anyString(), Mockito.anyInt());
        Mockito.verify(mListener, Mockito.times(0))
                .testRunEnded(Mockito.anyLong(), (HashMap<String, Metric>) Mockito.any());
        assertEquals(0, tracker.getCount());
    }

    /**
     * If a device not available exception is thrown from a tests, and the poller is not the last
     * one alive, we wait and attempt to recover the device. In case of success, execution will
     * proceed.
     */
    @Test
    public void testRun_dnae_NotLastDevice() throws Exception {
        List<IRemoteTest> testsList = new ArrayList<>();
        // Add one bad test first that will throw an exception.
        IRemoteTest badTest = new StubTest();
        OptionSetter setter = new OptionSetter(badTest);
        setter.setOptionValue("test-throw-not-available", "true");
        testsList.add(badTest);
        // Add tests that can run
        int numTests = 5;
        for (int i = 0; i < numTests; i++) {
            IRemoteTest test = new StubTest();
            OptionSetter s = new OptionSetter(test);
            s.setOptionValue("run-a-test", "true");
            testsList.add(test);
        }
        CountDownLatch tracker = new CountDownLatch(3);
        TestsPoolPoller poller = new TestsPoolPoller(testsList, tracker);
        poller.setMetricCollectors(mMetricCollectors);
        poller.setDevice(mDevice);

        poller.run(mListener);
        // The callbacks from all the other tests because the device was recovered
        Mockito.verify(mListener, Mockito.times(numTests))
                .testRunStarted(Mockito.anyString(), Mockito.anyInt());
        Mockito.verify(mListener, Mockito.times(numTests)).testStarted(Mockito.any());
        Mockito.verify(mListener, Mockito.times(numTests))
                .testEnded(Mockito.any(), (HashMap<String, Metric>) Mockito.any());
        Mockito.verify(mListener, Mockito.times(numTests))
                .testRunEnded(Mockito.anyLong(), (HashMap<String, Metric>) Mockito.any());
        Mockito.verify(mDevice).waitForDeviceAvailable(Mockito.anyLong());
        Mockito.verify(mDevice).reboot();
        assertEquals(2, tracker.getCount());
    }
}
