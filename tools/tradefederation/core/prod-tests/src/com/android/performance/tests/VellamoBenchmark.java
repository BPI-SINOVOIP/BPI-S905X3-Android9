/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.android.performance.tests;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Map;

/**
 * A harness that launches VellamoBenchmark and reports result. Requires
 * VellamoBenchmark apk.
 */
public class VellamoBenchmark implements IDeviceTest, IRemoteTest {

    private static final String LOGTAG = "VAUTOMATIC";
    private static final String RUN_KEY = "vellamobenchmark-3202";
    private static final String PACKAGE_NAME = "com.quicinc.vellamo";
    private static final long TIMEOUT_MS = 30 * 60 * 1000;
    private static final long POLLING_INTERVAL_MS = 10 * 1000;
    private static final int INDEX_NAME = 0;
    private static final int INDEX_CODE = 4;
    private static final int INDEX_SCORE = 5;

    private ITestDevice mDevice;

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
        listener.testRunStarted(RUN_KEY, 0);
        listener.testStarted(testId);

        long testStartTime = System.currentTimeMillis();
        Map<String, String> metrics = new HashMap<String, String>();
        String errMsg = null;

        boolean isTimedOut = false;
        boolean isResultGenerated = false;
        boolean hasScore = false;
        double sumScore = 0;
        int errorCode = 0;
        device.clearErrorDialogs();
        isTimedOut = false;

        long benchmarkStartTime = System.currentTimeMillis();
        // start the vellamo benchmark app and run all the tests
        // the documentation and binary for the Vellamo 3.2.2 for Automation APK
        // can be found here:
        // https://b.corp.google.com/issue?id=23107318
        CLog.i("Starting Vellamo Benchmark");
        device.executeShellCommand("am start -a com.quicinc.vellamo.AUTOMATIC"
                + " -e w com.quicinc.skunkworks.wvb" // use System WebView
                + " -n com.quicinc.vellamo/.main.MainActivity");
        String line;
        while (!isResultGenerated && !isTimedOut) {
            RunUtil.getDefault().sleep(POLLING_INTERVAL_MS);
            isTimedOut = (System.currentTimeMillis() - benchmarkStartTime >= TIMEOUT_MS);

            // get the logcat and parse
            try (InputStreamSource logcatSource = device.getLogcat();
                    BufferedReader logcat =
                            new BufferedReader(
                                    new InputStreamReader(logcatSource.createInputStream()))) {
                while ((line = logcat.readLine()) != null) {
                    // filter only output from the Vellamo process
                    if (!line.contains(LOGTAG)) {
                        continue;
                    }
                    line = line.substring(line.indexOf(LOGTAG) + LOGTAG.length());
                    // we need to see if the score is generated since there are some
                    // cases the result with </automatic> tag is generated but no score is included
                    if (line.contains("</automatic>")) {
                        if (hasScore) {
                            isResultGenerated = true;
                            break;
                        }
                    }
                    // get the score out
                    if (line.contains(" b: ")) {
                        hasScore = true;
                        String[] results = line.split(" b: ")[1].split(",");
                        errorCode = Integer.parseInt(results[INDEX_CODE]);
                        if (errorCode != 0) {
                            CLog.w("Non-zero error code: %d from becnhmark '%s'",
                                    errorCode, results[INDEX_NAME]);
                        } else {
                            sumScore += Double.parseDouble(results[INDEX_SCORE]);
                        }
                        metrics.put(results[INDEX_NAME], results[INDEX_SCORE]);
                        CLog.i("%s :: %s", results[INDEX_NAME], results[INDEX_SCORE]);
                    }
                }
            } catch (IOException e) {
                CLog.e(e);
            }

            if (null == device.getProcessByName(PACKAGE_NAME)) {
                break;
            }
        }

        if (isTimedOut) {
            errMsg = "Vellamo Benchmark timed out.";
        } else {
            CLog.i("Done running Vellamo Benchmark");
        }
        if (!hasScore) {
            errMsg = "Test ended but no scores can be found.";
        }
        if (errMsg != null) {
            CLog.e(errMsg);
            listener.testFailed(testId, errMsg);
        }
        long durationMs = System.currentTimeMillis() - testStartTime;
        metrics.put("total", Double.toString(sumScore));
        CLog.i("total :: %f", sumScore);
        listener.testEnded(testId, new HashMap<String, Metric>());
        listener.testRunEnded(durationMs, TfMetricProtoUtil.upgradeConvert(metrics));
    }
}
