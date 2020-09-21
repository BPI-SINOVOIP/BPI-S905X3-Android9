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
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * Runs the Camera stress testcases.
 * FIXME: more details
 * <p/>
 * Note that this test will not run properly unless /sdcard is mounted and writable.
 */
public class CameraStressTest implements IDeviceTest, IRemoteTest {
    private static final String LOG_TAG = "CameraStressTest";

    ITestDevice mTestDevice = null;

    // Constants for running the tests
    private static final String TEST_PACKAGE_NAME = "com.google.android.camera.tests";
    private static final String TEST_RUNNER = "com.android.camera.stress.CameraStressTestRunner";

    //Max test timeout - 3 hrs
    private static final int MAX_TEST_TIMEOUT = 3 * 60 * 60 * 1000;

    private final String mOutputPath = "mediaStressOut.txt";

    /**
     * Stores the test cases that we should consider running.
     *
     * <p>This currently consists of "startup" and "latency"
     */
    private List<TestInfo> mTestCases = new ArrayList<>();

    // Options for the running the gCam test
    @Option(name = "gCam", description = "Run gCam back image capture test")
    private boolean mGcam = false;

    /**
     * A struct that contains useful info about the tests to run
     */
    static class TestInfo {
        public String mTestName = null;
        public String mClassName = null;
        public String mTestMetricsName = null;
        public Map<String, String> mInstrumentationArgs = new HashMap<>();
        public RegexTrie<String> mPatternMap = new RegexTrie<>();

        @Override
        public String toString() {
            return String.format("TestInfo: name(%s) class(%s) metric(%s) patterns(%s)", mTestName,
                    mClassName, mTestMetricsName, mPatternMap);
        }
    }

    /**
     * Set up the pattern map for parsing output files
     * <p/>
     * Exposed for unit meta-testing
     */
    static RegexTrie<String> getPatternMap() {
        RegexTrie<String> patMap = new RegexTrie<>();
        patMap.put("SwitchPreview", "^Camera Switch Mode:");

        // For versions of the on-device test that don't differentiate between front and back camera
        patMap.put("ImageCapture", "^Camera Image Capture");
        patMap.put("VideoRecording", "^Camera Video Capture");

        // For versions that do differentiate
        patMap.put("FrontImageCapture", "^Front Camera Image Capture");
        patMap.put("ImageCapture", "^Back Camera Image Capture");
        patMap.put("FrontVideoRecording", "^Front Camera Video Capture");
        patMap.put("VideoRecording", "^Back Camera Video Capture");

        // Actual metrics to collect for a given key
        patMap.put("loopCount", "^No of loops :(\\d+)");
        patMap.put("iters", "^loop:.+,(\\d+)");

        return patMap;
    }

    /**
     * Set up the configurations for the test cases we want to run
     */
    private void testInfoSetup() {
        RegexTrie<String> patMap = getPatternMap();
        TestInfo t = new TestInfo();

        if (mGcam) {
            // Back Image capture stress test for gCam
            t.mTestName = "testBackImageCapture";
            t.mClassName = "com.android.camera.stress.ImageCapture";
            t.mTestMetricsName = "GCamApplicationStress";
            t.mInstrumentationArgs.put("image_iterations", Integer.toString(100));
            t.mPatternMap = patMap;
            mTestCases.add(t);

        } else {
            // Image capture stress test
            t.mTestName = "imagecap";
            t.mClassName = "com.android.camera.stress.ImageCapture";
            t.mTestMetricsName = "CameraApplicationStress";
            t.mInstrumentationArgs.put("image_iterations", Integer.toString(100));
            t.mPatternMap = patMap;
            mTestCases.add(t);

            // Image capture stress test
            t = new TestInfo();
            t.mTestName = "videocap";
            t.mClassName = "com.android.camera.stress.VideoCapture";
            t.mTestMetricsName = "CameraApplicationStress";
            t.mInstrumentationArgs.put("video_iterations", Integer.toString(100));
            t.mPatternMap = patMap;
            mTestCases.add(t);

            // "SwitchPreview" stress test
            t = new TestInfo();
            t.mTestName = "switch";
            t.mClassName = "com.android.camera.stress.SwitchPreview";
            t.mTestMetricsName = "CameraApplicationStress";
            t.mPatternMap = patMap;
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
            logOutputFiles(test, listener);
        }

        cleanTmpFiles();
    }

    private void executeTest(TestInfo test, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(TEST_PACKAGE_NAME,
                TEST_RUNNER, mTestDevice.getIDevice());
        CollectingTestListener auxListener = new CollectingTestListener();

        runner.setClassName(test.mClassName);
        runner.setMaxTimeToOutputResponse(MAX_TEST_TIMEOUT, TimeUnit.MILLISECONDS);
        if (mGcam){
            runner.setMethodName(test.mClassName, test.mTestName);
        }

        Set<String> argumentKeys = test.mInstrumentationArgs.keySet();
        for (String s : argumentKeys) {
            runner.addInstrumentationArg(s, test.mInstrumentationArgs.get(s));
        }

        mTestDevice.runInstrumentationTests(runner, listener, auxListener);

        // Grab a bugreport if warranted
        if (auxListener.hasFailedTests()) {
            Log.e(LOG_TAG, String.format("Grabbing bugreport after test '%s' finished with " +
                    "%d failures.", test.mTestName, auxListener.getNumAllFailedTests()));
            InputStreamSource bugreport = mTestDevice.getBugreport();
            listener.testLog(String.format("bugreport-%s.txt", test.mTestName),
                    LogDataType.BUGREPORT, bugreport);
            bugreport.cancel();
        }
    }

    /**
     * Clean up temp files from test runs
     * <p />
     * Note that all photos on the test device will be removed
     */
    private void cleanTmpFiles() throws DeviceNotAvailableException {
        String extStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        mTestDevice.executeShellCommand(String.format("rm -r %s/DCIM/Camera", extStore));
        mTestDevice.executeShellCommand(String.format("rm %s/%s", extStore, mOutputPath));
    }

    /**
     * Pull the output file from the device, add it to the logs, and also parse out the relevant
     * test metrics and report them.  Additionally, pull the memory file (if it exists) and report
     * it.
     */
    private void logOutputFiles(TestInfo test, ITestInvocationListener listener)
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
            listener.testLog(String.format("output-%s.txt", test.mTestName), LogDataType.TEXT,
                    outputSource);

            // Parse the output file to upload aggregated metrics
            parseOutputFile(test, new FileInputStream(outputFile), listener);
        } catch (IOException e) {
            Log.e(LOG_TAG, String.format("IOException while reading or parsing output file: %s", e));
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

        String contents;
        try {
            contents = StreamUtil.getStringFromStream(dataStream);
        } catch (IOException e) {
            Log.e(LOG_TAG, String.format("Got IOException during %s test processing: %s",
                    test.mTestName, e));
            return;
        }

        String key = null;
        Integer countExpected = null;
        Integer countActual = null;

        List<String> lines = Arrays.asList(contents.split("\n"));
        ListIterator<String> lineIter = lines.listIterator();
        String line;
        while (lineIter.hasNext()) {
            line = lineIter.next();
            List<List<String>> capture = new ArrayList<>(1);
            String pattern = test.mPatternMap.retrieve(capture, line);
            if (pattern != null) {
                if ("loopCount".equals(pattern)) {
                    // First capture in first (only) string
                    countExpected = Integer.parseInt(capture.get(0).get(0));
                } else if ("iters".equals(pattern)) {
                    // First capture in first (only) string
                    countActual = Integer.parseInt(capture.get(0).get(0));

                    if (countActual != null) {
                        // countActual starts counting at 0
                        countActual += 1;
                    }
                } else {
                    // Assume that the pattern is the name of a key

                    // commit, if there was a previous key
                    if (key != null) {
                        int value = coalesceLoopCounts(countActual, countExpected);
                        runMetrics.put(key, Integer.toString(value));
                    }

                    key = pattern;
                    countExpected = null;
                    countActual = null;
                }

                Log.d(LOG_TAG, String.format("Got %s key '%s' and captures '%s'",
                        test.mTestName, key, capture.toString()));
            } else if (line.isEmpty()) {
                // ignore
                continue;
            } else {
                Log.e(LOG_TAG, String.format("Got unmatched line: %s", line));
                continue;
            }

            // commit the final key, if there was one
            if (key != null) {
                int value = coalesceLoopCounts(countActual, countExpected);
                runMetrics.put(key, Integer.toString(value));
            }
        }

        reportMetrics(listener, test, runMetrics);
    }

    /**
     * Given an actual and an expected iteration count, determine a single metric to report.
     */
    private int coalesceLoopCounts(Integer actual, Integer expected) {
        if (expected == null || expected <= 0) {
            return -1;
        } else if (actual == null) {
            return expected;
        } else {
            return actual;
        }
    }

    /**
     * Report run metrics by creating an empty test run to stick them in
     * <p />
     * Exposed for unit testing
     */
    void reportMetrics(ITestInvocationListener listener, TestInfo test,
            Map<String, String> metrics) {
        // Create an empty testRun to report the parsed runMetrics
        Log.e(LOG_TAG, String.format("About to report metrics for %s: %s", test.mTestMetricsName,
                metrics));
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
     * A meta-test to ensure that bits of the BluetoothStressTest are working properly
     */
    public static class MetaTest extends TestCase {
        private CameraStressTest mTestInstance = null;

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
            mTestInstance = new CameraStressTest() {
                @Override
                void reportMetrics(ITestInvocationListener l, TestInfo test,
                        Map<String, String> metrics) {
                    mReportedTestInfo = test;
                    mReportedMetrics = metrics;
                }
            };

            // Image capture stress test
            mTestInfo = new TestInfo();
            TestInfo t = mTestInfo;  // for convenience
            t.mTestName = "capture";
            t.mClassName = "com.android.camera.stress.ImageCapture";
            t.mTestMetricsName = "camera_application_stress";
            t.mPatternMap = getPatternMap();
        }

        /**
         * Make sure that parsing works for devices sending output in the old format
         */
        public void testParse_old() throws Exception {
            String output = join(
                    "Camera Image Capture",
                    "No of loops :100",
                    "loop:  ,0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10 ,11 ,12 ,13 ,14 ,15 ,16 ,17 ,18 " +
                        ",19 ,20 ,21 ,22 ,23 ,24 ,25 ,26 ,27 ,28 ,29 ,30 ,31 ,32 ,33 ,34 ,35 " +
                        ",36 ,37 ,38 ,39 ,40 ,41 ,42",
                    "Camera Video Capture",
                    "No of loops :100",
                    "loop:  ,0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10 ,11 ,12 ,13 ,14 ,15 ,16 ,17 ,18 " +
                        ",19 ,20 ,21 ,22 ,23 ,24 ,25 ,26 ,27 ,28 ,29 ,30 ,31 ,32 ,33 ,34 ,35 " +
                        ",36 ,37 ,38 ,39 ,40 ,41 ,42 ,43 ,44 ,45 ,46 ,47 ,48 ,49 ,50 ,51 ,52 " +
                        ",53 ,54 ,55 ,56 ,57 ,58 ,59 ,60 ,61 ,62 ,63 ,64 ,65 ,66 ,67 ,68 ,69 " +
                        ",70 ,71 ,72 ,73 ,74 ,75 ,76 ,77 ,78 ,79 ,80 ,81 ,82 ,83 ,84 ,85 ,86 " +
                        ",87 ,88 ,89 ,90 ,91 ,92 ,93 ,94 ,95 ,96 ,97 ,98 ,99",
                    "Camera Switch Mode:",
                    "No of loops :200",
                    "loop:  ,0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10 ,11 ,12 ,13");

            InputStream iStream = new ByteArrayInputStream(output.getBytes());
            mTestInstance.parseOutputFile(mTestInfo, iStream, null);
            assertEquals(mTestInfo, mReportedTestInfo);
            assertNotNull(mReportedMetrics);
            Log.e(LOG_TAG, String.format("Got reported metrics: %s", mReportedMetrics.toString()));
            assertEquals(3, mReportedMetrics.size());
            assertEquals("43", mReportedMetrics.get("ImageCapture"));
            assertEquals("100", mReportedMetrics.get("VideoRecording"));
            assertEquals("14", mReportedMetrics.get("SwitchPreview"));
        }

        /**
         * Make sure that parsing works for devices sending output in the new format
         */
        public void testParse_new() throws Exception {
            String output = join(
                    "Camera Stress Test result",
                    "/folder/subfolder/data/CameraStressTest_git_honeycomb-mr1-release_" +
                        "1700614441c02617_109535_CameraStressOut.txt",
                    "Back Camera Image Capture",
                    "No of loops :100",
                    "loop:  ,0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10 ,11 ,12 ,13 ,14 ,15 ,16 ,17 ,18 " +
                        ",19 ,20 ,21 ,22 ,23 ,24 ,25 ,26 ,27 ,28 ,29 ,30 ,31 ,32 ,33 ,34 ,35 ,36 " +
                        ",37 ,38 ,39 ,40 ,41 ,42 ,43 ,44 ,45 ,46 ,47 ,48 ,49 ,50 ,51 ,52 ,53 ,54 " +
                        ",55 ,56 ,57 ,58 ,59 ,60 ,61 ,62 ,63 ,64 ,65 ,66 ,67 ,68 ,69 ,70 ,71 ,72 " +
                        ",73 ,74 ,75 ,76 ,77 ,78 ,79 ,80 ,81 ,82 ,83 ,84 ,85 ,86 ,87 ,88 ,89 ,90 " +
                        ",91 ,92 ,93 ,94 ,95 ,96 ,97 ,98 ,99",
                    "Front Camera Image Capture",
                    "No of loops :100",
                    "loop:  ,0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10 ,11 ,12 ,13 ,14 ,15 ,16 ,17 ,18 " +
                        ",19 ,20 ,21 ,22 ,23 ,24 ,25 ,26 ,27 ,28 ,29 ,30 ,31 ,32 ,33 ,34 ,35 ,36 " +
                        ",37 ,38 ,39 ,40 ,41 ,42 ,43 ,44 ,45 ,46 ,47 ,48 ,49 ,50 ,51 ,52 ,53 ,54 " +
                        ",55 ,56 ,57 ,58 ,59 ,60 ,61 ,62 ,63 ,64 ,65 ,66 ,67 ,68 ,69 ,70 ,71 ,72 " +
                        ",73 ,74 ,75 ,76 ,77 ,78 ,79 ,80 ,81 ,82 ,83 ,84 ,85 ,86 ,87 ,88 ,89 ,90 " +
                        ",91 ,92 ,93 ,94 ,95 ,96 ,97 ,98",
                    "Back Camera Video Capture",
                    "No of loops :100",
                    "loop:  ,0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10 ,11 ,12 ,13 ,14 ,15 ,16 ,17 ,18 " +
                        ",19 ,20 ,21 ,22 ,23 ,24 ,25 ,26 ,27 ,28 ,29 ,30 ,31 ,32 ,33 ,34 ,35 ,36 " +
                        ",37 ,38 ,39 ,40 ,41 ,42 ,43 ,44 ,45 ,46 ,47 ,48 ,49 ,50 ,51 ,52 ,53 ,54 " +
                        ",55 ,56 ,57 ,58 ,59 ,60 ,61 ,62 ,63 ,64 ,65 ,66 ,67 ,68 ,69 ,70 ,71 ,72 " +
                        ",73 ,74 ,75 ,76 ,77 ,78 ,79 ,80 ,81 ,82 ,83 ,84 ,85 ,86 ,87 ,88 ,89 ,90 " +
                        ",91 ,92 ,93 ,94 ,95 ,96 ,97",
                    "Front Camera Video Capture",
                    "No of loops :100",
                    "loop:  ,0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10 ,11 ,12 ,13 ,14 ,15 ,16 ,17 ,18 " +
                        ",19 ,20 ,21 ,22 ,23 ,24 ,25 ,26 ,27 ,28 ,29 ,30 ,31 ,32 ,33 ,34 ,35 ,36 " +
                        ",37 ,38 ,39 ,40 ,41 ,42 ,43 ,44 ,45 ,46 ,47 ,48 ,49 ,50 ,51 ,52 ,53 ,54 " +
                        ",55 ,56 ,57 ,58 ,59 ,60 ,61 ,62 ,63 ,64 ,65 ,66 ,67 ,68 ,69 ,70 ,71 ,72 " +
                        ",73 ,74 ,75 ,76 ,77 ,78 ,79 ,80 ,81 ,82 ,83 ,84 ,85 ,86 ,87 ,88 ,89 ,90 " +
                        ",91 ,92 ,93 ,94 ,95 ,96 ,97 ,98 ,99",
                    "Camera Switch Mode:",
                    "No of loops :200",
                    "loop:  ,0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10 ,11 ,12 ,13 ,14 ,15 ,16 ,17 ,18 " +
                        ",19 ,20 ,21 ,22 ,23 ,24 ,25 ,26 ,27 ,28 ,29 ,30 ,31 ,32 ,33 ,34 ,35 ,36 " +
                        ",37 ,38 ,39 ,40 ,41 ,42 ,43 ,44 ,45 ,46 ,47 ,48 ,49 ,50 ,51 ,52 ,53 ,54 " +
                        ",55 ,56 ,57 ,58 ,59 ,60 ,61 ,62 ,63 ,64 ,65 ,66 ,67 ,68 ,69 ,70 ,71 ,72 " +
                        ",73 ,74 ,75 ,76 ,77 ,78 ,79 ,80 ,81 ,82 ,83 ,84 ,85 ,86 ,87 ,88 ,89 ,90 " +
                        ",91 ,92 ,93 ,94 ,95 ,96 ,97 ,98 ,99 ,100 ,101 ,102 ,103 ,104 ,105 ,106 " +
                        ",107 ,108 ,109 ,110 ,111 ,112 ,113 ,114 ,115 ,116 ,117 ,118 ,119 ,120 " +
                        ",121 ,122 ,123 ,124 ,125 ,126 ,127 ,128 ,129 ,130 ,131 ,132 ,133 ,134 " +
                        ",135 ,136 ,137 ,138 ,139 ,140 ,141 ,142 ,143 ,144 ,145 ,146 ,147 ,148 " +
                        ",149 ,150 ,151 ,152 ,153 ,154 ,155 ,156 ,157 ,158 ,159 ,160 ,161 ,162 " +
                        ",163 ,164 ,165 ,166 ,167 ,168 ,169 ,170 ,171 ,172 ,173 ,174 ,175 ,176 " +
                        ",177 ,178 ,179 ,180 ,181 ,182 ,183 ,184 ,185 ,186 ,187 ,188 ,189 ,190 " +
                        ",191 ,192 ,193 ,194 ,195 ,196 ,197 ,198 ,199");

            InputStream iStream = new ByteArrayInputStream(output.getBytes());
            mTestInstance.parseOutputFile(mTestInfo, iStream, null);
            assertEquals(mTestInfo, mReportedTestInfo);
            assertNotNull(mReportedMetrics);
            Log.e(LOG_TAG, String.format("Got reported metrics: %s", mReportedMetrics.toString()));
            assertEquals(5, mReportedMetrics.size());
            assertEquals("100", mReportedMetrics.get("ImageCapture"));
            assertEquals("99", mReportedMetrics.get("FrontImageCapture"));
            assertEquals("98", mReportedMetrics.get("VideoRecording"));
            assertEquals("100", mReportedMetrics.get("FrontVideoRecording"));
            assertEquals("200", mReportedMetrics.get("SwitchPreview"));
        }
    }
}

