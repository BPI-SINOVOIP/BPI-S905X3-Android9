/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.tradefed.result;

import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import com.google.common.annotations.VisibleForTesting;

import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * A {@link ITestInvocationListener} that will collect all test results.
 *
 * <p>Although the data structures used in this object are thread-safe, the {@link
 * ITestInvocationListener} callbacks must be called in the correct order.
 */
public class CollectingTestListener implements ITestInvocationListener, ILogSaverListener {

    // Stores the test results
    // Uses a synchronized map to make thread safe.
    // Uses a LinkedHashmap to have predictable iteration order
    private Map<String, TestRunResult> mRunResultsMap =
        Collections.synchronizedMap(new LinkedHashMap<String, TestRunResult>());
    private Map<TestRunResult, IInvocationContext> mModuleContextMap =
            Collections.synchronizedMap(new LinkedHashMap<TestRunResult, IInvocationContext>());
    private TestRunResult mCurrentResults =  new TestRunResult();
    private IInvocationContext mCurrentModuleContext = null;

    /** represents sums of tests in each TestStatus state for all runs.
     * Indexed by TestStatus.ordinal() */
    private int[] mStatusCounts = new int[TestStatus.values().length];
    /** tracks if mStatusCounts is accurate, or if it needs to be recalculated */
    private boolean mIsCountDirty = true;

    @Option(name = "aggregate-metrics", description =
        "attempt to add test metrics values for test runs with the same name." )
    private boolean mIsAggregateMetrics = false;

    private IBuildInfo mBuildInfo;
    private IInvocationContext mContext;

    /** Toggle the 'aggregate metrics' option */
    protected void setIsAggregrateMetrics(boolean aggregate) {
        mIsAggregateMetrics = aggregate;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationStarted(IInvocationContext context) {
        mContext = context;
        if (context != null) {
            mBuildInfo = context.getBuildInfos().get(0);
        }
    }

    /**
     * Return the primary build info that was reported via {@link
     * #invocationStarted(IInvocationContext)}. Primary build is the build returned by the first
     * build provider of the running configuration. Returns null if there is no context (no build to
     * test case).
     */
    public IBuildInfo getPrimaryBuildInfo() {
        if (mContext == null) {
            return null;
        } else {
            return mContext.getBuildInfos().get(0);
        }
    }

    /**
     * Return the invocation context that was reported via
     * {@link #invocationStarted(IInvocationContext)}
     */
    public IInvocationContext getInvocationContext() {
        return mContext;
    }

    /**
     * Returns the build info.
     *
     * @deprecated rely on the {@link IBuildInfo} from {@link #getInvocationContext()}.
     */
    @Deprecated
    public IBuildInfo getBuildInfo() {
        return mBuildInfo;
    }

    /**
     * Set the build info. Should only be used for testing.
     *
     * @deprecated Not necessary for testing anymore.
     */
    @VisibleForTesting
    @Deprecated
    public void setBuildInfo(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    @Override
    public void testModuleStarted(IInvocationContext moduleContext) {
        mCurrentModuleContext = moduleContext;
    }

    @Override
    public void testModuleEnded() {
        mCurrentModuleContext = null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStarted(String name, int numTests) {
        if (mRunResultsMap.containsKey(name)) {
            // rerun of previous run. Add test results to it
            mCurrentResults = mRunResultsMap.get(name);
        } else {
            // new run
            mCurrentResults = new TestRunResult();
            mCurrentResults.setAggregateMetrics(mIsAggregateMetrics);

            mRunResultsMap.put(name, mCurrentResults);
            // track the module context associated with the results.
            if (mCurrentModuleContext != null) {
                mModuleContextMap.put(mCurrentResults, mCurrentModuleContext);
            }
        }
        mCurrentResults.testRunStarted(name, numTests);
        mIsCountDirty = true;
    }

    /** {@inheritDoc} */
    @Override
    public void testStarted(TestDescription test) {
        testStarted(test, System.currentTimeMillis());
    }

    /** {@inheritDoc} */
    @Override
    public void testStarted(TestDescription test, long startTime) {
        mIsCountDirty = true;
        mCurrentResults.testStarted(test, startTime);
    }

    /** {@inheritDoc} */
    @Override
    public void testEnded(TestDescription test, HashMap<String, Metric> testMetrics) {
        testEnded(test, System.currentTimeMillis(), testMetrics);
    }

    /** {@inheritDoc} */
    @Override
    public void testEnded(TestDescription test, long endTime, HashMap<String, Metric> testMetrics) {
        mIsCountDirty = true;
        mCurrentResults.testEnded(test, endTime, testMetrics);
    }

    /** {@inheritDoc} */
    @Override
    public void testFailed(TestDescription test, String trace) {
        mIsCountDirty = true;
        mCurrentResults.testFailed(test, trace);
    }

    @Override
    public void testAssumptionFailure(TestDescription test, String trace) {
        mIsCountDirty = true;
        mCurrentResults.testAssumptionFailure(test, trace);

    }

    @Override
    public void testIgnored(TestDescription test) {
        mIsCountDirty = true;
        mCurrentResults.testIgnored(test);
    }

    /** {@inheritDoc} */
    @Override
    public void testRunEnded(long elapsedTime, HashMap<String, Metric> runMetrics) {
        mIsCountDirty = true;
        mCurrentResults.testRunEnded(elapsedTime, runMetrics);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunFailed(String errorMessage) {
        mIsCountDirty = true;
        mCurrentResults.testRunFailed(errorMessage);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStopped(long elapsedTime) {
        mIsCountDirty = true;
        mCurrentResults.testRunStopped(elapsedTime);
    }

    /**
     * Gets the results for the current test run.
     * <p/>
     * Note the results may not be complete. It is recommended to test the value of {@link
     * TestRunResult#isRunComplete()} and/or (@link TestRunResult#isRunFailure()} as appropriate
     * before processing the results.
     *
     * @return the {@link TestRunResult} representing data collected during last test run
     */
    public TestRunResult getCurrentRunResults() {
        return mCurrentResults;
    }

    /**
     * Gets the results for all test runs.
     */
    public Collection<TestRunResult> getRunResults() {
        return mRunResultsMap.values();
    }

    /**
     * Returns the {@link IInvocationContext} of the module associated with the results or null if
     * it was not associated with any module.
     */
    public IInvocationContext getModuleContextForRunResult(TestRunResult res) {
        return mModuleContextMap.get(res);
    }

    /** Returns True if the result map already has an entry for the run name. */
    public boolean hasResultFor(String runName) {
        return mRunResultsMap.containsKey(runName);
    }

    /** Gets the total number of complete tests for all runs. */
    public int getNumTotalTests() {
        int total = 0;
        // force test count
        getNumTestsInState(TestStatus.PASSED);
        for (TestStatus s : TestStatus.values()) {
            total += mStatusCounts[s.ordinal()];
        }
        return total;
    }

    /**
     * Gets the number of tests in given state for this run.
     */
    public int getNumTestsInState(TestStatus status) {
        if (mIsCountDirty) {
            for (TestStatus s : TestStatus.values()) {
                mStatusCounts[s.ordinal()] = 0;
                for (TestRunResult result : mRunResultsMap.values()) {
                    mStatusCounts[s.ordinal()] += result.getNumTestsInState(s);
                }
            }
            mIsCountDirty = false;
        }
        return mStatusCounts[status.ordinal()];
    }

    /**
     * @return true if invocation had any failed or assumption failed tests.
     */
    public boolean hasFailedTests() {
        return getNumAllFailedTests() > 0;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationEnded(long elapsedTime) {
        // ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationFailed(Throwable cause) {
        // ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public TestSummary getSummary() {
        // ignore
        return null;
    }

    /** {@inheritDoc} */
    @Override
    public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
        // ignore, logAssociation is implemented.
    }

    /** {@inheritDoc} */
    @Override
    public void testLogSaved(
            String dataName, LogDataType dataType, InputStreamSource dataStream, LogFile logFile) {
        // ignore, logAssociation is used to save the files
    }

    /** {@inheritDoc} */
    @Override
    public void setLogSaver(ILogSaver logSaver) {
        // CollectingTestListener does not need the logSaver
    }

    /** {@inheritDoc} */
    @Override
    public void logAssociation(String dataName, LogFile logFile) {
        mCurrentResults.testLogSaved(dataName, logFile);
    }

    /**
     * Return total number of tests in a failure state (only failed, assumption failures do not
     * count toward it).
     */
    public int getNumAllFailedTests() {
        return getNumTestsInState(TestStatus.FAILURE);
    }

    /**
     * Return total number of test runs in a failure state
     */
    public int getNumAllFailedTestRuns() {
        int count = 0;
        for (Map.Entry<String, TestRunResult> e : mRunResultsMap.entrySet()) {
            if (e.getValue().isRunFailure()) {
                count++;
            }
        }
        return count;
    }
}
