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

import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

/**
 * A {@link ITestInvocationListener} that forwards invocation results to a list of other listeners.
 */
public class ResultForwarder implements ITestInvocationListener {

    private List<ITestInvocationListener> mListeners;

    /**
     * Create a {@link ResultForwarder} with deferred listener setting.  Intended only for use by
     * subclasses.
     */
    protected ResultForwarder() {
        mListeners = Collections.emptyList();
    }

    /**
     * Create a {@link ResultForwarder}.
     *
     * @param listeners the real {@link ITestInvocationListener}s to forward results to
     */
    public ResultForwarder(List<ITestInvocationListener> listeners) {
        mListeners = listeners;
    }

    /**
     * Alternate variable arg constructor for {@link ResultForwarder}.
     *
     * @param listeners the real {@link ITestInvocationListener}s to forward results to
     */
    public ResultForwarder(ITestInvocationListener... listeners) {
        mListeners = Arrays.asList(listeners);
    }

    /**
     * Set the listeners after construction.  Intended only for use by subclasses.
     *
     * @param listeners the real {@link ITestInvocationListener}s to forward results to
     */
    protected void setListeners(List<ITestInvocationListener> listeners) {
        mListeners = listeners;
    }

    /**
     * Set the listeners after construction.  Intended only for use by subclasses.
     *
     * @param listeners the real {@link ITestInvocationListener}s to forward results to
     */
    protected void setListeners(ITestInvocationListener... listeners) {
        mListeners = Arrays.asList(listeners);
    }

    /**
     * Get the list of listeners.  Intended only for use by subclasses.
     *
     * @return The list of {@link ITestInvocationListener}s.
     */
    protected List<ITestInvocationListener> getListeners() {
        return mListeners;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationStarted(IInvocationContext context) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.invocationStarted(context);
            } catch (RuntimeException e) {
                CLog.e(
                        "Exception while invoking %s#invocationStarted",
                        listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationFailed(Throwable cause) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.invocationFailed(cause);
            } catch (RuntimeException e) {
                CLog.e(
                        "Exception while invoking %s#invocationFailed",
                        listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationEnded(long elapsedTime) {
        InvocationSummaryHelper.reportInvocationEnded(mListeners, elapsedTime);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public TestSummary getSummary() {
        // should never be called
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.testLog(dataName, dataType, dataStream);
            } catch (RuntimeException e) {
                CLog.e("Exception while invoking %s#testLog", listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStarted(String runName, int testCount) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.testRunStarted(runName, testCount);
            } catch (RuntimeException e) {
                CLog.e("Exception while invoking %s#testRunStarted", listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunFailed(String errorMessage) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.testRunFailed(errorMessage);
            } catch (RuntimeException e) {
                CLog.e("Exception while invoking %s#testRunFailed", listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStopped(long elapsedTime) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.testRunStopped(elapsedTime);
            } catch (RuntimeException e) {
                CLog.e("Exception while invoking %s#testRunStopped", listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void testRunEnded(long elapsedTime, HashMap<String, Metric> runMetrics) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.testRunEnded(elapsedTime, runMetrics);
            } catch (RuntimeException e) {
                CLog.e("Exception while invoking %s#testRunEnded", listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void testStarted(TestDescription test) {
        testStarted(test, System.currentTimeMillis());
    }

    /** {@inheritDoc} */
    @Override
    public void testStarted(TestDescription test, long startTime) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.testStarted(test, startTime);
            } catch (RuntimeException e) {
                CLog.e("Exception while invoking %s#testStarted", listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void testFailed(TestDescription test, String trace) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.testFailed(test, trace);
            } catch (RuntimeException e) {
                CLog.e("Exception while invoking %s#testFailed", listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void testEnded(TestDescription test, HashMap<String, Metric> testMetrics) {
        testEnded(test, System.currentTimeMillis(), testMetrics);
    }

    /** {@inheritDoc} */
    @Override
    public void testEnded(TestDescription test, long endTime, HashMap<String, Metric> testMetrics) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.testEnded(test, endTime, testMetrics);
            } catch (RuntimeException e) {
                CLog.e("Exception while invoking %s#testEnded", listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    @Override
    public void testAssumptionFailure(TestDescription test, String trace) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.testAssumptionFailure(test, trace);
            } catch (RuntimeException e) {
                CLog.e(
                        "Exception while invoking %s#testAssumptionFailure",
                        listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    @Override
    public void testIgnored(TestDescription test) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.testIgnored(test);
            } catch (RuntimeException e) {
                CLog.e("Exception while invoking %s#testIgnored", listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    @Override
    public void testModuleStarted(IInvocationContext moduleContext) {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.testModuleStarted(moduleContext);
            } catch (RuntimeException e) {
                CLog.e("Exception while invoking testModuleStarted");
                CLog.e(e);
            }
        }
    }

    @Override
    public void testModuleEnded() {
        for (ITestInvocationListener listener : mListeners) {
            try {
                listener.testModuleEnded();
            } catch (RuntimeException e) {
                CLog.e("Exception while invoking testModuleEnded");
                CLog.e(e);
            }
        }
    }
}
