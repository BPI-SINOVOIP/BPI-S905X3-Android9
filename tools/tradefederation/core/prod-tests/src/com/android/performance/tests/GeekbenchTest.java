/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.performance.tests;

import com.android.ddmlib.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;
import org.junit.Assert;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * A harness that launches Geekbench and reports result.
 * Requires Geekbench binary and plar file in device temporary directory.
 */
public class GeekbenchTest implements IDeviceTest, IRemoteTest {

    private static final String RUN_KEY = "geekbench3";
    private static final int MAX_ATTEMPTS = 3;
    private static final int TIMEOUT_MS = 10 * 60 * 1000;

    private static final String DEVICE_TEMPORARY_DIR_PATH = "/data/local/tmp/";
    private static final String GEEKBENCH_BINARY_FILENAME = "geekbench_armeabi-v7a_32";
    private static final String GEEKBENCH_BINARY_DEVICE_PATH =
            DEVICE_TEMPORARY_DIR_PATH + GEEKBENCH_BINARY_FILENAME;
    private static final String GEEKBENCH_PLAR_DEVICE_PATH =
            DEVICE_TEMPORARY_DIR_PATH + "geekbench.plar";
    private static final String GEEKBENCH_RESULT_DEVICE_PATH =
            DEVICE_TEMPORARY_DIR_PATH + "result.txt";

    private static final String OVERALL_SCORE_NAME = "Overall Score";
    private static final Map<String, String> METRICS_KEY_MAP = createMetricsKeyMap();

    private ITestDevice mDevice;

    private static Map<String, String> createMetricsKeyMap() {
        Map<String, String> result = new HashMap<String, String>();
        result.put(OVERALL_SCORE_NAME, "overall");
        result.put("Integer", "integer");
        result.put("Floating Point", "floating-point");
        result.put("Memory", "memory");
        return Collections.unmodifiableMap(result);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        TestDescription testId = new TestDescription(getClass().getCanonicalName(), RUN_KEY);
        ITestDevice device = getDevice();

        // delete old results
        device.executeShellCommand(String.format("rm %s", GEEKBENCH_RESULT_DEVICE_PATH));

        Assert.assertTrue(String.format("Geekbench binary not found on device: %s",
                GEEKBENCH_BINARY_DEVICE_PATH), device.doesFileExist(GEEKBENCH_BINARY_DEVICE_PATH));
        Assert.assertTrue(String.format("Geekbench binary not found on device: %s",
                GEEKBENCH_PLAR_DEVICE_PATH), device.doesFileExist(GEEKBENCH_PLAR_DEVICE_PATH));

        listener.testRunStarted(RUN_KEY, 0);
        listener.testStarted(testId);

        long testStartTime = System.currentTimeMillis();
        Map<String, String> metrics = new HashMap<String, String>();
        String errMsg = null;

        // start geekbench and wait for test to complete
        CollectingOutputReceiver receiver = new CollectingOutputReceiver();
        device.executeShellCommand(String.format("%s --no-upload --export-json %s",
                GEEKBENCH_BINARY_DEVICE_PATH, GEEKBENCH_RESULT_DEVICE_PATH), receiver,
                TIMEOUT_MS, TimeUnit.MILLISECONDS, MAX_ATTEMPTS);
        CLog.v(receiver.getOutput());

        // pull result from device
        File benchmarkReport = device.pullFile(GEEKBENCH_RESULT_DEVICE_PATH);
        if (benchmarkReport != null) {
            // parse result
            CLog.i("== Geekbench 3 result ==");
            Map<String, String> benchmarkResult = parseResultJSON(benchmarkReport);
            if (benchmarkResult == null) {
                errMsg = "Failed to parse Geekbench result JSON.";
            } else {
                metrics = benchmarkResult;
            }

            // delete results from device and host
            benchmarkReport.delete();
            device.executeShellCommand("rm " + GEEKBENCH_RESULT_DEVICE_PATH);
        } else {
            errMsg = "Geekbench report not found.";
        }

        if (errMsg != null) {
            CLog.e(errMsg);
            listener.testFailed(testId, errMsg);
            listener.testEnded(testId, TfMetricProtoUtil.upgradeConvert(metrics));
            listener.testRunFailed(errMsg);
        } else {
            long durationMs = System.currentTimeMillis() - testStartTime;
            listener.testEnded(testId, TfMetricProtoUtil.upgradeConvert(metrics));
            listener.testRunEnded(durationMs, TfMetricProtoUtil.upgradeConvert(metrics));
        }
    }

    private Map<String, String> parseResultJSON(File resultJson) {
        Map<String, String> benchmarkResult = new HashMap<String, String>();
        try {
            BufferedReader bufferedReader = new BufferedReader(new FileReader(resultJson));
            String line = bufferedReader.readLine();
            bufferedReader.close();
            if (line != null) {
                JSONTokener tokener = new JSONTokener(line);
                JSONObject root = new JSONObject(tokener);
                String overallScore = root.getString("score");
                benchmarkResult.put(METRICS_KEY_MAP.get(OVERALL_SCORE_NAME), overallScore);
                CLog.i(String.format("Overall: %s", overallScore));
                String overallScoreMulticore = root.getString("multicore_score");
                benchmarkResult.put(METRICS_KEY_MAP.get(OVERALL_SCORE_NAME) + "-multi",
                        overallScoreMulticore);
                CLog.i(String.format("Overall-multicore: %s", overallScore));
                JSONArray arr = root.getJSONArray("sections");
                for (int i = 0; i < arr.length(); i++) {
                    JSONObject section = arr.getJSONObject(i);
                    String sectionName = section.getString("name");
                    String sectionScore = section.getString("score");
                    String sectionScoreMulticore = section.getString("multicore_score");
                    if (METRICS_KEY_MAP.containsKey(sectionName)) {
                        CLog.i(String.format("%s: %s", sectionName, sectionScore));
                        CLog.i(String.format("%s-multicore: %s", sectionName,
                                sectionScoreMulticore));
                        benchmarkResult.put(METRICS_KEY_MAP.get(sectionName), sectionScore);
                        CLog.i(String.format("%s-multicore: %s", sectionName,
                                sectionScoreMulticore));
                        benchmarkResult.put(METRICS_KEY_MAP.get(sectionName) + "-multi",
                                sectionScoreMulticore);
                    }
                }
            }
        } catch (IOException e) {
            CLog.e("Geekbench3 result file not found.");
            return null;
        } catch (JSONException e) {
            CLog.e("Error parsing Geekbench3 JSON result.");
            return null;
        }
        return benchmarkResult;
    }
}
