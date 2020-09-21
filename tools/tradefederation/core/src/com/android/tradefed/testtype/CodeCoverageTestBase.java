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
package com.android.tradefed.testtype;

import static com.google.common.base.Preconditions.checkState;

import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.ICompressionStrategy;
import com.android.tradefed.util.ListInstrumentationParser;
import com.android.tradefed.util.ListInstrumentationParser.InstrumentationTarget;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Strings;
import com.google.common.collect.ImmutableMap;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * An abstract base class which runs installed instrumentation test(s) and collects execution data
 * from each test that was run. Subclasses should implement the {@link #getReportFormat()} method
 * to convert the execution data into a human readable report and log it.
 */
public abstract class CodeCoverageTestBase<T extends CodeCoverageReportFormat>
        implements IDeviceTest, IRemoteTest, IBuildReceiver {

    private ITestDevice mDevice = null;
    private IBuildInfo mBuild = null;

    @Option(name = "package",
            description = "Only run instrumentation targets with the given package name")
    private List<String> mPackageFilter = new ArrayList<>();

    @Option(name = "runner",
            description = "Only run instrumentation targets with the given test runner")
    private List<String> mRunnerFilter = new ArrayList<>();

    @Option(
        name = "instrumentation-arg",
        description = "Additional instrumentation arguments to provide to the runner"
    )
    private Map<String, String> mInstrumentationArgs = new HashMap<String, String>();

    @Option(name = "max-tests-per-chunk",
            description = "Maximum number of tests to execute in a single call to 'am instrument'. "
            + "Used to limit the number of tests that need to be re-run if one of them crashes.")
    private int mMaxTestsPerChunk = Integer.MAX_VALUE;

    @Option(name = "compression-strategy",
            description = "Class name of an ICompressionStrategy that will be used to compress the "
            + "coverage report into a single archive file.")
    private String mCompressionStrategy = "com.android.tradefed.util.ZipCompressionStrategy";

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
    public void setBuild(IBuildInfo buildInfo) {
        mBuild = buildInfo;
    }

    /** Returns the {@link IBuildInfo} for this invocation. */
    IBuildInfo getBuild() {
        return mBuild;
    }

    /** Returns the package filter as set by the --package option(s). */
    List<String> getPackageFilter() {
        return mPackageFilter;
    }

    /** Sets the package-filter option for testing. */
    @VisibleForTesting
    void setPackageFilter(List<String> packageFilter) {
        mPackageFilter = packageFilter;
    }

    /** Returns the runner filter as set by the --runner option(s). */
    List<String> getRunnerFilter() {
        return mRunnerFilter;
    }

    /** Sets the runner-filter option for testing. */
    @VisibleForTesting
    void setRunnerFilter(List<String> runnerFilter) {
        mRunnerFilter = runnerFilter;
    }

    /** Returns the instrumentation arguments as set by the --instrumentation-arg option(s). */
    Map<String, String> getInstrumentationArgs() {
        return mInstrumentationArgs;
    }

    /** Sets the instrumentation-arg options for testing. */
    @VisibleForTesting
    void setInstrumentationArgs(Map<String, String> instrumentationArgs) {
        mInstrumentationArgs = ImmutableMap.copyOf(instrumentationArgs);
    }

    /** Returns the maximum number of tests to run at once as set by --max-tests-per-chunk. */
    int getMaxTestsPerChunk() {
        return mMaxTestsPerChunk;
    }

    /** Sets the max-tests-per-chunk option for testing. */
    @VisibleForTesting
    void setMaxTestsPerChunk(int maxTestsPerChunk) {
        mMaxTestsPerChunk = maxTestsPerChunk;
    }

    /** Returns the compression strategy that should be used to archive the coverage report.  */
    ICompressionStrategy getCompressionStrategy() {
        try {
            Class<?> clazz = Class.forName(mCompressionStrategy);
            return clazz.asSubclass(ICompressionStrategy.class).newInstance();
        } catch (ClassNotFoundException e) {
            throw new RuntimeException("Unknown compression strategy: %s", e);
        } catch (ClassCastException e) {
            String msg = String.format("%s does not implement ICompressionStrategy",
                    mCompressionStrategy);
            throw new RuntimeException(msg, e);
        } catch (IllegalAccessException | InstantiationException e) {
            String msg = String.format("Could not instantiate %s. The compression strategy must "
                    + "have a public no-args constructor.", mCompressionStrategy);
            throw new RuntimeException(msg, e);
        }
    }

    /** Returns the list of output formats to use when generating the coverage report. */
    protected abstract List<T> getReportFormat();

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(final ITestInvocationListener listener) throws DeviceNotAvailableException {

        File reportDir = null;
        File reportArchive = null;
        // Initialize a listener to collect logged coverage files
        try (CoverageCollectingListener coverageListener =
                new CoverageCollectingListener(getDevice(), listener)) {

            // Make sure there are some installed instrumentation targets
            Collection<InstrumentationTarget> instrumentationTargets = getInstrumentationTargets();
            if (instrumentationTargets.isEmpty()) {
                throw new RuntimeException("No instrumentation targets found");
            }

            // Run each of the installed instrumentation targets
            for (InstrumentationTarget target : instrumentationTargets) {
                // Compute the number of shards to use
                int numShards = doesRunnerSupportSharding(target) ? getNumberOfShards(target) : 1;

                // Split the test into shards and invoke each chunk separately in order to limit the
                // number of test methods that need to be re-run if the test crashes.
                for (int shardIndex = 0; shardIndex < numShards; shardIndex++) {
                    // Run the current shard
                    TestRunResult result = runTest(target, shardIndex, numShards, coverageListener);

                    // If the shard ran to completion and the coverage file was generated
                    String coverageFile = result.getRunMetrics().get(
                            CodeCoverageTest.COVERAGE_REMOTE_FILE_LABEL);
                    if (!result.isRunFailure() && getDevice().doesFileExist(coverageFile)) {
                        // Move on to the next shard
                        continue;
                    }

                    // Something went wrong with this shard, so re-run the tests individually
                    for (TestDescription identifier : collectTests(target, shardIndex, numShards)) {
                        runTest(target, identifier, coverageListener);
                    }
                }
            }

            // Generate the coverage report(s) and log it
            List<File> measurements = coverageListener.getCoverageFiles();
            for (T format : getReportFormat()) {
                File report = generateCoverageReport(measurements, format);
                try {
                    doLogReport("coverage", format.getLogDataType(), report, listener);
                } finally {
                    FileUtil.recursiveDelete(report);
                }
            }
        } catch (IOException e) {
            // Rethrow
            throw new RuntimeException(e);
        } finally {
            // Cleanup
            FileUtil.recursiveDelete(reportDir);
            FileUtil.deleteFile(reportArchive);
            cleanup();
        }
    }

    /**
     * Generates a human-readable coverage report from the given execution data. This method is
     * called after all of the tests have finished running.
     *
     * @param executionData The execution data files collected while running the tests.
     * @param format The output format of the generated coverage report.
     */
    protected abstract File generateCoverageReport(Collection<File> executionData, T format)
            throws IOException;

    /**
     * Cleans up any resources allocated during a test run. Called at the end of the
     * {@link #run(ITestInvocationListener)} after all coverage reports have been logged. This
     * method is a stub, but can be overridden by subclasses as necessary.
     */
    protected void cleanup() { }

    /**
     * Logs the given data with the provided logger. The {@code data} can be a regular file, or a
     * directory. If the data is a directory, it is compressed into a single archive file before
     * being logged.
     *
     * @param dataName The name to use when logging the data.
     * @param dataType The {@link LogDataType} of the data. Ignored if {@code data} is a directory.
     * @param data The data to log. Can be a regular file, or a directory.
     * @param logger The {@link ITestLogger} with which to log the data.
     */
    void doLogReport(String dataName, LogDataType dataType, File data, ITestLogger logger)
            throws IOException {

        // If the data is a directory, compress it first
        InputStreamSource streamSource;
        if (data.isDirectory()) {
            ICompressionStrategy strategy = getCompressionStrategy();
            dataType = strategy.getLogDataType();
            streamSource = new FileInputStreamSource(strategy.compress(data), true);
        } else {
            streamSource = new FileInputStreamSource(data);
        }

        // Log the data
        logger.testLog(dataName, dataType, streamSource);
        streamSource.close();
    }

    /** Returns a new {@link ListInstrumentationParser}. Exposed for unit testing. */
    ListInstrumentationParser internalCreateListInstrumentationParser() {
        return new ListInstrumentationParser();
    }

    /** Returns the list of instrumentation targets to run. */
    Set<InstrumentationTarget> getInstrumentationTargets()
            throws DeviceNotAvailableException {

        Set<InstrumentationTarget> ret = new HashSet<>();

        // Run pm list instrumentation to get the available instrumentation targets
        ListInstrumentationParser parser = internalCreateListInstrumentationParser();
        getDevice().executeShellCommand("pm list instrumentation", parser);

        // If the package or runner filters are set, only include targets that match
        for (InstrumentationTarget target : parser.getInstrumentationTargets()) {
            List<String> packageFilter = getPackageFilter();
            List<String> runnerFilter = getRunnerFilter();
            if ((packageFilter.isEmpty() || packageFilter.contains(target.packageName)) &&
                    (runnerFilter.isEmpty() || runnerFilter.contains(target.runnerName))) {
                ret.add(target);
            }
        }

        return ret;
    }

    /** Checks whether the given {@link InstrumentationTarget} supports sharding. */
    boolean doesRunnerSupportSharding(InstrumentationTarget target)
            throws DeviceNotAvailableException {
        // Compare the number of tests for a given shard with the total number of tests
        return collectTests(target, 0, 2).size() < collectTests(target).size();
    }

    /** Returns all of the {@link TestDescription}s for the given target. */
    Collection<TestDescription> collectTests(InstrumentationTarget target)
            throws DeviceNotAvailableException {
        return collectTests(target, 0, 1);
    }

    /** Returns all of the {@link TestDescription}s for the given target and shard. */
    Collection<TestDescription> collectTests(
            InstrumentationTarget target, int shardIndex, int numShards)
            throws DeviceNotAvailableException {

        // Create a runner and enable test collection
        IRemoteAndroidTestRunner runner = createTestRunner(target, shardIndex, numShards);
        runner.setTestCollection(true);

        // Run the test and collect the test identifiers
        CollectingTestListener listener = new CollectingTestListener();
        getDevice().runInstrumentationTests(runner, listener);

        return listener.getCurrentRunResults().getCompletedTests();
    }

    /** Returns a new {@link IRemoteAndroidTestRunner} instance. Exposed for unit testing. */
    IRemoteAndroidTestRunner internalCreateTestRunner(String packageName, String runnerName) {
        return new RemoteAndroidTestRunner(packageName, runnerName, getDevice().getIDevice());
    }

    /** Returns a new {@link IRemoteAndroidTestRunner} instance for the given target and shard. */
    IRemoteAndroidTestRunner createTestRunner(InstrumentationTarget target,
            int shardIndex, int numShards) {

        // Get a new IRemoteAndroidTestRunner instance
        IRemoteAndroidTestRunner ret = internalCreateTestRunner(
                target.packageName, target.runnerName);

        // Add instrumentation arguments
        for (Map.Entry<String, String> argEntry : getInstrumentationArgs().entrySet()) {
            ret.addInstrumentationArg(argEntry.getKey(), argEntry.getValue());
        }

        // Add shard options if necessary
        if (numShards > 1) {
            ret.addInstrumentationArg("shardIndex", Integer.toString(shardIndex));
            ret.addInstrumentationArg("numShards", Integer.toString(numShards));
        }

        return ret;
    }

    /** Computes the number of shards that should be used when invoking the given target. */
    int getNumberOfShards(InstrumentationTarget target) throws DeviceNotAvailableException {
        double numTests = collectTests(target).size();
        return (int)Math.ceil(numTests / getMaxTestsPerChunk());
    }

    /**
     * Runs a single shard from the given {@code target}.
     *
     * @param target The instrumentation target to run.
     * @param shardIndex The index of the shard to run.
     * @param numShards The total number of shards for this target.
     * @param listener The {@link ITestInvocationListener} to be notified of tests results.
     * @return The results for the executed test run.
     */
    TestRunResult runTest(InstrumentationTarget target, int shardIndex, int numShards,
            ITestInvocationListener listener) throws DeviceNotAvailableException {
        return runTest(createTest(target, shardIndex, numShards), listener);
    }

    /**
     * Runs a single test method from the given {@code target}.
     *
     * @param target The instrumentation target to run.
     * @param identifier The individual test method to run.
     * @param listener The {@link ITestInvocationListener} to be notified of tests results.
     * @return The results for the executed test run.
     */
    TestRunResult runTest(
            InstrumentationTarget target,
            TestDescription identifier,
            ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        return runTest(createTest(target, identifier), listener);
    }

    /** Runs the given {@link InstrumentationTest} and returns the {@link TestRunResult}. */
    TestRunResult runTest(InstrumentationTest test, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        // Run the test, and return the run results
        CollectingTestListener results = new CollectingTestListener();
        test.run(new ResultForwarder(results, listener));
        return results.getCurrentRunResults();
    }

    /** Returns a new {@link InstrumentationTest}. Exposed for unit testing. */
    InstrumentationTest internalCreateTest() {
        return new InstrumentationTest();
    }

    /** Returns a new {@link InstrumentationTest} for the given target. */
    InstrumentationTest createTest(InstrumentationTarget target) {
        // Get a new InstrumentationTest instance
        InstrumentationTest ret = internalCreateTest();
        ret.setDevice(getDevice());
        ret.setPackageName(target.packageName);
        ret.setRunnerName(target.runnerName);

        // Disable rerun mode, we want to stop the tests as soon as we fail.
        ret.setRerunMode(false);

        // Add instrumentation arguments
        for (Map.Entry<String, String> argEntry : getInstrumentationArgs().entrySet()) {
            ret.addInstrumentationArg(argEntry.getKey(), argEntry.getValue());
        }
        ret.addInstrumentationArg("coverage", "true");

        return ret;
    }

    /** Returns a new {@link InstrumentationTest} for the identified test on the given target. */
    InstrumentationTest createTest(InstrumentationTarget target, TestDescription identifier) {
        // Get a new InstrumentationTest instance
        InstrumentationTest ret = createTest(target);

        // Set the specific test method to run
        ret.setClassName(identifier.getClassName());
        ret.setMethodName(identifier.getTestName());

        return ret;
    }

    /** Returns a new {@link InstrumentationTest} for a particular shard on the given target. */
    InstrumentationTest createTest(InstrumentationTarget target, int shardIndex, int numShards) {
        // Get a new InstrumentationTest instance
        InstrumentationTest ret = createTest(target);

        // Add shard options if necessary
        if (numShards > 1) {
            ret.addInstrumentationArg("shardIndex", Integer.toString(shardIndex));
            ret.addInstrumentationArg("numShards", Integer.toString(numShards));
        }

        return ret;
    }

    /** A {@link ResultForwarder} which collects coverage files. */
    public static class CoverageCollectingListener extends ResultForwarder
            implements AutoCloseable {

        private ITestDevice mDevice;
        private List<File> mCoverageFiles = new ArrayList<>();
        private File mCoverageDir;
        private String mCurrentRunName;

        public CoverageCollectingListener(ITestDevice device, ITestInvocationListener... listeners)
                throws IOException {
            super(listeners);

            mDevice = device;

            // Initialize a directory to store the coverage files
            mCoverageDir = FileUtil.createTempDir("execution_data");
        }

        /** Returns the list of collected coverage files. */
        public List<File> getCoverageFiles() {
            checkState(mCoverageDir != null, "This object is closed");
            return mCoverageFiles;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
            super.testLog(dataName, dataType, dataStream);
            checkState(mCoverageDir != null, "This object is closed");

            // We only care about coverage files
            if (LogDataType.COVERAGE.equals(dataType)) {
                // Save coverage data to a temporary location, and don't inform the listeners yet
                try {
                    File coverageFile =
                            FileUtil.createTempFile(dataName + "_", ".exec", mCoverageDir);
                    FileUtil.writeToFile(dataStream.createInputStream(), coverageFile);
                    mCoverageFiles.add(coverageFile);
                    CLog.d("Got coverage file: %s", coverageFile.getAbsolutePath());
                } catch (IOException e) {
                    CLog.e("Failed to save coverage file");
                    CLog.e(e);
                }
            }
        }

        /** {@inheritDoc} */
        @Override
        public void testRunStarted(String runName, int testCount) {
            super.testRunStarted(runName, testCount);
            mCurrentRunName = runName;
        }

        /** {@inheritDoc} */
        @Override
        public void testRunEnded(long elapsedTime, HashMap<String, Metric> runMetrics) {
            // Look for the coverage file path from the run metrics
            Metric coverageFilePathMetric =
                    runMetrics.get(CodeCoverageTest.COVERAGE_REMOTE_FILE_LABEL);

            if (coverageFilePathMetric != null) {
                String coverageFilePath =
                        coverageFilePathMetric.getMeasurements().getSingleString();
                if (!Strings.isNullOrEmpty(coverageFilePath)) {
                    CLog.d("Coverage file at %s", coverageFilePath);

                    // Try to pull the coverage measurements off of the device
                    File coverageFile = null;
                    try {
                        coverageFile = mDevice.pullFile(coverageFilePath);
                        if (coverageFile != null) {
                            try (FileInputStreamSource source =
                                    new FileInputStreamSource(coverageFile)) {
                                testLog(
                                        mCurrentRunName + "_runtime_coverage",
                                        LogDataType.COVERAGE,
                                        source);
                            }
                        } else {
                            CLog.w(
                                    "Failed to pull coverage file from device: %s",
                                    coverageFilePath);
                        }
                    } catch (DeviceNotAvailableException e) {
                        // Nothing we can do, so just log the error.
                        CLog.w(e);
                    } finally {
                        FileUtil.deleteFile(coverageFile);
                    }
                }
            }

            super.testRunEnded(elapsedTime, runMetrics);
        }

        /** {@inheritDoc} */
        @Override
        public void close() {
            FileUtil.recursiveDelete(mCoverageDir);
            mCoverageDir = null;
        }
    }
}
