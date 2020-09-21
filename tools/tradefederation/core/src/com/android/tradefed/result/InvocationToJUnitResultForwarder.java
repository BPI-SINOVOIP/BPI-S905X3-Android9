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

import com.android.ddmlib.Log;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import junit.framework.AssertionFailedError;
import junit.framework.Test;
import junit.framework.TestListener;
import junit.framework.TestResult;

import org.junit.AssumptionViolatedException;

import java.util.HashMap;

/**
 * A class that listens to {@link ITestInvocationListener} events and forwards them to a
 * {@link junit.framework.TestListener}.
 * <p/>
 */
 public class InvocationToJUnitResultForwarder implements ITestInvocationListener {

    private static final String LOG_TAG = "InvocationToJUnitResultForwarder";
    private TestListener mJUnitListener;

    public InvocationToJUnitResultForwarder(TestListener junitListener) {
        mJUnitListener = junitListener;
    }

    protected TestListener getJUnitListener() {
        return mJUnitListener;
    }

    /** {@inheritDoc} */
    @Override
    public void testEnded(TestDescription test, HashMap<String, Metric> testMetrics) {
        mJUnitListener.endTest(new TestIdentifierResult(test));
    }

    /** {@inheritDoc} */
    @Override
    public void testFailed(TestDescription testId, String trace) {
        Test test = new TestIdentifierResult(testId);
        // TODO: is it accurate to represent the trace as AssertionFailedError?
        mJUnitListener.addFailure(test, new AssertionFailedError(trace));
    }

    @Override
    public void testAssumptionFailure(TestDescription testId, String trace) {
        Test test = new TestIdentifierResult(testId);
        AssumptionViolatedException throwable = new AssumptionViolatedException(trace);
        mJUnitListener.addError(test, throwable);
    }

    /** {@inheritDoc} */
    @Override
    public void testRunEnded(long elapsedTime, HashMap<String, Metric> runMetrics) {
       // TODO: no run ended method on TestListener - would be good to propagate the elaspedTime
       // info up
       Log.i(LOG_TAG, String.format("run ended %d ms", elapsedTime));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunFailed(String errorMessage) {
        // TODO: no run failed method on TestListener - would be good to propagate this up
        Log.e(LOG_TAG, String.format("run failed: %s", errorMessage));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStarted(String runName, int testCount) {
        // TODO: no run started method on TestResult - would be good to propagate this up
        Log.i(LOG_TAG, String.format("run %s started: %d tests", runName, testCount));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStopped(long elapsedTime) {
        Log.i(LOG_TAG, String.format("run stopped: %d ms", elapsedTime));
    }

    /** {@inheritDoc} */
    @Override
    public void testStarted(TestDescription test) {
        Log.d(LOG_TAG, test.toString());
        mJUnitListener.startTest(new TestIdentifierResult(test));
    }

    /**
     * A class that converts a {@link TestDescription} to a JUnit {@link Test}
     *
     * <p>TODO: The JUnit {@link TestListener} seems to assume a descriptive interface of some sort
     * for Test, that is not in its defined methods. Assume for now that its toString()
     */
    static class TestIdentifierResult implements Test {
        private final TestDescription mTestId;

        private TestIdentifierResult(TestDescription testId) {
            mTestId = testId;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public int countTestCases() {
            return 1;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void run(TestResult result) {
            // this class for reporting purposes only, ignore
            throw new UnsupportedOperationException();
        }

        /**
         * Specialize this to base equality on TestIdentifier.
         */
        @Override
        public boolean equals(Object other) {
            return mTestId.equals(other);
        }

        /**
         * Specialize this to base hashCode on TestIdentifier.
         */
        @Override
        public int hashCode() {
            return mTestId.hashCode();
        }

        /**
         * Return a user-friendly descriptive string.
         */
        @Override
        public String toString() {
            // TODO: use ':' or '#' as separator? The eternal debate rages on!
            return String.format("%s:%s", mTestId.getClassName(), mTestId.getTestName());
        }
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

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationStarted(IInvocationContext context) {
        // ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLog(String dataName, LogDataType logData, InputStreamSource dataStream) {
        // ignore
    }

    @Override
    public void testIgnored(TestDescription test) {
        // ignore
    }
}
