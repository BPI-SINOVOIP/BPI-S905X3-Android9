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

import com.android.ddmlib.MultiLineReceiver;
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
import com.android.tradefed.util.SimplePerfStatResultParser;
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
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Test which runs various micro_bench tests and reports the results.
 * <p>
 * Tests include:
 * </p><ul>
 * <li>{@code micro_bench sleep 1 ITERATIONS}</li>
 * <li>{@code micro_bench memset ARG ITERATIONS} where ARG is defined by {@code memset-small}</li>
 * <li>{@code micro_bench memset ARG ITERATIONS} where ARG is defined by {@code memset-large}</li>
 * <li>{@code micro_bench memcpy ARG ITERATIONS} where ARG is defined by {@code memcpy-small}</li>
 * <li>{@code micro_bench memcpy ARG ITERATIONS} where ARG is defined by {@code memcpy-large}</li>
 * <li>{@code micro_bench memread ARG ITERATIONS} where ARG is defined by {@code memread}</li>
 * </ul>
 */
public class MicroBenchTest implements IDeviceTest, IRemoteTest {
    private final static String RUN_KEY = "micro_bench";

    private final static Pattern SLEEP_PATTERN = Pattern.compile(
            "sleep\\(\\d+\\) took (\\d+.\\d+) seconds");
    private final static Pattern MEMSET_PATTERN = Pattern.compile(
            "memset \\d+x\\d+ bytes took (\\d+.\\d+) seconds \\((\\d+.\\d+) MB/s\\)");
    private final static Pattern MEMCPY_PATTERN = Pattern.compile(
            "memcpy \\d+x\\d+ bytes took (\\d+.\\d+) seconds \\((\\d+.\\d+) MB/s\\)");
    private final static Pattern MEMREAD_PATTERN = Pattern.compile(
            "memread \\d+x\\d+ bytes took (\\d+.\\d+) seconds \\((\\d+.\\d+) MB/s\\)");

    private final static String BINARY_NAME = "micro_bench|#ABI32#|";
    private final static String SLEEP_CMD = "%s sleep 1 %d";
    // memset, memcpy, and memread pass in another argument befor passing the cmd format string to
    // TestCase, so escape the command and iteration formats.
    private final static String MEMSET_CMD = "%%s memset %d %%d";
    private final static String MEMCPY_CMD = "%%s memcpy %d %%d";
    private final static String MEMREAD_CMD = "%%s memread %d %%d";

    @Option(name = AbiFormatter.FORCE_ABI_STRING,
            description = AbiFormatter.FORCE_ABI_DESCRIPTION,
            importance = Importance.IF_UNSET)
    private String mForceAbi = null;

    @Option(name = "sleep-iterations")
    private Integer mSleepIterations = 10;

    @Option(name = "memset-iterations")
    private Integer mMemsetIterations = 100;

    @Option(name = "memset-small")
    private Integer mMemsetSmall = 8 * 1024; // 8 KB

    @Option(name = "memset-large")
    private Integer mMemsetLarge = 2 * 1024 * 1024; // 2 MB

    @Option(name = "memcpy-iterations")
    private Integer mMemcpyIterations = 100;

    @Option(name = "memcpy-small")
    private Integer mMemcpySmall = 8 * 1024; // 8 KB

    @Option(name = "memcpy-large")
    private Integer mMemcpyLarge = 2 * 1024 * 1024; // 2 MB

    @Option(name = "memread-iterations")
    private Integer mMemreadIterations = 100;

    @Option(name = "memread")
    private Integer mMemreadArg = 8 * 1024; // 8 KB

    @Option(name = "stop-framework", description = "Stop the framework before running tests")
    private boolean mStopFramework = true;

    @Option(name = "profile-mode", description =
            "In profile mode, microbench runs memset, memcpy, and "
                    + "memread from 32 bytes to 8 MB, doubling the size each time.")
    private boolean mProfileMode = false;

    @Option(name = "simpleperf-mode",
            description = "Whether use simpleperf to get low level metrics")
    private static boolean mSimpleperfMode = false;

    @Option(name = "simpleperf-argu", description = "simpleperf arguments")
    private List<String> mSimpleperfArgu = new ArrayList<String>();

    @Option(
        name = "simpleperf-cmd-timeout",
        description = "Timeout (in seconds) while running simpleperf shell command."
    )
    private int mSimpleperfCmdTimeout = 240;

    String mFormattedBinaryName = null;
    ITestDevice mTestDevice = null;
    SimplePerfUtil mSpUtil = null;

    /**
     * Class used to read and parse micro_bench output.
     */
    private static class MicroBenchReceiver extends MultiLineReceiver {
        private Pattern mPattern;
        private int mTimeGroup = -1;
        private int mPerfGroup = -1;
        private SimpleStats mTimeStats = null;
        private SimpleStats mPerfStats = null;
        private Map<String, Double> mSimpleperfMetricsMap = new HashMap<String, Double>();

        private boolean mSimpleperfOutput;

        /**
         * Create a receiver for parsing micro_bench output.
         *
         * @param pattern The regex {@link Pattern} to parse each line of output. Time and/or
         * performance numbers should be their own group.
         * @param timeGroup the index of the time data in the pattern.
         * @param perfGroup the index of the performance data in the pattern.
         */
        public MicroBenchReceiver(Pattern pattern, int timeGroup, int perfGroup) {
            mPattern = pattern;
            mTimeGroup = timeGroup;
            if (mTimeGroup > 0) {
                mTimeStats = new SimpleStats();
            }
            mPerfGroup = perfGroup;
            if (mPerfGroup > 0) {
                mPerfStats = new SimpleStats();
            }
            mSimpleperfOutput = false;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void processNewLines(String[] lines) {
            for (String line : lines) {
                if (!mSimpleperfOutput || !mSimpleperfMode) {
                    if (line.contains("Performance counter statistics:")) {
                        mSimpleperfOutput = true;
                        continue;
                    }
                    CLog.v(line);
                    Matcher m = mPattern.matcher(line);
                    if (m.matches()) {
                        if (mTimeStats != null) {
                            mTimeStats.add(
                                    Double.valueOf(m.group(mTimeGroup)) * 1000d); // secs to ms
                        }
                        if (mPerfStats != null) {
                            mPerfStats.add(Double.valueOf(m.group(mPerfGroup)));
                        }
                    }
                } else {
                    List<String> spResult = SimplePerfStatResultParser.parseSingleLine(line);
                    if (spResult.size() == 1) {
                        CLog.d(String.format("Total run time: %s", spResult.get(0)));
                    } else if (spResult.size() == 3) {
                        try {
                            Double metricValue = NumberFormat.getNumberInstance(Locale.US)
                                    .parse(spResult.get(0)).doubleValue();
                            mSimpleperfMetricsMap.put(spResult.get(1), metricValue);
                        } catch (ParseException e) {
                            CLog.e("Simpleperf metrics parse failure: " + e.toString());
                        }
                    } else {
                        CLog.d("Skipping line: " + line);
                    }
                }
            }
        }

        /**
         * Get the time stats.
         *
         * @return a {@link SimpleStats} object containing the time stats in ms.
         */
        public SimpleStats getTimeStats() {
            return mTimeStats;
        }

        /**
         * Get the performance stats.
         *
         * @return a {@link SimpleStats} object containing the performance stats in MB/s.
         */
        public SimpleStats getPerfStats() {
            return mPerfStats;
        }

        public Map<String, Double> getSimpleperfMetricsMap() {
            return mSimpleperfMetricsMap;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean isCancelled() {
            return false;
        }
    }

    /**
     * Class used to hold test data and run individual tests.
     */
    private class TestCase {
        private String mTestName;
        private String mTestKey;
        private String mCommand;
        private int mIterations = 0;
        private MicroBenchReceiver mReceiver;

        /**
         * Create a test case.
         *
         * @param testName The name of the test
         * @param testKey The key of the test used to report metrics.
         * @param command The command to run. Must contain a {@code %d} in the place of the number
         * of iterations.
         * @param iterations The amount of iterations to run.
         * @param pattern The regex {@link Pattern} to parse each line of output. Time and/or
         * performance numbers should be their own group.
         * @param timeGroup the index of the time data in the pattern.
         * @param perfGroup the index of the performance data in the pattern.
         */
        public TestCase(String testName, String testKey, String command, int iterations,
                Pattern pattern, int timeGroup, int perfGroup) {
            mTestName = testName;
            mTestKey = testKey;
            mCommand = command;
            mIterations = iterations;
            mReceiver = new MicroBenchReceiver(pattern, timeGroup, perfGroup);
        }

        /**
         * Runs the test and adds the results to the test metrics.
         *
         * @param listener the {@link ITestInvocationListener}.
         * @param metrics the metrics map for the results.
         * @throws DeviceNotAvailableException if the device is not available while running the
         * test.
         */
        public void runTest(ITestInvocationListener listener, Map<String, String> metrics)
                throws DeviceNotAvailableException {
            if (mIterations == 0) {
                return;
            }

            CLog.i("Running %s test", mTestName);

            TestDescription testId = new TestDescription(getClass().getCanonicalName(), mTestKey);
            listener.testStarted(testId);
            final String cmd = String.format(mCommand, mFormattedBinaryName, mIterations);

            if (mSimpleperfMode) {
                mSpUtil.executeCommand(cmd, mReceiver, mSimpleperfCmdTimeout, TimeUnit.SECONDS, 1);
            } else {
                mTestDevice.executeShellCommand(cmd, mReceiver);
            }

            Boolean timePass = parseStats(mReceiver.getTimeStats(), "time", metrics);
            Boolean perfPass = parseStats(mReceiver.getPerfStats(), "perf", metrics);

            for (Map.Entry<String, Double> entry :
                    mReceiver.getSimpleperfMetricsMap().entrySet()) {
                metrics.put(String.format("%s_%s", mTestKey, entry.getKey()),
                        Double.toString(entry.getValue() / mIterations));
            }

            if (!timePass || !perfPass) {
                listener.testFailed(testId,
                        "Iteration count mismatch (see host log).");
            }
            listener.testEnded(testId, new HashMap<String, Metric>());
        }

        /**
         * Parse the stats from {@link MicroBenchReceiver} and add them to the metrics
         *
         * @param stats {@link SimpleStats} from {@link MicroBenchReceiver} or {@code null} if
         * there are no stats.
         * @param category the category of the stats, either {@code perf} or {@code time}.
         * @param metrics the metrics map to add the parsed stats to.
         * @return {@code true} if the stats were successfully parsed and added or if
         * {@code stats == null}, {@code false} if there was an error.
         */
        private boolean parseStats(SimpleStats stats, String category,
                Map<String, String> metrics) {
            if (stats == null) {
                return true;
            }

            String unit = "perf".equals(category) ? "MB/s" : "ms";

            CLog.i("%s %s results: size = %d, mean = %f %s, stdev = %f %s", mTestName, category,
                    stats.size(), stats.mean(), unit, stats.stdev(), unit);

            if (stats.mean() == null) {
                CLog.e("Expected non null mean for %s %s", mTestName, category);
                return false;
            }

            if (stats.size() != mIterations) {
                CLog.e("Expected iterations (%d) not equal to sample size (%d) for %s %s",
                        mIterations, stats.size(), mTestName, category);
                return false;
            }

            if (metrics != null) {
                metrics.put(String.format("%s_%s", mTestKey, category),
                        Double.toString(stats.mean()));
            }
            return true;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        String name, cmd;
        List<TestCase> testCases = new ArrayList<TestCase>();

        mFormattedBinaryName = AbiFormatter.formatCmdForAbi(BINARY_NAME, mForceAbi);

        Assert.assertNotNull(mTestDevice);
        // TODO: Replace this with a more generic way to check for the presence of a binary
        final String output = mTestDevice.executeShellCommand(mFormattedBinaryName);
        Assert.assertFalse(output.contains(mFormattedBinaryName + ": not found"));

        if (mSimpleperfMode) {
            mSpUtil = SimplePerfUtil.newInstance(mTestDevice, SimplePerfType.STAT);
            if (mSimpleperfArgu.size() == 0) {
                mSimpleperfArgu.add("-e cpu-cycles:k,cpu-cycles:u");
            }
            mSpUtil.setArgumentList(mSimpleperfArgu);
        }

        if (mStopFramework) {
            mTestDevice.executeShellCommand("stop");
        }

        if (mProfileMode) {
            for (int i = 1 << 5; i < 1 << 24; i <<= 1) {
                name = String.format("memset %d", i);
                cmd = String.format(MEMSET_CMD, i);
                testCases.add(new TestCase(name, String.format("memset_%d", i), cmd,
                        mMemsetIterations, MEMSET_PATTERN, 1, 2));
            }

            for (int i = 1 << 5; i < 1 << 24; i <<= 1) {
                name = String.format("memcpy %d", i);
                cmd = String.format(MEMCPY_CMD, i);
                testCases.add(new TestCase(name, String.format("memcpy_%d", i), cmd,
                        mMemcpyIterations, MEMCPY_PATTERN, 1, 2));
            }

            for (int i = 1 << 5; i < 1 << 24; i <<= 1) {
                name = String.format("memread %d", i);
                cmd = String.format(MEMREAD_CMD, i);
                testCases.add(new TestCase(name, String.format("memread_%d", i), cmd,
                        mMemreadIterations, MEMREAD_PATTERN, 1, 2));
            }

            // Don't want to report test metrics in profile mode
            for (TestCase test : testCases) {
                test.runTest(listener, null);
            }
        } else {
            // Sleep test case
            testCases.add(new TestCase("sleep", "sleep", SLEEP_CMD, mSleepIterations, SLEEP_PATTERN,
                    1, -1));

            // memset small test case
            name = String.format("memset %d", mMemsetSmall);
            cmd = String.format(MEMSET_CMD, mMemsetSmall);
            testCases.add(new TestCase(name, "memset_small", cmd, mMemsetIterations, MEMSET_PATTERN,
                    1, 2));

            // memset large test case
            name = String.format("memset %d", mMemsetLarge);
            cmd = String.format(MEMSET_CMD, mMemsetLarge);
            testCases.add(new TestCase(name, "memset_large", cmd, mMemsetIterations, MEMSET_PATTERN,
                    1, 2));

            // memcpy small test case
            name = String.format("memcpy %d", mMemcpySmall);
            cmd = String.format(MEMCPY_CMD, mMemcpySmall);
            testCases.add(new TestCase(name, "memcpy_small", cmd, mMemcpyIterations, MEMCPY_PATTERN,
                    1, 2));

            // memcpy large test case
            name = String.format("memcpy %d", mMemcpyLarge);
            cmd = String.format(MEMCPY_CMD, mMemcpyLarge);
            testCases.add(new TestCase(name, "memcpy_large", cmd, mMemcpyIterations, MEMCPY_PATTERN,
                    1, 2));

            // memread test case
            name = String.format("memread %d", mMemreadArg);
            cmd = String.format(MEMREAD_CMD, mMemreadArg);
            testCases.add(new TestCase(name, "memread", cmd, mMemreadIterations, MEMREAD_PATTERN,
                    1, 2));

            listener.testRunStarted(RUN_KEY, testCases.size());
            long beginTime = System.currentTimeMillis();
            Map<String, String> metrics = new HashMap<String, String>();
            for (TestCase test : testCases) {
                test.runTest(listener, metrics);
            }
            listener.testRunEnded(
                    (System.currentTimeMillis() - beginTime),
                    TfMetricProtoUtil.upgradeConvert(metrics));
        }

        if (mStopFramework) {
            mTestDevice.executeShellCommand("start");
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
