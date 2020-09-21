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
package com.android.tradefed.result;

import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner.LogAnnotation;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner.MetricAnnotation;
import com.android.tradefed.testtype.MetricTestCase.LogHolder;
import com.android.tradefed.util.StreamUtil;

import org.junit.AssumptionViolatedException;
import org.junit.runner.Description;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunListener;
import org.junit.runners.model.MultipleFailureException;

import java.lang.annotation.Annotation;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * Result forwarder from JUnit4 Runner.
 */
public class JUnit4ResultForwarder extends RunListener {

    private ITestInvocationListener mListener;
    private List<Throwable> mTestCaseFailures;

    public JUnit4ResultForwarder(ITestInvocationListener listener) {
        mListener = listener;
        mTestCaseFailures = new ArrayList<>();
    }

    @Override
    public void testFailure(Failure failure) throws Exception {
        Description description = failure.getDescription();
        if (description.getMethodName() == null) {
            // In case of exception in @BeforeClass, the method name will be null
            mListener.testRunFailed(String.format("Failed with trace: %s", failure.getTrace()));
            return;
        }
        mTestCaseFailures.add(failure.getException());
    }

    @Override
    public void testAssumptionFailure(Failure failure) {
        mTestCaseFailures.add(failure.getException());
    }

    @Override
    public void testStarted(Description description) {
        mTestCaseFailures.clear();
        TestDescription testid =
                new TestDescription(
                        description.getClassName(),
                        description.getMethodName(),
                        description.getAnnotations());
        mListener.testStarted(testid);
    }

    @Override
    public void testFinished(Description description) {
        TestDescription testid =
                new TestDescription(
                        description.getClassName(),
                        description.getMethodName(),
                        description.getAnnotations());
        handleFailures(testid);
        // Explore the Description to see if we find any Annotation metrics carrier
        HashMap<String, Metric> metrics = new HashMap<>();
        for (Description child : description.getChildren()) {
            for (Annotation a : child.getAnnotations()) {
                if (a instanceof MetricAnnotation) {
                    metrics.putAll(((MetricAnnotation) a).mMetrics);
                }
                if (a instanceof LogAnnotation) {
                    // Log all the logs found.
                    for (LogHolder log : ((LogAnnotation) a).mLogs) {
                        mListener.testLog(log.mDataName, log.mDataType, log.mDataStream);
                        StreamUtil.cancel(log.mDataStream);
                    }
                    ((LogAnnotation) a).mLogs.clear();
                }
            }
        }
        //description.
        mListener.testEnded(testid, metrics);
    }

    @Override
    public void testIgnored(Description description) throws Exception {
        TestDescription testid =
                new TestDescription(
                        description.getClassName(),
                        description.getMethodName(),
                        description.getAnnotations());
        // We complete the event life cycle since JUnit4 fireIgnored is not within fireTestStarted
        // and fireTestEnded.
        mListener.testStarted(testid);
        mListener.testIgnored(testid);
        mListener.testEnded(testid, new HashMap<String, Metric>());
    }

    /**
     * Handle all the failure received from the JUnit4 tests, if a single
     * AssumptionViolatedException is received then treat the test as assumption failure. Otherwise
     * treat everything else as failure.
     */
    private void handleFailures(TestDescription testid) {
        if (mTestCaseFailures.isEmpty()) {
            return;
        }
        if (mTestCaseFailures.size() == 1) {
            Throwable t = mTestCaseFailures.get(0);
            if (t instanceof AssumptionViolatedException) {
                mListener.testAssumptionFailure(testid, StreamUtil.getStackTrace(t));
            } else {
                mListener.testFailed(testid, StreamUtil.getStackTrace(t));
            }
        } else {
            MultipleFailureException multiException =
                    new MultipleFailureException(mTestCaseFailures);
            mListener.testFailed(testid, StreamUtil.getStackTrace(multiException));
        }
    }
}
