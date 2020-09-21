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
package com.android.tradefed.invoker;

import com.android.ddmlib.Log.LogLevel;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.ILogSaverListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogFile;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.util.TimeUtil;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

/**
 * A {@link ITestInvocationListener} that collects results from a invocation shard (aka an
 * invocation split to run on multiple resources in parallel), and forwards them to another
 * listener.
 */
public class ShardListener extends CollectingTestListener {

    private ITestInvocationListener mMasterListener;

    /**
     * Create a {@link ShardListener}.
     *
     * @param master the {@link ITestInvocationListener} the results should be forwarded. To prevent
     *     collisions with other {@link ShardListener}s, this object will synchronize on
     *     <var>master</var> when forwarding results. And results will only be sent once the
     *     invocation shard completes.
     */
    public ShardListener(ITestInvocationListener master) {
        mMasterListener = master;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationStarted(IInvocationContext context) {
        super.invocationStarted(context);
        synchronized (mMasterListener) {
            mMasterListener.invocationStarted(context);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationFailed(Throwable cause) {
        super.invocationFailed(cause);
        synchronized (mMasterListener) {
            mMasterListener.invocationFailed(cause);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
        // forward testLog results immediately, since result reporters might take action on it.
        synchronized (mMasterListener) {
            if (mMasterListener instanceof ShardMasterResultForwarder) {
                // If the listener is a log saver, we should simply forward the testLog not save
                // again.
                ((ShardMasterResultForwarder) mMasterListener)
                        .testLogForward(dataName, dataType, dataStream);
            } else {
                mMasterListener.testLog(dataName, dataType, dataStream);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void testLogSaved(
            String dataName, LogDataType dataType, InputStreamSource dataStream, LogFile logFile) {
        super.testLogSaved(dataName, dataType, dataStream, logFile);
        // Forward the testLogSaved callback.
        synchronized (mMasterListener) {
            if (mMasterListener instanceof ILogSaverListener) {
                ((ILogSaverListener) mMasterListener)
                        .testLogSaved(dataName, dataType, dataStream, logFile);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void testRunEnded(long elapsedTime, HashMap<String, Metric> runMetrics) {
        super.testRunEnded(elapsedTime, runMetrics);
        CLog.logAndDisplay(LogLevel.INFO, "Sharded test completed: %s",
                getCurrentRunResults().getName());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunFailed(String failureMessage) {
        super.testRunFailed(failureMessage);
        CLog.logAndDisplay(LogLevel.ERROR, "FAILED: %s failed with message: %s",
                getCurrentRunResults().getName(), failureMessage);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationEnded(long elapsedTime) {
        super.invocationEnded(elapsedTime);
        synchronized (mMasterListener) {
            logShardContent(getRunResults());
            IInvocationContext moduleContext = null;
            for (TestRunResult runResult : getRunResults()) {
                // Stop or start the module
                if (moduleContext != null
                        && !getModuleContextForRunResult(runResult).equals(moduleContext)) {
                    mMasterListener.testModuleEnded();
                    moduleContext = null;
                }
                if (moduleContext == null && getModuleContextForRunResult(runResult) != null) {
                    moduleContext = getModuleContextForRunResult(runResult);
                    mMasterListener.testModuleStarted(moduleContext);
                }

                mMasterListener.testRunStarted(runResult.getName(), runResult.getNumTests());
                forwardTestResults(runResult.getTestResults());
                if (runResult.isRunFailure()) {
                    mMasterListener.testRunFailed(runResult.getRunFailureMessage());
                }

                // Provide a strong association of the run to its logs.
                forwardLogAssociation(runResult.getRunLoggedFiles(), mMasterListener);

                mMasterListener.testRunEnded(
                        runResult.getElapsedTime(), runResult.getRunProtoMetrics());
            }
            // Close the last module
            if (moduleContext != null) {
                mMasterListener.testModuleEnded();
                moduleContext = null;
            }
            mMasterListener.invocationEnded(elapsedTime);
        }
    }

    private void forwardTestResults(Map<TestDescription, TestResult> testResults) {
        for (Map.Entry<TestDescription, TestResult> testEntry : testResults.entrySet()) {
            mMasterListener.testStarted(testEntry.getKey(), testEntry.getValue().getStartTime());
            switch (testEntry.getValue().getStatus()) {
                case FAILURE:
                    mMasterListener.testFailed(testEntry.getKey(),
                            testEntry.getValue().getStackTrace());
                    break;
                case ASSUMPTION_FAILURE:
                    mMasterListener.testAssumptionFailure(testEntry.getKey(),
                            testEntry.getValue().getStackTrace());
                    break;
                case IGNORED:
                    mMasterListener.testIgnored(testEntry.getKey());
                    break;
                default:
                    break;
            }
            // Provide a strong association of the test to its logs.
            forwardLogAssociation(testEntry.getValue().getLoggedFiles(), mMasterListener);

            if (!testEntry.getValue().getStatus().equals(TestStatus.INCOMPLETE)) {
                mMasterListener.testEnded(
                        testEntry.getKey(),
                        testEntry.getValue().getEndTime(),
                        testEntry.getValue().getProtoMetrics());
            }
        }
    }

    /** Forward to the listener the logAssociated callback on the files. */
    private void forwardLogAssociation(
            Map<String, LogFile> loggedFiles, ITestInvocationListener listener) {
        for (Entry<String, LogFile> logFile : loggedFiles.entrySet()) {
            if (listener instanceof ILogSaverListener) {
                ((ILogSaverListener) listener).logAssociation(logFile.getKey(), logFile.getValue());
            }
        }
    }

    /** Log the content of the shard for easier debugging. */
    private void logShardContent(Collection<TestRunResult> listResults) {
        CLog.d("=================================================");
        CLog.d(
                "========== Shard Primary Device %s ==========",
                getInvocationContext().getDevices().get(0).getSerialNumber());
        for (TestRunResult runRes : listResults) {
            CLog.d(
                    "\tRan '%s' in %s",
                    runRes.getName(), TimeUtil.formatElapsedTime(runRes.getElapsedTime()));
        }
        CLog.d("=================================================");
    }
}
