/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.compatibility.common.tradefed.testtype;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.testtype.HostTest;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.util.HashMap;
import java.util.Map;

/**
 * Test runner for host-side JUnit tests.
 */
public class JarHostTest extends HostTest {

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        int numTests = 0;
        RuntimeException bufferedException = null;
        try {
            numTests = countTestCases();
        } catch (RuntimeException e) {
            bufferedException = e;
        }
        long startTime = System.currentTimeMillis();
        listener.testRunStarted(getClass().getName(), numTests);
        HostTestListener hostListener = new HostTestListener(listener);
        try {
            if (bufferedException != null) {
                throw bufferedException;
            }
            super.run(hostListener);
        } finally {
            HashMap<String, Metric> metrics = hostListener.getNewMetrics();
            metrics.putAll(TfMetricProtoUtil.upgradeConvert(hostListener.getMetrics()));
            listener.testRunEnded(System.currentTimeMillis() - startTime, metrics);
        }
    }

    /**
     * Wrapper listener that forwards all events except testRunStarted() and testRunEnded() to
     * the embedded listener. Each test class in the jar will invoke these events, which
     * HostTestListener withholds from listeners for console logging and result reporting.
     */
    public class HostTestListener extends ResultForwarder {

        private Map<String, String> mCollectedMetrics = new HashMap<>();
        private HashMap<String, Metric> mCollectedNewMetrics = new HashMap<>();

        public HostTestListener(ITestInvocationListener listener) {
            super(listener);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void testRunStarted(String name, int numTests) {
            CLog.d("HostTestListener.testRunStarted(%s, %d)", name, numTests);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void testRunEnded(long elapsedTime, Map<String, String> metrics) {
            CLog.d("HostTestListener.testRunEnded(%d, %s)", elapsedTime, metrics.toString());
            mCollectedMetrics.putAll(metrics);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void testRunEnded(long elapsedTime, HashMap<String, Metric> metrics) {
            CLog.d("HostTestListener.testRunEnded(%d, %s)", elapsedTime, metrics.toString());
            mCollectedNewMetrics.putAll(metrics);
        }

        /**
         * Returns all the metrics reported by the tests
         */
        Map<String, String> getMetrics() {
            return mCollectedMetrics;
        }

        /**
         * Returns all the proto metrics reported by the tests
         */
        HashMap<String, Metric> getNewMetrics() {
            return mCollectedNewMetrics;
        }
    }
}
