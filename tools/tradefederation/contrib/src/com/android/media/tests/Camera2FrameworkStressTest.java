/*
 * Copyright (C) 2016 The Android Open Source Project
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
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Camera2 framework stress test
 * This is a test invocation that runs stress tests against Camera2 framework (API & HAL) to
 * isolate stability issues from Camera application.
 * This invocation uses a Camera2InstrumentationTestRunner to run a set of
 * Camera framework stress tests.
 */
@OptionClass(alias = "camera2-framework-stress")
public class Camera2FrameworkStressTest extends CameraTestBase {

    // Keys in instrumentation test metrics
    private static final String RESULT_DIR = "/sdcard/camera-out/";
    private static final String RESULT_FILE_FORMAT = RESULT_DIR + "fwk-stress_camera_%s.txt";
    private static final Pattern RESULT_FILE_REGEX = Pattern.compile(
            "^fwk-stress_camera_(?<id>.+).txt");
    private static final String KEY_NUM_ATTEMPTS = "numAttempts";
    private static final String KEY_ITERATION = "iteration";

    public Camera2FrameworkStressTest() {
        // Note that default value in constructor will be overridden by the passing option from
        // a command line.
        setTestPackage("com.android.mediaframeworktest");
        setTestRunner("com.android.mediaframeworktest.Camera2InstrumentationTestRunner");
        setRuKey("CameraFrameworkStress");
        setTestTimeoutMs(2 * 60 * 60 * 1000);   // 2 hours
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
        public void handleMetricsOnTestEnded(TestDescription test, Map<String, String> testMetrics) {
            if (testMetrics == null) {
                return; // No-op if there is nothing to post.
            }
            for (Map.Entry<String, String> metric : testMetrics.entrySet()) {
                getAggregatedMetrics().put(metric.getKey(), metric.getValue());
            }
        }

        @Override
        public void testEnded(
                TestDescription test, long endTime, HashMap<String, Metric> testMetrics) {
            if (hasTestRunFatalError()) {
                CLog.v("The instrumentation result not found. Fall back to get the metrics from a "
                        + "log file. errorMsg: %s", getCollectingListener().getErrorMessage());
            }

            // For stress test, parse the metrics from a log file.
            testMetrics = TfMetricProtoUtil.upgradeConvert(parseLog(test.getTestName()));
            super.testEnded(test, endTime, testMetrics);
        }

        // Return null if failed to parse the result file or the test didn't even start.
        private Map<String, String> parseLog(String testName) {
            Map<String, String> postMetrics = new HashMap<String, String>();
            String resultFilePath;
            try {
                for (String cameraId : getCameraIdList(RESULT_DIR)) {
                    File outputFile = FileUtil.createTempFile("fwk-stress", ".txt");
                    resultFilePath = getResultFilePath(cameraId);
                    getDevice().pullFile(resultFilePath, outputFile);
                    if (outputFile == null) {
                        throw new DeviceNotAvailableException(String.format("Failed to pull the "
                                + "result file: %s", resultFilePath),
                                getDevice().getSerialNumber());
                    }

                    BufferedReader reader = new BufferedReader(new FileReader(outputFile));
                    String line;
                    Map<String, String> resultMap = new HashMap<String, String>();

                    // Parse results from log file that contain the key-value pairs.
                    // eg. "numAttempts=10|iteration=9[|cameraId=0]"
                    try {
                        while ((line = reader.readLine()) != null) {
                            String[] pairs = line.split("\\|");
                            for (String pair : pairs) {
                                String[] keyValue = pair.split("=");
                                // Each should be a pair of key and value.
                                String key = keyValue[0].trim();
                                String value = keyValue[1].trim();
                                resultMap.put(key, value);
                            }
                        }
                    } finally {
                        reader.close();
                    }

                    // Fail if a stress test doesn't start.
                    if (0 == Integer.parseInt(resultMap.get(KEY_NUM_ATTEMPTS))) {
                        CLog.w("Failed to start stress tests. test setup configured incorrectly?");
                        return null;
                    }
                    // Post the number of iterations only with the key name associated with
                    // test name and camera ID.
                    String keyName = testName + "_" + cameraId;
                    postMetrics.put(keyName, resultMap.get(KEY_ITERATION));
                }
            } catch (IOException e) {
                CLog.w("Couldn't parse the output log file");
                CLog.e(e);
            } catch (DeviceNotAvailableException e) {
                CLog.w("Could not pull file: %s, error:", RESULT_DIR);
                CLog.e(e);
            } catch (NumberFormatException e) {
                CLog.w("Could not find the key in file: %s, error:", KEY_NUM_ATTEMPTS);
                CLog.e(e);
            }
            return postMetrics;
        }
    }

    private ArrayList<String> getCameraIdList(String resultDir) throws DeviceNotAvailableException {
        // The result files are created per each camera ID
        ArrayList<String> cameraIds = new ArrayList<>();
        IFileEntry dirEntry = getDevice().getFileEntry(resultDir);
        if (dirEntry != null) {
            for (IFileEntry file : dirEntry.getChildren(false)) {
                String fileName = file.getName();
                Matcher matcher = RESULT_FILE_REGEX.matcher(fileName);
                if (matcher.matches()) {
                    cameraIds.add(matcher.group("id"));
                }
            }
        }

        if (cameraIds.isEmpty()) {
            CLog.w("No camera ID is found in %s. The resultToFile instrumentation argument is set"
                    + " to false?", resultDir);
        }
        return cameraIds;
    }

    private String getResultFilePath(String cameraId) {
        return String.format(RESULT_FILE_FORMAT, cameraId);
    }
}
