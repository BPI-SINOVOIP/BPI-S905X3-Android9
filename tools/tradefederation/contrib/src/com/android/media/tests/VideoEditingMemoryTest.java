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
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Runs the Video Editing Framework Memory Tests. This test exercise the basic
 * functionality of video editing test and capture the memory usage. The memory
 * usage test out is saved in /sdcard/VideoEditorStressMemOutput.txt and
 * VideoEditorMediaServerMemoryLog.txt.
 * <p/>
 * Note that this test will not run properly unless /sdcard is mounted and
 * writable.
 */
public class VideoEditingMemoryTest implements IDeviceTest, IRemoteTest {
    private static final String LOG_TAG = "VideoEditorMemoryTest";

    ITestDevice mTestDevice = null;

    // Constants for running the tests
    private static final String TEST_CLASS_NAME =
        "com.android.mediaframeworktest.stress.VideoEditorStressTest";
    private static final String TEST_PACKAGE_NAME = "com.android.mediaframeworktest";
    private static final String TEST_RUNNER_NAME = ".MediaPlayerStressTestRunner";

    //Max test timeout - 3 hrs
    private static final int MAX_TEST_TIMEOUT = 3 * 60 * 60 * 1000;

    /*
     * Pattern to find the test case name and test result.
     * Example of a matching line:
     * testStressAddRemoveEffects total diff = 0
     * The first string 'testStressAddRemoveEffects' represent the dashboard key and
     * the last string represent the test result.
     */
    public static final Pattern TOTAL_MEM_DIFF_PATTERN =
            Pattern.compile("(.+?)\\s.*diff.*\\s(-?\\d+)");

    public Map<String, String> mRunMetrics = new HashMap<>();
    public Map<String, String> mKeyMap = new HashMap<>();

    @Option(name = "getHeapDump", description = "Collect the heap")
    private boolean mGetHeapDump = false;

    public VideoEditingMemoryTest() {
        mKeyMap.put("VideoEditorStressMemOutput.txt", "VideoEditorMemory");
        mKeyMap.put("VideoEditorMediaServerMemoryLog.txt",
                "VideoEditorMemoryMediaServer");
    }


    @Override
    public void run(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);

        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(
                TEST_PACKAGE_NAME, TEST_RUNNER_NAME, mTestDevice.getIDevice());
        runner.setClassName(TEST_CLASS_NAME);
        runner.setMaxTimeToOutputResponse(MAX_TEST_TIMEOUT, TimeUnit.MILLISECONDS);
        if (mGetHeapDump) {
            runner.addInstrumentationArg("get_heap_dump", "getNativeHeap");
        }

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
        for(String outFile : mKeyMap.keySet()) {
            mTestDevice.executeShellCommand(String.format("rm %s/%s", extStore,
                    outFile));
        }
        if (mGetHeapDump) {
            mTestDevice.executeShellCommand(String.format("rm %s/%s", extStore,
                    "*.dump"));
        }
    }

    private void uploadHeapDumpFiles(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        // Pull and upload the heap dump output files.
        InputStreamSource outputSource = null;
        File outputFile = null;

        String extStore =
                mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);

        String out = mTestDevice.executeShellCommand(String.format("ls %s/%s",
                extStore, "*.dump"));
        String heapOutputFiles[] = out.split("\n");

        for (String heapOutputFile : heapOutputFiles) {
            try {
                outputFile = mTestDevice.pullFile(heapOutputFile.trim());
                if (outputFile == null) {
                    continue;
                }
                outputSource = new FileInputStreamSource(outputFile);
                listener.testLog(heapOutputFile, LogDataType.TEXT, outputSource);
            } finally {
                FileUtil.deleteFile(outputFile);
                StreamUtil.cancel(outputSource);
            }
        }
    }

    /**
     * Pull the output files from the device, add it to the logs, and also parse
     * out the relevant test metrics and report them.
     */
    private void logOutputFiles(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        File outputFile = null;
        InputStreamSource outputSource = null;

        if (mGetHeapDump) {
            // Upload all the heap dump files.
            uploadHeapDumpFiles(listener);
        }
        for (String resultFile : mKeyMap.keySet()) {
            try {
                outputFile = mTestDevice.pullFileFromExternal(resultFile);

                if (outputFile == null) {
                    return;
                }

                // Upload a verbatim copy of the output file
                Log.d(LOG_TAG, String.format(
                        "Sending %d byte file %s into the logosphere!",
                        outputFile.length(), outputFile));
                outputSource = new FileInputStreamSource(outputFile);
                listener.testLog(resultFile, LogDataType.TEXT, outputSource);

                // Parse the output file to upload aggregated metrics
                parseOutputFile(new FileInputStream(outputFile), listener, resultFile);
            } catch (IOException e) {
                Log.e(
                        LOG_TAG,
                        String.format("IOException while reading or parsing output file: %s", e));
            } finally {
                FileUtil.deleteFile(outputFile);
                StreamUtil.cancel(outputSource);
            }
        }
    }

    /**
     * Parse the relevant metrics from the Instrumentation test output file
     */
    private void parseOutputFile(InputStream dataStream,
            ITestInvocationListener listener, String outputFile) {

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
        String key;
        String memOut;

        while (lineIter.hasNext()){
            line = lineIter.next();

            // Look for the total diff
            Matcher m = TOTAL_MEM_DIFF_PATTERN.matcher(line);
            if (m.matches()){
                //First group match with the test key name.
                key = m.group(1);
                //Second group match witht the test result.
                memOut = m.group(2);
                mRunMetrics.put(key, memOut);
            }
        }
        reportMetrics(listener, outputFile);
    }

    /**
     * Report run metrics by creating an empty test run to stick them in
     * <p />
     * Exposed for unit testing
     */
    void reportMetrics(ITestInvocationListener listener, String outputFile) {
        Log.d(LOG_TAG, String.format("About to report metrics: %s", mRunMetrics));
        listener.testRunStarted(mKeyMap.get(outputFile), 0);
        listener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(mRunMetrics));
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
