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
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
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
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Runs the Media memory test. This test will do various media actions ( ie.
 * playback, recording and etc.) then capture the snapshot of mediaserver memory
 * usage. The test summary is save to /sdcard/mediaMemOutput.txt
 * <p/>
 * Note that this test will not run properly unless /sdcard is mounted and
 * writable.
 */
public class MediaMemoryTest implements IDeviceTest, IRemoteTest {

    ITestDevice mTestDevice = null;

    private static final String METRICS_RUN_NAME = "MediaMemoryLeak";

    // Constants for running the tests
    private static final String TEST_CLASS_NAME =
            "com.android.mediaframeworktest.performance.MediaPlayerPerformance";
    private static final String TEST_PACKAGE_NAME = "com.android.mediaframeworktest";
    private static final String TEST_RUNNER_NAME = ".MediaFrameworkPerfTestRunner";

    private final String mOutputPaths[] = {"mediaMemOutput.txt","mediaProcmemOutput.txt"};

    //Max test timeout - 4 hrs
    private static final int MAX_TEST_TIMEOUT = 4 * 60 * 60 * 1000;

    public Map<String, String> mPatternMap = new HashMap<>();
    private static final Pattern TOTAL_MEM_DIFF_PATTERN =
            Pattern.compile("^The total diff = (\\d+)");

    @Option(name = "getHeapDump", description = "Collect the heap ")
    private boolean mGetHeapDump = false;

    @Option(name = "getProcMem", description = "Collect the procmem info ")
    private boolean mGetProcMem = false;

    @Option(name = "testName", description = "Test name to run. May be repeated.")
    private Collection<String> mTests = new LinkedList<>();

    public MediaMemoryTest() {
        mPatternMap.put("testCameraPreviewMemoryUsage", "CameraPreview");
        mPatternMap.put("testRecordAudioOnlyMemoryUsage", "AudioRecord");
        mPatternMap.put("testH263VideoPlaybackMemoryUsage", "H263Playback");
        mPatternMap.put("testRecordVideoAudioMemoryUsage", "H263RecordVideoAudio");
        mPatternMap.put("testH263RecordVideoOnlyMemoryUsage", "H263RecordVideoOnly");
        mPatternMap.put("testH264VideoPlaybackMemoryUsage", "H264Playback");
        mPatternMap.put("testMpeg4RecordVideoOnlyMemoryUsage", "MPEG4RecordVideoOnly");
    }


    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);

        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(TEST_PACKAGE_NAME,
                TEST_RUNNER_NAME, mTestDevice.getIDevice());
        runner.setClassName(TEST_CLASS_NAME);
        runner.setMaxTimeToOutputResponse(MAX_TEST_TIMEOUT, TimeUnit.MILLISECONDS);
        if (mGetHeapDump) {
            runner.addInstrumentationArg("get_heap_dump", "true");
        }
        if (mGetProcMem) {
            runner.addInstrumentationArg("get_procmem", "true");
        }

        BugreportCollector bugListener = new BugreportCollector(listener,
                mTestDevice);
        bugListener.addPredicate(new BugreportCollector.Predicate(
                Relation.AFTER, Freq.EACH, Noun.TESTRUN));

        if (mTests.size() > 0) {
            for (String testName : mTests) {
                runner.setMethodName(TEST_CLASS_NAME, testName);
                mTestDevice.runInstrumentationTests(runner, bugListener);
            }
        } else {
            mTestDevice.runInstrumentationTests(runner, bugListener);
        }
        logOutputFiles(listener);
        cleanResultFile();
    }

    /**
     * Clean up the test result file from test run
     */
    private void cleanResultFile() throws DeviceNotAvailableException {
        String extStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        for(String outputPath : mOutputPaths){
            mTestDevice.executeShellCommand(String.format("rm %s/%s", extStore, outputPath));
        }
        if (mGetHeapDump) {
            mTestDevice.executeShellCommand(String.format("rm %s/%s", extStore, "*.dump"));
        }
    }

    private void uploadHeapDumpFiles(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        // Pull and upload the heap dump output files.
        InputStreamSource outputSource = null;
        File outputFile = null;

        String extStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);

        String out = mTestDevice.executeShellCommand(String.format("ls %s/%s",
                extStore, "*.dump"));
        String heapOutputFiles[] = out.split("\n");

        for (String heapFile : heapOutputFiles) {
            try {
                outputFile = mTestDevice.pullFile(heapFile.trim());
                if (outputFile == null) {
                    continue;
                }
                outputSource = new FileInputStreamSource(outputFile);
                listener.testLog(heapFile, LogDataType.TEXT, outputSource);
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
        for(String outputPath : mOutputPaths){
            try {
                outputFile = mTestDevice.pullFileFromExternal(outputPath);

                if (outputFile == null) {
                    return;
                }

                // Upload a verbatim copy of the output file
                CLog.d("Sending %d byte file %s into the logosphere!",
                        outputFile.length(), outputFile);
                outputSource = new FileInputStreamSource(outputFile);
                listener.testLog(outputPath, LogDataType.TEXT, outputSource);

                // Parse the output file to upload aggregated metrics
                parseOutputFile(new FileInputStream(outputFile), listener);
            } catch (IOException e) {
                CLog.e("IOException while reading or parsing output file: %s",
                       e.getMessage());
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
            ITestInvocationListener listener) {

        Map<String, String> runMetrics = new HashMap<>();

        // try to parse it
        String contents;
        try {
            contents = StreamUtil.getStringFromStream(dataStream);
        } catch (IOException e) {
            CLog.e("Got IOException during test processing: %s",
                   e.getMessage());
            return;
        }

        List<String> lines = Arrays.asList(contents.split("\n"));
        ListIterator<String> lineIter = lines.listIterator();
        String line;
        while (lineIter.hasNext()) {
            line = lineIter.next();
            if (mPatternMap.containsKey(line)) {

                String key = mPatternMap.get(line);
                // Look for the total diff
                while (lineIter.hasNext()) {
                    line = lineIter.next();
                    Matcher m = TOTAL_MEM_DIFF_PATTERN.matcher(line);
                    if (m.matches()) {
                        int result = Integer.parseInt(m.group(1));
                        runMetrics.put(key, Integer.toString(result));
                        break;
                    }
                }
            } else {
                CLog.e("Got unmatched line: %s", line);
                continue;
            }
        }
        reportMetrics(listener, runMetrics);
    }

    /**
     * Report run metrics by creating an empty test run to stick them in
     * <p />
     * Exposed for unit testing
     */
    void reportMetrics(ITestInvocationListener listener, Map<String, String> metrics) {
        CLog.d("About to report metrics: %s", metrics);
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
