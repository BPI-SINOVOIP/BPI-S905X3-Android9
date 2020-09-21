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
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.config.Option;
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
import com.android.tradefed.util.RegexTrie;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.SimpleStats;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import junit.framework.TestCase;

import org.junit.Assert;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Run the OpenGl performance test. The OpenGl performance test benchmarks the performance
 * of RenderScript in Android System.
 */
public class OpenGlPerformanceTest implements IDeviceTest, IRemoteTest {

    private ITestDevice mTestDevice = null;

    // Define instrumentation test package and runner.
    private static final String TEST_PACKAGE_NAME = "com.android.perftest";
    private static final String TEST_RUNNER_NAME = ".RsPerfTestRunner";
    private static final String TEST_CLASS = "com.android.perftest.RsBenchTest";
    private static final String OUTPUT_FILE = "rsbench_result";
    private static final int TEST_TIMER = 60 * 60 *1000; // test running timer 1 hour
    private static final long START_TIMER = 2 * 60 * 1000; // 2 minutes

    private final RegexTrie<String> mPatternMap = new RegexTrie<>();
    private Map<String, String[]> mKeyMap = new HashMap<>();
    private Map<String, Double[]> mTestResults = new HashMap<>();

    @Option(name="iterations",
            description="The number of iterations to run benchmark tests.")
    private int mIterations = 5;

    public OpenGlPerformanceTest() {
        // 3 tests, RenderScript Text Rendering
        mPatternMap.put("text1", "Fill screen with text 1 time, (\\d+.\\d+),");
        mPatternMap.put("text2", "Fill screen with text 3 times, (\\d+.\\d+),");
        mPatternMap.put("text3", "Fill screen with text 5 times, (\\d+.\\d+),");
        // 6 tests, RenderScript Texture Blending
        mPatternMap.put("10xSingleTexture", "Fill screen 10x singletexture, (\\d+.\\d+),");
        mPatternMap.put("Multitexture", "Fill screen 10x 3tex multitexture, (\\d+.\\d+),");
        mPatternMap.put("BlendedSingleTexture", "Fill screen 10x blended singletexture, " +
                "(\\d+.\\d+),");
        mPatternMap.put("BlendedMultiTexture", "Fill screen 10x blended 3tex multitexture, " +
                "(\\d+.\\d+),");
        mPatternMap.put("3xModulatedBlendedSingletexture",
                "Fill screen 3x modulate blended singletexture, (\\d+.\\d+),");
        mPatternMap.put("1xModulatedBlendedSingletexture",
                "Fill screen 1x modulate blended singletexture, (\\d+.\\d+),");
        // 3 tests, RenderScript Mesh
        mPatternMap.put("FullScreenMesh10", "Full screen mesh 10 by 10, (\\d+.\\d+),");
        mPatternMap.put("FullScrennMesh100", "Full screen mesh 100 by 100, (\\d+.\\d+),");
        mPatternMap.put("FullScreenMeshW4", "Full screen mesh W / 4 by H / 4, (\\d+.\\d+),");
        // 6 tests, RenderScript Geo Test (light)
        mPatternMap.put("GeoTestFlatColor1", "Geo test 25.6k flat color, (\\d+.\\d+),");
        mPatternMap.put("GeoTestFlatColor2", "Geo test 51.2k flat color, (\\d+.\\d+),");
        mPatternMap.put("GeoTestFlatColor3", "Geo test 204.8k small tries flat color, " +
                "(\\d+.\\d+),");
        mPatternMap.put("GeoTestSingleTexture1", "Geo test 25.6k single texture, (\\d+.\\d+),");
        mPatternMap.put("GeoTestSingleTexture2", "Geo test 51.2k single texture, (\\d+.\\d+),");
        mPatternMap.put("GeoTestSingleTexture3", "Geo test 204.8k small tries single texture, " +
                "(\\d+.\\d+),");
        // 9 tests, RenderScript Geo Test (heavy)
        mPatternMap.put("GeoTestHeavyVertex1","Geo test 25.6k geo heavy vertex, (\\d+.\\d+),");
        mPatternMap.put("GeoTestHeavyVertex2","Geo test 51.2k geo heavy vertex, (\\d+.\\d+),");
        mPatternMap.put("GeoTestHeavyVertex3", "Geo test 204.8k geo raster load heavy vertex, " +
                "(\\d+.\\d+),");
        mPatternMap.put("GeoTestHeavyFrag1", "Geo test 25.6k heavy fragment, (\\d+.\\d+),");
        mPatternMap.put("GeoTestHeavyFrag2", "Geo test 51.2k heavy fragment, (\\d+.\\d+),");
        mPatternMap.put("GeoTestHeavyFrag3", "Geo test 204.8k small tries heavy fragment, " +
                "(\\d+.\\d+),");
        mPatternMap.put("GeoTestHeavyFragHeavyVertex1",
                "Geo test 25.6k heavy fragment heavy vertex, (\\d+.\\d+),");
        mPatternMap.put("GeoTestHeavyFragHeavyVertex2",
                "Geo test 51.2k heavy fragment heavy vertex, (\\d+.\\d+),");
        mPatternMap.put("GeoTestHeavyFragHeavyVertex3",
                "Geo test 204.8k small tries heavy fragment heavy vertex, (\\d+.\\d+),");
        // 6 tests, RenerScript UI Test
        mPatternMap.put("UITestWithIcon10by10", "UI test with icon display 10 by 10, " +
                "(\\d+.\\d+),");
        mPatternMap.put("UITestWithIcon100by100", "UI test with icon display 100 by 100, " +
                "(\\d+.\\d+),");
        mPatternMap.put("UITestWithImageText3", "UI test with image and text display 3 pages, " +
                "(\\d+.\\d+),");
        mPatternMap.put("UITestWithImageText5", "UI test with image and text display 5 pages, " +
                "(\\d+.\\d+),");
        mPatternMap.put("UITestListView", "UI test with list view, (\\d+.\\d+),");
        mPatternMap.put("UITestLiveWallPaper", "UI test with live wallpaper, (\\d+.\\d+),");

        mKeyMap.put("graphics_text", (new String[]{"text1", "text2", "text3"}));
        mKeyMap.put("graphics_geo_light", (new String[]{"GeoTestFlatColor1", "GeoTestFlatColor2",
                "GeoTestFlatColor3", "GeoTestSingleTexture1", "GeoTestSingleTexture2",
                "GeoTestSingleTexture3"}));
        mKeyMap.put("graphics_mesh", (new String[]{"FullScreenMesh10","FullScrennMesh100",
                "FullScreenMeshW4"}));
        mKeyMap.put("graphics_geo_heavy", (new String[]{"GeoTestHeavyVertex1",
                "GeoTestHeavyVertex2", "GeoTestHeavyVertex3", "GeoTestHeavyFrag1",
                "GeoTestHeavyFrag2", "GeoTestHeavyFrag3", "GeoTestHeavyFragHeavyVertex1",
                "GeoTestHeavyFragHeavyVertex2", "GeoTestHeavyFragHeavyVertex3"}));
        mKeyMap.put("graphics_texture", (new String[]{"10xSingleTexture", "Multitexture",
                "BlendedSingleTexture", "BlendedMultiTexture", "3xModulatedBlendedSingletexture",
                "1xModulatedBlendedSingletexture"}));
        mKeyMap.put("graphics_ui", (new String[]{"UITestWithIcon10by10", "UITestWithIcon100by100",
                "UITestWithImageText3", "UITestWithImageText5",
                "UITestListView", "UITestLiveWallPaper"}));
    }

    /**
     * Run the OpenGl benchmark tests
     * Collect results and post results to test listener.
     */
    @Override
    public void run(ITestInvocationListener standardListener)
            throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);
        CLog.d("option values: mIterations(%d)", mIterations);

        // Start the test after device is fully booted and stable
        // FIXME: add option in TF to wait until device is booted and stable
        RunUtil.getDefault().sleep(START_TIMER);

        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(
                TEST_PACKAGE_NAME, TEST_RUNNER_NAME, mTestDevice.getIDevice());
        runner.addInstrumentationArg("iterations", Integer.toString(mIterations));
        runner.setClassName(TEST_CLASS);
        runner.setMaxTimeToOutputResponse(TEST_TIMER, TimeUnit.MILLISECONDS);
        // Add bugreport listener for failed test
        BugreportCollector bugListener = new
            BugreportCollector(standardListener, mTestDevice);
        bugListener.addPredicate(BugreportCollector.AFTER_FAILED_TESTCASES);
        bugListener.setDescriptiveName(TEST_CLASS);
        mTestDevice.runInstrumentationTests(runner, bugListener);
        logOutputFile(bugListener);
        cleanOutputFiles();
    }

    /**
     * Collect test results, report test results to test listener.
     *
     * @param listener
     */
    private void logOutputFile(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        // take a bug report, it is possible the system crashed
        try (InputStreamSource bugreport = mTestDevice.getBugreport()) {
            listener.testLog("bugreport.txt", LogDataType.BUGREPORT, bugreport);
        }
        File resFile = null;
        InputStreamSource outputSource = null;

        for (int i = 0; i < mIterations; i++) {
            // In each iteration, the test result is saved in a file rsbench_result*.csv
            // e.g. for 10 iterations, results are in rsbench_result0.csv - rsbench_result9.csv
            String outputFileName = String.format("%s%d.csv", OUTPUT_FILE, i);
            CLog.d("pull result %s", outputFileName);

            try {
                resFile = mTestDevice.pullFileFromExternal(outputFileName);
                if (resFile != null) {
                    // Save a copy of the output file
                    CLog.d("Sending %d byte file %s into the logosphere!",
                            resFile.length(), resFile);
                    outputSource = new FileInputStreamSource(resFile);
                    listener.testLog(outputFileName, LogDataType.TEXT,
                            outputSource);

                    // Parse the results file and report results to dash board
                    parseOutputFile(new FileInputStream(resFile), i);
                } else {
                    CLog.v("File %s doesn't exist or pulling the file failed", outputFileName);
                }
            } catch (IOException e) {
                CLog.e("IOException while reading outputfile %s", outputFileName);
            } finally {
                FileUtil.deleteFile(resFile);
                StreamUtil.cancel(outputSource);
            }
        }

        printTestResults();
        printKeyMap();

        Map<String, String> runMetrics = new HashMap<>();

        // After processing the output file, calculate average data and report data
        // Find the RU-ITEM keys mapping for data posting
        for (Map.Entry<String, String[]> entry: mKeyMap.entrySet()) {
            String[] itemKeys = entry.getValue();

            CLog.v("ru key: %s", entry.getKey());

            for (String key: itemKeys) {
                CLog.v("item key: %s", key);
                if (mTestResults.get(key) == null) {
                    continue;
                }
                SimpleStats simpleStats = new SimpleStats();
                simpleStats.addAll(Arrays.asList(mTestResults.get(key)));
                double averageFps = simpleStats.mean();
                runMetrics.put(key, String.valueOf(averageFps));
            }
            reportMetrics(entry.getKey(), runMetrics, listener);
            runMetrics.clear();
        }
    }

    // Parse one result file and save test name and fps value
    private void parseOutputFile(InputStream dataInputStream, int iterationId) {
        try {
            BufferedReader br= new BufferedReader(new InputStreamReader(dataInputStream));
            String line = null;
            while ((line = br.readLine()) != null) {
                List<List<String>> capture = new ArrayList<>(1);
                String key = mPatternMap.retrieve(capture, line);

                if (key != null) {
                    for (int i = 0; i < capture.size(); i++) {
                        CLog.v("caputre.get[%d]: %s", i, capture.get(i).toString());
                    }

                    CLog.v("Retrieve key: %s, catpure: %s",
                            key, capture.toString());

                    // for value into the corresponding
                    Double[] fps = mTestResults.get(key);
                    if (fps == null) {
                        fps = new Double[mIterations];
                        CLog.v("initialize fps array");
                    }
                    fps[iterationId] = Double.parseDouble(capture.get(0).get(0));
                    mTestResults.put(key, fps);
                }
            }
        } catch (IOException e) {
            CLog.e("IOException while reading from data stream");
            CLog.e(e);
            return;
        }
    }

    private void printKeyMap() {
        for (Map.Entry<String, String[]> entry:mKeyMap.entrySet()) {
            CLog.v("ru key: %s", entry.getKey());
            for (String itemKey: entry.getValue()) {
                CLog.v("item key: %s", itemKey);
            }
        }
    }

    private void printTestResults() {
        for (Map.Entry<String, Double[]> entry: mTestResults.entrySet()) {
            CLog.v("key %s:", entry.getKey());
            for (Double d: entry.getValue()) {
                CLog.v("value: %f", d.doubleValue());
            }
        }
    }

    /**
     * Report run metrics by creating an empty test run to stick them in
     * <p />
     * Exposed for unit testing
     */
    protected void reportMetrics(String metricsName, Map<String, String> metrics,
            ITestInvocationListener listener) {
        // Create an empty testRun to report the parsed runMetrics
        CLog.d("About to report metrics to %s: %s", metricsName, metrics);
        listener.testRunStarted(metricsName, 0);
        listener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(metrics));
    }

    /**
     * Clean up output files from the last test run
     */
    private void cleanOutputFiles() throws DeviceNotAvailableException {
        for (int i = 0; i < mIterations; i++) {
            String outputFileName = String.format("%s%d.csv", OUTPUT_FILE, i);
            String extStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
            mTestDevice.executeShellCommand(String.format("rm %s/%s", extStore, outputFileName));
        }
    }

    @Override
    public void setDevice(ITestDevice testDevice) {
        mTestDevice = testDevice;
    }

    @Override
    public ITestDevice getDevice() {
        return mTestDevice;
    }

    /**
     * A meta-test to ensure the parsing working properly
     * To run the meta test, run command
     * tradefed.sh run singleCommand host -n --class
     * 'com.android.graphics.tests.OpenGlPerformanceTest$MetaTest'
     */
    public static class MetaTest extends TestCase {
        private OpenGlPerformanceTest mTestInstance = null;

        private static String join(String... subStrings) {
            StringBuilder sb = new StringBuilder();
            for (String subString : subStrings) {
                sb.append(subString);
                sb.append("\n");
            }
            return sb.toString();
        }

        @Override
        public void setUp() throws Exception {
            mTestInstance = new OpenGlPerformanceTest() {};
        }

        // Make sure that parsing works
        public void testParseOutputFile() throws Exception {
            String output = join(
                    "Fill screen with text 1 time, 97.65624,",
                    "Fill screen with text 3 times, 36.576443,",
                    "Fill screen with text 5 times, 12.382367,",
                    "Fill screen 10x singletexture, 40.617382,",
                    "Fill screen 10x 3tex multitexture, 28.32861,",
                    "Fill screen 10x blended singletexture, 18.389112,",
                    "Fill screen 10x blended 3tex multitexture, 5.4448433,",
                    "Fill screen 3x modulate blended singletexture, 27.754648,",
                    "Fill screen 1x modulate blended singletexture, 58.207214,",
                    "Full screen mesh 10 by 10, 12.727504,",
                    "Full screen mesh 100 by 100, 4.489136,",
                    "Full screen mesh W / 4 by H / 4, 2.688967,",
                    "Geo test 25.6k flat color, 31.259768,",
                    "Geo test 51.2k flat color, 17.985611,",
                    "Geo test 204.8k small tries flat color, 3.6580455,",
                    "Geo test 25.6k single texture, 30.03905,",
                    "Geo test 51.2k single texture, 14.622021,",
                    "Geo test 204.8k small tries single texture, 3.7195458,",
                    "Geo test 25.6k geo heavy vertex, 20.104542,",
                    "Geo test 51.2k geo heavy vertex, 8.982305,",
                    "Geo test 204.8k geo raster load heavy vertex, 2.9978714,",
                    "Geo test 25.6k heavy fragment, 30.788176,",
                    "Geo test 51.2k heavy fragment, 6.938662,",
                    "Geo test 204.8k small tries heavy fragment, 2.9441204,",
                    "Geo test 25.6k heavy fragment heavy vertex, 16.331863,",
                    "Geo test 51.2k heavy fragment heavy vertex, 4.531243,",
                    "Geo test 204.8k small tries heavy fragment heavy vertex, 1.6915321,",
                    "UI test with icon display 10 by 10, 93.370674,",
                    "UI test with icon display 100 by 100, 1.2948335,",
                    "UI test with image and text display 3 pages, 99.50249,",
                    "UI test with image and text display 5 pages, 77.57951,",
                    "UI test with list view, 117.78562,",
                    "UI test with live wallpaper, 38.197098,");

            InputStream inputStream = new ByteArrayInputStream(output.getBytes());
            mTestInstance.parseOutputFile(inputStream, 0);
            assertNotNull(mTestInstance.mTestResults);

            // 3 tests, RenderScript Text Rendering
            assertEquals("97.65624", mTestInstance.mTestResults.get("text1")[0].toString());
            assertEquals("36.576443", mTestInstance.mTestResults.get("text2")[0].toString());
            assertEquals("12.382367", mTestInstance.mTestResults.get("text3")[0].toString());

            // 6 tests, RenderScript Texture Blending
            assertEquals("40.617382",
                    mTestInstance.mTestResults.get("10xSingleTexture")[0].toString());
            assertEquals("28.32861",
                    mTestInstance.mTestResults.get("Multitexture")[0].toString());
            assertEquals("18.389112",
                    mTestInstance.mTestResults.get("BlendedSingleTexture")[0].toString());
            assertEquals("5.4448433",
                    mTestInstance.mTestResults.get("BlendedMultiTexture")[0].toString());
            assertEquals("27.754648",
                    mTestInstance.mTestResults.get(
                            "3xModulatedBlendedSingletexture")[0].toString());
            assertEquals("58.207214",
                    mTestInstance.mTestResults.get(
                            "1xModulatedBlendedSingletexture")[0].toString());

            // 3 tests, RenderScript Mesh
            assertEquals("12.727504",
                    mTestInstance.mTestResults.get("FullScreenMesh10")[0].toString());
            assertEquals("4.489136",
                    mTestInstance.mTestResults.get("FullScrennMesh100")[0].toString());
            assertEquals("2.688967",
                    mTestInstance.mTestResults.get("FullScreenMeshW4")[0].toString());

            // 6 tests, RenderScript Geo Test (light)
            assertEquals("31.259768",
                    mTestInstance.mTestResults.get("GeoTestFlatColor1")[0].toString());
            assertEquals("17.985611",
                    mTestInstance.mTestResults.get("GeoTestFlatColor2")[0].toString());
            assertEquals("3.6580455",
                    mTestInstance.mTestResults.get("GeoTestFlatColor3")[0].toString());
            assertEquals("30.03905",
                    mTestInstance.mTestResults.get("GeoTestSingleTexture1")[0].toString());
            assertEquals("14.622021",
                    mTestInstance.mTestResults.get("GeoTestSingleTexture2")[0].toString());
            assertEquals("3.7195458",
                    mTestInstance.mTestResults.get("GeoTestSingleTexture3")[0].toString());

            // 9 tests, RenderScript Geo Test (heavy)
            assertEquals("20.104542",
                    mTestInstance.mTestResults.get("GeoTestHeavyVertex1")[0].toString());
            assertEquals("8.982305",
                    mTestInstance.mTestResults.get("GeoTestHeavyVertex2")[0].toString());
            assertEquals("2.9978714",
                    mTestInstance.mTestResults.get("GeoTestHeavyVertex3")[0].toString());
            assertEquals("30.788176",
                    mTestInstance.mTestResults.get("GeoTestHeavyFrag1")[0].toString());
            assertEquals("6.938662",
                    mTestInstance.mTestResults.get("GeoTestHeavyFrag2")[0].toString());
            assertEquals("2.9441204",
                    mTestInstance.mTestResults.get("GeoTestHeavyFrag3")[0].toString());
            assertEquals("16.331863",
                    mTestInstance.mTestResults.get("GeoTestHeavyFragHeavyVertex1")[0].toString());
            assertEquals("4.531243",
                    mTestInstance.mTestResults.get("GeoTestHeavyFragHeavyVertex2")[0].toString());
            assertEquals("1.6915321",
                    mTestInstance.mTestResults.get("GeoTestHeavyFragHeavyVertex3")[0].toString());

            // 6 tests, RenerScript UI Test
            assertEquals("93.370674",
                    mTestInstance.mTestResults.get("UITestWithIcon10by10")[0].toString());
            assertEquals("1.2948335",
                    mTestInstance.mTestResults.get("UITestWithIcon100by100")[0].toString());
            assertEquals("99.50249",
                    mTestInstance.mTestResults.get("UITestWithImageText3")[0].toString());
            assertEquals("77.57951",
                    mTestInstance.mTestResults.get("UITestWithImageText5")[0].toString());
            assertEquals("117.78562",
                    mTestInstance.mTestResults.get("UITestListView")[0].toString());
            assertEquals("38.197098",
                    mTestInstance.mTestResults.get("UITestLiveWallPaper")[0].toString());
        }

        // Verify parsing two iterations
        public void testParseTwoInputs() throws Exception {
            String output1 = join(
                    "Fill screen with text 1 time, 97.65624,",
                    "Fill screen with text 3 times, 36.576443,",
                    "UI test with live wallpaper, 38.197098,");
            String output2 = join(
                    "Fill screen with text 1 time, 97.56097,",
                    "Fill screen with text 3 times, 36.75119,",
                    "UI test with live wallpaper, 38.05175,");

            InputStream inputStream = new ByteArrayInputStream(output1.getBytes());
            mTestInstance.parseOutputFile(inputStream, 0);
            //mTestInstance.printTestResults();
            inputStream = new ByteArrayInputStream(output2.getBytes());
            mTestInstance.parseOutputFile(inputStream, 1);
            assertNotNull(mTestInstance.mTestResults);

            assertEquals("97.65624", mTestInstance.mTestResults.get("text1")[0].toString());
            assertEquals("36.576443", mTestInstance.mTestResults.get("text2")[0].toString());
            assertEquals("38.197098",
                    mTestInstance.mTestResults.get("UITestLiveWallPaper")[0].toString());

            assertEquals("97.56097", mTestInstance.mTestResults.get("text1")[1].toString());
            assertEquals("36.75119", mTestInstance.mTestResults.get("text2")[1].toString());
            assertEquals("38.05175",
                    mTestInstance.mTestResults.get("UITestLiveWallPaper")[1].toString());
        }
    }
}
