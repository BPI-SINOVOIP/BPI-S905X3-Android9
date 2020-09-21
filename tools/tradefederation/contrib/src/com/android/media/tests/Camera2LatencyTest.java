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

package com.android.media.tests;

import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;

import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Camera app latency test
 *
 * Runs Camera app latency test to measure Camera capture session time and reports the metrics.
 */
@OptionClass(alias = "camera-latency")
public class Camera2LatencyTest extends CameraTestBase {

    public Camera2LatencyTest() {
        setTestPackage("com.google.android.camera");
        setTestClass("com.android.camera.latency.CameraCaptureSessionTest");
        setTestRunner("android.test.InstrumentationTestRunner");
        setRuKey("CameraAppLatency");
        setTestTimeoutMs(60 * 60 * 1000);   // 1 hour
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        runInstrumentationTest(listener, new CollectingListener(listener));
    }

    /**
     * A listener to collect the results from each test run, then forward them to other listeners.
     */
    private class CollectingListener extends DefaultCollectingListener {

        public CollectingListener(ITestInvocationListener listener) {
            super(listener);
        }

        @Override
        public void handleMetricsOnTestEnded(TestDescription test, Map<String, String> testMetrics) {
            // Test metrics accumulated will be posted at the end of test run.
            getAggregatedMetrics().putAll(parseResults(test.getTestName(), testMetrics));
        }

        private Map<String, String> parseResults(String testName, Map<String, String> testMetrics) {
            final Pattern STATS_REGEX = Pattern.compile(
                    "^(?<latency>[0-9.]+)\\|(?<values>[0-9 .-]+)");

            // Parse activity time stats from the instrumentation result.
            // Format : <metric_key>=<average_of_latency>|<raw_data>
            // Example:
            // FirstCaptureResultTimeMs=38|13 48 ... 35
            // SecondCaptureResultTimeMs=29.2|65 24 ... 0
            // CreateTimeMs=373.6|382 364 ... 323
            //
            // Then report only the first two startup time of cold startup and average warm startup.
            Map<String, String> parsed = new HashMap<String, String>();
            for (Map.Entry<String, String> metric : testMetrics.entrySet()) {
                Matcher matcher = STATS_REGEX.matcher(metric.getValue());
                if (matcher.matches()) {
                    String keyName = String.format("%s_%s", testName, metric.getKey());
                    parsed.put(keyName, matcher.group("latency"));
                } else {
                    CLog.w(String.format("Stats not in correct format: %s", metric.getValue()));
                }
            }
            return parsed;
        }
    }
}
