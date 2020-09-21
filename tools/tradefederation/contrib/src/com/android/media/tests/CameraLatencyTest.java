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
import com.android.tradefed.result.CollectingTestListener;
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

import junit.framework.TestCase;

import org.junit.Assert;

import java.io.ByteArrayInputStream;
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
 * Runs the Camera latency testcases.
 * FIXME: more details
 * <p/>
 * Note that this test will not run properly unless /sdcard is mounted and writable.
 */
public class CameraLatencyTest implements IDeviceTest, IRemoteTest {
    ITestDevice mTestDevice = null;

    // Constants for running the tests
    private static final String TEST_PACKAGE_NAME = "com.google.android.camera.tests";

    private final String mOutputPath = "mediaStressOut.txt";

    //Max timeout for the test - 30 mins
    private static final int MAX_TEST_TIMEOUT = 30 * 60 * 1000;

    /**
     * Stores the test cases that we should consider running.
     *
     * <p>This currently consists of "startup" and "latency"
     */
    private List<TestInfo> mTestCases = new ArrayList<>();

    // Options for the running the gCam test
    @Option(name = "gCam", description = "Run gCam startup test")
    private boolean mGcam = false;


    /**
     * A struct that contains useful info about the tests to run
     */
    static class TestInfo {
        public String mTestName = null;
        public String mClassName = null;
        public String mTestMetricsName = null;
        public RegexTrie<String> mPatternMap = new RegexTrie<>();

        @Override
        public String toString() {
            return String.format("TestInfo: name(%s) class(%s) metric(%s) patterns(%s)", mTestName,
                    mClassName, mTestMetricsName, mPatternMap);
        }
    }

    /**
     * Set up the configurations for the test cases we want to run
     */
    private void testInfoSetup() {
        // Startup tests
        TestInfo t = new TestInfo();

        if (mGcam) {
            t.mTestName = "testLaunchCamera";
            t.mClassName = "com.android.camera.stress.CameraStartUp";
            t.mTestMetricsName = "GCameraStartup";
            RegexTrie<String> map = t.mPatternMap;
            map = t.mPatternMap;
            map.put("FirstCameraStartup", "^First Camera Startup: (\\d+)");
            map.put("CameraStartup", "^Camera average startup time: (\\d+) ms");
            mTestCases.add(t);
        } else {
            t.mTestName = "startup";
            t.mClassName = "com.android.camera.stress.CameraStartUp";
            t.mTestMetricsName = "CameraVideoRecorderStartup";
            RegexTrie<String> map = t.mPatternMap;
            map = t.mPatternMap;
            map.put("FirstCameraStartup", "^First Camera Startup: (\\d+)");
            map.put("CameraStartup", "^Camera average startup time: (\\d+) ms");
            map.put("FirstVideoStartup", "^First Video Startup: (\\d+)");
            map.put("VideoStartup", "^Video average startup time: (\\d+) ms");
            mTestCases.add(t);

            // Latency tests
            t = new TestInfo();
            t.mTestName = "latency";
            t.mClassName = "com.android.camera.stress.CameraLatency";
            t.mTestMetricsName = "CameraLatency";
            map = t.mPatternMap;
            map.put("AutoFocus", "^Avg AutoFocus = (\\d+)");
            map.put("ShutterLag", "^Avg mShutterLag = (\\d+)");
            map.put("Preview", "^Avg mShutterToPictureDisplayedTime = (\\d+)");
            map.put("RawPictureGeneration", "^Avg mPictureDisplayedToJpegCallbackTime = (\\d+)");
            map.put("GenTimeDiffOverJPEGAndRaw", "^Avg mJpegCallbackFinishTime = (\\d+)");
            map.put("FirstPreviewTime", "^Avg FirstPreviewTime = (\\d+)");
            mTestCases.add(t);
        }

    }

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);
        testInfoSetup();
        for (TestInfo test : mTestCases) {
            cleanTmpFiles();
            executeTest(test, listener);
            logOutputFile(test, listener);
        }

        cleanTmpFiles();
    }

    private void executeTest(TestInfo test, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(TEST_PACKAGE_NAME,
                mTestDevice.getIDevice());
        CollectingTestListener auxListener = new CollectingTestListener();

        runner.setClassName(test.mClassName);
        runner.setMaxTimeToOutputResponse(MAX_TEST_TIMEOUT, TimeUnit.MILLISECONDS);
        if (mGcam) {
            runner.setMethodName(test.mClassName, test.mTestName);
        }
        mTestDevice.runInstrumentationTests(runner, listener, auxListener);

        // Grab a bugreport if warranted
        if (auxListener.hasFailedTests()) {
            CLog.i("Grabbing bugreport after test '%s' finished with %d failures.",
                    test.mTestName, auxListener.getNumAllFailedTests());
            InputStreamSource bugreport = mTestDevice.getBugreport();
            listener.testLog(String.format("bugreport-%s.txt", test.mTestName),
                    LogDataType.BUGREPORT, bugreport);
            bugreport.close();
        }
    }

    /**
     * Clean up temp files from test runs
     * <p />
     * Note that all photos on the test device will be removed
     */
    private void cleanTmpFiles() throws DeviceNotAvailableException {
        String extStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        //TODO: Remove the DCIM folder when the bug is fixed.
        mTestDevice.executeShellCommand(String.format("rm %s/DCIM/Camera/*", extStore));
        mTestDevice.executeShellCommand(String.format("rm %s/%s", extStore, mOutputPath));
    }

    /**
     * Pull the output file from the device, add it to the logs, and also parse out the relevant
     * test metrics and report them.
     */
    private void logOutputFile(TestInfo test, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        File outputFile = null;
        InputStreamSource outputSource = null;
        try {
            outputFile = mTestDevice.pullFileFromExternal(mOutputPath);

            if (outputFile == null) {
                return;
            }

            // Upload a verbatim copy of the output file
            CLog.d("Sending %d byte file %s into the logosphere!", outputFile.length(), outputFile);
            outputSource = new FileInputStreamSource(outputFile);
            listener.testLog(String.format("output-%s.txt", test.mTestName), LogDataType.TEXT,
                    outputSource);

            // Parse the output file to upload aggregated metrics
            parseOutputFile(test, new FileInputStream(outputFile), listener);
        } catch (IOException e) {
            CLog.e("IOException while reading or parsing output file");
            CLog.e(e);
        } finally {
            FileUtil.deleteFile(outputFile);
            StreamUtil.cancel(outputSource);
        }
    }

    /**
     * Parse the relevant metrics from the Instrumentation test output file
     */
    private void parseOutputFile(TestInfo test, InputStream dataStream,
            ITestInvocationListener listener) {
        Map<String, String> runMetrics = new HashMap<>();

        // try to parse it
        String contents;
        try {
            contents = StreamUtil.getStringFromStream(dataStream);
        } catch (IOException e) {
            CLog.e("Got IOException during %s test processing", test.mTestName);
            CLog.e(e);
            return;
        }

        List<String> lines = Arrays.asList(contents.split("\n"));
        ListIterator<String> lineIter = lines.listIterator();
        String line;
        while (lineIter.hasNext()) {
            line = lineIter.next();
            List<List<String>> capture = new ArrayList<>(1);
            String key = test.mPatternMap.retrieve(capture, line);
            if (key != null) {
                CLog.d("Got %s key '%s' and captures '%s'", test.mTestName, key,
                        capture.toString());
            } else if (line.isEmpty()) {
                // ignore
                continue;
            } else {
                CLog.d("Got unmatched line: %s", line);
                continue;
            }

            runMetrics.put(key, capture.get(0).get(0));
        }

        reportMetrics(listener, test, runMetrics);
    }

    /**
     * Report run metrics by creating an empty test run to stick them in
     * <p />
     * Exposed for unit testing
     */
    void reportMetrics(ITestInvocationListener listener, TestInfo test,
            Map<String, String> metrics) {
        // Create an empty testRun to report the parsed runMetrics
        CLog.d("About to report metrics for %s: %s", test.mTestMetricsName, metrics);
        listener.testRunStarted(test.mTestMetricsName, 0);
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

    /**
     * A meta-test to ensure that bits of the CameraLatencyTest are working properly
     */
    public static class MetaTest extends TestCase {
        private CameraLatencyTest mTestInstance = null;

        private TestInfo mTestInfo = null;

        private TestInfo mReportedTestInfo = null;
        private Map<String, String> mReportedMetrics = null;

        private static String join(String... pieces) {
            StringBuilder sb = new StringBuilder();
            for (String piece : pieces) {
                sb.append(piece);
                sb.append("\n");
            }
            return sb.toString();
        }

        @Override
        public void setUp() throws Exception {
            mTestInstance = new CameraLatencyTest() {
                @Override
                void reportMetrics(ITestInvocationListener l, TestInfo test,
                        Map<String, String> metrics) {
                    mReportedTestInfo = test;
                    mReportedMetrics = metrics;
                }
            };

            // Startup tests
            mTestInfo = new TestInfo();
            TestInfo t = mTestInfo;  // convenience alias
            t.mTestName = "startup";
            t.mClassName = "com.android.camera.stress.CameraStartUp";
            t.mTestMetricsName = "camera_video_recorder_startup";
            RegexTrie<String> map = t.mPatternMap;
            map.put("FirstCameraStartup", "^First Camera Startup: (\\d+)");
            map.put("CameraStartup", "^Camera average startup time: (\\d+) ms");
            map.put("FirstVideoStartup", "^First Video Startup: (\\d+)");
            map.put("VideoStartup", "^Video average startup time: (\\d+) ms");
            map.put("FirstPreviewTime", "^Avg FirstPreviewTime = (\\d+)");
        }

        /**
         * Make sure that parsing works in the expected case
         */
        public void testParse() throws Exception {
            String output = join(
                    "First Camera Startup: 1421",  /* "FirstCameraStartup" key */
                    "Camerastartup time: ",
                    "Number of loop: 19",
                    "Individual Camera Startup Time = 1920 ,1929 ,1924 ,1871 ,1840 ,1892 ,1813 " +
                        ",1927 ,1856 ,1929 ,1911 ,1873 ,1381 ,1888 ,2405 ,1926 ,1840 ,2502 " +
                        ",2357 ,",
                    "",
                    "Camera average startup time: 1946 ms",  /* "CameraStartup" key */
                    "",
                    "First Video Startup: 2176",  /* "FirstVideoStartup" key */
                    "Camera Latency : ",
                    "Number of loop: 20",
                    "Avg AutoFocus = 2304",
                    "Avg mShutterLag = 403",
                    "Avg mShutterToPictureDisplayedTime = 369",
                    "Avg mPictureDisplayedToJpegCallbackTime = 50",
                    "Avg mJpegCallbackFinishTime = 1679",
                    "Avg FirstPreviewTime = 1340");

            InputStream iStream = new ByteArrayInputStream(output.getBytes());
            mTestInstance.parseOutputFile(mTestInfo, iStream, null);
            assertEquals(mTestInfo, mReportedTestInfo);
            assertNotNull(mReportedMetrics);
            assertEquals(4, mReportedMetrics.size());
            assertEquals("1946", mReportedMetrics.get("CameraStartup"));
            assertEquals("2176", mReportedMetrics.get("FirstVideoStartup"));
            assertEquals("1421", mReportedMetrics.get("FirstCameraStartup"));
            assertEquals("1340", mReportedMetrics.get("FirstPreviewTime"));
        }
    }
}
