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

package com.android.performance.tests;

import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.MemInfoItem;
import com.android.loganalysis.item.ProcrankItem;
import com.android.loganalysis.parser.BugreportParser;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.ITestDevice.RecoveryMode;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Map;

/**
 * Tests to gather device metrics from during and immediately after boot
 */
public class StartupMetricsTest implements IDeviceTest, IRemoteTest {
    public static final String BUGREPORT_LOG_NAME = "bugreport_startup.txt";

    @Option(name="boot-time-ms", description="Timeout in ms to wait for device to boot.")
    private long mBootTimeMs = 20 * 60 * 1000;

    @Option(name="boot-poll-time-ms", description="Delay in ms between polls for device to boot.")
    private long mBootPoolTimeMs = 500;

    @Option(name="post-boot-delay-ms",
            description="Delay in ms after boot complete before taking the bugreport.")
    private long mPostBootDelayMs = 60000;

    @Option(name="skip-memory-stats", description="Report boot time only, without memory stats.")
    private boolean mSkipMemoryStats = false;

    ITestDevice mTestDevice = null;

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);
        executeRebootTest(listener);
        if (!mSkipMemoryStats) {
            fetchBugReportMetrics(listener);
        }
    }

    /**
     * Check how long the device takes to come online and become available after
     * a reboot.
     *
     * @param listener the {@link ITestInvocationListener} of test results
     */
    void executeRebootTest(ITestInvocationListener listener) throws DeviceNotAvailableException {

        /*Not using /proc/uptime to get the initial boot time
        because it includes delays running 'permission utils',
        and the CPU throttling waiter*/
        Map<String, String> runMetrics = new HashMap<String, String>();
        //Initial reboot
        mTestDevice.rebootIntoBootloader();
        mTestDevice.setUseFastbootErase(false);
        mTestDevice.fastbootWipePartition("userdata");
        mTestDevice.executeFastbootCommand("reboot");
        mTestDevice.waitForDeviceOnline(10 * 60 * 1000);
        long initOnlineTime = System.currentTimeMillis();
        Assert.assertTrue(waitForBootComplete(mTestDevice, mBootTimeMs, mBootPoolTimeMs));
        long initAvailableTime = System.currentTimeMillis();
        double initUnavailDuration = initAvailableTime - initOnlineTime;
        runMetrics.put("init-boot", Double.toString(initUnavailDuration / 1000.0));

        mTestDevice.setRecoveryMode(RecoveryMode.NONE);
        CLog.d("Reboot test start.");
        mTestDevice.nonBlockingReboot();
        long startTime = System.currentTimeMillis();
        mTestDevice.waitForDeviceOnline();
        long onlineTime = System.currentTimeMillis();
        Assert.assertTrue(waitForBootComplete(mTestDevice, mBootTimeMs, mBootPoolTimeMs));
        long availableTime = System.currentTimeMillis();

        double offlineDuration = onlineTime - startTime;
        double unavailDuration = availableTime - startTime;
        CLog.d("Reboot: %f millis until online, %f until available",
                offlineDuration, unavailDuration);
        runMetrics.put("online", Double.toString(offlineDuration/1000.0));
        runMetrics.put("bootcomplete", Double.toString(unavailDuration/1000.0));

        reportMetrics(listener, "boottime", runMetrics);
    }

    /**
     * Fetch proc rank metrics from the bugreport after reboot.
     *
     * @param listener the {@link ITestInvocationListener} of test results
     * @throws DeviceNotAvailableException
     */
    void fetchBugReportMetrics(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        // Make sure the device is available and settled, before getting bugreport.
        mTestDevice.waitForDeviceAvailable();
        RunUtil.getDefault().sleep(mPostBootDelayMs);
        BugreportParser parser = new BugreportParser();
        BugreportItem bugreport = null;
        // Retrieve bugreport
        try (InputStreamSource bugSource = mTestDevice.getBugreport()) {
            listener.testLog(BUGREPORT_LOG_NAME, LogDataType.BUGREPORT, bugSource);
            bugreport = parser.parse(new BufferedReader(new InputStreamReader(
                    bugSource.createInputStream())));
        } catch (IOException e) {
            Assert.fail(String.format("Failed to fetch and parse bugreport for device %s: %s",
                    mTestDevice.getSerialNumber(), e));
        }

        if (bugreport != null) {
            // Process meminfo information and post it to the dashboard
            MemInfoItem item = bugreport.getMemInfo();
            if (item != null) {
                Map<String, String> memInfoMap = convertMap(item);
                reportMetrics(listener, "startup-meminfo", memInfoMap);
            }

            // Process procrank information and post it to the dashboard
            if (bugreport.getProcrank() != null) {
                parseProcRankMap(listener, bugreport.getProcrank());
            }
        }
    }

    /**
     * Helper method to convert a {@link MemInfoItem} to Map&lt;String, String&gt;.
     *
     * @param input the {@link Map} to convert from
     * @return output the converted {@link Map}
     */
    Map<String, String> convertMap(MemInfoItem item) {
        Map<String, String> output = new HashMap<String, String>();
        for (String key : item.keySet()) {
            output.put(key, item.get(key).toString());
        }
        return output;
    }

    /**
     * Report run metrics by creating an empty test run to stick them in
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

    /**
     * Aggregates the procrank data by the pss, rss, and uss values.
     *
     * @param listener the {@link ITestInvocationListener} of test results
     * @param procrank the {@link Map} parsed from brillopad for the procrank section
     */
    void parseProcRankMap(ITestInvocationListener listener, ProcrankItem procrank) {
        // final maps for pss, rss, and uss.
        Map<String, String> pssOutput = new HashMap<String, String>();
        Map<String, String> rssOutput = new HashMap<String, String>();
        Map<String, String> ussOutput = new HashMap<String, String>();
        // total number of processes.
        Integer numProcess = 0;
        // aggregate pss, rss, uss across all processes.
        Integer pssTotal = 0;
        Integer rssTotal = 0;
        Integer ussTotal = 0;

        for (Integer pid : procrank.getPids()) {
            numProcess++;
            Integer pss = procrank.getPss(pid);
            Integer rss = procrank.getRss(pid);
            Integer uss = procrank.getUss(pid);
            if (pss != null) {
                pssTotal += pss;
                pssOutput.put(procrank.getProcessName(pid), pss.toString());
            }
            if (rss != null) {
                rssTotal += rss;
                rssOutput.put(procrank.getProcessName(pid), rss.toString());
            }
            if (uss != null) {
                ussTotal += pss;
                ussOutput.put(procrank.getProcessName(pid), uss.toString());
            }
        }
        // Add aggregation data.
        pssOutput.put("count", numProcess.toString());
        pssOutput.put("total", pssTotal.toString());
        rssOutput.put("count", numProcess.toString());
        rssOutput.put("total", rssTotal.toString());
        ussOutput.put("count", numProcess.toString());
        ussOutput.put("total", ussTotal.toString());

        // Report metrics to dashboard
        reportMetrics(listener, "startup-procrank-pss", pssOutput);
        reportMetrics(listener, "startup-procrank-rss", rssOutput);
        reportMetrics(listener, "startup-procrank-uss", ussOutput);
    }

    /**
     * Blocks until the device's boot complete flag is set.
     *
     * @param device the {@link ITestDevice}
     * @param timeOut time in msecs to wait for the flag to be set
     * @param pollDelay time in msecs between checks
     * @return true if device's boot complete flag is set within the timeout
     * @throws DeviceNotAvailableException
     */
    private boolean waitForBootComplete(ITestDevice device,long timeOut, long pollDelay)
            throws DeviceNotAvailableException {
        long startTime = System.currentTimeMillis();
        while ((System.currentTimeMillis() - startTime) < timeOut) {
            String output = device.executeShellCommand("getprop dev.bootcomplete");
            output = output.replace('#', ' ').trim();
            if (output.equals("1")) {
                return true;
            }
            RunUtil.getDefault().sleep(pollDelay);
        }
        CLog.w("Device %s did not boot after %d ms", device.getSerialNumber(), timeOut);
        return false;
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
