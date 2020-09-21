/*
 * Copyright (C) 2012 The Android Open Source Project
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
package com.android.tradefed.testtype;

import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Measurements;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.BugreportCollector;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.testdefs.XmlDefsTest;
import com.android.tradefed.util.AbiFormatter;
import com.android.tradefed.util.ListInstrumentationParser;
import com.android.tradefed.util.ListInstrumentationParser.InstrumentationTarget;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * Runs all instrumentation found on current device.
 */
@OptionClass(alias = "installed-instrumentation")
public class InstalledInstrumentationsTest
        implements IDeviceTest, IResumableTest, IShardableTest, IStrictShardableTest {

    /** the metric key name for the test coverage target value */
    // TODO: move this to a more generic location
    public static final String COVERAGE_TARGET_KEY = XmlDefsTest.COVERAGE_TARGET_KEY;
    private static final String PM_LIST_CMD = "pm list instrumentation";
    private static final String LINE_SEPARATOR = "\\r?\\n";

    private ITestDevice mDevice;

    @Option(name = "shell-timeout",
            description="The defined timeout (in milliseconds) is used as a maximum waiting time "
                    + "when expecting the command output from the device. At any time, if the "
                    + "shell command does not output anything for a period longer than defined "
                    + "timeout the TF run terminates. For no timeout, set to 0.")
    private long mShellTimeout = 10 * 60 * 1000;  // default to 10 minutes

    @Option(name = "test-timeout",
            description="Sets timeout (in milliseconds) that will be applied to each test. In the "
                    + "event of a test timeout it will log the results and proceed with executing "
                    + "the next test. For no timeout, set to 0.")
    private int mTestTimeout = 5 * 60 * 1000;  // default to 5 minutes

    @Option(name = "size",
            description = "Restrict tests to a specific test size. " +
            "One of 'small', 'medium', 'large'",
            importance = Importance.IF_UNSET)
    private String mTestSize = null;

    @Option(name = "runner",
            description = "Restrict tests executed to a specific instrumentation class runner. " +
    "Installed instrumentations that do not have this runner will be skipped.")
    private String mRunner = null;

    @Option(name = "rerun",
            description = "Rerun unexecuted tests individually on same device if test run " +
            "fails to complete.")
    private boolean mIsRerunMode = true;

    @Option(name = "resume",
            description = "Schedule unexecuted tests for resumption on another device " +
            "if first device becomes unavailable.")
    private boolean mIsResumeMode = false;

    @Option(name = "send-coverage",
            description = "Send coverage target info to test listeners.")
    private boolean mSendCoverage = false;

    @Option(name = "bugreport-on-failure", description = "Sets which failed testcase events " +
            "cause a bugreport to be collected. a bugreport after failed testcases.  Note that " +
            "there is _no feedback mechanism_ between the test runner and the bugreport " +
            "collector, so use the EACH setting with due caution.")
    private BugreportCollector.Freq mBugreportFrequency = null;

    @Option(
        name = "bugreport-on-run-failure",
        description = "Take a bugreport if the instrumentation finish with a run failure"
    )
    private boolean mBugreportOnRunFailure = false;

    @Option(name = "screenshot-on-failure", description = "Take a screenshot on every test failure")
    private boolean mScreenshotOnFailure = false;

    @Option(name = "logcat-on-failure", description =
            "take a logcat snapshot on every test failure.")
    private boolean mLogcatOnFailures = false;

    @Option(name = "logcat-on-failure-size", description =
            "The max number of logcat data in bytes to capture when --logcat-on-failure is on. " +
            "Should be an amount that can comfortably fit in memory.")
    private int mMaxLogcatBytes = 500 * 1024; // 500K

    @Option(name = "class",
            description = "Only run tests in specified class")
    private String mTestClass = null;

    @Option(name = "package",
            description =
            "Only run tests within this specific java package. Will be ignored if --class is set.")
    private String mTestPackageName = null;

    @Option(name = "instrumentation-arg",
            description = "Additional instrumentation arguments to provide.")
    private Map<String, String> mInstrArgMap = new HashMap<String, String>();

    @Option(name = "rerun-from-file", description =
            "Use test file instead of separate adb commands for each test " +
            "when re-running instrumentations for tests that failed to run in previous attempts. ")
    private boolean mReRunUsingTestFile = false;

    @Option(name = "rerun-from-file-attempts", description =
            "Max attempts to rerun tests from file. -1 means rerun from file infinitely.")
    private int mReRunUsingTestFileAttempts = -1;

    @Option(name = "fallback-to-serial-rerun", description =
            "Rerun tests serially after rerun from file failed.")
    private boolean mFallbackToSerialRerun = true;

    @Option(name = "reboot-before-rerun", description =
            "Reboot a device before re-running instrumentations.")
    private boolean mRebootBeforeReRun = false;

    @Option(name = "shards", description =
            "Split test run into this many parallel shards")
    private int mShards = 0;

    @Option(name = "disable", description =
            "Disable the test by setting this flag to true.")
    private boolean mDisable = false;

    @Option(
        name = "coverage",
        description =
                "Collect code coverage for this test run. Note that the build under test must be a "
                        + "coverage build or else this will fail."
    )
    private boolean mCoverage = false;

    @Option(
        name = "hidden-api-checks",
        description =
                "If set to false, the '--no-hidden-api-checks' flag will be passed to the am "
                        + "instrument command. Only works for P or later."
    )
    private boolean mHiddenApiChecks = true;

    private int mTotalShards = 0;
    private int mShardIndex = 0;

    private List<InstrumentationTest> mTests = null;

    @Option(name = AbiFormatter.FORCE_ABI_STRING,
            description = AbiFormatter.FORCE_ABI_DESCRIPTION,
            importance = Importance.IF_UNSET)
    private String mForceAbi = null;

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
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * Set the send coverage flag.
     * <p/>
     * Exposed for unit testing.
     */
    void setSendCoverage(boolean sendCoverage) {
        mSendCoverage = sendCoverage;
    }

    /**
     * Gets the list of {@link InstrumentationTest}s contained within.
     * <p/>
     * Exposed for unit testing.
     */
    List<InstrumentationTest> getTests() {
        return mTests;
    }

    /**
     * Sets the number of total shards this test should be split into.
     * <p/>
     * Exposed for unit testing.
     */
    void setTotalShards(int totalShards) {
        mTotalShards = totalShards;
    }

    /**
     * Sets the shard index number of this test.
     * <p/>
     * Exposed for unit testing.
     */
    void setShardIndex(int shardIndex) {
        mShardIndex = shardIndex;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (getDevice() == null) {
            throw new IllegalArgumentException("Device has not been set");
        }

        if (mDisable) {
            return;
        }
        buildTests();
        doRun(listener);
    }

    /**
     * Build the list of tests to run from the device, if not done already. Note: Can be called
     * multiple times in case of resumed runs.
     * @throws DeviceNotAvailableException
     */
    private void buildTests() throws DeviceNotAvailableException {
        if (mTests == null) {

            ListInstrumentationParser parser = new ListInstrumentationParser();
            String pmListOutput = getDevice().executeShellCommand(PM_LIST_CMD);
            String pmListLines[] = pmListOutput.split(LINE_SEPARATOR);
            parser.processNewLines(pmListLines);

            if (parser.getInstrumentationTargets().isEmpty()) {
                throw new IllegalArgumentException(String.format(
                        "No instrumentations were found on device %s - <%s>", getDevice()
                                .getSerialNumber(), pmListOutput));
            }

            int numUnshardedTests = 0;
            mTests = new LinkedList<InstrumentationTest>();
            for (InstrumentationTarget target : parser.getInstrumentationTargets()) {
                if (mRunner == null || mRunner.equals(target.runnerName)) {
                    // Some older instrumentations are not shardable. As a result, we should try to
                    // shard these instrumentations by APKs, rather than by test methods.
                    if (mTotalShards > 0 && !target.isShardable()) {
                        numUnshardedTests += 1;
                        if ((numUnshardedTests - 1) % mTotalShards != mShardIndex) {
                            continue;
                        }
                    }

                    InstrumentationTest t = createInstrumentationTest();
                    try {
                        // Copies all current argument values to the new runner that will be
                        // used to actually run the tests.
                        OptionCopier.copyOptions(InstalledInstrumentationsTest.this, t);
                    } catch (ConfigurationException e) {
                        // Bail out rather than run tests with unexpected options
                        throw new RuntimeException("failed to copy instrumentation options", e);
                    }
                    t.setPackageName(target.packageName);
                    t.setRunnerName(target.runnerName);
                    t.setCoverageTarget(target.targetName);
                    if (mTotalShards > 0 && target.isShardable()) {
                        t.addInstrumentationArg("shardIndex", Integer.toString(mShardIndex));
                        t.addInstrumentationArg("numShards", Integer.toString(mTotalShards));
                    }
                    mTests.add(t);
                }
            }
        }
    }

    /**
     * Run the previously built tests.
     *
     * @param listener the {@link ITestInvocationListener}
     * @throws DeviceNotAvailableException
     */
    private void doRun(ITestInvocationListener listener) throws DeviceNotAvailableException {
        while (!mTests.isEmpty()) {
            InstrumentationTest test = mTests.get(0);

            CLog.d("Running test %s on %s", test.getPackageName(), getDevice().getSerialNumber());

            if (mSendCoverage && test.getCoverageTarget() != null) {
                sendCoverage(test.getPackageName(), test.getCoverageTarget(), listener);
            }
            test.setDevice(getDevice());
            test.setClassName(mTestClass);
            test.setTestPackageName(mTestPackageName);
            test.run(listener);
            // test completed, remove from list
            mTests.remove(0);
        }
    }

    /**
     * Forwards the tests coverage target info as a test metric.
     *
     * @param packageName
     * @param coverageTarget
     * @param listener
     */
    private void sendCoverage(String packageName, String coverageTarget,
            ITestInvocationListener listener) {
        HashMap<String, Metric> coverageMetric = new HashMap<String, Metric>();
        Metric metric =
                Metric.newBuilder()
                        .setMeasurements(
                                Measurements.newBuilder().setSingleString(coverageTarget).build())
                        .build();
        coverageMetric.put(COVERAGE_TARGET_KEY, metric);
        listener.testRunStarted(packageName, 0);
        listener.testRunEnded(0, coverageMetric);
    }

    long getShellTimeout() {
        return mShellTimeout;
    }

    int getTestTimeout() {
        return mTestTimeout;
    }

    String getTestSize() {
        return mTestSize;
    }

    /**
     * Creates the {@link InstrumentationTest} to use. Exposed for unit testing.
     */
    InstrumentationTest createInstrumentationTest() {
        // We do not know what kind of instrumentation we will find, so we don't enforce the ddmlib
        // format for AndroidJUnitRunner.
        InstrumentationTest test = new InstrumentationTest();
        test.setEnforceFormat(false);
        return test;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isResumable() {
        // hack to not resume if tests were never run
        // TODO: fix this properly in TestInvocation
        if (mTests == null) {
            return false;
        }
        return mIsResumeMode;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Collection<IRemoteTest> split() {
        if (mShards > 1) {
            Collection<IRemoteTest> shards = new ArrayList<>(mShards);
            for (int index = 0; index < mShards; index++) {
                shards.add(getTestShard(mShards, index));
            }
            return shards;
        }
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IRemoteTest getTestShard(int shardCount, int shardIndex) {
        InstalledInstrumentationsTest shard = new InstalledInstrumentationsTest();
        try {
            OptionCopier.copyOptions(this, shard);
        } catch (ConfigurationException e) {
            CLog.e("failed to copy instrumentation options: %s", e.getMessage());
        }
        shard.mShards = 0;
        shard.mShardIndex = shardIndex;
        shard.mTotalShards = shardCount;
        return shard;
    }
}
