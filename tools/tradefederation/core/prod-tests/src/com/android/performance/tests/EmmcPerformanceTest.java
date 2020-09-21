/*
 * Copyright (C) 2016 The Android Open Source Project
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

import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.AbiFormatter;
import com.android.tradefed.util.SimplePerfResult;
import com.android.tradefed.util.SimplePerfUtil;
import com.android.tradefed.util.SimplePerfUtil.SimplePerfType;
import com.android.tradefed.util.SimpleStats;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.text.NumberFormat;
import java.text.ParseException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * This test is targeting eMMC performance on read/ write.
 */
public class EmmcPerformanceTest implements IDeviceTest, IRemoteTest {
    private enum TestType {
        DD,
        RANDOM;
    }

    private static final String RUN_KEY = "emmc_performance_tests";

    private static final String SEQUENTIAL_READ_KEY = "sequential_read";
    private static final String SEQUENTIAL_WRITE_KEY = "sequential_write";
    private static final String RANDOM_READ_KEY = "random_read";
    private static final String RANDOM_WRITE_KEY = "random_write";
    private static final String PERF_RANDOM = "/data/local/tmp/rand_emmc_perf|#ABI32#|";

    private static final Pattern DD_PATTERN = Pattern.compile(
            "\\d+ bytes transferred in \\d+\\.\\d+ secs \\((\\d+) bytes/sec\\)");

    private static final Pattern EMMC_RANDOM_PATTERN = Pattern.compile(
            "(\\d+) (\\d+)byte iops/sec");
    private static final int BLOCK_SIZE = 1048576;
    private static final int SEQ_COUNT = 200;

    @Option(name = "cpufreq", description = "The path to the cpufreq directory on the DUT.")
    private String mCpufreq = "/sys/devices/system/cpu/cpu0/cpufreq";

    @Option(name = "auto-discover-cache-info",
            description =
                    "Indicate if test should attempt auto discover cache path and partition size "
                            + "from the test device. Default to be false, ie. manually set "
                            + "cache-device and cache-partition-size, or use default."
                            + " If fail to discover, it will fallback to what is set in "
                            + "cache-device")
    private boolean mAutoDiscoverCacheInfo = false;

    @Option(name = "cache-device", description = "The path to the cache block device on the DUT." +
            "  Nakasi: /dev/block/platform/sdhci-tegra.3/by-name/CAC\n" +
            "  Prime: /dev/block/platform/omap/omap_hsmmc.0/by-name/cache\n" +
            "  Stingray: /dev/block/platform/sdhci-tegra.3/by-name/cache\n" +
            "  Crespo: /dev/block/platform/s3c-sdhci.0/by-name/userdata\n",
            importance = Importance.IF_UNSET)
    private String mCache = null;

    @Option(name = "iterations", description = "The number of iterations to run")
    private int mIterations = 100;

    @Option(name = AbiFormatter.FORCE_ABI_STRING,
            description = AbiFormatter.FORCE_ABI_DESCRIPTION,
            importance = Importance.IF_UNSET)
    private String mForceAbi = null;

    @Option(name = "cache-partition-size", description = "Cache partiton size in MB")
    private static int mCachePartitionSize = 100;

    @Option(name = "simpleperf-mode",
            description = "Whether use simpleperf to get low level metrics")
    private boolean mSimpleperfMode = false;

    @Option(name = "simpleperf-argu", description = "simpleperf arguments")
    private List<String> mSimpleperfArgu = new ArrayList<String>();

    ITestDevice mTestDevice = null;
    SimplePerfUtil mSpUtil = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        try {
            setUp();

            listener.testRunStarted(RUN_KEY, 5);
            long beginTime = System.currentTimeMillis();
            Map<String, String> metrics = new HashMap<String, String>();

            runSequentialRead(mIterations, listener, metrics);
            runSequentialWrite(mIterations, listener, metrics);
            // FIXME: Figure out cache issues with random read and reenable test.
            // runRandomRead(mIterations, listener, metrics);
            // runRandomWrite(mIterations, listener, metrics);

            CLog.d("Metrics: %s", metrics.toString());
            listener.testRunEnded(
                    (System.currentTimeMillis() - beginTime),
                    TfMetricProtoUtil.upgradeConvert(metrics));
        } finally {
            cleanUp();
        }
    }

    /**
     * Run the sequential read test.
     */
    private void runSequentialRead(int iterations, ITestInvocationListener listener,
            Map<String, String> metrics) throws DeviceNotAvailableException {
        String command = String.format("dd if=%s of=/dev/null bs=%d count=%d", mCache, BLOCK_SIZE,
                SEQ_COUNT);
        runTest(SEQUENTIAL_READ_KEY, command, TestType.DD, true, iterations, listener, metrics);
    }

    /**
     * Run the sequential write test.
     */
    private void runSequentialWrite(int iterations, ITestInvocationListener listener,
            Map<String, String> metrics) throws DeviceNotAvailableException {
        String command = String.format("dd if=/dev/zero of=%s bs=%d count=%d", mCache, BLOCK_SIZE,
                SEQ_COUNT);
        runTest(SEQUENTIAL_WRITE_KEY, command, TestType.DD, false, iterations, listener, metrics);
    }

    /**
     * Run the random read test.
     */
    @SuppressWarnings("unused")
    private void runRandomRead(int iterations, ITestInvocationListener listener,
            Map<String, String> metrics) throws DeviceNotAvailableException {
        String command = String.format("%s -r %d %s",
                AbiFormatter.formatCmdForAbi(PERF_RANDOM, mForceAbi), mCachePartitionSize, mCache);
        runTest(RANDOM_READ_KEY, command, TestType.RANDOM, true, iterations, listener, metrics);
    }

    /**
     * Run the random write test with OSYNC disabled.
     */
    private void runRandomWrite(int iterations, ITestInvocationListener listener,
            Map<String, String> metrics) throws DeviceNotAvailableException {
        String command = String.format("%s -w %d %s",
                AbiFormatter.formatCmdForAbi(PERF_RANDOM, mForceAbi), mCachePartitionSize, mCache);
        runTest(RANDOM_WRITE_KEY, command, TestType.RANDOM, false, iterations, listener, metrics);
    }

    /**
     * Run a test for a number of iterations.
     *
     * @param testKey the key used to report metrics.
     * @param command the command to be run on the device.
     * @param type the {@link TestType}, which determines how each iteration should be run.
     * @param dropCache whether to drop the cache before starting each iteration.
     * @param iterations the number of iterations to run.
     * @param listener the {@link ITestInvocationListener}.
     * @param metrics the map to store metrics of.
     * @throws DeviceNotAvailableException If the device was not available.
     */
    private void runTest(String testKey, String command, TestType type, boolean dropCache,
            int iterations, ITestInvocationListener listener, Map<String, String> metrics)
            throws DeviceNotAvailableException {
        CLog.i("Starting test %s", testKey);

        TestDescription id = new TestDescription(RUN_KEY, testKey);
        listener.testStarted(id);

        Map<String, SimpleStats> simpleperfMetricsMap = new HashMap<String, SimpleStats>();
        SimpleStats stats = new SimpleStats();
        for (int i = 0; i < iterations; i++) {
            if (dropCache) {
                dropCache();
            }

            Double kbps = null;
            switch (type) {
                case DD:
                    kbps = runDdIteration(command, simpleperfMetricsMap);
                    break;
                case RANDOM:
                    kbps = runRandomIteration(command, simpleperfMetricsMap);
                    break;
            }

            if (kbps != null) {
                CLog.i("Result for %s, iteration %d: %f KBps", testKey, i + 1, kbps);
                stats.add(kbps);
            } else {
                CLog.w("Skipping %s, iteration %d", testKey, i + 1);
            }
        }

        if (stats.mean() != null) {
            metrics.put(testKey, Double.toString(stats.median()));
            for (Map.Entry<String, SimpleStats> entry : simpleperfMetricsMap.entrySet()) {
                metrics.put(String.format("%s_%s", testKey, entry.getKey()),
                        Double.toString(entry.getValue().median()));
            }
        } else {
            listener.testFailed(id, "No metrics to report (see log)");
        }
        CLog.i("Test %s finished: mean=%f, stdev=%f, samples=%d", testKey, stats.mean(),
                stats.stdev(), stats.size());
        listener.testEnded(id, new HashMap<String, Metric>());
    }

    /**
     * Run a single iteration of the dd (sequential) test.
     *
     * @param command the command to run on the device.
     * @param simpleperfMetricsMap the map contain simpleperf metrics aggregated results
     * @return The speed of the test in KBps or null if there was an error running or parsing the
     * test.
     * @throws DeviceNotAvailableException If the device was not available.
     */
    private Double runDdIteration(String command, Map<String, SimpleStats> simpleperfMetricsMap)
            throws DeviceNotAvailableException {
        String[] output;
        SimplePerfResult spResult = null;
        if (mSimpleperfMode) {
            spResult = mSpUtil.executeCommand(command);
            output = spResult.getCommandRawOutput().split("\n");
        } else {
            output = mTestDevice.executeShellCommand(command).split("\n");
        }
        String line = output[output.length - 1].trim();

        Matcher m = DD_PATTERN.matcher(line);
        if (m.matches()) {
            simpleperfResultAggregation(spResult, simpleperfMetricsMap);
            return convertBpsToKBps(Double.parseDouble(m.group(1)));
        } else {
            CLog.w("Line \"%s\" did not match expected output, ignoring", line);
            return null;
        }
    }

    /**
     * Run a single iteration of the random test.
     *
     * @param command the command to run on the device.
     * @param simpleperfMetricsMap the map contain simpleperf metrics aggregated results
     * @return The speed of the test in KBps or null if there was an error running or parsing the
     * test.
     * @throws DeviceNotAvailableException If the device was not available.
     */
    private Double runRandomIteration(String command, Map<String, SimpleStats> simpleperfMetricsMap)
            throws DeviceNotAvailableException {
        String output;
        SimplePerfResult spResult = null;
        if (mSimpleperfMode) {
            spResult = mSpUtil.executeCommand(command);
            output = spResult.getCommandRawOutput();
        } else {
            output = mTestDevice.executeShellCommand(command);
        }
        Matcher m = EMMC_RANDOM_PATTERN.matcher(output.trim());
        if (m.matches()) {
            simpleperfResultAggregation(spResult, simpleperfMetricsMap);
            return convertIopsToKBps(Double.parseDouble(m.group(1)));
        } else {
            CLog.w("Line \"%s\" did not match expected output, ignoring", output);
            return null;
        }
    }

    /**
     * Helper function to aggregate simpleperf results
     *
     * @param spResult object that holds simpleperf results
     * @param simpleperfMetricsMap map holds aggregated simpleperf results
     */
    private void simpleperfResultAggregation(SimplePerfResult spResult,
            Map<String, SimpleStats> simpleperfMetricsMap) {
        if (mSimpleperfMode) {
            Assert.assertNotNull("simpleperf result is null object", spResult);
            for (Map.Entry<String, String> entry : spResult.getBenchmarkMetrics().entrySet()) {
                try {
                    Double metricValue = NumberFormat.getNumberInstance(Locale.US)
                            .parse(entry.getValue()).doubleValue();
                    if (!simpleperfMetricsMap.containsKey(entry.getKey())) {
                        SimpleStats newStat = new SimpleStats();
                        simpleperfMetricsMap.put(entry.getKey(), newStat);
                    }
                    simpleperfMetricsMap.get(entry.getKey()).add(metricValue);
                } catch (ParseException e) {
                    CLog.e("Simpleperf metrics parse failure: " + e.toString());
                }
            }
        }
    }

    /**
     * Drop the disk cache on the device.
     */
    private void dropCache() throws DeviceNotAvailableException {
        mTestDevice.executeShellCommand("echo 3 > /proc/sys/vm/drop_caches");
    }

    /**
     * Convert bytes / sec reported by the dd tests into KBps.
     */
    private double convertBpsToKBps(double bps) {
        return bps / 1024;
    }

    /**
     * Convert the iops reported by the random tests into KBps.
     * <p>
     * The iops is number of 4kB block reads/writes per sec.  This makes the conversion factor 4.
     * </p>
     */
    private double convertIopsToKBps(double iops) {
        return 4 * iops;
    }

    /**
     * Setup the device for tests by unmounting partitions and maxing the cpu speed.
     */
    private void setUp() throws DeviceNotAvailableException {
        if (mAutoDiscoverCacheInfo) {
            discoverCacheInfo();
        }
        mTestDevice.executeShellCommand("umount /sdcard");
        mTestDevice.executeShellCommand("umount /data");
        mTestDevice.executeShellCommand("umount /cache");

        mTestDevice.executeShellCommand(
                String.format("cat %s/cpuinfo_max_freq > %s/scaling_max_freq", mCpufreq, mCpufreq));
        mTestDevice.executeShellCommand(
                String.format("cat %s/cpuinfo_max_freq > %s/scaling_min_freq", mCpufreq, mCpufreq));

        if (mSimpleperfMode) {
            mSpUtil = SimplePerfUtil.newInstance(mTestDevice, SimplePerfType.STAT);
            if (mSimpleperfArgu.size() == 0) {
                mSimpleperfArgu.add("-e cpu-cycles:k,cpu-cycles:u");
            }
            mSpUtil.setArgumentList(mSimpleperfArgu);
        }
    }

    /**
     * Attempt to detect cache path and cache partition size automatically
     */
    private void discoverCacheInfo() throws DeviceNotAvailableException {
        // Expected output look similar to the following:
        //
        // > ... vdc dump | grep cache
        // 0 4123 /dev/block/platform/soc/7824900.sdhci/by-name/cache /cache ext4 rw, \
        // seclabel,nosuid,nodev,noatime,discard,data=ordered 0 0
        if (mTestDevice.enableAdbRoot()) {
            String output = mTestDevice.executeShellCommand("vdc dump | grep cache");
            CLog.d("Output from shell command 'vdc dump | grep cache':\n%s", output);
            String[] segments = output.split("\\s+");
            if (segments.length >= 3) {
                mCache = segments[2];
            } else {
                CLog.w("Fail to detect cache path. Fall back to use '%s'", mCache);
            }
        } else {
            CLog.d("Cannot get cache path because device %s is not rooted.",
                    mTestDevice.getSerialNumber());
        }

        // Expected output looks similar to the following:
        //
        // > ... df cache
        // Filesystem            1K-blocks Used Available Use% Mounted on
        // /dev/block/mmcblk0p34     60400   56     60344   1% /cache
        String output = mTestDevice.executeShellCommand("df cache");
        CLog.d(String.format("Output from shell command 'df cache':\n%s", output));
        String[] lines = output.split("\r?\n");
        if (lines.length >= 2) {
            String[] segments = lines[1].split("\\s+");
            if (segments.length >= 2) {
                if (lines[0].toLowerCase().contains("1k-blocks")) {
                    mCachePartitionSize = Integer.parseInt(segments[1]) / 1024;
                } else {
                    throw new IllegalArgumentException("Unknown unit for the cache size.");
                }
            }
        }

        CLog.d("cache-device is set to %s ...", mCache);
        CLog.d("cache-partition-size is set to %d ...", mCachePartitionSize);
    }

    /**
     * Clean up the device by formatting a new cache partition.
     */
    private void cleanUp() throws DeviceNotAvailableException {
        mTestDevice.executeShellCommand(String.format("mke2fs %s", mCache));
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

