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

package com.android.media.tests;

import com.android.ddmlib.IDevice;
import com.android.ddmlib.Log;
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.BugreportCollector;
import com.android.tradefed.result.BugreportCollector.Freq;
import com.android.tradefed.result.BugreportCollector.Noun;
import com.android.tradefed.result.BugreportCollector.Relation;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.RegexTrie;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Runs the Media Player stress test. This test will play the video files under
 * the /sdcard/samples folder and capture the video playback event statistics in
 * a text file under /sdcard/PlaybackTestResult.txt
 * <p/>
 * Note that this test will not run properly unless /sdcard is mounted and
 * writable.
 */
public class MediaPlayerStressTest implements IDeviceTest, IRemoteTest {
    private static final String LOG_TAG = "MediaPlayerStress";

    ITestDevice mTestDevice = null;
    @Option(name = "test-class", importance = Importance.ALWAYS)
    private String mTestClassName =
            "com.android.mediaframeworktest.stress.MediaPlayerStressTest";
    @Option(name = "metrics-name", importance = Importance.ALWAYS)
    private String mMetricsRunName = "MediaPlayerStress";
    @Option(name = "result-file", importance = Importance.ALWAYS)
    private String mOutputPath = "PlaybackTestResult.txt";

    //Max test timeout - 10 hrs
    private static final int MAX_TEST_TIMEOUT = 10 * 60 * 60 * 1000;

    // Constants for running the tests
    private static final String TEST_PACKAGE_NAME = "com.android.mediaframeworktest";
    private static final String TEST_RUNNER_NAME = ".MediaPlayerStressTestRunner";

    public RegexTrie<String> mPatternMap = new RegexTrie<>();

    public MediaPlayerStressTest() {
        mPatternMap.put("PlaybackPass", "^Total Complete: (\\d+)");
        mPatternMap.put("PlaybackCrash", "^Total Error: (\\d+)");
        mPatternMap.put("TrackLagging", "^Total Track Lagging: (\\d+)");
        mPatternMap.put("BadInterleave", "^Total Bad Interleaving: (\\d+)");
        mPatternMap.put("FailedToCompleteWithNoError",
                "^Total Failed To Complete With No Error: (\\d+)");
    }

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);
        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(TEST_PACKAGE_NAME,
                TEST_RUNNER_NAME, mTestDevice.getIDevice());
        runner.setClassName(mTestClassName);
        runner.setMaxTimeToOutputResponse(MAX_TEST_TIMEOUT, TimeUnit.MILLISECONDS);

        BugreportCollector bugListener = new BugreportCollector(listener,
                mTestDevice);
        bugListener.addPredicate(BugreportCollector.AFTER_FAILED_TESTCASES);
        bugListener.setDescriptiveName("media_player_stress_test");
        bugListener.addPredicate(new BugreportCollector.Predicate(
                Relation.AFTER, Freq.EACH, Noun.TESTRUN));

        mTestDevice.runInstrumentationTests(runner, bugListener);

        logOutputFile(listener);
        cleanResultFile();
    }

    /**
     * Clean up the test result file from test run
     */
    private void cleanResultFile() throws DeviceNotAvailableException {
        String extStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        mTestDevice.executeShellCommand(String.format("rm %s/%s", extStore, mOutputPath));
    }

    /**
     * Pull the output file from the device, add it to the logs, and also parse
     * out the relevant test metrics and report them.
     */
    private void logOutputFile(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        File outputFile = null;
        InputStreamSource outputSource = null;
        try {
            outputFile = mTestDevice.pullFileFromExternal(mOutputPath);

            if (outputFile == null) {
                return;
            }

            // Upload a verbatim copy of the output file
            Log.d(LOG_TAG, String.format("Sending %d byte file %s into the logosphere!",
                    outputFile.length(), outputFile));
            outputSource = new FileInputStreamSource(outputFile);
            listener.testLog(mOutputPath, LogDataType.TEXT, outputSource);
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
     * Parse the relevant metrics from the Instrumentation test output file
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
        ListIterator<String> lineIter = lines.listIterator();
        String line;
        while (lineIter.hasNext()) {
            line = lineIter.next();
            List<List<String>> capture = new ArrayList<>(1);
            String key = mPatternMap.retrieve(capture, line);
            if (key != null) {
                Log.d(LOG_TAG, String.format("Got '%s' and captures '%s'",
                        key, capture.toString()));
            } else if (line.isEmpty()) {
                // ignore
                continue;
            } else {
                Log.d(LOG_TAG, String.format("Got unmatched line: %s", line));
                continue;
            }
            runMetrics.put(key, capture.get(0).get(0));
        }
        reportMetrics(listener, runMetrics);
    }

    /**
     * Report run metrics by creating an empty test run to stick them in
     * <p />
     * Exposed for unit testing
     */
    void reportMetrics(ITestInvocationListener listener, Map<String, String> metrics) {
        Log.d(LOG_TAG, String.format("About to report metrics: %s", metrics));
        listener.testRunStarted(mMetricsRunName, 0);
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
