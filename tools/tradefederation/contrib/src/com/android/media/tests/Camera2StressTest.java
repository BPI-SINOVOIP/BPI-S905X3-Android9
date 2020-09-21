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
import com.android.tradefed.device.IFileEntry;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.FileUtil;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Camera2 stress test
 * Since Camera stress test can drain the battery seriously. Need to split
 * the test suite into separate test invocation for each test method.
 * <p/>
 */
@OptionClass(alias = "camera2-stress")
public class Camera2StressTest extends CameraTestBase {

    private static final String RESULT_FILE = "/sdcard/camera-out/stress.txt";
    private static final String FAILURE_SCREENSHOT_DIR = "/sdcard/camera-screenshot/";
    private static final String KEY_NUM_ATTEMPTS = "numAttempts";
    private static final String KEY_ITERATION = "iteration";

    public Camera2StressTest() {
        setTestPackage("com.google.android.camera");
        setTestClass("com.android.camera.stress.CameraStressTest");
        setTestRunner("android.test.InstrumentationTestRunner");
        setRuKey("CameraAppStress");
        setTestTimeoutMs(6 * 60 * 60 * 1000);   // 6 hours
        setLogcatOnFailure(true);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        runInstrumentationTest(listener, new CollectingListener(listener));
    }

    /**
     * A listener to collect the output from test run and fatal errors
     */
    private class CollectingListener extends DefaultCollectingListener {

        public CollectingListener(ITestInvocationListener listener) {
            super(listener);
        }

        @Override
        public void testEnded(
                TestDescription test, long endTime, HashMap<String, Metric> testMetrics) {
            if (hasTestRunFatalError()) {
                CLog.v("The instrumentation result not found. Fall back to get the metrics from a "
                        + "log file. errorMsg: %s", getCollectingListener().getErrorMessage());
            }
            // TODO: Will get the additional metrics to file to prevent result loss

            // Don't need to report the KEY_NUM_ATTEMPS to dashboard
            // Iteration will be read from file
            testMetrics.remove(KEY_NUM_ATTEMPTS);
            testMetrics.remove(KEY_ITERATION);

            // add testMethod name to the metric
            Map<String, String> namedTestMetrics = new HashMap<>();
            for (Entry<String, Metric> entry : testMetrics.entrySet()) {
                namedTestMetrics.put(
                        test.getTestName() + entry.getKey(),
                        entry.getValue().getMeasurements().getSingleString());
            }

            // parse the iterations metrics from the stress log files
            parseLog(test.getTestName(), namedTestMetrics);

            postScreenshotOnFailure(test);
            super.testEnded(test, endTime, namedTestMetrics);
        }

        private void postScreenshotOnFailure(TestDescription test) {
            File tmpDir = null;
            try {
                IFileEntry screenshotDir = getDevice().getFileEntry(FAILURE_SCREENSHOT_DIR);
                if (screenshotDir != null && screenshotDir.isDirectory()) {
                    tmpDir = FileUtil.createTempDir("screenshot");
                    for (IFileEntry remoteFile : screenshotDir.getChildren(false)) {
                        if (remoteFile.isDirectory()) {
                            continue;
                        }
                        if (!remoteFile.getName().contains(test.getTestName())) {
                            CLog.v("Skipping the screenshot (%s) that doesn't match the current "
                                    + "test (%s)", remoteFile.getName(), test.getTestName());
                            continue;
                        }
                        File screenshot = new File(tmpDir, remoteFile.getName());
                        if (!getDevice().pullFile(remoteFile.getFullPath(), screenshot)) {
                            CLog.w("Could not pull screenshot: %s", remoteFile.getFullPath());
                            continue;
                        }
                        testLog(
                                "screenshot_" + screenshot.getName(),
                                LogDataType.PNG,
                                new FileInputStreamSource(screenshot));
                    }
                }
            } catch (DeviceNotAvailableException e) {
                CLog.e(e);
            } catch (IOException e) {
                CLog.e(e);
            } finally {
                FileUtil.recursiveDelete(tmpDir);
            }
        }

        // Return null if failed to parse the result file or the test didn't even start.
        private void parseLog(String testName, Map<String, String> testMetrics) {
            try {
                File outputFile = FileUtil.createTempFile("stress", ".txt");
                getDevice().pullFile(RESULT_FILE, outputFile);
                if (outputFile == null) {
                    throw new DeviceNotAvailableException(String.format("Failed to pull the result"
                            + "file: %s", RESULT_FILE), getDevice().getSerialNumber());
                }
                BufferedReader reader = new BufferedReader(new FileReader(outputFile));
                String line;
                Map<String, String> resultMap = new HashMap<>();

                // Parse results from log file that contain the key-value pairs.
                // eg. "numAttempts=10|iteration=9"
                try {
                    while ((line = reader.readLine()) != null) {
                        String[] pairs = line.split("\\|");
                        for (String pair : pairs) {
                            String[] keyValue = pair.split("=");
                            // Each should be a pair of key and value.
                            String key = keyValue[0].trim();
                            String value = keyValue[1].trim();
                            resultMap.put(key, value);
                            CLog.v("%s: %s", key, value);
                        }
                    }
                } finally {
                    reader.close();
                }

                // Fail if a stress test doesn't start.
                if (0 == Integer.parseInt(resultMap.get(KEY_NUM_ATTEMPTS))) {
                    CLog.w("Failed to start stress tests. test setup configured incorrectly?");
                    return;
                }
                // Post the number of iterations only with the test name as key.
                testMetrics.put(testName, resultMap.get(KEY_ITERATION));
            } catch (IOException e) {
                CLog.w("Couldn't parse the output log file:");
                CLog.e(e);
            } catch (DeviceNotAvailableException e) {
                CLog.w("Could not pull file: %s, error:", RESULT_FILE);
                CLog.e(e);
            } catch (NumberFormatException e) {
                CLog.w("Could not find the key in file: %s, error:", KEY_NUM_ATTEMPTS);
                CLog.e(e);
            }
        }
    }
}
