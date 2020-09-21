/*
 * Copyright (C) 2017 The Android Open Source Project
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

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.ILogSaverListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogFile;
import com.android.tradefed.result.LogSaverResultForwarder;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.testtype.IRemoteTest;

import java.util.HashMap;

/**
 * Listener attached to each {@link IRemoteTest} of each module in order to collect the list of
 * results.
 */
public class ModuleListener extends CollectingTestListener {

    private int mExpectedTestCount = 0;
    private boolean mSkip = false;
    private boolean mTestFailed = false;
    private int mTestsRan = 1;
    private ITestInvocationListener mMainListener;
    private boolean mHasFailed = false;

    /** Constructor. */
    public ModuleListener(ITestInvocationListener listener) {
        mMainListener = listener;
        setIsAggregrateMetrics(true);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStarted(String name, int numTests) {
        if (!hasResultFor(name)) {
            // No results for it yet, brand new set of tests, we expect them all.
            mExpectedTestCount += numTests;
        } else {
            TestRunResult currentResult = getCurrentRunResults();
            // We have results but the run wasn't complete.
            if (!currentResult.isRunComplete()) {
                mExpectedTestCount += numTests;
            }
        }
        super.testRunStarted(name, numTests);
    }

    /** {@inheritDoc} */
    @Override
    public void testRunFailed(String errorMessage) {
        mHasFailed = true;
        CLog.d("ModuleListener.testRunFailed(%s)", errorMessage);
        super.testRunFailed(errorMessage);
    }

    /** Returns whether or not the listener session has failed. */
    public boolean hasFailed() {
        return mHasFailed;
    }

    /** {@inheritDoc} */
    @Override
    public void testStarted(TestDescription test, long startTime) {
        CLog.d("ModuleListener.testStarted(%s)", test.toString());
        mTestFailed = false;
        super.testStarted(test, startTime);
        if (mSkip) {
            super.testIgnored(test);
        }
    }

    /** Helper to log the test passed if it didn't fail. */
    private void logTestPassed(String testName) {
        if (!mTestFailed) {
            CLog.logAndDisplay(
                    LogLevel.INFO, "[%d/%d] %s pass", mTestsRan, mExpectedTestCount, testName);
        }
        mTestsRan++;
    }
    
    /** {@inheritDoc} */
    @Override
    public void testEnded(TestDescription test, HashMap<String, Metric> testMetrics) {
        testEnded(test, System.currentTimeMillis(), testMetrics);
    }

    /** {@inheritDoc} */
    @Override
    public void testEnded(TestDescription test, long endTime, HashMap<String, Metric> testMetrics) {
        logTestPassed(test.toString());
        super.testEnded(test, endTime, testMetrics);
    }

    /** {@inheritDoc} */
    @Override
    public void testFailed(TestDescription test, String trace) {
        if (mSkip) {
            return;
        }
        CLog.logAndDisplay(
                LogLevel.INFO,
                "[%d/%d] %s fail:\n%s",
                mTestsRan,
                mExpectedTestCount,
                test.toString(),
                trace);
        mTestFailed = true;
        super.testFailed(test, trace);
    }

    /** {@inheritDoc} */
    @Override
    public int getNumTotalTests() {
        return mExpectedTestCount;
    }

    /** Whether or not to mark all the test cases skipped. */
    public void setMarkTestsSkipped(boolean skip) {
        mSkip = skip;
    }

    /** {@inheritDoc} */
    @Override
    public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
        if (mMainListener instanceof LogSaverResultForwarder) {
            // If the listener is a log saver, we should simply forward the testLog not save again.
            ((LogSaverResultForwarder) mMainListener)
                    .testLogForward(dataName, dataType, dataStream);
        } else {
            super.testLog(dataName, dataType, dataStream);
        }
    }

    /** {@inheritDoc} */
    @Override
    public void testLogSaved(
            String dataName, LogDataType dataType, InputStreamSource dataStream, LogFile logFile) {
        // Forward to CollectingTestListener to store the logs
        super.testLogSaved(dataName, dataType, dataStream, logFile);
        // Forward to the main listener so logs are properly reported to the end result_reporters.
        if (mMainListener instanceof ILogSaverListener) {
            ((ILogSaverListener) mMainListener)
                    .testLogSaved(dataName, dataType, dataStream, logFile);
        }
    }
}
