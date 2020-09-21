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
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

/**
 * Test that measures the data usage for an idle device and reports it to the result listener.
 */
public class DataIdleTest implements IDeviceTest, IRemoteTest {

    ITestDevice mTestDevice = null;
    DataIdleTestHelper mTestHelper = null;

    @Option(name = "test-package-name", description = "Android test package name.")
    private String mTestPackageName;

    @Option(name = "test-class-name", description = "Optional test class name to test.")
    private String mTestClassName;

    @Option(name = "test-method-name", description = "Optional test method name to run.")
    private String mTestMethodName;

    @Option(name = "test-label",
            description = "Test label to identify the test run.")
    private String mTestLabel;

    @Option(name = "mobile-data-only",
            description = "If this test is to use mobile data or not.")
    private boolean mMobileDataOnly;

    @Option(name = "idle-time-msecs", description = "Time in msecs to wait for data to propagate.")
    private int mIdleTime = 60 * 60 * 1000;

    private static final String BUG_REPORT_LABEL = "bugreport";
    private static final String DUMPSYS_REPORT_LABEL = "dumpsys";

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);
        mTestHelper = new DataIdleTestHelper(mTestDevice);
        // if mobile data only, then wifi should be disabled, or vice versa
        Assert.assertEquals("incorrect wifi status for current test parameters",
                mMobileDataOnly, !mTestDevice.isWifiEnabled());
        // Test the Internet connection.
        Assert.assertTrue("Failed to connect to get data.", mTestHelper.pingTest());

        CLog.v("Sleeping for %d", mIdleTime);
        RunUtil.getDefault().sleep(mIdleTime);


        // Run test to dump all the data stats gathered by the system.
        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(mTestPackageName,
                mTestDevice.getIDevice());

        if (mTestClassName != null) {
            runner.setClassName(mTestClassName);
        }
        if (mTestClassName != null && mTestMethodName != null) {
            runner.setMethodName(mTestClassName, mTestMethodName);
        }

        CollectingTestListener collectingListener = new CollectingTestListener();
        Assert.assertTrue(mTestDevice.runInstrumentationTests(runner, collectingListener));

        // Collect bandwidth metrics from the instrumentation test out.
        Map<String, String> idleTestMetrics = new HashMap<String, String>();
        Collection<TestResult> testResults =
                collectingListener.getCurrentRunResults().getTestResults().values();
        if (testResults != null && testResults.iterator().hasNext()) {
            Map<String, String> testMetrics = testResults.iterator().next().getMetrics();
            if (testMetrics != null) {
                idleTestMetrics.putAll(testMetrics);
                idleTestMetrics.put("Idle time", Integer.toString(mIdleTime));
                reportMetrics(listener, mTestLabel, idleTestMetrics);
            }
        }
        // Capture the bugreport.
        logBugReport(listener);
        logNetStats(listener);
    }

    /**
     * Capture the bugreport and log it.
     * @param listener {@link ITestInvocationListener}
     */
    void logBugReport(ITestInvocationListener listener) {
        try (InputStreamSource bugreport = mTestDevice.getBugreport()) {
            listener.testLog(BUG_REPORT_LABEL, LogDataType.BUGREPORT, bugreport);
        }
    }

    /**
     * Fetch the whole netstats details.
     * @param listener {@link ITestInvocationListener}
     * @throws DeviceNotAvailableException
     */
    void logNetStats(ITestInvocationListener listener) throws DeviceNotAvailableException {
        String output = mTestDevice.executeShellCommand("dumpsys netstats detail full");
        InputStreamSource is = new ByteArrayInputStreamSource(output.getBytes());
        listener.testLog(DUMPSYS_REPORT_LABEL, LogDataType.TEXT, is);
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
        CLog.d("About to report metrics: %s", metrics);
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

}
