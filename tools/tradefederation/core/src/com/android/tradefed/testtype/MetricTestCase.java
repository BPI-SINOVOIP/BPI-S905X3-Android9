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
package com.android.tradefed.testtype;

import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.SnapshotInputStreamSource;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import junit.framework.TestCase;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * Extension of {@link TestCase} that allows to log metrics when running as part of TradeFed. Either
 * directly as a {@link DeviceTestCase} or as part of a {@link HostTest}. TODO: Evaluate if having
 * run metric (not only test metric) make sense for JUnit3 tests.
 */
public class MetricTestCase extends TestCase {

    public HashMap<String, Metric> mMetrics = new HashMap<>();
    public List<LogHolder> mLogs = new ArrayList<>();

    public MetricTestCase() {
        super();
    }

    /** Constructs a test case with the given name. Inherited from {@link TestCase} constructor. */
    public MetricTestCase(String name) {
        super(name);
    }

    /**
     * Log a metric for the test case.
     *
     * @param key the key under which the metric will be found.
     * @param value associated to the key.
     */
    public final void addTestMetric(String key, String value) {
        mMetrics.put(key, TfMetricProtoUtil.stringToMetric(value));
    }

    public final void addTestMetric(String key, Metric metric) {
        mMetrics.put(key, metric);
    }

    /**
     * Callback from JUnit3 forwarder in order to get the logs from a test.
     *
     * @param dataName a String descriptive name of the data. e.g. "device_logcat". Note dataName
     *     may not be unique per invocation. ie implementers must be able to handle multiple calls
     *     with same dataName
     * @param dataType the LogDataType of the data
     * @param dataStream the InputStreamSource of the data. Implementers should call
     *     createInputStream to start reading the data, and ensure to close the resulting
     *     InputStream when complete. Callers should ensure the source of the data remains present
     *     and accessible until the testLog method completes.
     */
    public final void addTestLog(
            String dataName, LogDataType dataType, InputStreamSource dataStream) {
        mLogs.add(new LogHolder(dataName, dataType, dataStream));
    }

    /** Structure to hold a log file to be reported. */
    public static class LogHolder {
        public final String mDataName;
        public final LogDataType mDataType;
        public final InputStreamSource mDataStream;

        public LogHolder(String dataName, LogDataType dataType, InputStreamSource dataStream) {
            mDataName = dataName;
            mDataType = dataType;
            // We hold a copy because the caller will most likely cancel the stream after.
            mDataStream =
                    new SnapshotInputStreamSource("LogHolder", dataStream.createInputStream());
        }
    }
}
