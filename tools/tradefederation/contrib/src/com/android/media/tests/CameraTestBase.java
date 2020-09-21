/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.media.tests;

import com.android.ddmlib.CollectingOutputReceiver;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.InstrumentationTest;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.TimeUnit;

/**
 * Camera test base class
 *
 * Camera2StressTest, CameraStartupTest, Camera2LatencyTest and CameraPerformanceTest use this base
 * class for Camera ivvavik and later.
 */
public class CameraTestBase implements IDeviceTest, IRemoteTest, IConfigurationReceiver {

    private static final long SHELL_TIMEOUT_MS = 60 * 1000;  // 1 min
    private static final int SHELL_MAX_ATTEMPTS = 3;
    protected static final String PROCESS_CAMERA_DAEMON = "mm-qcamera-daemon";
    protected static final String PROCESS_MEDIASERVER = "mediaserver";
    protected static final String PROCESS_CAMERA_APP = "com.google.android.GoogleCamera";
    protected static final String DUMP_ION_HEAPS_COMMAND = "cat /d/ion/heaps/system";
    protected static final String ARGUMENT_TEST_ITERATIONS = "iterations";

    @Option(name = "test-package", description = "Test package to run.")
    private String mTestPackage = "com.google.android.camera";

    @Option(name = "test-class", description = "Test class to run.")
    private String mTestClass = null;

    @Option(name = "test-methods", description = "Test method to run. May be repeated.")
    private Collection<String> mTestMethods = new ArrayList<>();

    @Option(name = "test-runner", description = "Test runner for test instrumentation.")
    private String mTestRunner = "android.test.InstrumentationTestRunner";

    @Option(name = "test-timeout", description = "Max time allowed in ms for a test run.")
    private int mTestTimeoutMs = 60 * 60 * 1000; // 1 hour

    @Option(name = "shell-timeout",
            description="The defined timeout (in milliseconds) is used as a maximum waiting time "
                    + "when expecting the command output from the device. At any time, if the "
                    + "shell command does not output anything for a period longer than defined "
                    + "timeout the TF run terminates. For no timeout, set to 0.")
    private long mShellTimeoutMs = 60 * 60 * 1000;  // default to 1 hour

    @Option(name = "ru-key", description = "Result key to use when posting to the dashboard.")
    private String mRuKey = null;

    @Option(name = "logcat-on-failure", description =
            "take a logcat snapshot on every test failure.")
    private boolean mLogcatOnFailure = false;

    @Option(
        name = "instrumentation-arg",
        description = "Additional instrumentation arguments to provide."
    )
    private Map<String, String> mInstrArgMap = new HashMap<>();

    @Option(name = "dump-meminfo", description =
            "take a dumpsys meminfo at a given interval time.")
    private boolean mDumpMeminfo = false;

    @Option(name="dump-meminfo-interval-ms",
            description="Interval of calling dumpsys meminfo in milliseconds.")
    private int mMeminfoIntervalMs = 5 * 60 * 1000;     // 5 minutes

    @Option(name = "dump-ion-heap", description =
            "dump ION allocations at the end of test.")
    private boolean mDumpIonHeap = false;

    @Option(name = "dump-thread-count", description =
            "Count the number of threads in Camera process at a given interval time.")
    private boolean mDumpThreadCount = false;

    @Option(name="dump-thread-count-interval-ms",
            description="Interval of calling ps to count the number of threads in milliseconds.")
    private int mThreadCountIntervalMs = 5 * 60 * 1000; // 5 minutes

    @Option(name="iterations", description="The number of iterations to run. Default to 1. "
            + "This takes effect only when Camera2InstrumentationTestRunner is used to execute "
            + "framework stress tests.")
    private int mIterations = 1;

    private ITestDevice mDevice = null;

    // A base listener to collect the results from each test run. Test results will be forwarded
    // to other listeners.
    private AbstractCollectingListener mCollectingListener = null;

    private long mStartTimeMs = 0;

    private MeminfoTimer mMeminfoTimer = null;
    private ThreadTrackerTimer mThreadTrackerTimer = null;

    protected IConfiguration mConfiguration;

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        // ignore
    }

    /**
     * Run Camera instrumentation test with a default listener.
     *
     * @param listener the ITestInvocationListener of test results
     * @throws DeviceNotAvailableException
     */
    protected void runInstrumentationTest(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        if (mCollectingListener == null) {
            mCollectingListener = new DefaultCollectingListener(listener);
        }
        runInstrumentationTest(listener, mCollectingListener);
    }

    /**
     * Run Camera instrumentation test with a listener to gather the metrics from the individual
     * test runs.
     *
     * @param listener the ITestInvocationListener of test results
     * @param collectingListener the {@link CollectingTestListener} to collect the metrics from
     *                           test runs
     * @throws DeviceNotAvailableException
     */
    protected void runInstrumentationTest(ITestInvocationListener listener,
            AbstractCollectingListener collectingListener)
            throws DeviceNotAvailableException {
        Assert.assertNotNull(collectingListener);
        mCollectingListener = collectingListener;

        InstrumentationTest instr = new InstrumentationTest();
        instr.setDevice(getDevice());
        instr.setPackageName(getTestPackage());
        instr.setRunnerName(getTestRunner());
        instr.setClassName(getTestClass());
        instr.setTestTimeout(getTestTimeoutMs());
        instr.setShellTimeout(getShellTimeoutMs());
        instr.setLogcatOnFailure(mLogcatOnFailure);
        instr.setRunName(getRuKey());
        instr.setRerunMode(false);

        // Set test iteration.
        if (getIterationCount() > 1) {
            CLog.v("Setting test iterations: %d", getIterationCount());
            Map<String, String> instrArgMap = getInstrumentationArgMap();
            instrArgMap.put(ARGUMENT_TEST_ITERATIONS, String.valueOf(getIterationCount()));
        }

        for (Map.Entry<String, String> entry : getInstrumentationArgMap().entrySet()) {
            instr.addInstrumentationArg(entry.getKey(), entry.getValue());
        }

        // Check if dumpheap needs to be taken for native processes before test runs.
        if (shouldDumpMeminfo()) {
            mMeminfoTimer = new MeminfoTimer(0, mMeminfoIntervalMs);
        }
        if (shouldDumpThreadCount()) {
            long delayMs = mThreadCountIntervalMs / 2;  // Not to run all dump at the same interval.
            mThreadTrackerTimer = new ThreadTrackerTimer(delayMs, mThreadCountIntervalMs);
        }

        // Run tests.
        mStartTimeMs = System.currentTimeMillis();
        if (mTestMethods.size() > 0) {
            CLog.d(String.format("The number of test methods is: %d", mTestMethods.size()));
            for (String testName : mTestMethods) {
                instr.setMethodName(testName);
                instr.run(mCollectingListener);
            }
        } else {
            instr.run(mCollectingListener);
        }

        dumpIonHeaps(mCollectingListener, getTestClass());
    }

    /**
     * A base listener to collect all test results and metrics from Camera instrumentation test run.
     * Abstract methods can be overridden to handle test metrics or inform of test run ended.
     */
    protected abstract class AbstractCollectingListener extends CollectingTestListener {

        private ITestInvocationListener mListener = null;
        private Map<String, String> mMetrics = new HashMap<>();
        private Map<String, String> mFatalErrors = new HashMap<>();

        private static final String INCOMPLETE_TEST_ERR_MSG_PREFIX =
                "Test failed to run to completion. Reason: 'Instrumentation run failed";

        public AbstractCollectingListener(ITestInvocationListener listener) {
            mListener = listener;
        }

        /**
         * Override only when subclasses need to get the test metrics from an individual
         * instrumentation test. To aggregate the metrics from each test, update the
         * getAggregatedMetrics and post them at the end of test run.
         *
         * @param test identifies the test
         * @param testMetrics a {@link Map} of the metrics emitted
         */
        abstract public void handleMetricsOnTestEnded(TestDescription test,
                Map<String, String> testMetrics);

        /**
         * Override only when it needs to inform subclasses of instrumentation test run ended,
         * so that subclasses have a chance to peek the aggregated results at the end of test run
         * and to decide what metrics to be posted.
         * Either {@link ITestInvocationListener#testRunEnded} or
         * {@link ITestInvocationListener#testRunFailed} should be called in this function to
         * report the test run status.
         *
         * @param listener - the ITestInvocationListener of test results
         * @param elapsedTime - device reported elapsed time, in milliseconds
         * @param runMetrics - key-value pairs reported at the end of an instrumentation test run.
         *                   Use getAggregatedMetrics to retrieve the metrics aggregated
         *                   from an individual test, instead.
         */
        abstract public void handleTestRunEnded(ITestInvocationListener listener,
                long elapsedTime, Map<String, String> runMetrics);

        /**
         * Report the end of an individual camera test and delegate handling the collected metrics
         * to subclasses. Do not override testEnded to manipulate the test metrics after each test.
         * Instead, use handleMetricsOnTestEnded.
         *
         * @param test identifies the test
         * @param testMetrics a {@link Map} of the metrics emitted
         */
        @Override
        public void testEnded(
                TestDescription test, long endTime, HashMap<String, Metric> testMetrics) {
            super.testEnded(test, endTime, testMetrics);
            handleMetricsOnTestEnded(test, TfMetricProtoUtil.compatibleConvert(testMetrics));
            stopDumping(test);
            mListener.testEnded(test, endTime, testMetrics);
        }

        @Override
        public void testStarted(TestDescription test, long startTime) {
            super.testStarted(test, startTime);
            startDumping(test);
            mListener.testStarted(test, startTime);
        }

        @Override
        public void testFailed(TestDescription test, String trace) {
            super.testFailed(test, trace);
            // If the test failed to run to complete, this is an exceptional case.
            // Let this test run fail so that it can rerun.
            if (trace.startsWith(INCOMPLETE_TEST_ERR_MSG_PREFIX)) {
                mFatalErrors.put(test.getTestName(), trace);
                CLog.d("Test (%s) failed due to fatal error : %s", test.getTestName(), trace);
            }
            mListener.testFailed(test, trace);
        }

        @Override
        public void testRunFailed(String errorMessage) {
            super.testRunFailed(errorMessage);
            mFatalErrors.put(getRuKey(), errorMessage);
        }

        @Override
        public void testRunEnded(long elapsedTime, HashMap<String, Metric> runMetrics) {
            super.testRunEnded(elapsedTime, runMetrics);
            handleTestRunEnded(mListener, elapsedTime,
                    TfMetricProtoUtil.compatibleConvert(runMetrics));
            // never be called since handleTestRunEnded will handle it if needed.
            //mListener.testRunEnded(elapsedTime, runMetrics);
        }

        @Override
        public void testRunStarted(String runName, int testCount) {
            super.testRunStarted(runName, testCount);
            mListener.testRunStarted(runName, testCount);
        }

        @Override
        public void testRunStopped(long elapsedTime) {
            super.testRunStopped(elapsedTime);
            mListener.testRunStopped(elapsedTime);
        }

        @Override
        public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
            super.testLog(dataName, dataType, dataStream);
            mListener.testLog(dataName, dataType, dataStream);
        }

        protected void startDumping(TestDescription test) {
            if (shouldDumpMeminfo()) {
                mMeminfoTimer.start(test);
            }
            if (shouldDumpThreadCount()) {
                mThreadTrackerTimer.start(test);
            }
        }

        protected void stopDumping(TestDescription test) {
            InputStreamSource outputSource = null;
            File outputFile = null;
            if (shouldDumpMeminfo()) {
                mMeminfoTimer.stop();
                // Grab a snapshot of meminfo file and post it to dashboard.
                try {
                    outputFile = mMeminfoTimer.getOutputFile();
                    outputSource = new FileInputStreamSource(outputFile, true /* delete */);
                    String logName = String.format("meminfo_%s", test.getTestName());
                    mListener.testLog(logName, LogDataType.TEXT, outputSource);
                } finally {
                    StreamUtil.cancel(outputSource);
                }
            }
            if (shouldDumpThreadCount()) {
                mThreadTrackerTimer.stop();
                try {
                    outputFile = mThreadTrackerTimer.getOutputFile();
                    outputSource = new FileInputStreamSource(outputFile, true /* delete */);
                    String logName = String.format("ps_%s", test.getTestName());
                    mListener.testLog(logName, LogDataType.TEXT, outputSource);
                } finally {
                    StreamUtil.cancel(outputSource);
                }
            }
        }

        public Map<String, String> getAggregatedMetrics() {
            return mMetrics;
        }

        public ITestInvocationListener getListeners() {
            return mListener;
        }

        /**
         * Determine that the test run failed with fatal errors.
         *
         * @return True if test run has a failure due to fatal error.
         */
        public boolean hasTestRunFatalError() {
            return (getNumTotalTests() > 0 && mFatalErrors.size() > 0);
        }

        public Map<String, String> getFatalErrors() {
            return mFatalErrors;
        }

        public String getErrorMessage() {
            StringBuilder sb = new StringBuilder();
            for (Map.Entry<String, String> error : mFatalErrors.entrySet()) {
                sb.append(error.getKey());
                sb.append(" : ");
                sb.append(error.getValue());
                sb.append("\n");
            }
            return sb.toString();
        }
    }

    protected class DefaultCollectingListener extends AbstractCollectingListener {

        public DefaultCollectingListener(ITestInvocationListener listener) {
            super(listener);
        }

        @Override
        public void handleMetricsOnTestEnded(TestDescription test, Map<String, String> testMetrics) {
            if (testMetrics == null) {
                return; // No-op if there is nothing to post.
            }
            getAggregatedMetrics().putAll(testMetrics);
        }

        @Override
        public void handleTestRunEnded(ITestInvocationListener listener, long elapsedTime,
                Map<String, String> runMetrics) {
            // Post aggregated metrics at the end of test run.
            listener.testRunEnded(getTestDurationMs(), TfMetricProtoUtil.upgradeConvert(getAggregatedMetrics()));
        }
    }

    // TODO: Leverage AUPT to collect system logs (meminfo, ION allocations and processes/threads)
    private class MeminfoTimer {

        private static final String LOG_HEADER =
                "uptime,pssCameraDaemon,pssCameraApp,ramTotal,ramFree,ramUsed";
        private static final String DUMPSYS_MEMINFO_COMMAND =
                "dumpsys meminfo -c | grep -w -e " + "^ram -e ^time";
        private String[] mDumpsysMemInfoProc = {
            PROCESS_CAMERA_DAEMON, PROCESS_CAMERA_APP, PROCESS_MEDIASERVER
        };
        private static final int STATE_STOPPED = 0;
        private static final int STATE_SCHEDULED = 1;
        private static final int STATE_RUNNING = 2;

        private int mState = STATE_STOPPED;
        private Timer mTimer = new Timer(true); // run as a daemon thread
        private long mDelayMs = 0;
        private long mPeriodMs = 60 * 1000;  // 60 sec
        private File mOutputFile = null;
        private String mCommand;

        public MeminfoTimer(long delayMs, long periodMs) {
            mDelayMs = delayMs;
            mPeriodMs = periodMs;
            mCommand = DUMPSYS_MEMINFO_COMMAND;
            for (String process : mDumpsysMemInfoProc) {
                mCommand += " -e " + process;
            }
        }

        synchronized void start(TestDescription test) {
            if (isRunning()) {
                stop();
            }
            // Create an output file.
            if (createOutputFile(test) == null) {
                CLog.w("Stop collecting meminfo since meminfo log file not found.");
                mState = STATE_STOPPED;
                return; // No-op
            }
            mTimer.scheduleAtFixedRate(new TimerTask() {
                @Override
                public void run() {
                    mState = STATE_RUNNING;
                    dumpMeminfo(mCommand, mOutputFile);
                }
            }, mDelayMs, mPeriodMs);
            mState = STATE_SCHEDULED;
        }

        synchronized void stop() {
            mState = STATE_STOPPED;
            mTimer.cancel();
        }

        synchronized boolean isRunning() {
            return (mState == STATE_RUNNING);
        }

        public File getOutputFile() {
            return mOutputFile;
        }

        private File createOutputFile(TestDescription test) {
            try {
                mOutputFile = FileUtil.createTempFile(
                        String.format("meminfo_%s", test.getTestName()), "csv");
                BufferedWriter writer = new BufferedWriter(new FileWriter(mOutputFile, false));
                writer.write(LOG_HEADER);
                writer.newLine();
                writer.flush();
                writer.close();
            } catch (IOException e) {
                CLog.w("Failed to create meminfo log file %s:", mOutputFile.getAbsolutePath());
                CLog.e(e);
                return null;
            }
            return mOutputFile;
        }
    }

    void dumpMeminfo(String command, File outputFile) {
        try {
            CollectingOutputReceiver receiver = new CollectingOutputReceiver();
            // Dump meminfo in a compact form.
            getDevice().executeShellCommand(command, receiver,
                    SHELL_TIMEOUT_MS, TimeUnit.MILLISECONDS, SHELL_MAX_ATTEMPTS);
            printMeminfo(outputFile, receiver.getOutput());
        } catch (DeviceNotAvailableException e) {
            CLog.w("Failed to dump meminfo:");
            CLog.e(e);
        }
    }

    void printMeminfo(File outputFile, String meminfo) {
        // Parse meminfo and print each iteration in a line in a .csv format. The meminfo output
        // are separated into three different formats:
        //
        // Format: time,<uptime>,<realtime>
        // eg. "time,59459911,63354673"
        //
        // Format: proc,<oom_label>,<process_name>,<pid>,<pss>,<hasActivities>
        // eg. "proc,native,mm-qcamera-daemon,522,12881,e"
        //     "proc,fore,com.google.android.GoogleCamera,26560,70880,a"
        //
        // Format: ram,<total>,<free>,<used>
        // eg. "ram,1857364,810189,541516"
        BufferedWriter writer = null;
        BufferedReader reader = null;
        try {
            final String DELIMITER = ",";
            writer = new BufferedWriter(new FileWriter(outputFile, true));
            reader = new BufferedReader(new StringReader(meminfo));
            String line;
            String uptime = null;
            String pssCameraNative = null;
            String pssCameraApp = null;
            String ramTotal = null;
            String ramFree = null;
            String ramUsed = null;
            while ((line = reader.readLine()) != null) {
                if (line.startsWith("time")) {
                    uptime = line.split(DELIMITER)[1];
                } else if (line.startsWith("ram")) {
                    String[] ram = line.split(DELIMITER);
                    ramTotal = ram[1];
                    ramFree = ram[2];
                    ramUsed = ram[3];
                } else if (line.contains(PROCESS_CAMERA_DAEMON)) {
                    pssCameraNative = line.split(DELIMITER)[4];
                } else if (line.contains(PROCESS_CAMERA_APP)) {
                    pssCameraApp = line.split(DELIMITER)[4];
                }
            }
            String printMsg = String.format(
                    "%s,%s,%s,%s,%s,%s", uptime, pssCameraNative, pssCameraApp,
                    ramTotal, ramFree, ramUsed);
            writer.write(printMsg);
            writer.newLine();
            writer.flush();
        } catch (IOException e) {
            CLog.w("Failed to print meminfo to %s:", outputFile.getAbsolutePath());
            CLog.e(e);
        } finally {
            StreamUtil.close(writer);
        }
    }

    // TODO: Leverage AUPT to collect system logs (meminfo, ION allocations and processes/threads)
    private class ThreadTrackerTimer {

        // list all threads in a given process, remove the first header line, squeeze whitespaces,
        // select thread name (in 14th column), then sort and group by its name.
        // Examples:
        //    3 SoundPoolThread
        //    3 SoundPool
        //    2 Camera Job Disp
        //    1 pool-7-thread-1
        //    1 pool-6-thread-1
        // FIXME: Resolve the error "sh: syntax error: '|' unexpected" using the command below
        // $ /system/bin/ps -t -p %s | tr -s ' ' | cut -d' ' -f13- | sort | uniq -c | sort -nr"
        private static final String PS_COMMAND_FORMAT = "/system/bin/ps -t -p %s";
        private static final String PGREP_COMMAND_FORMAT = "pgrep %s";
        private static final int STATE_STOPPED = 0;
        private static final int STATE_SCHEDULED = 1;
        private static final int STATE_RUNNING = 2;

        private int mState = STATE_STOPPED;
        private Timer mTimer = new Timer(true); // run as a daemon thread
        private long mDelayMs = 0;
        private long mPeriodMs = 60 * 1000;  // 60 sec
        private File mOutputFile = null;

        public ThreadTrackerTimer(long delayMs, long periodMs) {
            mDelayMs = delayMs;
            mPeriodMs = periodMs;
        }

        synchronized void start(TestDescription test) {
            if (isRunning()) {
                stop();
            }
            // Create an output file.
            if (createOutputFile(test) == null) {
                CLog.w("Stop collecting thread counts since log file not found.");
                mState = STATE_STOPPED;
                return; // No-op
            }
            mTimer.scheduleAtFixedRate(new TimerTask() {
                @Override
                public void run() {
                    mState = STATE_RUNNING;
                    dumpThreadCount(PS_COMMAND_FORMAT, getPid(PROCESS_CAMERA_APP), mOutputFile);
                }
            }, mDelayMs, mPeriodMs);
            mState = STATE_SCHEDULED;
        }

        synchronized void stop() {
            mState = STATE_STOPPED;
            mTimer.cancel();
        }

        synchronized boolean isRunning() {
            return (mState == STATE_RUNNING);
        }

        public File getOutputFile() {
            return mOutputFile;
        }

        File createOutputFile(TestDescription test) {
            try {
                mOutputFile = FileUtil.createTempFile(
                        String.format("ps_%s", test.getTestName()), "txt");
                new BufferedWriter(new FileWriter(mOutputFile, false)).close();
            } catch (IOException e) {
                CLog.w("Failed to create processes and threads file %s:",
                        mOutputFile.getAbsolutePath());
                CLog.e(e);
                return null;
            }
            return mOutputFile;
        }

        String getPid(String processName) {
            String result = null;
            try {
                result = getDevice().executeShellCommand(String.format(PGREP_COMMAND_FORMAT,
                        processName));
            } catch (DeviceNotAvailableException e) {
                CLog.w("Failed to get pid %s:", processName);
                CLog.e(e);
            }
            return result;
        }

        String getUptime() {
            String uptime = null;
            try {
                // uptime will typically have a format like "5278.73 1866.80".  Use the first one
                // (which is wall-time)
                uptime = getDevice().executeShellCommand("cat /proc/uptime").split(" ")[0];
                Float.parseFloat(uptime);
            } catch (NumberFormatException e) {
                CLog.w("Failed to get valid uptime %s: %s", uptime, e);
            } catch (DeviceNotAvailableException e) {
                CLog.w("Failed to get valid uptime: %s", e);
            }
            return uptime;
        }

        void dumpThreadCount(String commandFormat, String pid, File outputFile) {
            try {
                if ("".equals(pid)) {
                    return;
                }
                String result = getDevice().executeShellCommand(String.format(commandFormat, pid));
                String header = String.format("UPTIME: %s", getUptime());
                BufferedWriter writer = new BufferedWriter(new FileWriter(outputFile, true));
                writer.write(header);
                writer.newLine();
                writer.write(result);
                writer.newLine();
                writer.flush();
                writer.close();
            } catch (DeviceNotAvailableException | IOException e) {
                CLog.w("Failed to dump thread count:");
                CLog.e(e);
            }
        }
    }

    // TODO: Leverage AUPT to collect system logs (meminfo, ION allocations and
    // processes/threads)
    protected void dumpIonHeaps(ITestInvocationListener listener, String testClass) {
        if (!shouldDumpIonHeap()) {
            return; // No-op if option is not set.
        }
        try {
            String result = getDevice().executeShellCommand(DUMP_ION_HEAPS_COMMAND);
            if (!"".equals(result)) {
                String fileName = String.format("ionheaps_%s_onEnd", testClass);
                listener.testLog(fileName, LogDataType.TEXT,
                        new ByteArrayInputStreamSource(result.getBytes()));
            }
        } catch (DeviceNotAvailableException e) {
            CLog.w("Failed to dump ION heaps:");
            CLog.e(e);
        }
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
    public void setConfiguration(IConfiguration configuration) {
        mConfiguration = configuration;
    }

    /**
     * Get the {@link IRunUtil} instance to use.
     * <p/>
     * Exposed so unit tests can mock.
     */
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /**
     * Get the duration of Camera test instrumentation in milliseconds.
     *
     * @return the duration of Camera instrumentation test until it is called.
     */
    public long getTestDurationMs() {
        return System.currentTimeMillis() - mStartTimeMs;
    }

    public String getTestPackage() {
        return mTestPackage;
    }

    public void setTestPackage(String testPackage) {
        mTestPackage = testPackage;
    }

    public String getTestClass() {
        return mTestClass;
    }

    public void setTestClass(String testClass) {
        mTestClass = testClass;
    }

    public String getTestRunner() {
        return mTestRunner;
    }

    public void setTestRunner(String testRunner) {
        mTestRunner = testRunner;
    }

    public int getTestTimeoutMs() {
        return mTestTimeoutMs;
    }

    public void setTestTimeoutMs(int testTimeoutMs) {
        mTestTimeoutMs = testTimeoutMs;
    }

    public long getShellTimeoutMs() {
        return mShellTimeoutMs;
    }

    public void setShellTimeoutMs(long shellTimeoutMs) {
        mShellTimeoutMs = shellTimeoutMs;
    }

    public String getRuKey() {
        return mRuKey;
    }

    public void setRuKey(String ruKey) {
        mRuKey = ruKey;
    }

    public boolean shouldDumpMeminfo() {
        return mDumpMeminfo;
    }

    public boolean shouldDumpIonHeap() {
        return mDumpIonHeap;
    }

    public boolean shouldDumpThreadCount() {
        return mDumpThreadCount;
    }

    public AbstractCollectingListener getCollectingListener() {
        return mCollectingListener;
    }

    public void setLogcatOnFailure(boolean logcatOnFailure) {
        mLogcatOnFailure = logcatOnFailure;
    }

    public int getIterationCount() {
        return mIterations;
    }

    public Map<String, String> getInstrumentationArgMap() { return mInstrArgMap; }
}
