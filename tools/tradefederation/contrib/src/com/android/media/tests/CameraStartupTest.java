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

import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.targetprep.TemperatureThrottlingWaiter;
import com.android.tradefed.util.MultiMap;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Camera app startup test
 *
 * Runs CameraActivityTest to measure Camera startup time and reports the metrics.
 */
@OptionClass(alias = "camera-startup")
public class CameraStartupTest extends CameraTestBase {

    private static final Pattern STATS_REGEX = Pattern.compile(
        "^(?<coldStartup>[0-9.]+)\\|(?<warmStartup>[0-9.]+)\\|(?<values>[0-9 .-]+)");
    private static final String PREFIX_COLD_STARTUP = "Cold";
    // all metrics are expected to be less than 10 mins and greater than 0.
    private static final int METRICS_MAX_THRESHOLD_MS = 10 * 60 * 1000;
    private static final int METRICS_MIN_THRESHOLD_MS = 0;
    private static final String INVALID_VALUE = "-1";

    @Option(name="num-test-runs", description="The number of test runs. A instrumentation "
            + "test will be repeatedly executed. Then it posts the average of test results.")
    private int mNumTestRuns = 1;

    @Option(name="delay-between-test-runs", description="Time delay between multiple test runs, "
            + "in msecs. Used to wait for device to cool down. "
            + "Note that this will be ignored when TemperatureThrottlingWaiter is configured.")
    private long mDelayBetweenTestRunsMs = 120 * 1000;  // 2 minutes

    private MultiMap<String, String> mMultipleRunMetrics = new MultiMap<String, String>();
    private Map<String, String> mAverageMultipleRunMetrics = new HashMap<String, String>();
    private long mTestRunsDurationMs = 0;

    public CameraStartupTest() {
        setTestPackage("com.google.android.camera");
        setTestClass("com.android.camera.latency.CameraStartupTest");
        setTestRunner("android.test.InstrumentationTestRunner");
        setRuKey("CameraAppStartup");
        setTestTimeoutMs(60 * 60 * 1000);   // 1 hour
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        runMultipleInstrumentationTests(listener, mNumTestRuns);
    }

    private void runMultipleInstrumentationTests(ITestInvocationListener listener, int numTestRuns)
            throws DeviceNotAvailableException {
        Assert.assertTrue(numTestRuns > 0);

        mTestRunsDurationMs = 0;
        for (int i = 0; i < numTestRuns; ++i) {
            CLog.v("Running multiple instrumentation tests... [%d/%d]", i + 1, numTestRuns);
            CollectingListener singleRunListener = new CollectingListener(listener);
            runInstrumentationTest(listener, singleRunListener);
            mTestRunsDurationMs += getTestDurationMs();

            if (singleRunListener.hasFailedTests() ||
                    singleRunListener.hasTestRunFatalError()) {
                exitTestRunsOnError(listener, singleRunListener.getErrorMessage());
                return;
            }
            if (i + 1 < numTestRuns) {  // Skipping preparation on the last run
                postSetupTestRun();
            }
        }

        // Post the average of metrics collected in multiple instrumentation test runs.
        postMultipleRunMetrics(listener);
        CLog.v("multiple instrumentation tests end");
    }

    private void exitTestRunsOnError(ITestInvocationListener listener, String errorMessage) {
        CLog.e("The instrumentation result not found. Test runs may have failed due to exceptions."
                + " Test results will not be posted. errorMsg: %s", errorMessage);
        listener.testRunFailed(errorMessage);
        listener.testRunEnded(mTestRunsDurationMs, new HashMap<String, Metric>());
    }

    private void postMultipleRunMetrics(ITestInvocationListener listener) {
        listener.testRunEnded(mTestRunsDurationMs,
                TfMetricProtoUtil.upgradeConvert(getAverageMultipleRunMetrics()));
    }

    private void postSetupTestRun() throws DeviceNotAvailableException {
        // Reboot for a cold start up of Camera application
        CLog.d("Cold start: Rebooting...");
        getDevice().reboot();

        // Wait for device to cool down to target temperature
        // Use TemperatureThrottlingWaiter if configured, otherwise just wait for
        // a specific amount of time.
        CLog.d("Cold start: Waiting for device to cool down...");
        boolean usedTemperatureThrottlingWaiter = false;
        for (ITargetPreparer preparer : mConfiguration.getTargetPreparers()) {
            if (preparer instanceof TemperatureThrottlingWaiter) {
                usedTemperatureThrottlingWaiter = true;
                try {
                    preparer.setUp(getDevice(), null);
                } catch (TargetSetupError e) {
                    CLog.w("No-op even when temperature is still high after wait timeout. "
                        + "error: %s", e.getMessage());
                } catch (BuildError e) {
                    // This should not happen.
                }
            }
        }
        if (!usedTemperatureThrottlingWaiter) {
            getRunUtil().sleep(mDelayBetweenTestRunsMs);
        }
        CLog.d("Device gets prepared for the next test run.");
    }

    // Call this function once at the end to get the average.
    private Map<String, String> getAverageMultipleRunMetrics() {
        Assert.assertTrue(mMultipleRunMetrics.size() > 0);

        Set<String> keys = mMultipleRunMetrics.keySet();
        mAverageMultipleRunMetrics.clear();
        for (String key : keys) {
            int sum = 0;
            int size = 0;
            boolean isInvalid = false;
            for (String valueString : mMultipleRunMetrics.get(key)) {
                int value = Integer.parseInt(valueString);
                // If value is out of valid range, skip posting the result associated with the key
                if (value > METRICS_MAX_THRESHOLD_MS || value < METRICS_MIN_THRESHOLD_MS) {
                    isInvalid = true;
                    break;
                }
                sum += value;
                ++size;
            }

            String valueString = INVALID_VALUE;
            if (isInvalid) {
                CLog.w("Value is out of valid range. Key: %s ", key);
            } else {
                valueString = String.format("%d", (sum / size));
            }
            mAverageMultipleRunMetrics.put(key, valueString);
        }
        return mAverageMultipleRunMetrics;
    }

    /**
     * A listener to collect the output from test run and fatal errors
     */
    private class CollectingListener extends DefaultCollectingListener {

        public CollectingListener(ITestInvocationListener listener) {
            super(listener);
        }

        @Override
        public void handleMetricsOnTestEnded(TestDescription test, Map<String, String> testMetrics) {
            // Test metrics accumulated will be posted at the end of test run.
            getAggregatedMetrics().putAll(parseResults(testMetrics));
        }

        @Override
        public void handleTestRunEnded(ITestInvocationListener listener, long elapsedTime,
                Map<String, String> runMetrics) {
            // Do not post aggregated metrics from a single run to a dashboard. Instead, it needs
            // to collect all metrics from multiple test runs.
            mMultipleRunMetrics.putAll(getAggregatedMetrics());
        }

        public Map<String, String> parseResults(Map<String, String> testMetrics) {
            // Parse activity time stats from the instrumentation result.
            // Format : <metric_key>=<cold_startup>|<average_of_warm_startups>|<all_startups>
            // Example:
            // VideoStartupTimeMs=1098|1184.6|1098 1222 ... 788
            // VideoOnCreateTimeMs=138|103.3|138 114 ... 114
            // VideoOnResumeTimeMs=39|40.4|39 36 ... 41
            // VideoFirstPreviewFrameTimeMs=0|0.0|0 0 ... 0
            // CameraStartupTimeMs=2388|1045.4|2388 1109 ... 746
            // CameraOnCreateTimeMs=574|122.7|574 124 ... 109
            // CameraOnResumeTimeMs=610|504.6|610 543 ... 278
            // CameraFirstPreviewFrameTimeMs=0|0.0|0 0 ... 0
            //
            // Then report only the first two startup time of cold startup and average warm startup.
            Map<String, String> parsed = new HashMap<String, String>();
            for (Map.Entry<String, String> metric : testMetrics.entrySet()) {
                Matcher matcher = STATS_REGEX.matcher(metric.getValue());
                String keyName = metric.getKey();
                String coldStartupValue = INVALID_VALUE;
                String warmStartupValue = INVALID_VALUE;
                if (matcher.matches()) {
                    coldStartupValue = matcher.group("coldStartup");
                    warmStartupValue = matcher.group("warmStartup");
                }
                parsed.put(PREFIX_COLD_STARTUP + keyName, coldStartupValue);
                parsed.put(keyName, warmStartupValue);
            }
            return parsed;
        }
    }
}
