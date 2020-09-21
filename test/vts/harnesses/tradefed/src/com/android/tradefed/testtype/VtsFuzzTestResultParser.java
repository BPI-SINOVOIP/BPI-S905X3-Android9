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

import com.android.ddmlib.IShellOutputReceiver;
import com.android.ddmlib.Log;
import com.android.ddmlib.MultiLineReceiver;
import com.android.ddmlib.testrunner.ITestRunListener;
import com.android.tradefed.result.ITestLifeCycleReceiver;
import com.android.tradefed.result.TestDescription;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Map;

/**
 * Parses the 'raw output mode' results of VTS fuzz tests that run from shell, and informs
 * a ITestRunListener of the results.
 * <p>Sample format of output expected:
 *
 * <pre>
 * ...
 * [  PASSED  ]
 * </pre>
 *
 * If a fuzz test run finds a problem, the process crashes or hangs and thus [  PASSED  ] is not
 * printed. All other lines are ignored.
 */
public class VtsFuzzTestResultParser extends MultiLineReceiver {
    private static final String LOG_TAG = "GTestResultParser";

    // Variables to keep track of state
    private int mNumTestsRun = 0;
    private int mNumTestsExpected = 1;
    private int mTotalNumberOfTestFailed = 0;
    private final String mTestRunName;
    private final Collection<ITestLifeCycleReceiver> mTestListeners;
    private TestDescription mTestId;

    /** True if start of test has already been reported to listener. */
    private boolean mTestRunStartReported = false;

    /** Stack data structure to keep the logs. */
    private StringBuilder mStackTrace = null;

    /** True if current test run has been canceled by user. */
    private boolean mIsCancelled = false;

    /** Prefixes used to demarcate and identify output. */
    private static class Prefixes {
        @SuppressWarnings("unused")
        private static final String PASSED_MARKER = "[  PASSED  ]";
        private static final String FAILED_MARKER = "[  FAILED  ]";
        private static final String TIMEOUT_MARKER = "[ TIMEOUT  ]";
    }

    /**
     * Creates the GTestResultParser.
     *
     * @param testRunName the test run name to provide to
     *            {@link ITestRunListener#testRunStarted(String, int)}
     * @param listeners informed of test results as the tests are executing
     */
    public VtsFuzzTestResultParser(
            String testRunName, Collection<ITestLifeCycleReceiver> listeners) {
        mTestRunName = testRunName;
        mTestListeners = new ArrayList<>(listeners);
        mStackTrace = new StringBuilder();
    }

    /**
     * Creates the GTestResultParser for a single listener.
     *
     * @param testRunName the test run name to provide to
     *            {@link ITestRunListener#testRunStarted(String, int)}
     * @param listener informed of test results as the tests are executing
     */
    public VtsFuzzTestResultParser(String testRunName, ITestLifeCycleReceiver listener) {
        mTestRunName = testRunName;
        mTestListeners = new ArrayList<>(1);
        mTestListeners.add(listener);
        mStackTrace = new StringBuilder();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void processNewLines(String[] lines) {
        if (!mTestRunStartReported) {
            // current test results are cleared out after every complete test run,
            // if it's not null, assume the last test caused this and report as a test failure
            mTestId = new TestDescription("fuzzer", mTestRunName);
            for (ITestLifeCycleReceiver listener : mTestListeners) {
                listener.testRunStarted(mTestRunName, mNumTestsExpected);
            }
            mTestRunStartReported = true;
        }

        for (String line : lines) {
            if (line.contains(Prefixes.PASSED_MARKER)) {
                doTestEnded(true);
            } else if (line.contains(Prefixes.FAILED_MARKER)
                    || line.contains(Prefixes.TIMEOUT_MARKER)) {
                doTestEnded(false);
            } else {
                mStackTrace.append(line + "\r\n");
            }
            Log.v(LOG_TAG, line);
        }
    }

    /**
     * Helper method to do the work necessary when a test has ended.
     *
     * @param testPassed Indicates whether the test passed or failed (set to true if passed, false
     *          if failed)
     */
    private void doTestEnded(boolean testPassed) {
        if (!testPassed) {  // test failed
            for (ITestLifeCycleReceiver listener : mTestListeners) {
                listener.testFailed(mTestId, mStackTrace.toString());
            }
            ++mTotalNumberOfTestFailed;
        }

        // For all cases (pass or fail), we ultimately need to report test has ended
        for (ITestLifeCycleReceiver listener : mTestListeners) {
            Map <String, String> emptyMap = Collections.emptyMap();
            listener.testEnded(mTestId, emptyMap);
        }

        ++mNumTestsRun;
    }

    /**
     * Process an instrumentation run failure
     *
     * @param errorMsg The message to output about the nature of the error
     */
    private void handleTestRunFailed(String errorMsg) {
        errorMsg = (errorMsg == null ? "Unknown error" : errorMsg);
        Log.i(LOG_TAG, String.format("Test run failed: %s", errorMsg));

        // If there was any stack trace during the test run, append it to the "test failed"
        // error message so we have an idea of what caused the crash/failure.
        Map<String, String> emptyMap = Collections.emptyMap();
        for (ITestLifeCycleReceiver listener : mTestListeners) {
            listener.testFailed(mTestId, mStackTrace.toString());
            listener.testEnded(mTestId, emptyMap);
        }

        // Report the test run failed
        for (ITestLifeCycleReceiver listener : mTestListeners) {
            listener.testRunFailed(errorMsg);
        }
    }

    /**
     * Called by parent when adb session is complete.
     */
    @Override
    public void done() {
        super.done();
        if (mNumTestsRun < mNumTestsExpected) {
            handleTestRunFailed(String.format("Test run failed. Expected %d tests, received %d",
                    mNumTestsExpected, mNumTestsRun));
        }
    }

    /**
     * Returns true if test run canceled.
     *
     * @see IShellOutputReceiver#isCancelled()
     */
    @Override
    public boolean isCancelled() {
        return mIsCancelled;
    }

    /**
     * Requests cancellation of test run.
     */
    public void cancel() {
        mIsCancelled = true;
    }
}
