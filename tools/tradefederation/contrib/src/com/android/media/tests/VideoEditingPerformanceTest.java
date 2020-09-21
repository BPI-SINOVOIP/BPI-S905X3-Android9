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
 * Runs the Video Editing Framework Performance Test.The performance test result
 * is saved in /sdcard/VideoEditorPerformance.txt
 * <p/>
 * Note that this test will not run properly unless /sdcard is mounted and
 * writable.
 */
public class VideoEditingPerformanceTest implements IDeviceTest, IRemoteTest {
    private static final String LOG_TAG = "VideoEditingPerformanceTest";

    ITestDevice mTestDevice = null;

    private static final String METRICS_RUN_NAME = "VideoEditor";

    //Max test timeout - 3 hrs
    private static final int MAX_TEST_TIMEOUT = 3 * 60 * 60 * 1000;

    // Constants for running the tests
    private static final String TEST_CLASS_NAME =
        "com.android.mediaframeworktest.performance.VideoEditorPerformance";
    private static final String TEST_PACKAGE_NAME = "com.android.mediaframeworktest";
    private static final String TEST_RUNNER_NAME = ".MediaFrameworkPerfTestRunner";

    private static final String OUTPUT_PATH = "VideoEditorPerformance.txt";

    private final RegexTrie<String> mPatternMap = new RegexTrie<>();

    public VideoEditingPerformanceTest() {
        mPatternMap.put("ImageItemCreate",
                "^.*Time taken to Create  Media Image Item :(\\d+)");
        mPatternMap.put("mageItemAdd",
                "^.*Time taken to add  Media Image Item :(\\d+)");
        mPatternMap.put("ImageItemRemove",
                "^.*Time taken to remove  Media Image Item :(\\d+)");
        mPatternMap.put("ImageItemCreate640x480",
                "^.*Time taken to Create  Media Image Item.*640x480.*:(\\d+)");
        mPatternMap.put("ImageItemAdd640x480",
                "^.*Time taken to add  Media Image Item.*640x480.*:(\\d+)");
        mPatternMap.put("ImageItemRemove640x480",
                "^.*Time taken to remove  Media Image Item.*640x480.*:(\\d+)");
        mPatternMap.put("CrossFadeTransitionCreate",
                "^.*Time taken to Create CrossFade Transition :(\\d+)");
        mPatternMap.put("CrossFadeTransitionAdd",
                "^.*Time taken to add CrossFade Transition :(\\d+)");
        mPatternMap.put("CrossFadeTransitionRemove",
                "^.*Time taken to remove CrossFade Transition :(\\d+)");
        mPatternMap.put("VideoItemCreate",
                "^.*Time taken to Create Media Video Item :(\\d+)");
        mPatternMap.put("VideoItemAdd",
                "^.*Time taken to Add  Media Video Item :(\\d+)");
        mPatternMap.put("VideoItemRemove",
                "^.*Time taken to remove  Media Video Item :(\\d+)");
        mPatternMap.put("EffectOverlappingTransition",
                "^.*Time taken to testPerformanceEffectOverlappingTransition :(\\d+.\\d+)");
        mPatternMap.put("ExportStoryboard",
                "^.*Time taken to do ONE export of storyboard duration 69000 is :(\\d+)");
        mPatternMap.put("PreviewWithTransition",
                "^.*Time taken to Generate Preview with transition :(\\d+.\\d+)");
        mPatternMap.put("OverlayCreate",
                "^.*Time taken to add & create Overlay :(\\d+)");
        mPatternMap.put("OverlayRemove",
                "^.*Time taken to remove  Overlay :(\\d+)");
        mPatternMap.put("GetVideoThumbnails",
                "^.*Duration taken to get Video Thumbnails :(\\d+)");
        mPatternMap.put("TransitionWithEffectOverlapping",
                "^.*Time taken to TransitionWithEffectOverlapping :(\\d+.\\d+)");
        mPatternMap.put("MediaPropertiesGet",
                "^.*Time taken to get Media Properties :(\\d+)");
        mPatternMap.put("AACLCAdd",
                "^.*Time taken for 1st Audio Track.*AACLC.*:(\\d+)");
        mPatternMap.put("AMRNBAdd",
                "^.*Time taken for 2nd Audio Track.*AMRNB.*:(\\d+)");
        mPatternMap.put("KenBurnGeneration",
                "^.*Time taken to Generate KenBurn Effect :(\\d+.\\d+)");
        mPatternMap.put("ThumbnailsGeneration",
                "^.*Time taken Thumbnail generation :(\\d+.\\d+)");
        mPatternMap.put("H264ThumbnailGeneration",
                "^.*Time taken for Thumbnail generation :(\\d+.\\d+)");
    }

    @Override
    public void run(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);

        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(
                TEST_PACKAGE_NAME, TEST_RUNNER_NAME, mTestDevice.getIDevice());
        runner.setClassName(TEST_CLASS_NAME);
        runner.setMaxTimeToOutputResponse(MAX_TEST_TIMEOUT, TimeUnit.MILLISECONDS);

        BugreportCollector bugListener = new BugreportCollector(listener,
                mTestDevice);
        bugListener.addPredicate(new BugreportCollector.Predicate(
                Relation.AFTER, Freq.EACH, Noun.TESTRUN));

        mTestDevice.runInstrumentationTests(runner, bugListener);

        logOutputFiles(listener);
        cleanResultFile();
    }

    /**
     * Clean up the test result file from test run
     */
    private void cleanResultFile() throws DeviceNotAvailableException {
        String extStore =
            mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        mTestDevice.executeShellCommand(String.format("rm %s/%s", extStore, OUTPUT_PATH));
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
            outputFile = mTestDevice.pullFileFromExternal(OUTPUT_PATH);

            if (outputFile == null) {
                return;
            }

            // Upload a verbatim copy of the output file
            Log.d(LOG_TAG, String.format(
                    "Sending %d byte file %s into the logosphere!",
                    outputFile.length(), outputFile));
            outputSource = new FileInputStreamSource(outputFile);
            listener.testLog(OUTPUT_PATH, LogDataType.TEXT, outputSource);

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
                Log.d(LOG_TAG, String.format("Got '%s' and captures '%s'", key,
                        capture.toString()));
            } else if (line.isEmpty()) {
                // ignore
                continue;
            } else {
                Log.e(LOG_TAG, String.format("Got unmatched line: %s", line));
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
    void reportMetrics(ITestInvocationListener listener,
            Map<String, String> metrics) {
        Log.d(LOG_TAG, String.format("About to report metrics: %s", metrics));
        listener.testRunStarted(METRICS_RUN_NAME, 0);
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
