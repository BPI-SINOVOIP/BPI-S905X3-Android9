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

package com.android.framework.tests;

import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.LogcatItem;
import com.android.loganalysis.parser.BugreportParser;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Test that instruments a stress test package, gathers iterations metrics, and posts the results.
 */
public class FrameworkStressTest implements IDeviceTest, IRemoteTest {
    public static final String BUGREPORT_LOG_NAME = "bugreport_stress.txt";

    ITestDevice mTestDevice = null;

    @Option(name = "test-package-name", description = "Android test package name.")
    private String mTestPackageName;

    @Option(name = "test-class-name", description = "Test class name.")
    private String mTestClassName;

    @Option(name = "dashboard-test-label",
            description = "Test label to use when posting to dashboard.")
    private String mDashboardTestLabel;

    @Option(name = "setup-shell-command",
            description = "Setup shell command to run before the test, if any.")
    private String mSetupShellCommand;

    private static final String CURRENT_ITERATION_LABEL= "currentiterations";
    private static final String ANR_COUNT_LABEL = "anrs";
    private static final String JAVA_CRASH_COUNT_LABEL = "java_crashes";
    private static final String NATIVE_CRASH_COUNT_LABEL = "native_crashes";
    private static final String ITERATION_COUNT_LABEL = "iterations";

    private int mNumAnrsTotal = 0;
    private int mNumJavaCrashesTotal = 0;
    private int mNumNativeCrashesTotal = 0;

    @Override
    public void run(final ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);
        if (mSetupShellCommand != null) {
            mTestDevice.executeShellCommand(mSetupShellCommand);
        }
        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(mTestPackageName,
                mTestDevice.getIDevice());
        runner.setClassName(mTestClassName);
        CollectingTestListener collectingListener = new CollectingMetricsTestListener(listener);
        mTestDevice.runInstrumentationTests(runner, collectingListener, listener);

        Map<TestDescription, TestResult> testResults =
                collectingListener.getCurrentRunResults().getTestResults();
        if (testResults != null) {
            for (Entry<TestDescription, TestResult> e : testResults.entrySet()) {
                TestResult res = e.getValue();
                Map<String, String> testMetrics = res.getMetrics();
                if (testMetrics != null) {
                    CLog.d(testMetrics.toString());
                    // Post everything to the dashboard.
                    String label = String.format("%s#%s", mDashboardTestLabel,
                            e.getKey().getTestName());
                    reportMetrics(listener, label, testMetrics);
                }
            }
        }
    }

    /**
     * Report run metrics by creating an empty test run to stick them in.
     *
     * @param listener the {@link ITestInvocationListener} of test results
     * @param runName the test name
     * @param metrics the {@link Map} that contains metrics for the given test
     */
    void reportMetrics(ITestInvocationListener listener, String runName,
            Map<String, String> metrics) {
        // Create an empty testRun to report the parsed runMetrics
        CLog.d("About to report metrics: %s with label: %s", metrics, runName);
        listener.testRunStarted(runName, 0);
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
     * Helper class to collect the Framework metrics at the end of each test.
     */
    private class CollectingMetricsTestListener extends CollectingTestListener {

        private ITestInvocationListener mListener;

        public CollectingMetricsTestListener(ITestInvocationListener listener) {
            mListener = listener;
        }

        @Override
        public void testEnded(TestDescription test, Map<String, String> testMetrics) {
            // Retrieve bugreport
            BugreportParser parser = new BugreportParser();
            BugreportItem bugreport = null;
            try (InputStreamSource bugSource = mTestDevice.getBugreport()) {
                mListener.testLog(BUGREPORT_LOG_NAME, LogDataType.BUGREPORT, bugSource);
                bugreport = parser.parse(new BufferedReader(new InputStreamReader(
                        bugSource.createInputStream())));
                Assert.assertNotNull(bugreport);
                Assert.assertNotNull(bugreport.getSystemLog());
            } catch (IOException e) {
                Assert.fail(String.format("Failed to fetch and parse bugreport for device %s: "
                        + "%s", mTestDevice.getSerialNumber(), e));
            }
            LogcatItem systemLog = bugreport.getSystemLog();
            // We only add errors found since last test run.
            Integer numArns = systemLog.getAnrs().size() - mNumAnrsTotal;
            mNumAnrsTotal = systemLog.getAnrs().size();
            testMetrics.put(ANR_COUNT_LABEL, numArns.toString());

            Integer numJavaCrashes = systemLog.getJavaCrashes().size() - mNumJavaCrashesTotal;
            mNumJavaCrashesTotal = systemLog.getJavaCrashes().size();
            testMetrics.put(JAVA_CRASH_COUNT_LABEL, numJavaCrashes.toString());

            Integer numNativeCrashes = systemLog.getNativeCrashes().size() - mNumNativeCrashesTotal;
            mNumNativeCrashesTotal = systemLog.getNativeCrashes().size();
            testMetrics.put(NATIVE_CRASH_COUNT_LABEL, numNativeCrashes.toString());

            Integer numSuccessfulIterations =
                    Integer.parseInt(testMetrics.get(CURRENT_ITERATION_LABEL))
                    - numArns - numJavaCrashes - numNativeCrashes;
            testMetrics.put(ITERATION_COUNT_LABEL, numSuccessfulIterations.toString());

            super.testEnded(test, testMetrics);
        }
    }
}
