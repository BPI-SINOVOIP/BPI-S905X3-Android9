/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tradefed.testtype.suite;

import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;

import com.android.tradefed.device.metric.IMetricCollector;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogSaverResultForwarder;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.ITestFilterReceiver;

import com.google.common.annotations.VisibleForTesting;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * A wrapper class works on the {@link IRemoteTest} to granulate the IRemoteTest in testcase level.
 * An IRemoteTest can contain multiple testcases. Previously, these testcases are treated as a
 * whole: When IRemoteTest runs, all testcases will run. Some IRemoteTest (The ones that implements
 * ITestFilterReceiver) can accept a whitelist of testcases and only run those testcases. This class
 * takes advantage of the existing feature and provides a more flexible way to run test suite.
 *
 * <ul>
 *   <li> Single testcase can be retried multiple times (within the same IRemoteTest run) to reduce
 *       the non-test-error failure rates.
 *   <li> The retried testcases are dynamically collected from previous run failures.
 * </ul>
 *
 * <p>Note:
 *
 * <ul>
 *   <li> The prerequisite to run a subset of test cases is that the test type should implement the
 *       interface {@link ITestFilterReceiver}.
 *   <li> X is customized max retry number.
 * </ul>
 */
public class GranularRetriableTestWrapper implements IRemoteTest {

    private boolean mIsGranulatedTestCaseRetriable;
    private IRemoteTest mTest;
    private boolean mSkipTestCases;
    private List<IMetricCollector> mRunMetricCollectors;
    private ITestInvocationListener mMainListener;
    private TestFailureListener mFailureListener;
    private IInvocationContext mModuleInvocationContext;
    private IConfiguration mModuleConfiguration;
    private List<TestRunResult> mTestRunResultCollector;
    private List<ModuleListener> mModuleListenerCollector;
    private List<ITestInvocationListener> mModuleLevelListeners;
    private ILogSaver mLogSaver;
    private String mModuleId;
    private boolean mIsMetricCollectorInitialized;
    private int mMaxRunLimit;

    public GranularRetriableTestWrapper(
            IRemoteTest test,
            TestFailureListener failureListener,
            List<ITestInvocationListener> moduleLevelListeners,
            int maxRunLimit) {
        mTest = test;
        mTestRunResultCollector = new ArrayList<TestRunResult>();
        mFailureListener = failureListener;
        mModuleListenerCollector = new ArrayList<ModuleListener>();
        mIsMetricCollectorInitialized = false;
        mModuleLevelListeners = moduleLevelListeners;
        mMaxRunLimit = maxRunLimit;
        // TODO(b/77548917): Right now we only support ITestFilterReciever. We should expect to
        // support ITestFile*Filter*Receiver in the future.
        if (test instanceof ITestFilterReceiver) {
            mIsGranulatedTestCaseRetriable = true;
        } else {
            mIsGranulatedTestCaseRetriable = false;
        }
    }

    /**
     * Set the {@link ModuleDefinition} name as a {@link GranularRetriableTestWrapper} attribute.
     *
     * @param moduleId the name of the moduleDefinition.
     */
    public void setModuleId(String moduleId) {
        mModuleId = moduleId;
    }

    /**
     * Set the {@link ModuleDefinition} RunStrategy as a {@link GranularRetriableTestWrapper}
     * attribute.
     *
     * @param skipTestCases whether the testcases should be skipped.
     */
    public void setMarkTestsSkipped(boolean skipTestCases) {
        mSkipTestCases = skipTestCases;
    }

    /**
     * Set the {@link ModuleDefinition}'s runMetricCollector as a {@link
     * GranularRetriableTestWrapper} attribute.
     *
     * @param runMetricCollectors A list of MetricCollector for the module.
     */
    public void setMetricCollectors(List<IMetricCollector> runMetricCollectors) {
        mRunMetricCollectors = runMetricCollectors;
    }

    /**
     * Set the {@link ModuleDefinition}'s ModuleConfig as a {@link GranularRetriableTestWrapper}
     * attribute.
     *
     * @param moduleConfiguration Provide the module metrics.
     */
    public void setModuleConfig(IConfiguration moduleConfiguration) {
        mModuleConfiguration = moduleConfiguration;
    }

    /**
     * Set the {@link IInvocationContext} as a {@link GranularRetriableTestWrapper} attribute.
     *
     * @param moduleInvocationContext The wrapper uses the InvocationContext to initialize the
     *     MetricCollector when necessary.
     */
    public void setInvocationContext(IInvocationContext moduleInvocationContext) {
        mModuleInvocationContext = moduleInvocationContext;
    }

    /**
     * Set the Module's {@link ILogSaver} as a {@link GranularRetriableTestWrapper} attribute.
     *
     * @param logSaver The listeners for each test run should save the logs.
     */
    public void setLogSaver(ILogSaver logSaver) {
        mLogSaver = logSaver;
    }

    @VisibleForTesting
    ModuleListener createModuleListener() {
        return new ModuleListener(mMainListener);
    }

    /**
     * Initialize a new {@link ModuleListener} for each test run.
     *
     * @return a {@link ITestInvocationListener} listener which contains the new {@link
     *     ModuleListener}, the main {@link ITestInvocationListener} and main {@link
     *     TestFailureListener}, and wrapped by RunMetricsCollector and Module MetricCollector (if
     *     not initialized).
     */
    private ITestInvocationListener prepareRunListener() {
        ModuleListener moduleListener = createModuleListener();
        mModuleListenerCollector.add(moduleListener);
        moduleListener.setMarkTestsSkipped(mSkipTestCases);
        List<ITestInvocationListener> currentTestListener = new ArrayList<>();
        // Add all the module level listeners, including TestFailureListener
        if (mModuleLevelListeners != null) {
            currentTestListener.addAll(mModuleLevelListeners);
        }
        currentTestListener.add(moduleListener);

        ITestInvocationListener runListener =
                new LogSaverResultForwarder(mLogSaver, currentTestListener);
        if (mFailureListener != null) {
            mFailureListener.setLogger(runListener);
            currentTestListener.add(mFailureListener);
        }

        // TODO: For RunMetricCollector and moduleMetricCollector, we only gather the
        // metrics in the first run. This part can be improved if we want to gather metrics for
        // every run.
        if (!mIsMetricCollectorInitialized) {
            if (mRunMetricCollectors != null) {
                // Module only init the collectors here to avoid triggering the collectors when
                // replaying the cached events at the end. This ensure metrics are capture at
                // the proper time in the invocation.
                for (IMetricCollector collector : mRunMetricCollectors) {
                    runListener = collector.init(mModuleInvocationContext, runListener);
                }
            }
            // The module collectors itself are added: this list will be very limited.
            for (IMetricCollector collector : mModuleConfiguration.getMetricCollectors()) {
                runListener = collector.init(mModuleInvocationContext, runListener);
            }
            mIsMetricCollectorInitialized = true;
        }
        return runListener;
    }

    /**
     * Schedule a series of {@link IRemoteTest} "run". TODO: Customize the retry strategy; Each run
     * is granularized to a subset of the whole testcases.
     *
     * @param listener The ResultForwarder listener which contains a new moduleListener for each
     *     run.
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        mMainListener = listener;
        int count = 0;
        while (count < mMaxRunLimit) {
            intraModuleRun();
            count += 1;
            ModuleListener previousTestRunListener =
                    mModuleListenerCollector.get(mModuleListenerCollector.size() - 1);
            if (!previousTestRunListener.hasFailedTests()) {
                CLog.d("The test has no failed testcases. No need to retry.");
                break;
            }
            if (count == mMaxRunLimit) {
                CLog.d(
                        "The test has reached its max number of run attempt: %d time(s)",
                        mMaxRunLimit);
                break;
            }
            if (mIsGranulatedTestCaseRetriable) {
                Set<TestDescription> failedTests =
                        previousTestRunListener.getCurrentRunResults().getFailedTests();
                addRetriedTestsToIncludeFilters(failedTests);
            } else {
                // If the IRemoteTest can't support running a single testcase, we retry the whole
                // testcase list.
                CLog.d(
                        "The test is not supported to run testcases in intra-module level. Trying "
                                + "to run the whole test again...");
            }
        }
    }

    /**
     * Update the arguments of {@link IRemoteTest} to only run failed tests. This arguments/logic is
     * implemented differently for each IRemoteTest testtype in the overridden
     * ITestFilterReceiver.addIncludeFilter method.
     *
     * @param testDescriptions The set of failed testDescriptions to retry.
     */
    private void addRetriedTestsToIncludeFilters(Set<TestDescription> testDescriptions) {
        // TODO(b/77548917): Right now we only support ITestFilterReciever. We should expect to
        // support ITestFile*Filter*Receiver in the future.
        if (mTest instanceof ITestFilterReceiver) {
            for (TestDescription testCase : testDescriptions) {
                String filter = testCase.toString();
                ((ITestFilterReceiver) mTest).addIncludeFilter(filter);
            }
        }
    }

    /**
     * The workflow for each individual {@link IRemoteTest} run. TODO: When this function is called,
     * the IRemoteTest should already has the subset of testcases identified.
     */
    @VisibleForTesting
    final void intraModuleRun() throws DeviceNotAvailableException, DeviceUnresponsiveException {
        ITestInvocationListener runListener = prepareRunListener();
        try {
            mTest.run(runListener);
        } catch (RuntimeException re) {
            CLog.e("Module '%s' - test '%s' threw exception:", mModuleId, mTest.getClass());
            CLog.e(re);
            CLog.e("Proceeding to the next test.");
            runListener.testRunFailed(re.getMessage());
        } catch (DeviceUnresponsiveException due) {
            // being able to catch a DeviceUnresponsiveException here implies that recovery was
            // successful, and test execution should proceed to next module.
            CLog.w(
                    "Ignored DeviceUnresponsiveException because recovery was "
                            + "successful, proceeding with next module. Stack trace:");
            CLog.w(due);
            CLog.w("Proceeding to the next test.");
            runListener.testRunFailed(due.getMessage());
        } catch (DeviceNotAvailableException dnae) {
            throw dnae;
        } finally {
            ModuleListener currentModuleListener =
                    mModuleListenerCollector.get(mModuleListenerCollector.size() - 1);
            mTestRunResultCollector.addAll(currentModuleListener.getRunResults());
        }
    }

    /** Get the merged TestRunResults from each {@link IRemoteTest} run. */
    public TestRunResult getFinalTestRunResult() {
        return TestRunResult.merge(mTestRunResultCollector);
    }

    @VisibleForTesting
    List<TestRunResult> getTestRunResultCollector() {
        return mTestRunResultCollector;
    }

    /** Check if any testRunResult has ever failed. */
    public boolean hasFailed() {
        for (ModuleListener listener : mModuleListenerCollector) {
            if (listener.hasFailed()) {
                return true;
            }
        }
        return false;
    }

    /**
     * Calculate the number of testcases in the {@link IRemoteTest}. This value distincts the same
     * testcases that are rescheduled multiple times.
     */
    public int getNumIndividualTests() {
        if (mModuleListenerCollector.isEmpty()) {
            return 0;
        }
        return mModuleListenerCollector.get(0).getNumTotalTests();
    }
}
