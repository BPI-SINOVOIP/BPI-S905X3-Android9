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
 * Camera burst startup test
 *
 * <p>Runs Camera device performance test to measure time for taking a burst shot.
 */
@OptionClass(alias = "camera-burst-shot")
public class CameraBurstStartupTest extends CameraTestBase {

    private static final Pattern STATS_REGEX = Pattern.compile("^(?<average>[0-9.]+)");

    public CameraBurstStartupTest() {
        setTestPackage("com.google.android.camera");
        setTestClass("com.android.camera.latency.BurstStartupTest");
        setTestRunner("android.test.InstrumentationTestRunner");
        setRuKey("CameraBurstStartup");
        setTestTimeoutMs(60 * 60 * 1000); // 1 hour
    }

    /** {@inheritDoc} */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        runInstrumentationTest(listener, new CollectingListener(listener));
    }

    /** A listener to collect the output from test run and fatal errors */
    private class CollectingListener extends DefaultCollectingListener {

        public CollectingListener(ITestInvocationListener listener) {
            super(listener);
        }

        @Override
        public void handleMetricsOnTestEnded(TestDescription test, Map<String, String> testMetrics) {
            // Test metrics accumulated will be posted at the end of test run.
            getAggregatedMetrics().putAll(parseResults(test.getTestName(), testMetrics));
        }

        public Map<String, String> parseResults(String testName, Map<String, String> testMetrics) {
            // Parse burst startup stats from the instrumentation result.
            Map<String, String> parsed = new HashMap<String, String>();
            for (Map.Entry<String, String> metric : testMetrics.entrySet()) {
                Matcher matcher = STATS_REGEX.matcher(metric.getValue());

                if (matcher.matches()) {
                    // Key name consists of a pair of test name and metric name.
                    String keyName = String.format("%s_%s", testName, metric.getKey());
                    parsed.put(keyName, matcher.group("average"));
                } else {
                    CLog.w(String.format("Stats not in correct format: %s", metric.getValue()));
                }
            }
            return parsed;
        }
    }
}
