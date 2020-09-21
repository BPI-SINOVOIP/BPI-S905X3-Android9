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
import com.android.framework.tests.BandwidthStats.CompareResult;
import com.android.framework.tests.BandwidthStats.ComparisonRecord;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil.IRunnableResult;
import com.android.tradefed.util.MultiMap;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.net.HttpHelper;
import com.android.tradefed.util.net.IHttpHelper;
import com.android.tradefed.util.net.IHttpHelper.DataSizeException;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

/**
 * Test that instruments a bandwidth test, gathers bandwidth metrics, and posts
 * the results to the Release Dashboard.
 */
public class BandwidthMicroBenchMarkTest implements IDeviceTest, IRemoteTest {

    ITestDevice mTestDevice = null;

    @Option(name = "test-package-name", description = "Android test package name.")
    private String mTestPackageName;

    @Option(name = "test-class-name", description = "Test class name.")
    private String mTestClassName;

    @Option(name = "test-method-name", description = "Test method name.")
    private String mTestMethodName;

    @Option(name = "test-label",
            description = "Test label to identify the test run.")
    private String mTestLabel;

    @Option(name = "bandwidth-test-server",
            description = "Test label to use when posting to dashboard.",
            importance=Option.Importance.IF_UNSET)
    private String mTestServer;

    @Option(name = "ssid",
            description = "The ssid to use for the wifi connection.")
    private String mSsid;

    @Option(name = "initial-server-poll-interval-ms",
            description = "The initial poll interval in msecs for querying the test server.")
    private int mInitialPollIntervalMs = 1 * 1000;

    @Option(name = "server-total-timeout-ms",
            description = "The total timeout in msecs for querying the test server.")
    private int mTotalTimeoutMs = 40 * 60 * 1000;

    @Option(name = "server-query-op-timeout-ms",
            description = "The timeout in msecs for a single operation to query the test server.")
    private int mQueryOpTimeoutMs = 2 * 60 * 1000;

    @Option(name="difference-threshold",
            description="The maximum allowed difference between network stats in percent")
    private int mDifferenceThreshold = 5;

    @Option(name="server-difference-threshold",
            description="The maximum difference between the stats reported by the " +
            "server and the device in percent")
    private int mServerDifferenceThreshold = 6;

    @Option(name = "compact-ru-key",
            description = "Name of the reporting unit for pass/fail results")
    private String mCompactRuKey;

    @Option(name = "iface", description="Network interface on the device to use for stats",
            importance = Option.Importance.ALWAYS)
    private String mIface;

    private static final String TEST_RUNNER = "com.android.bandwidthtest.BandwidthTestRunner";
    private static final String TEST_SERVER_QUERY = "query";
    private static final String DEVICE_ID_LABEL = "device_id";
    private static final String TIMESTAMP_LABEL = "timestamp";


    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);

        Assert.assertNotNull("Need a test server, specify it using --bandwidth-test-server",
                mTestServer);

        // Run test
        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(mTestPackageName,
                TEST_RUNNER, mTestDevice.getIDevice());
        runner.setMethodName(mTestClassName, mTestMethodName);
        if (mSsid != null) {
            runner.addInstrumentationArg("ssid", mSsid);
        }
        runner.addInstrumentationArg("server", mTestServer);

        CollectingTestListener collectingListener = new CollectingTestListener();
        Assert.assertTrue(
                mTestDevice.runInstrumentationTests(runner, collectingListener, listener));

        // Collect bandwidth metrics from the instrumentation test out.
        Map<String, String> bandwidthTestMetrics = new HashMap<String, String>();
        Collection<TestResult> testResults =
                collectingListener.getCurrentRunResults().getTestResults().values();
        if (testResults != null && testResults.iterator().hasNext()) {
            Map<String, String> testMetrics = testResults.iterator().next().getMetrics();
            if (testMetrics != null) {
                bandwidthTestMetrics.putAll(testMetrics);
            }
        }

        // Fetch the data from the test server.
        String deviceId = bandwidthTestMetrics.get(DEVICE_ID_LABEL);
        String timestamp = bandwidthTestMetrics.get(TIMESTAMP_LABEL);
        Assert.assertNotNull("Failed to fetch deviceId from server", deviceId);
        Assert.assertNotNull("Failed to fetch timestamp from server", timestamp);
        Map<String, String> serverData = fetchDataFromTestServer(deviceId, timestamp);

        // Calculate additional network sanity stats - pre-framework logic network stats
        BandwidthUtils bw = new BandwidthUtils(mTestDevice, mIface);
        reportPassFail(listener, mCompactRuKey, bw, serverData, bandwidthTestMetrics);

        saveFile("/proc/net/dev", "proc_net_dev", listener);
        saveFile("/proc/net/xt_qtaguid/stats", "qtaguid_stats", listener);

    }

    private void saveFile(String remoteFilename, String spongeName,
            ITestInvocationListener listener) throws DeviceNotAvailableException {
        File f = mTestDevice.pullFile(remoteFilename);
        if (f == null) {
            CLog.w("Failed to pull %s", remoteFilename);
            return;
        }

        saveFile(spongeName, listener, f);
    }

    private void saveFile(String spongeName, ITestInvocationListener listener, File file) {
        try (InputStreamSource stream = new FileInputStreamSource(file)) {
            listener.testLog(spongeName, LogDataType.TEXT, stream);
        }
    }

    /**
     * Fetch the bandwidth test data recorded on the test server.
     *
     * @param deviceId
     * @param timestamp
     * @return a map of the data that was recorded by the test server.
     */
    private Map<String, String> fetchDataFromTestServer(String deviceId, String timestamp) {
        IHttpHelper httphelper = new HttpHelper();
        MultiMap<String,String> params = new MultiMap<String,String> ();
        params.put("device_id", deviceId);
        params.put("timestamp", timestamp);
        String queryUrl = mTestServer;
        if (!queryUrl.endsWith("/")) {
            queryUrl += "/";
        }
        queryUrl += TEST_SERVER_QUERY;
        QueryRunnable runnable = new QueryRunnable(httphelper, queryUrl, params);
        if (RunUtil.getDefault().runEscalatingTimedRetry(mQueryOpTimeoutMs, mInitialPollIntervalMs,
                mQueryOpTimeoutMs, mTotalTimeoutMs, runnable)) {
            return runnable.getServerResponse();
        } else {
            CLog.w("Failed to query test server", runnable.getException());
        }
        return null;
    }

    private static class QueryRunnable implements IRunnableResult {
        private final IHttpHelper mHttpHelper;
        private final String mBaseUrl;
        private final MultiMap<String,String> mParams;
        private Map<String, String> mServerResponse = null;
        private Exception mException = null;

        public QueryRunnable(IHttpHelper helper, String testServerUrl,
                MultiMap<String,String> params) {
            mHttpHelper = helper;
            mBaseUrl = testServerUrl;
            mParams = params;
        }

        /**
         * Perform a single bandwidth test server query, storing the response or
         * the associated exception in case of error.
         */
        @Override
        public boolean run() {
            try {
                String serverResponse = mHttpHelper.doGet(mHttpHelper.buildUrl(mBaseUrl, mParams));
                mServerResponse = parseServerResponse(serverResponse);
                return true;
            } catch (IOException e) {
                CLog.i("IOException %s when contacting test server", e.getMessage());
                mException = e;
            } catch (DataSizeException e) {
                CLog.i("Unexpected oversized response when contacting test server");
                mException = e;
            }
            return false;
        }

        /**
         * Returns exception.
         *
         * @return the last {@link Exception} that occurred when performing
         *         run().
         */
        public Exception getException() {
            return mException;
        }

        /**
         * Returns the server response.
         *
         * @return a map of the server response.
         */
        public Map<String, String> getServerResponse() {
            return mServerResponse;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void cancel() {
            // ignore
        }
    }

    /**
     * Helper to parse test server's response into a map
     * <p>
     * Exposed for unit testing.
     *
     * @param serverResponse {@link String} for the test server http request
     * @return a map representation of the server response
     */
    public static Map<String, String> parseServerResponse(String serverResponse) {
        // No such test run was recorded.
        if (serverResponse == null || serverResponse.trim().length() == 0) {
            return null;
        }
        final String[] responseLines = serverResponse.split("\n");
        Map<String, String> results = new HashMap<String, String>();
        for (String responseLine : responseLines) {
            final String[] responsePairs = responseLine.split(" ");
            for (String responsePair : responsePairs) {
                final String[] pair = responsePair.split(":", 2);
                if (pair.length >= 2) {
                    results.put(pair[0], pair[1]);
                } else {
                    CLog.w("Invalid server response: %s", responsePair);
                }
            }
        }
        return results;
    }

    /**
     * Fetch the last stats from event log and calculate the differences.
     *
     * @throws DeviceNotAvailableException
     */
    private boolean evaluateEventLog(ITestInvocationListener listener) throws DeviceNotAvailableException {
        // issue a force update of stats
        String res = mTestDevice.executeShellCommand("dumpsys netstats poll");
        if (!res.contains("Forced poll")) {
            CLog.w("Failed to force a poll on the device.");
        }
        // fetch events log
        String log = mTestDevice.executeShellCommand("logcat -d -b events");
        if (log != null) {
            return evaluateStats("netstats_mobile_sample", log, listener);
        }
        return false;
    }

    /**
     * Parse a log output for a given key and calculate the network stats.
     *
     * @param key {@link String} to search for in the log
     * @param log obtained from adb logcat -b events
     * @param listener the {@link ITestInvocationListener} where to report results.
     */
    private boolean evaluateStats(String key, String log, ITestInvocationListener listener) {
        File filteredEventLog = null;
        BufferedWriter out = null;
        boolean passed = true;

        try {
            filteredEventLog = File.createTempFile(String.format("%s_event_log", key), ".txt");
            out = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(filteredEventLog)));
            String[] parts = log.split("\n");
            for (int i = parts.length - 1; i > 0; i--) {
                String str = parts[i];
                if (str.contains(key)) {
                    out.write(str);
                    passed = passed && evaluateEventLogLine(str);
                }
            }
            out.flush();
            saveFile(key + "_event_log", listener, filteredEventLog);
            return passed;
        } catch (FileNotFoundException e) {
            CLog.w("Could not create file to save event log: %s", e.getMessage());
            return false;
        } catch (IOException e) {
            CLog.w("Could not save event log file: %s", e.getMessage());
        } finally {
            StreamUtil.close(out);
            FileUtil.deleteFile(filteredEventLog);
        }
        return false;
    }

    private boolean evaluateEventLogLine(String line) {
        int start = line.lastIndexOf("[");
        int end = line.lastIndexOf("]");
        String subStr = line.substring(start + 1, end);
        String[] statsStrArray = subStr.split(",");
        if (statsStrArray.length != 13) {
            CLog.e("Failed to parse for \"%s\" in log.", line);
            return false;
        }
        long xtRb = Long.parseLong(statsStrArray[4].trim());
        long xtTb = Long.parseLong(statsStrArray[5].trim());
        long xtRp = Long.parseLong(statsStrArray[6].trim());
        long xtTp = Long.parseLong(statsStrArray[7].trim());
        long uidRb = Long.parseLong(statsStrArray[8].trim());
        long uidTb = Long.parseLong(statsStrArray[9].trim());
        long uidRp = Long.parseLong(statsStrArray[10].trim());
        long uidTp = Long.parseLong(statsStrArray[11].trim());

        BandwidthStats xtStats = new BandwidthStats(xtRb, xtRp, xtTb, xtTp);
        BandwidthStats uidStats = new BandwidthStats(uidRb, uidRp, uidTb, uidTp);
        boolean result = true;
        CompareResult compareResult = xtStats.compareAll(uidStats, mDifferenceThreshold);
        result &= compareResult.getResult();
        if (!compareResult.getResult()) {
            CLog.i("Failure comparing netstats_mobile_sample xt and uid");
            printFailures(compareResult);
        }
        if (!result) {
            CLog.i("Failed line: %s", line);
        }
        return result;
    }

    /**
     * Compare the data reported by instrumentation to uid breakdown reported by the kernel,
     * the sum of uid breakdown and the total reported by the kernel and the data reported by
     * instrumentation to the data reported by the server.
     * @param listener result reporter
     * @param compactRuKey key to use when posting to rdb.
     * @param utils data parsed from the kernel.
     * @param instrumentationData data reported by the test.
     * @param serverData data reported by the server.
     * @throws DeviceNotAvailableException
     */
    private void reportPassFail(ITestInvocationListener listener, String compactRuKey,
            BandwidthUtils utils, Map<String, String> serverData,
            Map<String, String> instrumentationData) throws DeviceNotAvailableException {
        if (compactRuKey == null) return;

        int passCount = 0;
        int failCount = 0;

        // Calculate the difference between what framework reports and what the kernel reports
        boolean download = Boolean.parseBoolean(serverData.get("download"));
        long frameworkUidBytes = 0;
        if (download) {
            frameworkUidBytes = Long.parseLong(instrumentationData.get("PROF_rx"));
        } else {
            frameworkUidBytes = Long.parseLong(instrumentationData.get("PROF_tx"));
        }

        // Compare data reported by the server and the instrumentation
        long serverBytes = Long.parseLong(serverData.get("size"));
        float diff = Math.abs(BandwidthStats.computePercentDifference(
                serverBytes, frameworkUidBytes));
        if (diff < mServerDifferenceThreshold) {
            passCount += 1;
        } else {
            CLog.i("Comparing between server and instrumentation failed expected %d got %d",
                    serverBytes, frameworkUidBytes);
            failCount += 1;
        }

        if (evaluateEventLog(listener)) {
            passCount += 1;
        } else {
            failCount += 1;
        }

        Map<String, String> postMetrics = new HashMap<String, String>();
        postMetrics.put("Pass", String.valueOf(passCount));
        postMetrics.put("Fail", String.valueOf(failCount));

        listener.testRunStarted(compactRuKey, 0);
        listener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(postMetrics));
    }

    private void printFailures(CompareResult result) {
        for (ComparisonRecord failure : result.getFailures()) {
            CLog.i(failure.toString());
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mTestDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mTestDevice;
    }
}
