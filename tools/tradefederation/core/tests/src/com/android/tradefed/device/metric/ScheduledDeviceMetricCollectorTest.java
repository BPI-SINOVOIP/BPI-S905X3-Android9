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
package com.android.tradefed.device.metric;

import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Measurements;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.RunUtil;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mockito;

import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link ScheduledDeviceMetricCollector}. */
@RunWith(JUnit4.class)
public class ScheduledDeviceMetricCollectorTest {
    private Map<String, ITestDevice> mDevicesWithNames = new HashMap<>();

    public static class TestableAsyncTimer extends ScheduledDeviceMetricCollector {
        private int mInternalCounter = 0;

        @Override
        void collect(ITestDevice device, DeviceMetricData runData) throws InterruptedException {
            mInternalCounter++;
            runData.addMetricForDevice(
                    device,
                    "key" + mInternalCounter,
                    Metric.newBuilder()
                            .setMeasurements(
                                    Measurements.newBuilder()
                                            .setSingleString("value" + mInternalCounter)));
        }
    }

    private TestableAsyncTimer mBase;
    private IInvocationContext mContext;
    private ITestInvocationListener mMockListener;

    @Before
    public void setUp() {
        mBase = new TestableAsyncTimer();
        mContext = new InvocationContext();
        mMockListener = Mockito.mock(ITestInvocationListener.class);
    }

    /** Test the periodic run of the collector once testRunStarted has been called. */
    @Test
    public void testSetupAndPeriodicRunSingleDevice() throws Exception {
        // Setup the context with the devices.
        mDevicesWithNames.put("test device 1", mock(ITestDevice.class));
        mContext.addAllocatedDevice(mDevicesWithNames);

        OptionSetter setter = new OptionSetter(mBase);
        // 100 ms interval
        setter.setOptionValue("interval", "100");
        HashMap<String, Metric> metrics = new HashMap<>();
        mBase.init(mContext, mMockListener);
        try {
            mBase.testRunStarted("testRun", 1);
            RunUtil.getDefault().sleep(500);
        } finally {
            mBase.testRunEnded(0l, metrics);
        }
        // We give it 500msec to run and 100msec interval we should easily have at least three
        // iterations
        assertTrue(metrics.containsKey("key1"));
        assertTrue(metrics.containsKey("key2"));
        assertTrue(metrics.containsKey("key3"));
    }

    /**
     * Test the periodic run of the collector on multiple devices once testRunStarted has been
     * called.
     */
    @Test
    public void testSetupAndPeriodicRunMultipleDevices() throws Exception {
        // Setup the context with the devices.
        mDevicesWithNames.put("test device 1", mock(ITestDevice.class));
        mDevicesWithNames.put("test device 2", mock(ITestDevice.class));
        mContext.addAllocatedDevice(mDevicesWithNames);

        OptionSetter setter = new OptionSetter(mBase);
        // 100 ms interval
        setter.setOptionValue("interval", "100");
        HashMap<String, Metric> metrics = new HashMap<>();
        mBase.init(mContext, mMockListener);
        try {
            mBase.testRunStarted("testRun", 1);
            RunUtil.getDefault().sleep(500);
        } finally {
            mBase.testRunEnded(0l, metrics);
        }
        // We give it 500msec to run and 100msec interval we should easily have at least two
        // iterations one for each device. The order of execution is arbitrary so check for prefix
        // only.
        assertTrue(metrics.keySet().stream().anyMatch(key -> key.startsWith("{test device 1}")));
        assertTrue(metrics.keySet().stream().anyMatch(key -> key.startsWith("{test device 2}")));
    }
}
