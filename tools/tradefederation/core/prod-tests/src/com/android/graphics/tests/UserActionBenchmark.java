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

import com.android.ddmlib.IDevice;
import com.android.ddmlib.Log;
import com.android.ddmlib.NullOutputReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
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

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Runs the user action framerate benchmark test. This test capture
 * use the scripted monkey to inject the keyevent to mimic the real
 * user action, capture the average framerate and save the result
 * file to the sdcard.
 * <p/>
 * Note that this test will not run properly unless /sdcard is mounted and
 * writable.
 */
public class UserActionBenchmark implements IDeviceTest, IRemoteTest {
    private static final String LOG_TAG = "UserActionBenchmark";

    ITestDevice mTestDevice = null;

    private static final long START_TIMER = 2 * 60 * 1000; // 2 minutes

    @Option(name = "test-output-filename", description = "The test output filename.")
    private String mDeviceTestOutputFilename = "avgFrameRateOut.txt";

    // The time in ms to wait the scripted monkey finish.
    private static final int CMD_TIMEOUT = 60 * 60 * 1000;

    private static final Pattern AVERAGE_FPS = Pattern.compile("(.*):(\\d+.\\d+)");

    @Option(name = "test-case", description = "The name of test-cases  to run. May be repeated.")
    private Collection<String> mTestCases = new ArrayList<>();

    @Option(name = "iteration", description = "Test run iteration")
    private int mIteration = 1;

    @Option(name = "throttle", description = "Scripted monkey throttle time")
    private int mThrottle = 500; // in milliseconds

    @Option(name = "script-path", description = "Test script path")
    private String mScriptPath = "userActionFPSScript";

    @Option(name = "test-label", description = "Test label")
    private String mTestLabel = "UserActionFramerateBenchmark";

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);

        // Start the test after device is fully booted and stable
        // FIXME: add option in TF to wait until device is booted and stable
        RunUtil.getDefault().sleep(START_TIMER);

        String extStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        String scriptFullPath =
            String.format("%s/%s/%s", extStore, mScriptPath, mTestDevice.getProductType());
        for (String testCase : mTestCases) {
            // Start the scripted monkey command
            mTestDevice.executeShellCommand(String.format(
                "monkey -f /%s/%s.txt --throttle %d %d", scriptFullPath,
                testCase, mThrottle, mIteration), new NullOutputReceiver(),
                CMD_TIMEOUT, TimeUnit.MILLISECONDS, 2);
            logOutputFiles(listener);
            cleanResultFile();
        }
    }

    /**
     * Clean up the test result file from test run
     */
    private void cleanResultFile() throws DeviceNotAvailableException {
        String extStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        mTestDevice.executeShellCommand(String.format("rm %s/%s", extStore,
                mDeviceTestOutputFilename));
    }

    /**
     * Pull the output files from the device, add it to the logs, and also parse
     * out the relevant test metrics and report them.
     */
    private void logOutputFiles(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        File outputFile = null;
        InputStreamSource outputSource = null;
        try {
            outputFile = mTestDevice.pullFileFromExternal(mDeviceTestOutputFilename);

            if (outputFile == null) {
                return;
            }

            // Upload a verbatim copy of the output file
            Log.d(LOG_TAG, String.format("Sending %d byte file %s into the logosphere!",
                    outputFile.length(), outputFile));
            outputSource = new FileInputStreamSource(outputFile);
            listener.testLog(mDeviceTestOutputFilename, LogDataType.TEXT, outputSource);

            // Parse the output file to upload aggregated metrics
            parseOutputFile(new FileInputStream(outputFile), listener);
        } catch (IOException e) {
            Log.e(LOG_TAG, String.format(
                            "IOException while reading or parsing output file: %s", e));
        } finally {
            FileUtil.deleteFile(outputFile);
            StreamUtil.cancel(outputSource);
        }
    }

    /**
     * Parse the test result, calculate the average and parse the metrics
     * from the scripted monkey test output file
     */
    private void parseOutputFile(InputStream dataStream,
            ITestInvocationListener listener) {

        Map<String, String> runMetrics = new HashMap<>();

        // try to parse it
        String contents;
        try {
            contents = StreamUtil.getStringFromStream(dataStream);
        } catch (IOException e) {
            Log.e(LOG_TAG, String.format(
                    "Got IOException during test processing: %s", e));
            return;
        }

        List<String> lines = Arrays.asList(contents.split("\n"));
        String key = null;
        float averageResult;
        float totalResult = 0;
        int counter = 0;

        // collect the result and calculate the average
        for (String line: lines) {
            Matcher m = AVERAGE_FPS.matcher(line);
            if (m.matches()) {
                key = m.group(1);
                totalResult += Float.parseFloat(m.group(2));
                counter++;
            }
        }
        averageResult = totalResult / counter;
        Log.i(LOG_TAG, String.format("averageResult = %s\n", averageResult));
        runMetrics.put(key, Float.toString(averageResult));
        reportMetrics(listener, runMetrics);
    }

    /**
     * Report run metrics by creating an empty test run to stick them in
     * <p />
     * Exposed for unit testing
     */
    void reportMetrics(ITestInvocationListener listener, Map<String, String> metrics) {
        Log.d(LOG_TAG, String.format("About to report metrics: %s", metrics));
        listener.testRunStarted(mTestLabel, 0);
        listener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(metrics));
    }

    @Override
    public void setDevice(ITestDevice device) {
        mTestDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mTestDevice;
    }
}
