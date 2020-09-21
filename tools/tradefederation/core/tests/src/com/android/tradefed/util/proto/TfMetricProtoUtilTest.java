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
package com.android.tradefed.util.proto;

import static org.junit.Assert.assertEquals;

import com.android.tradefed.metrics.proto.MetricMeasurement.DataType;
import com.android.tradefed.metrics.proto.MetricMeasurement.DoubleValues;
import com.android.tradefed.metrics.proto.MetricMeasurement.Measurements;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.metrics.proto.MetricMeasurement.StringValues;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link TfMetricProtoUtil}. */
@RunWith(JUnit4.class)
public class TfMetricProtoUtilTest {

    /**
     * Test that the downgrade conversion to the old map converts the simple value in String. but
     * complex values are lost since there are no good way to convert them.
     */
    @Test
    public void testCompatibleConvert() {
        Map<String, Metric> metrics = new HashMap<>();
        // float
        Measurements m1 = Measurements.newBuilder().setSingleDouble(5.5f).build();
        metrics.put("key1", createMetric(m1));

        // Integer
        Measurements m2 = Measurements.newBuilder().setSingleInt(10).build();
        metrics.put("key2", createMetric(m2));

        // String
        Measurements m3 = Measurements.newBuilder().setSingleString("value").build();
        metrics.put("key3", createMetric(m3));

        // List of floats
        Measurements m4 =
                Measurements.newBuilder()
                        .setDoubleValues(DoubleValues.newBuilder().addDoubleValue(5.5f).build())
                        .build();
        metrics.put("key4", createMetric(m4));

        // List of strings
        Measurements m5 =
                Measurements.newBuilder()
                        .setStringValues(StringValues.newBuilder().addStringValue("value").build())
                        .build();
        metrics.put("key5", createMetric(m5));

        // No measurements
        Measurements m6 = Measurements.newBuilder().build();
        metrics.put("key6", createMetric(m6));

        Map<String, String> results = TfMetricProtoUtil.compatibleConvert(metrics);
        // Only the single values are converted.
        assertEquals("5.5", results.get("key1"));
        assertEquals("10", results.get("key2"));
        assertEquals("value", results.get("key3"));
        assertEquals(3, results.size());
    }

    /** Test the utility that create a single string measurement from a String. */
    @Test
    public void testStringConvert() {
        Metric m = TfMetricProtoUtil.stringToMetric("test");
        assertEquals("test", m.getMeasurements().getSingleString());
        assertEquals(DataType.RAW, m.getType());
    }

    private Metric createMetric(Measurements value) {
        return Metric.newBuilder().setMeasurements(value).build();
    }
}
