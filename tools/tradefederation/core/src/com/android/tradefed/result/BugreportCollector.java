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
package com.android.tradefed.result;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;

/**
 * A pass-through {@link ITestInvocationListener} that collects bugreports when configurable events
 * occur and then calls {@link ITestInvocationListener#testLog} on its children after each
 * bugreport is collected.
 * <p />
 * Behaviors: (FIXME: finish this)
 * <ul>
 *   <li>Capture after each if any testcases failed</li>
 *   <li>Capture after each testcase</li>
 *   <li>Capture after each failed testcase</li>
 *   <li>Capture </li>
 * </ul>
 */
public class BugreportCollector implements ITestInvocationListener {
    /** A predefined predicate which fires after each failed testcase */
    public static final Predicate AFTER_FAILED_TESTCASES =
            p(Relation.AFTER, Freq.EACH, Noun.FAILED_TESTCASE);
    /** A predefined predicate which fires as the first invocation begins */
    public static final Predicate AT_START =
            p(Relation.AT_START_OF, Freq.EACH, Noun.INVOCATION);
    // FIXME: add other useful predefined predicates

    public static interface SubPredicate {}

    public static enum Noun implements SubPredicate {
        // FIXME: find a reasonable way to detect runtime restarts
        // FIXME: try to make sure there aren't multiple ways to specify a single condition
        TESTCASE,
        FAILED_TESTCASE,
        TESTRUN,
        FAILED_TESTRUN,
        INVOCATION,
        FAILED_INVOCATION;
    }

    public static enum Relation implements SubPredicate {
        AFTER,
        AT_START_OF;
    }

    public static enum Freq implements SubPredicate {
        EACH,
        FIRST;
    }

    public static enum Filter implements SubPredicate {
        WITH_FAILING,
        WITH_PASSING,
        WITH_ANY;
    }

    /**
     * A full predicate describing when to capture a bugreport.  Has the following required elements
     * and [optional elements]:
     * RelationP TimingP Noun [FilterP Noun]
     */
    public static class Predicate {
        List<SubPredicate> mSubPredicates = new ArrayList<SubPredicate>(3);
        List<SubPredicate> mFilterSubPredicates = null;

        public Predicate(Relation rp, Freq fp, Noun n) throws IllegalArgumentException {
            assertValidPredicate(rp, fp, n);

            mSubPredicates.add(rp);
            mSubPredicates.add(fp);
            mSubPredicates.add(n);
        }

        public Predicate(Relation rp, Freq fp, Noun fpN, Filter filterP, Noun filterPN)
                throws IllegalArgumentException {
            mSubPredicates.add(rp);
            mSubPredicates.add(fp);
            mSubPredicates.add(fpN);
            mFilterSubPredicates = new ArrayList<SubPredicate>(2);
            mFilterSubPredicates.add(filterP);
            mFilterSubPredicates.add(filterPN);
        }

        public static void assertValidPredicate(Relation rp, Freq fp, Noun n)
                throws IllegalArgumentException {
            if (rp == Relation.AT_START_OF) {
                // It doesn't make sense to say AT_START_OF FAILED_(x) since we'll only know that it
                // failed in the AFTER case.
                if (n == Noun.FAILED_TESTCASE || n == Noun.FAILED_TESTRUN ||
                        n == Noun.FAILED_INVOCATION) {
                    throw new IllegalArgumentException(String.format(
                            "Illegal predicate: %s %s isn't valid since we can only check " +
                            "failure on the AFTER event.", fp, n));
                }
            }
            if (n == Noun.INVOCATION || n == Noun.FAILED_INVOCATION) {
                // Block "FIRST INVOCATION" for disambiguation, since there will only ever be one
                // invocation
                if (fp == Freq.FIRST) {
                    throw new IllegalArgumentException(String.format(
                            "Illegal predicate: Since there is only one invocation, please use " +
                            "%s %s rather than %s %s for disambiguation.", Freq.EACH, n, fp, n));
                }
            }
        }

        protected List<SubPredicate> getPredicate() {
            return mSubPredicates;
        }

        protected List<SubPredicate> getFilterPredicate() {
            return mFilterSubPredicates;
        }

        public boolean partialMatch(Predicate otherP) {
            return mSubPredicates.equals(otherP.getPredicate());
        }

        public boolean fullMatch(Predicate otherP) {
            if (partialMatch(otherP)) {
                if (mFilterSubPredicates == null) {
                    return otherP.getFilterPredicate() == null;
                } else {
                    return mFilterSubPredicates.equals(otherP.getFilterPredicate());
                }
            }
            return false;
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            ListIterator<SubPredicate> iter = mSubPredicates.listIterator();
            while (iter.hasNext()) {
                SubPredicate p = iter.next();
                sb.append(p.toString());
                if (iter.hasNext()) {
                    sb.append("_");
                }
            }

            return sb.toString();
        }

        @Override
        public boolean equals(Object other) {
            if (other instanceof Predicate) {
                Predicate otherP = (Predicate) other;
                return fullMatch(otherP);
            } else {
                return false;
            }
        }

        @Override
        public int hashCode() {
            return mSubPredicates.hashCode();
        }
    }

    // Now that the Predicate framework is done, actually start on the BugreportCollector class
    /**
     * We keep an internal {@link CollectingTestListener} instead of subclassing to make sure that
     * we @Override all of the applicable interface methods (instead of having them fall through to
     * implementations in {@link CollectingTestListener}).
     */
    private CollectingTestListener mCollector = new CollectingTestListener();
    private ITestInvocationListener mListener;
    private ITestDevice mTestDevice;
    private List<Predicate> mPredicates = new LinkedList<Predicate>();
    @SuppressWarnings("unused")
    private boolean mAsynchronous = false;
    @SuppressWarnings("unused")
    private boolean mCapturedBugreport = false;

    /**
     * How long to potentially wait for the device to be Online before we try to capture a
     * bugreport.  If negative, no check will be performed
     */
    private int mDeviceWaitTimeSecs = 40; // default to 40s
    private String mDescriptiveName = null;
    // FIXME: Add support for minimum wait time between successive bugreports
    // FIXME: get rid of reset() method

    // Caching for counts that CollectingTestListener doesn't store
    private int mNumFailedRuns = 0;

    public BugreportCollector(ITestInvocationListener listener, ITestDevice testDevice) {
        if (listener == null) {
            throw new NullPointerException("listener must be non-null.");
        }
        if (testDevice == null) {
            throw new NullPointerException("device must be non-null.");
        }
        mListener = listener;
        mTestDevice = testDevice;
    }

    public void addPredicate(Predicate p) {
        mPredicates.add(p);
    }

    /**
     * Set the time (in seconds) to wait for the device to be Online before we try to capture a
     * bugreport.  If negative, no check will be performed.  Any {@link DeviceNotAvailableException}
     * encountered during this check will be logged and ignored.
     */
    public void setDeviceWaitTime(int waitTime) {
        mDeviceWaitTimeSecs = waitTime;
    }

    /**
     * Block until the collector is not collecting any bugreports.  If the collector isn't actively
     * collecting a bugreport, return immediately
     */
    public void blockUntilIdle() {
        // FIXME
        return;
    }

    /**
     * Set whether bugreport collection should collect the bugreport in a different thread
     * ({@code asynchronous = true}), or block the caller until the bugreport is captured
     * ({@code asynchronous = false}).
     */
    public void setAsynchronous(boolean asynchronous) {
        // FIXME do something
        mAsynchronous = asynchronous;
    }

    /**
     * Set the descriptive name to use when recording bugreports.  If {@code null},
     * {@code BugreportCollector} will fall back to the default behavior of serializing the name of
     * the event that caused the bugreport to be collected.
     */
    public void setDescriptiveName(String name) {
        mDescriptiveName = name;
    }

    /**
     * Actually capture a bugreport and pass it to our child listener.
     */
    void grabBugreport(String logDesc) {
        CLog.v("About to grab bugreport for %s; custom name is %s.", logDesc, mDescriptiveName);
        if (mDescriptiveName != null) {
            logDesc = mDescriptiveName;
        }
        String logName = String.format("bug-%s.%d", logDesc, System.currentTimeMillis());
        CLog.v("Log name is %s", logName);
        if (mDeviceWaitTimeSecs >= 0) {
            try {
                mTestDevice.waitForDeviceOnline((long)mDeviceWaitTimeSecs * 1000);
            } catch (DeviceNotAvailableException e) {
                // Because we want to be as transparent as possible, we don't let this exception
                // bubble up; if a problem happens that actually affects the test, the test will
                // run into it.  If the test doesn't care (or, for instance, expects the device to
                // be unavailable for a period of time), then we don't care.
                CLog.e("Caught DeviceNotAvailableException while trying to capture bugreport");
                CLog.e(e);
            }
        }
        try (InputStreamSource bugreport = mTestDevice.getBugreport()) {
            mListener.testLog(logName, LogDataType.BUGREPORT, bugreport);
        }
    }

    Predicate getPredicate(Predicate predicate) {
        for (Predicate p : mPredicates) {
            if (p.partialMatch(predicate)) {
                return p;
            }
        }
        return null;
    }

    Predicate search(Relation relation, Collection<Freq> freqs, Noun noun) {
        for (Predicate pred : mPredicates) {
            for (Freq freq : freqs) {
                CLog.v("Search checking predicate %s", p(relation, freq, noun));
                if (pred.partialMatch(p(relation, freq, noun))) {
                    return pred;
                }
            }
        }
        return null;
    }

    boolean check(Relation relation, Noun noun) {
        return check(relation, noun, null);
    }

    boolean check(Relation relation, Noun noun, TestDescription test) {
        // Expect to get something like "AFTER", "TESTCASE"

        // All freqs that could match _right now_.  Should be added in decreasing order of
        // specificity (so the most specific option has the ability to match first)
        List<Freq> applicableFreqs = new ArrayList<Freq>(2 /* total # freqs in enum */);
        applicableFreqs.add(Freq.EACH);

        TestRunResult curResult = mCollector.getCurrentRunResults();
        switch (relation) {
            case AFTER:
                switch (noun) {
                    case TESTCASE:
                        // FIXME: grab the name of the testcase that just finished
                        if (curResult.getNumTests() == 1) {
                            applicableFreqs.add(Freq.FIRST);
                        }
                        break;

                    case FAILED_TESTCASE:
                        if (curResult.getNumAllFailedTests() == 1) {
                            applicableFreqs.add(Freq.FIRST);
                        }
                        break;

                    case TESTRUN:
                        if (mCollector.getRunResults().size() == 1) {
                            applicableFreqs.add(Freq.FIRST);
                        }
                        break;

                    case FAILED_TESTRUN:
                        if (mNumFailedRuns == 1) {
                            applicableFreqs.add(Freq.FIRST);
                        }
                        break;
                    default:
                        break;
                }
                break; // case AFTER

            case AT_START_OF:
                switch (noun) {
                    case TESTCASE:
                        if (curResult.getNumTests() == 1) {
                            applicableFreqs.add(Freq.FIRST);
                        }
                        break;

                    case TESTRUN:
                        if (mCollector.getRunResults().size() == 1) {
                            applicableFreqs.add(Freq.FIRST);
                        }
                        break;
                    default:
                        break;
                }
                break; // case AT_START_OF
        }

        Predicate storedP = search(relation, applicableFreqs, noun);
        if (storedP != null) {
            CLog.v("Found storedP %s for relation %s and noun %s", storedP, relation, noun);
            String desc = storedP.toString();
            // Try to generate a useful description
            if (test != null) {
                // We use "__" instead of "#" here because of ambiguity in automatically making
                // HTML links containing the "#" character -- it could just as easily be a real hash
                // character as an HTML fragment specification.
                final String testName = String.format("%s__%s", test.getClassName(),
                        test.getTestName());
                switch (noun) {
                    case TESTCASE:
                        // bug-FooBarTest#testMethodName
                        desc = testName;
                        break;

                    case FAILED_TESTCASE:
                        // bug-FAILED-FooBarTest#testMethodName
                        desc = String.format("FAILED-%s", testName);
                        break;

                    default:
                        break;
                }
            }

            CLog.v("Grabbing bugreport.");
            grabBugreport(desc);
            mCapturedBugreport = true;
            return true;
        } else {
            return false;
        }
    }

    void reset() {
        mCapturedBugreport = false;
    }

    /**
     * Convenience method to build a predicate from subpredicates
     */
    private static Predicate p(Relation rp, Freq fp, Noun n) throws IllegalArgumentException {
        return new Predicate(rp, fp, n);
    }

    /**
     * Convenience method to build a predicate from subpredicates
     */
    @SuppressWarnings("unused")
    private static Predicate p(Relation rp, Freq fp, Noun fpN, Filter filterP, Noun filterPN)
            throws IllegalArgumentException {
        return new Predicate(rp, fp, fpN, filterP, filterPN);
    }


    // Methods from the {@link ITestInvocationListener} interface
    @Override
    public void testEnded(TestDescription test, HashMap<String, Metric> testMetrics) {
        mListener.testEnded(test, testMetrics);
        mCollector.testEnded(test, testMetrics);
        check(Relation.AFTER, Noun.TESTCASE, test);
        reset();
    }

    /** {@inheritDoc} */
    @Override
    public void testFailed(TestDescription test, String trace) {
        mListener.testFailed(test, trace);
        mCollector.testFailed(test, trace);
        check(Relation.AFTER, Noun.FAILED_TESTCASE, test);
        reset();
    }

    /** {@inheritDoc} */
    @Override
    public void testAssumptionFailure(TestDescription test, String trace) {
        mListener.testAssumptionFailure(test, trace);
        mCollector.testAssumptionFailure(test, trace);
        check(Relation.AFTER, Noun.FAILED_TESTCASE, test);
        reset();
    }

    /** {@inheritDoc} */
    @Override
    public void testRunEnded(long elapsedTime, HashMap<String, Metric> runMetrics) {
        mListener.testRunEnded(elapsedTime, runMetrics);
        mCollector.testRunEnded(elapsedTime, runMetrics);
        check(Relation.AFTER, Noun.TESTRUN);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunFailed(String errorMessage) {
        mListener.testRunFailed(errorMessage);
        mCollector.testRunFailed(errorMessage);
        check(Relation.AFTER, Noun.FAILED_TESTRUN);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStarted(String runName, int testCount) {
        mListener.testRunStarted(runName, testCount);
        mCollector.testRunStarted(runName, testCount);
        check(Relation.AT_START_OF, Noun.TESTRUN);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStopped(long elapsedTime) {
        mListener.testRunStopped(elapsedTime);
        mCollector.testRunStopped(elapsedTime);
        // FIXME: figure out how to expose this
    }

    /** {@inheritDoc} */
    @Override
    public void testStarted(TestDescription test) {
        mListener.testStarted(test);
        mCollector.testStarted(test);
        check(Relation.AT_START_OF, Noun.TESTCASE, test);
    }

    // Methods from the {@link ITestInvocationListener} interface
    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationStarted(IInvocationContext context) {
        mListener.invocationStarted(context);
        mCollector.invocationStarted(context);
        check(Relation.AT_START_OF, Noun.INVOCATION);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
        mListener.testLog(dataName, dataType, dataStream);
        mCollector.testLog(dataName, dataType, dataStream);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationEnded(long elapsedTime) {
        mListener.invocationEnded(elapsedTime);
        mCollector.invocationEnded(elapsedTime);
        check(Relation.AFTER, Noun.INVOCATION);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationFailed(Throwable cause) {
        mListener.invocationFailed(cause);
        mCollector.invocationFailed(cause);
        check(Relation.AFTER, Noun.FAILED_INVOCATION);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public TestSummary getSummary() {
        return mListener.getSummary();
    }

    @Override
    public void testIgnored(TestDescription test) {
        // ignore
    }
}

