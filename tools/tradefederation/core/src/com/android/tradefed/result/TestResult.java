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
package com.android.tradefed.result;

import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;

/** Container for a result of a single test. */
public class TestResult {

    private TestStatus mStatus;
    private String mStackTrace;
    private Map<String, String> mMetrics;
    private HashMap<String, Metric> mProtoMetrics;
    private Map<String, LogFile> mLoggedFiles;
    // the start and end time of the test, measured via {@link System#currentTimeMillis()}
    private long mStartTime = 0;
    private long mEndTime = 0;

    public TestResult() {
        mStatus = TestStatus.INCOMPLETE;
        mStartTime = System.currentTimeMillis();
        mLoggedFiles = new LinkedHashMap<String, LogFile>();
        mProtoMetrics = new HashMap<>();
    }

    /** Get the {@link TestStatus} result of the test. */
    public TestStatus getStatus() {
        return mStatus;
    }

    /**
     * Get the associated {@link String} stack trace. Should be <code>null</code> if {@link
     * #getStatus()} is {@link TestStatus#PASSED}.
     */
    public String getStackTrace() {
        return mStackTrace;
    }

    /** Get the associated test metrics. */
    public Map<String, String> getMetrics() {
        return mMetrics;
    }

    /** Get the associated test metrics in proto format. */
    public HashMap<String, Metric> getProtoMetrics() {
        return mProtoMetrics;
    }

    /** Set the test metrics, overriding any previous values. */
    public void setMetrics(Map<String, String> metrics) {
        mMetrics = metrics;
    }

    /** Set the test proto metrics format, overriding any previous values. */
    public void setProtoMetrics(HashMap<String, Metric> metrics) {
        mProtoMetrics = metrics;
    }

    /** Add a logged file tracking associated with that test case */
    public void addLoggedFile(String dataName, LogFile loggedFile) {
        mLoggedFiles.put(dataName, loggedFile);
    }

    /** Returns a copy of the map containing all the logged file associated with that test case. */
    public Map<String, LogFile> getLoggedFiles() {
        return new LinkedHashMap<>(mLoggedFiles);
    }

    /**
     * Return the {@link System#currentTimeMillis()} time that the {@link
     * ITestInvocationListener#testStarted(TestDescription)} event was received.
     */
    public long getStartTime() {
        return mStartTime;
    }

    /**
     * Allows to set the time when the test was started, to be used with {@link
     * ITestInvocationListener#testStarted(TestDescription, long)}.
     */
    public void setStartTime(long startTime) {
        mStartTime = startTime;
    }

    /**
     * Return the {@link System#currentTimeMillis()} time that the {@link
     * ITestInvocationListener#testEnded(TestDescription, Map)} event was received.
     */
    public long getEndTime() {
        return mEndTime;
    }

    /** Set the {@link TestStatus}. */
    public TestResult setStatus(TestStatus status) {
        mStatus = status;
        return this;
    }

    /** Set the stack trace. */
    public void setStackTrace(String trace) {
        mStackTrace = trace;
    }

    /** Sets the end time */
    public void setEndTime(long currentTimeMillis) {
        mEndTime = currentTimeMillis;
    }

    @Override
    public int hashCode() {
        return Arrays.hashCode(new Object[] {mMetrics, mStackTrace, mStatus});
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        TestResult other = (TestResult) obj;
        return equal(mMetrics, other.mMetrics)
                && equal(mStackTrace, other.mStackTrace)
                && equal(mStatus, other.mStatus);
    }

    private static boolean equal(Object a, Object b) {
        return a == b || (a != null && a.equals(b));
    }

    /**
     * Create an exact copy of given TestResult.
     *
     * @param result The TestResult to copy from.
     */
    public static TestResult clone(TestResult result) {
        TestResult newResult = new TestResult();
        newResult.setStatus(result.getStatus());
        newResult.setStackTrace(result.getStackTrace());
        newResult.setMetrics(result.getMetrics());
        newResult.setProtoMetrics(result.getProtoMetrics());
        newResult.setStartTime(result.getStartTime());
        newResult.setEndTime(result.getEndTime());
        // The LoggedFiles map contains the log file info whenever testLogSaved is called.
        // e.g. testLogSaved("coverage", new LogFile("/fake/path", "fake//url", LogDataType.TEXT))
        for (Map.Entry<String, LogFile> logFile : result.getLoggedFiles().entrySet()) {
            newResult.addLoggedFile(logFile.getKey(), logFile.getValue());
        }
        return newResult;
    }
}
