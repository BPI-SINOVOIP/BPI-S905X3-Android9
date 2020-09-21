/*
 * Copyright (C) 2015 The Android Open Source Project
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
import com.android.ddmlib.Log.LogLevel;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * Result reporter to print the test results to the console.
 * <p>
 * Prints each test run, each test case, and test metrics, test logs, and test file locations.
 * <p>
 */
@OptionClass(alias = "console-result-reporter")
public class ConsoleResultReporter extends CollectingTestListener implements ILogSaverListener {
    private static final String LOG_TAG = ConsoleResultReporter.class.getSimpleName();

    @Option(name = "suppress-passed-tests", description = "For functional tests, ommit summary for "
            + "passing tests, only print failed and ignored ones")
    private boolean mSuppressPassedTest = false;

    private List<LogFile> mLogFiles = new LinkedList<>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationEnded(long elapsedTime) {
        Log.logAndDisplay(LogLevel.INFO, LOG_TAG, getInvocationSummary());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLogSaved(String dataName, LogDataType dataType, InputStreamSource dataStream,
            LogFile logFile) {
        mLogFiles.add(logFile);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setLogSaver(ILogSaver logSaver) {
        // Ignore. This class doesn't save any additional files.
    }

    /**
     * Get the invocation summary as a string.
     */
    String getInvocationSummary() {
        if (getRunResults().isEmpty() && mLogFiles.isEmpty()) {
            return "No test results\n";
        }

        StringBuilder sb = new StringBuilder();
        for (TestRunResult testRunResult : getRunResults()) {
            sb.append(getTestRunSummary(testRunResult));
        }
        if (!mLogFiles.isEmpty()) {
            sb.append("Log Files:\n");
            for (LogFile logFile : mLogFiles) {
                final String url = logFile.getUrl();
                sb.append(String.format("  %s\n", url != null ? url : logFile.getPath()));
            }
        }
        return "Test results:\n" + sb.toString().trim() + "\n";
    }

    /**
     * Get the test run summary as a string including run metrics.
     */
    String getTestRunSummary(TestRunResult testRunResult) {
        StringBuilder sb = new StringBuilder();
        sb.append(String.format("%s:", testRunResult.getName()));
        if (testRunResult.getNumTests() > 0) {
            sb.append(String.format(" %d Test%s, %d Passed, %d Failed, %d Ignored",
                    testRunResult.getNumCompleteTests(),
                    testRunResult.getNumCompleteTests() == 1 ? "" : "s", // Pluralize Test
                    testRunResult.getNumTestsInState(TestStatus.PASSED),
                    testRunResult.getNumAllFailedTests(),
                    testRunResult.getNumTestsInState(TestStatus.IGNORED)));
        } else if (testRunResult.getRunMetrics().size() == 0) {
            sb.append(" No results");
        }
        sb.append("\n");
        Map<TestDescription, TestResult> testResults = testRunResult.getTestResults();
        for (Map.Entry<TestDescription, TestResult> entry : testResults.entrySet()) {
            if (mSuppressPassedTest && TestStatus.PASSED.equals(entry.getValue().getStatus())) {
                continue;
            }
            sb.append(getTestSummary(entry.getKey(), entry.getValue()));
        }
        Map<String, String> metrics = testRunResult.getRunMetrics();
        if (metrics != null && !metrics.isEmpty()) {
            List<String> metricKeys = new ArrayList<String>(metrics.keySet());
            Collections.sort(metricKeys);
            for (String metricKey : metricKeys) {
                sb.append(String.format("  %s: %s\n", metricKey, metrics.get(metricKey)));
            }
        }
        sb.append("\n");
        return sb.toString();
    }

    /**
     * Get the test summary as string including test metrics.
     */
    String getTestSummary(TestDescription testId, TestResult testResult) {
        StringBuilder sb = new StringBuilder();
        sb.append(String.format("  %s: %s (%dms)\n", testId.toString(), testResult.getStatus(),
                testResult.getEndTime() - testResult.getStartTime()));
        String stack = testResult.getStackTrace();
        if (stack != null && !stack.isEmpty()) {
            sb.append("  stack=\n");
            String lines[] = stack.split("\\r?\\n");
            for (String line : lines) {
                sb.append(String.format("    %s\n", line));
            }
        }
        Map<String, String> metrics = testResult.getMetrics();
        if (metrics != null && !metrics.isEmpty()) {
            List<String> metricKeys = new ArrayList<String>(metrics.keySet());
            Collections.sort(metricKeys);
            for (String metricKey : metricKeys) {
                sb.append(String.format("    %s: %s\n", metricKey, metrics.get(metricKey)));
            }
        }

        return sb.toString();
    }
}
