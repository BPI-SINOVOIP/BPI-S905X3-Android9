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

package com.android.tradefed.result;

import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import java.util.HashMap;

/**
 * A proxy listener to translate test method, class, and package names as results are reported.
 */
public abstract class NameMangleListener implements ITestInvocationListener {
    private final ITestInvocationListener mListener;

    public NameMangleListener(ITestInvocationListener listener) {
        if (listener == null) throw new NullPointerException();
        mListener = listener;
    }

    /**
     * This method is run on all {@link TestDescription}s that are passed to the {@link
     * #testStarted(TestDescription)}, {@link #testFailed(TestDescription, String)}, and {@link
     * #testEnded(TestDescription, HashMap)} callbacks. The method should return a
     * possibly-different {@link TestDescription} that will be passed to the downstream {@link
     * ITestInvocationListener} that was specified during construction.
     *
     * <p>The implementation should be careful to not modify the original {@link TestDescription}.
     *
     * <p>The default implementation passes the incoming identifier through unmodified.
     */
    protected TestDescription mangleTestId(TestDescription test) {
        return test;
    }

    /**
     * This method is run on all test run names that are passed to the
     * {@link #testRunStarted(String, int)} callback.  The method should return a possibly-different
     * test run name that will be passed to the downstream {@link ITestInvocationListener} that was
     * specified during construction.
     * <p />
     * The implementation should be careful to not modify the original run name.
     * <p />
     * The default implementation passes the incoming test run name through unmodified.
     */
    protected String mangleTestRunName(String name) {
        return name;
    }

    // ITestLifeCycleReceiver methods

    /** {@inheritDoc} */
    @Override
    public void testEnded(TestDescription test, HashMap<String, Metric> testMetrics) {
        final TestDescription mangledTestId = mangleTestId(test);
        mListener.testEnded(mangledTestId, testMetrics);
    }

    /** {@inheritDoc} */
    @Override
    public void testFailed(TestDescription test, String trace) {
        final TestDescription mangledTestId = mangleTestId(test);
        mListener.testFailed(mangledTestId, trace);
    }

    /** {@inheritDoc} */
    @Override
    public void testAssumptionFailure(TestDescription test, String trace) {
        final TestDescription mangledTestId = mangleTestId(test);
        mListener.testAssumptionFailure(mangledTestId, trace);
    }

    /** {@inheritDoc} */
    @Override
    public void testIgnored(TestDescription test) {
        final TestDescription mangledTestId = mangleTestId(test);
        mListener.testIgnored(mangledTestId);
    }

    /** {@inheritDoc} */
    @Override
    public void testRunEnded(long elapsedTime, HashMap<String, Metric> runMetrics) {
        mListener.testRunEnded(elapsedTime, runMetrics);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunFailed(String errorMessage) {
        mListener.testRunFailed(errorMessage);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStarted(String runName, int testCount) {
        final String mangledName = mangleTestRunName(runName);
        mListener.testRunStarted(mangledName, testCount);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStopped(long elapsedTime) {
        mListener.testRunStopped(elapsedTime);
    }

    /** {@inheritDoc} */
    @Override
    public void testStarted(TestDescription test) {
        final TestDescription mangledTestId = mangleTestId(test);
        mListener.testStarted(mangledTestId);
    }


    // ITestInvocationListener methods
    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationStarted(IInvocationContext context) {
        mListener.invocationStarted(context);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
        mListener.testLog(dataName, dataType, dataStream);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationEnded(long elapsedTime) {
        mListener.invocationEnded(elapsedTime);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationFailed(Throwable cause) {
        mListener.invocationFailed(cause);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public TestSummary getSummary() {
        return mListener.getSummary();
    }
}

