/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.graphics.tests;

import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.BugreportCollector;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Run the ImageProcessing test. The test provides benchmark for image processing
 * in Android System.
 */
public class ImageProcessingTest implements IDeviceTest, IRemoteTest {

    private ITestDevice mTestDevice = null;

    // Define instrumentation test package and runner.
    private static final String TEST_PACKAGE_NAME = "com.android.rs.image";
    private static final String TEST_RUNNER_NAME = ".ImageProcessingTestRunner";
    private static final String TEST_CLASS = "com.android.rs.image.ImageProcessingTest";
    private static final long START_TIMER = 2 * 60 * 1000; // 2 minutes

    // Define keys for data posting
    private static final String TEST_RUN_NAME = "graphics_image_processing";
    private static final String BENCHMARK_KEY = "Benchmark";
    private static final String TEST_NAME_KEY = "Testname";

    //Max test timeout - 30 mins
    private static final int MAX_TEST_TIMEOUT = 30 * 60 * 1000;

    /**
     * Run the ImageProcessing benchmark test, parse test results.
     */
    @Override
    public void run(ITestInvocationListener standardListener)
            throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);
        // Start the test after device is fully booted and stable
        // FIXME: add option in TF to wait until device is booted and stable
        RunUtil.getDefault().sleep(START_TIMER);

        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(
                TEST_PACKAGE_NAME, TEST_RUNNER_NAME, mTestDevice.getIDevice());
        runner.setClassName(TEST_CLASS);
        runner.setMaxTimeToOutputResponse(MAX_TEST_TIMEOUT, TimeUnit.MILLISECONDS);
        // Add bugreport listener for failed test
        BugreportCollector bugListener = new
            BugreportCollector(standardListener, mTestDevice);
        bugListener.addPredicate(BugreportCollector.AFTER_FAILED_TESTCASES);
        bugListener.setDescriptiveName(TEST_CLASS);

        // Add collecting listener for test results collecting
        CollectingTestListener collectListener = new CollectingTestListener();
        mTestDevice.runInstrumentationTests(runner, collectListener, bugListener, standardListener);

        // Capture a bugreport after the test
        try (InputStreamSource bugreport = mTestDevice.getBugreport()) {
            standardListener.testLog("bugreport.txt", LogDataType.BUGREPORT, bugreport);
        }

        // Collect test metrics from the instrumentation test output.
        Map<String, String> resultMetrics = new HashMap<String, String>();
        Collection<TestResult> testRunResults =
                collectListener.getCurrentRunResults().getTestResults().values();

        if (testRunResults != null) {
            for (TestResult testCaseResult: testRunResults) {
                Map<String, String> testMetrics = testCaseResult.getMetrics();
                // Each test metrics includes the following:
                // Testname=LEVELS_VEC3_FULL
                // Benchmark=50.171757
                // Iterations=5
                if (testMetrics != null) {
                    String schemaKey = null;
                    String schemaValue = null;
                    for (Map.Entry<String, String> entry : testMetrics.entrySet()) {
                        String entryKey = entry.getKey();
                        String entryValue = entry.getValue();
                        if (TEST_NAME_KEY.equals(entryKey)) {
                            schemaKey = entryValue;
                        } else if (BENCHMARK_KEY.equals(entryKey)) {
                            schemaValue = entryValue;
                        }
                    }
                    if (schemaKey != null && schemaValue != null) {
                        CLog.v(String.format("%s: %s", schemaKey, schemaValue));
                        resultMetrics.put(schemaKey, schemaValue);
                    }
                }
            }
            // Post results to the dashboard.
            reportMetrics(TEST_RUN_NAME, standardListener, resultMetrics);
        }
    }

    /**
     * Report metrics by creating an empty test run to stick them in
     */
    private void reportMetrics(String metricsName, ITestInvocationListener listener,
            Map<String, String> metrics) {
        // Create an empty testRun to report the parsed runMetrics
        CLog.d("About to report metrics to %s: %s", metricsName, metrics);
        listener.testRunStarted(metricsName, 0);
        listener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(metrics));
    }

    @Override
    public void setDevice(ITestDevice testDevice) {
        mTestDevice = testDevice;
    }

    @Override
    public ITestDevice getDevice() {
        return mTestDevice;
    }
}
