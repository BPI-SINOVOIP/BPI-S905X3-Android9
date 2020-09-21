/*
 * Copyright (C) 2012 The Android Open Source Project
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

import com.android.ddmlib.IDevice;
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.BugreportCollector;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Run UiPerformanceTest suite which measures the performance of
 * system operations and majors applications.
 */
public class UiPerformanceTest implements IDeviceTest, IRemoteTest {
    private ITestDevice mTestDevice = null;
    // Define instrumentation test package and runner.
    private static final String TEST_PACKAGE_NAME =
            "com.android.testing.uiautomation.platform.uiperformance";
    // TODO: Add TEST_CLASS_NAME later when different tests requiring
    //       different configurations.
    private static final String TEST_RUNNER_NAME =
            "com.android.testing.uiautomation.UiAutomationTestRunner";
    private static final String OUTPUT_FILE_NAME = "UiPerfTestsOutput.txt"; // output file
    private static final String RAW_DATA_DIRECTORY = "UiPerformanceRawData"; // raw data directory

    private static final String TEST_CASE_PREFIX = "test";
    private static final long START_TIMER = 2 * 60 * 1000; // 2 minutes

    private static final Pattern JANKINESS_PATTERN =
            Pattern.compile("^number of jankiness: (\\d+)");
    private static final Pattern MEDIAN_FRAME_LATENCY_PATTERN =
            Pattern.compile("^median of frame latency: (\\d+)");
    private static final Pattern FRAME_RATE_PATTERN =
            Pattern.compile("^average frame rate: (\\d+\\.\\d+)");
    private static final Pattern[] mPatterns = {
        JANKINESS_PATTERN, FRAME_RATE_PATTERN, MEDIAN_FRAME_LATENCY_PATTERN
    };
    private static final String[] ITEM_KEYS = {"number_jankiness", "frame_rate", "frame_latency"};

    @Override
    public void setDevice(ITestDevice testDevice) {
        mTestDevice = testDevice;
    }

    @Override
    public ITestDevice getDevice() {
        return mTestDevice;
    }

    private void setupDevice() throws DeviceNotAvailableException {
        cleanOutputFiles();
        String extStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        String rawFileDir = String.format("%s/%s", extStore, RAW_DATA_DIRECTORY);

        if (!mTestDevice.doesFileExist(rawFileDir)) {
            CLog.v(String.format("The raw directory %s doesn't exist.", RAW_DATA_DIRECTORY));
            mTestDevice.executeShellCommand(String.format("mkdir \"%s\"", rawFileDir));
        } else {
            // remove files
            mTestDevice.executeShellCommand(String.format("rm %s/*", rawFileDir));
            CLog.v("remove files under the raw data directory");
        }
    }

    /**
     * Run UiPerformanceTests and parsing results from test output.
     */
    @Override
    public void run(ITestInvocationListener standardListener)
            throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);
        setupDevice();
        // start the test until device is fully booted and stable
        RunUtil.getDefault().sleep(START_TIMER);
        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(
                TEST_PACKAGE_NAME, TEST_RUNNER_NAME, mTestDevice.getIDevice());

        // Add bugreport listener for failed test
        BugreportCollector bugListener = new
            BugreportCollector(standardListener, mTestDevice);
        bugListener.addPredicate(BugreportCollector.AFTER_FAILED_TESTCASES);
        bugListener.setDescriptiveName(this.getClass().getName());
        mTestDevice.runInstrumentationTests(runner, bugListener);
        logOutputFile(bugListener);
        pullRawDataFile(bugListener);
        cleanOutputFiles();
    }

    private void pullRawDataFile(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        String extStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        String rawFileDir = String.format("%s/%s", extStore, RAW_DATA_DIRECTORY);

        String rawFileList =
                mTestDevice.executeShellCommand(String.format("ls \"%s\"", rawFileDir));
        String[] rawFileString = rawFileList.split("\r?\n");
        File resFile = null;
        InputStreamSource outputSource = null;
        for (int i = 0; i < rawFileString.length; i++) {
            CLog.v("file %d is:  \"%s\"", i, rawFileString[i]);
            try {
                resFile = mTestDevice.pullFileFromExternal(
                        String.format("%s/%s", RAW_DATA_DIRECTORY, rawFileString[i]));
                outputSource = new FileInputStreamSource(resFile, true /* delete */);
                listener.testLog(rawFileString[i], LogDataType.TEXT, outputSource);
            } finally {
                StreamUtil.cancel(outputSource);
            }
        }
    }


    // Parse the output file
    private void logOutputFile(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        // catch a bugreport after the test
        try (InputStreamSource bugreport = mTestDevice.getBugreport()) {
            listener.testLog("bugreport", LogDataType.BUGREPORT, bugreport);
        }

        File resFile = null;
        InputStreamSource outputSource = null;
        Map<String, String> runMetrics = new HashMap<>();
        BufferedReader br = null;
        try {
            resFile = mTestDevice.pullFileFromExternal(OUTPUT_FILE_NAME);
            if (resFile == null) {
                CLog.v("File %s doesn't exist or pulling the file failed");
                return;
            }
            CLog.d("output file: %s", resFile.getPath());
            // Save a copy of the output file
            CLog.d("Sending %d byte file %s into the logosphere!",
                    resFile.length(), resFile);
            outputSource = new FileInputStreamSource(resFile);
            listener.testLog(OUTPUT_FILE_NAME, LogDataType.TEXT, outputSource);

            // Parse the results file
            br = new BufferedReader(new FileReader(resFile));
            String line = null;
            String unitKey = null;
            int size = mPatterns.length;
            while ((line = br.readLine()) != null) {
                if (line.startsWith(TEST_CASE_PREFIX)) {
                    // report the previous test case results
                    if (unitKey != null) {
                        reportMetrics(unitKey, listener, runMetrics);
                    }
                    runMetrics.clear();
                    // processing the next test case
                    unitKey = line.trim();
                    continue;
                } else {
                    for (int i = 0; i < size; i++) {
                        Matcher match = mPatterns[i].matcher(line);
                        if (match.matches()) {
                            String value = match.group(1);
                            runMetrics.put(ITEM_KEYS[i], value);
                            break;
                        }
                    }
                }
            }
            reportMetrics(unitKey, listener, runMetrics);
        } catch (IOException e) {
            CLog.e("IOException while reading outputfile %s", OUTPUT_FILE_NAME);
        } finally {
            FileUtil.deleteFile(resFile);
            StreamUtil.cancel(outputSource);
            StreamUtil.close(br);
        }
    }

    // Report run metrics by creating an empty test run to stick them in
    private void reportMetrics(String metricsName, ITestInvocationListener listener,
            Map<String, String> metrics) {
        // Create an empty testRun to report the parsed runMetrics
        CLog.d("About to report metrics to %s: %s", metricsName, metrics);
        listener.testRunStarted(metricsName, 0);
        listener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(metrics));
    }

    // clean up output file
    private void cleanOutputFiles() throws DeviceNotAvailableException {
        String extStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        mTestDevice.executeShellCommand(String.format("rm %s/%s", extStore, OUTPUT_FILE_NAME));
    }
}
