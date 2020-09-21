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


import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.mock;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Measurements;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.HashMap;

/** Unit tests for {@link DeviceMetricData}. */
@RunWith(JUnit4.class)
public class DeviceMetricDataTest {
    private DeviceMetricData mData;
    private IInvocationContext mContext;
    private ITestDevice mDevice1;

    @Before
    public void setUp() {
        mContext = new InvocationContext();
        mDevice1 = mock(ITestDevice.class);
        mContext.addAllocatedDevice("device1", mDevice1);
        mData = new DeviceMetricData(mContext);
    }

    /**
     * Test that when adding metrics, if only a single device is available the metrics are left
     * untouched.
     */
    @Test
    public void testSingleDeviceAdd() {
        mData.addMetric(
                "test",
                Metric.newBuilder()
                        .setMeasurements(Measurements.newBuilder().setSingleString("testValue")));
        HashMap<String, Metric> resMetrics = new HashMap<>();
        mData.addToMetrics(resMetrics);
        assertEquals("testValue", resMetrics.get("test").getMeasurements().getSingleString());
    }

    /**
     * Test that when adding metrics, if the device is specified but there is only one device, we do
     * not modify the key to be unique.
     */
    @Test
    public void testSingleDeviceAddToDevice() {
        mData.addMetricForDevice(
                mDevice1,
                "test",
                Metric.newBuilder()
                        .setMeasurements(Measurements.newBuilder().setSingleString("testValue")));
        HashMap<String, Metric> resMetrics = new HashMap<>();
        mData.addToMetrics(resMetrics);
        assertEquals("testValue", resMetrics.get("test").getMeasurements().getSingleString());
    }

    /**
     * Test that when adding metrics, if multiple devices are available, the metrics are associated
     * with the first device by default.
     */
    @Test
    public void testMultiDeviceAdd() {
        mContext.addAllocatedDevice("device2", mock(ITestDevice.class));
        mData.addMetric(
                "test",
                Metric.newBuilder()
                        .setMeasurements(Measurements.newBuilder().setSingleString("testValue")));
        HashMap<String, Metric> resMetrics = new HashMap<>();
        mData.addToMetrics(resMetrics);
        assertNull(resMetrics.get("test"));
        // Metric key was associated with the device name.
        assertEquals(
                "testValue", resMetrics.get("{device1}:test").getMeasurements().getSingleString());
    }

    /**
     * Test that when adding metrics, if multiple devices are available, metric is associated with
     * the requested device.
     */
    @Test
    public void testMultiDeviceAdd_forDevice() {
        ITestDevice device2 = mock(ITestDevice.class);
        mContext.addAllocatedDevice("device2", device2);
        mData.addMetricForDevice(
                device2,
                "test",
                Metric.newBuilder()
                        .setMeasurements(Measurements.newBuilder().setSingleString("testValue")));
        HashMap<String, Metric> resMetrics = new HashMap<>();
        mData.addToMetrics(resMetrics);
        assertNull(resMetrics.get("test"));
        // Metric key was associated with the device name.
        assertEquals(
                "testValue", resMetrics.get("{device2}:test").getMeasurements().getSingleString());
    }
}
