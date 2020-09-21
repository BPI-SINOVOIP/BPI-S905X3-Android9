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
package com.android.wireless.tests;

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
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Run the WiFi stress tests. This test stresses WiFi soft ap, WiFi scanning
 * and WiFi reconnection in which device switches between cellular and WiFi connection.
 */
public class WifiStressTest implements IRemoteTest, IDeviceTest {
    private ITestDevice mTestDevice = null;
    private static final long START_TIMER = 5 * 60 * 1000; //5 minutes
    // Define instrumentation test package and runner.
    private static final String TEST_PACKAGE_NAME = "com.android.connectivitymanagertest";
    private static final String TEST_RUNNER_NAME = ".ConnectivityManagerStressTestRunner";

    private static final Pattern ITERATION_PATTERN =
        Pattern.compile("^iteration (\\d+) out of (\\d+)");
    private static final int AP_TEST_TIMER = 3 * 60 * 60 * 1000; // 3 hours
    private static final int SCAN_TEST_TIMER = 30 * 60 * 1000; // 30 minutes
    private static final int RECONNECT_TEST_TIMER = 12 * 60 * 60 * 1000; // 12 hours

    private String mOutputFile = "WifiStressTestOutput.txt";

    /**
     * Stores the test cases that we should consider running.
     * <p/>
     * This currently consists of "ap", "scanning", and "reconnection" tests.
     */
    private List<TestInfo> mTestList = null;

    private static class TestInfo {
        public String mTestName = null;
        public String mTestClass = null;
        public String mTestMethod = null;
        public String mTestMetricsName = null;
        public int mTestTimer;
        public RegexTrie<String> mPatternMap = null;

        @Override
        public String toString() {
            return String.format("TestInfo: mTestName(%s), mTestClass(%s), mTestMethod(%s)," +
                    " mTestMetricsName(%s), mPatternMap(%s), mTestTimer(%d)", mTestName,
                    mTestClass, mTestMethod, mTestMetricsName, mPatternMap.toString(), mTestTimer);
        }
    }

    @Option(
        name = "ap-iteration",
        description = "The number of iterations to run soft ap stress test"
    )
    private String mApIteration = "0";

    @Option(name="idle-time",
        description="The device idle time after screen off")
    private String mIdleTime = "30"; // 30 seconds

    @Option(name="reconnect-iteration",
            description="The number of iterations to run WiFi reconnection stress test")
    private String mReconnectionIteration = "100";

    @Option(name="reconnect-password",
            description="The password for the above ssid in WiFi reconnection stress test")
    private String mReconnectionPassword = "androidwifi";

    @Option(name="reconnect-ssid",
        description="The ssid for WiFi recoonection stress test")
    private String mReconnectionSsid = "securenetdhcp";

    @Option(name="reconnection-test",
        description="Option to run the wifi reconnection stress test")
    private boolean mReconnectionTestFlag = true;

    @Option(name="scan-iteration",
        description="The number of iterations to run WiFi scanning test")
    private String mScanIteration = "100";

    @Option(name="scan-test",
            description="Option to run the scan stress test")
    private boolean mScanTestFlag = true;

    @Option(name="skip-set-device-screen-timeout",
            description="Option to skip screen timeout configuration")
    private boolean mSkipSetDeviceScreenTimeout = false;

    @Option(name="tether-test",
            description="Option to run the tethering stress test")
    private boolean mTetherTestFlag = true;

    @Option(name="wifi-only")
    private boolean mWifiOnly = false;

    private void setupTests() {
        if (mTestList != null) {
            return;
        }
        mTestList = new ArrayList<>(3);

        // Add WiFi scanning test
        TestInfo t = new TestInfo();
        t.mTestName = "WifiScanning";
        t.mTestClass = "com.android.connectivitymanagertest.stress.WifiStressTest";
        t.mTestMethod = "testWifiScanning";
        t.mTestMetricsName = "wifi_scan_performance";
        t.mTestTimer = SCAN_TEST_TIMER;
        t.mPatternMap = new RegexTrie<>();
        t.mPatternMap.put("avg_scan_time", "^average scanning time is (\\d+)");
        t.mPatternMap.put("scan_quality","ssid appear (\\d+) out of (\\d+) scan iterations");
        if (mScanTestFlag) {
            mTestList.add(t);
        }

        // Add WiFi reconnection test
        t = new TestInfo();
        t.mTestName = "WifiReconnectionStress";
        t.mTestClass = "com.android.connectivitymanagertest.stress.WifiStressTest";
        t.mTestMethod = "testWifiReconnectionAfterSleep";
        t.mTestMetricsName = "wifi_stress";
        t.mTestTimer = RECONNECT_TEST_TIMER;
        t.mPatternMap = new RegexTrie<>();
        t.mPatternMap.put("wifi_reconnection_stress", ITERATION_PATTERN);
        if (mReconnectionTestFlag) {
            mTestList.add(t);
        }
    }

    /**
     * Configure screen timeout property
     * @throws DeviceNotAvailableException
     */
    private void setDeviceScreenTimeout() throws DeviceNotAvailableException {
        // Set device screen_off_timeout as svc power can be set to false in the Wi-Fi test
        String command = ("sqlite3 /data/data/com.android.providers.settings/databases/settings.db "
                + "\"UPDATE system SET value=\'600000\' WHERE name=\'screen_off_timeout\';\"");
        CLog.d("Command to set screen timeout value to 10 minutes: %s", command);
        mTestDevice.executeShellCommand(command);

        // reboot to allow the setting to take effect, post setup will be taken care by the reboot
        mTestDevice.reboot();
    }

    /**
     * Enable/disable screen never timeout property
     * @param on
     * @throws DeviceNotAvailableException
     */
    private void setScreenProperty(boolean on) throws DeviceNotAvailableException {
        CLog.d("set svc power stay on " + on);
        mTestDevice.executeShellCommand("svc power stayon " + on);
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
     * Run the Wi-Fi stress test
     * Collect results and post results to dashboard
     */
    @Override
    public void run(ITestInvocationListener standardListener)
            throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);
        setupTests();
        if (!mSkipSetDeviceScreenTimeout) {
            setDeviceScreenTimeout();
        }
        RunUtil.getDefault().sleep(START_TIMER);

        if (!mWifiOnly) {
            final RadioHelper radioHelper = new RadioHelper(mTestDevice);
            Assert.assertTrue("Radio activation failed", radioHelper.radioActivation());
            Assert.assertTrue("Data setup failed", radioHelper.waitForDataSetup());
        }

        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(
                TEST_PACKAGE_NAME, TEST_RUNNER_NAME, mTestDevice.getIDevice());
        runner.addInstrumentationArg("softap_iterations", mApIteration);
        runner.addInstrumentationArg("scan_iterations", mScanIteration);
        runner.addInstrumentationArg("reconnect_iterations", mReconnectionIteration);
        runner.addInstrumentationArg("reconnect_ssid", mReconnectionSsid);
        runner.addInstrumentationArg("reconnect_password", mReconnectionPassword);
        runner.addInstrumentationArg("sleep_time", mIdleTime);
        if (mWifiOnly) {
            runner.addInstrumentationArg("wifi-only", String.valueOf(mWifiOnly));
        }

        // Add bugreport listener for failed test
        BugreportCollector bugListener = new
            BugreportCollector(standardListener, mTestDevice);
        bugListener.addPredicate(BugreportCollector.AFTER_FAILED_TESTCASES);
        // Device may reboot during the test, to capture a bugreport after that,
        // wait for 30 seconds for device to be online, otherwise, bugreport will be empty
        bugListener.setDeviceWaitTime(30);

        for (TestInfo testCase : mTestList) {
            // for Wi-Fi reconnection test,
            if ("WifiReconnectionStress".equals(testCase.mTestName)) {
                setScreenProperty(false);
            } else {
                setScreenProperty(true);
            }
            CLog.d("TestInfo: " + testCase.toString());
            runner.setClassName(testCase.mTestClass);
            runner.setMethodName(testCase.mTestClass, testCase.mTestMethod);
            runner.setMaxTimeToOutputResponse(testCase.mTestTimer, TimeUnit.MILLISECONDS);
            bugListener.setDescriptiveName(testCase.mTestName);
            mTestDevice.runInstrumentationTests(runner, bugListener);
            logOutputFile(testCase, bugListener);
            cleanOutputFiles();
        }
    }

    /**
     * Collect test results, report test results to dash board.
     *
     * @param test
     * @param listener
     */
    private void logOutputFile(TestInfo test, ITestInvocationListener listener)
        throws DeviceNotAvailableException {
        File resFile = null;
        InputStreamSource outputSource = null;

        try {
            resFile = mTestDevice.pullFileFromExternal(mOutputFile);
            if (resFile != null) {
                // Save a copy of the output file
                CLog.d("Sending %d byte file %s into the logosphere!",
                        resFile.length(), resFile);
                outputSource = new FileInputStreamSource(resFile);
                listener.testLog(String.format("result-%s.txt", test.mTestName), LogDataType.TEXT,
                        outputSource);

                // Parse the results file and post results to test listener
                parseOutputFile(test, resFile, listener);
            }
        } finally {
            FileUtil.deleteFile(resFile);
            StreamUtil.cancel(outputSource);
        }
    }

    private void parseOutputFile(TestInfo test, File dataFile,
            ITestInvocationListener listener) {
        Map<String, String> runMetrics = new HashMap<>();
        Map<String, String> runScanMetrics = null;
        boolean isScanningTest = "WifiScanning".equals(test.mTestName);
        Integer iteration = null;
        BufferedReader br = null;
        try {
            br = new BufferedReader(new FileReader(dataFile));
            String line = null;
            while ((line = br.readLine()) != null) {
                List<List<String>> capture = new ArrayList<>(1);
                String key = test.mPatternMap.retrieve(capture, line);
                if (key != null) {
                    CLog.d("In output file of test case %s: retrieve key: %s, " +
                            "catpure: %s", test.mTestName, key, capture.toString());
                    //Save results in the metrics
                    if ("scan_quality".equals(key)) {
                        // For scanning test, calculate the scan quality
                        int count = Integer.parseInt(capture.get(0).get(0));
                        int total = Integer.parseInt(capture.get(0).get(1));
                        int quality = 0;
                        if (total != 0) {
                            quality = (100 * count) / total;
                        }
                        runMetrics.put(key, Integer.toString(quality));
                    } else {
                        runMetrics.put(key, capture.get(0).get(0));
                    }
                } else {
                    // For scanning test, iterations will also be counted.
                    if (isScanningTest) {
                        Matcher m = ITERATION_PATTERN.matcher(line);
                        if (m.matches()) {
                            iteration = Integer.parseInt(m.group(1));
                        }
                    }
                }
            }
            if (isScanningTest) {
                runScanMetrics = new HashMap<>(1);
                if (iteration == null) {
                    // no matching is found
                    CLog.d("No iteration logs found in %s, set to 0", mOutputFile);
                    iteration = Integer.valueOf(0);
                }
                runScanMetrics.put("wifi_scan_stress", iteration.toString());
            }

            // Report results
            reportMetrics(test.mTestMetricsName, listener, runMetrics);
            if (isScanningTest) {
                reportMetrics("wifi_stress", listener, runScanMetrics);
            }
        } catch (IOException e) {
            CLog.e("IOException while reading from data stream");
            CLog.e(e);
            return;
        } finally {
            StreamUtil.close(br);
        }
    }

    /**
     * Report run metrics by creating an empty test run to stick them in
     * <p />
     * Exposed for unit testing
     */
    private void reportMetrics(String metricsName, ITestInvocationListener listener,
            Map<String, String> metrics) {
        // Create an empty testRun to report the parsed runMetrics
        CLog.d("About to report metrics to %s: %s", metricsName, metrics);
        listener.testRunStarted(metricsName, 0);
        listener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(metrics));
    }

    /**
     * Clean up output files from the last test run
     */
    private void cleanOutputFiles() throws DeviceNotAvailableException {
        CLog.d("Remove output file: %s", mOutputFile);
        String extStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        mTestDevice.executeShellCommand(String.format("rm %s/%s", extStore, mOutputFile));
    }
}
