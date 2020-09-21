/*
 * Copyright (C) 2018 The Android Open Source Project
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
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Collections;
import java.util.HashMap;

/** Unit tests for {@link TemperatureCollector}. */
@RunWith(JUnit4.class)
public class TemperatureCollectorTest {

    private TemperatureCollector mCollector;
    private IInvocationContext mContext;
    private ITestDevice mDevice;
    private ITestInvocationListener mListener;

    @Before
    public void setup() throws Exception {
        mCollector = new TemperatureCollector();
        mContext = mock(IInvocationContext.class);
        mDevice = mock(ITestDevice.class);
        when(mDevice.isAdbRoot()).thenReturn(true);
        when(mContext.getDevices()).thenReturn(Collections.singletonList(mDevice));
        mListener = mock(ITestInvocationListener.class);
        mCollector.init(mContext, mListener);
        OptionSetter setter = new OptionSetter(mCollector);
        setter.setOptionValue(
                "device-temperature-file-path", "/sys/class/hwmon/hwmon1/device/msm_therm");
    }

    @Test
    public void testCollector() throws Exception {
        when(mDevice.executeShellCommand(eq("cat /sys/class/hwmon/hwmon1/device/msm_therm")))
                .thenReturn("Result:32 Raw:7e51", "Result:22 Raw:7b51");

        DeviceMetricData data = new DeviceMetricData(mContext);
        mCollector.onStart(data);
        mCollector.collect(mDevice, data);
        mCollector.collect(mDevice, data);
        mCollector.onEnd(data);

        verify(mDevice, times(2))
                .executeShellCommand(eq("cat /sys/class/hwmon/hwmon1/device/msm_therm"));

        HashMap<String, Metric> results = new HashMap<>();
        data.addToMetrics(results);
        assertEquals(32D, results.get("max_temperature").getMeasurements().getSingleDouble(), 0);
        assertEquals(22D, results.get("min_temperature").getMeasurements().getSingleDouble(), 0);
    }

    @Test
    public void testCollectorNoData() throws Exception {
        when(mDevice.executeShellCommand(eq("cat /sys/class/hwmon/hwmon1/device/msm_therm")))
                .thenReturn(
                        "cat: /sys/class/hwmon/hwmon1/device/msm_therm: No such file or directory");

        DeviceMetricData data = new DeviceMetricData(mContext);
        mCollector.onStart(data);
        mCollector.collect(mDevice, data);
        mCollector.onEnd(data);

        verify(mDevice).executeShellCommand(eq("cat /sys/class/hwmon/hwmon1/device/msm_therm"));

        HashMap<String, Metric> results = new HashMap<>();
        data.addToMetrics(results);
        assertTrue(results.isEmpty());
    }
}
