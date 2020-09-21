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
package com.android.tradefed.testtype.metricregression;

import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.MetricsXmlParser;
import com.android.tradefed.util.MultiMap;
import com.android.tradefed.util.Pair;

import com.google.common.annotations.VisibleForTesting;

/** A metrics object to hold run metrics and test metrics parsed by {@link MetricsXmlParser} */
public class Metrics {
    private int mNumRuns;
    private int mNumTests = -1;
    private final boolean mStrictMode;
    private final MultiMap<String, Double> mRunMetrics = new MultiMap<>();
    private final MultiMap<Pair<TestDescription, String>, Double> mTestMetrics = new MultiMap<>();

    /** Throw when metrics validation fails in strict mode. */
    public static class MetricsException extends RuntimeException {
        MetricsException(String cause) {
            super(cause);
        }
    }

    /**
     * Constructs an empty Metrics object.
     *
     * @param strictMode whether exception should be thrown when validation fails
     */
    public Metrics(boolean strictMode) {
        mStrictMode = strictMode;
    }

    /**
     * Sets the number of tests. This method also checks if each call sets the same number of test,
     * since this number should be consistent across multiple runs.
     *
     * @param numTests the number of tests
     * @throws MetricsException if subsequent calls set a different number.
     */
    public void setNumTests(int numTests) {
        if (mNumTests == -1) {
            mNumTests = numTests;
        } else {
            if (mNumTests != numTests) {
                String msg =
                        String.format(
                                "Number of test entries differ: expect #%d actual #%d",
                                mNumTests, numTests);
                throw new MetricsException(msg);
            }
        }
    }

    /**
     * Adds a run metric.
     *
     * @param name metric name
     * @param value metric value
     */
    public void addRunMetric(String name, String value) {
        try {
            mRunMetrics.put(name, Double.parseDouble(value));
        } catch (NumberFormatException e) {
            // This is normal. We often get some string metrics like device name. Just log it.
            CLog.w(String.format("Run metric \"%s\" is not a number: \"%s\"", name, value));
        }
    }

    /**
     * Adds a test metric.
     *
     * @param testId TestDescription of the metric
     * @param name metric name
     * @param value metric value
     */
    public void addTestMetric(TestDescription testId, String name, String value) {
        Pair<TestDescription, String> metricId = new Pair<>(testId, name);
        try {
            mTestMetrics.put(metricId, Double.parseDouble(value));
        } catch (NumberFormatException e) {
            // This is normal. We often get some string metrics like device name. Just log it.
            CLog.w(
                    String.format(
                            "Test %s metric \"%s\" is not a number: \"%s\"", testId, name, value));
        }
    }

    /**
     * Validates that the number of entries of each metric equals to the number of runs.
     *
     * @param numRuns number of runs
     * @throws MetricsException when validation fails in strict mode
     */
    public void validate(int numRuns) {
        mNumRuns = numRuns;
        for (String name : mRunMetrics.keySet()) {
            if (mRunMetrics.get(name).size() < mNumRuns) {
                error(
                        String.format(
                                "Run metric \"%s\" too few entries: expected #%d actual #%d",
                                name, mNumRuns, mRunMetrics.get(name).size()));
            }
        }
        for (Pair<TestDescription, String> id : mTestMetrics.keySet()) {
            if (mTestMetrics.get(id).size() < mNumRuns) {
                error(
                        String.format(
                                "Test %s metric \"%s\" too few entries: expected #%d actual #%d",
                                id.first, id.second, mNumRuns, mTestMetrics.get(id).size()));
            }
        }
    }

    /**
     * Validates with after-patch Metrics object. Make sure two metrics object contain same run
     * metric entries and test metric entries. Assume this object contains before-patch metrics.
     *
     * @param after a Metrics object containing after-patch metrics
     * @throws MetricsException when cross validation fails in strict mode
     */
    public void crossValidate(Metrics after) {
        if (mNumTests != after.mNumTests) {
            error(
                    String.format(
                            "Number of test entries differ: before #%d after #%d",
                            mNumTests, after.mNumTests));
        }

        for (String name : mRunMetrics.keySet()) {
            if (!after.mRunMetrics.containsKey(name)) {
                warn(String.format("Run metric \"%s\" only in before-patch run.", name));
            }
        }

        for (String name : after.mRunMetrics.keySet()) {
            if (!mRunMetrics.containsKey(name)) {
                warn(String.format("Run metric \"%s\" only in after-patch run.", name));
            }
        }

        for (Pair<TestDescription, String> id : mTestMetrics.keySet()) {
            if (!after.mTestMetrics.containsKey(id)) {
                warn(
                        String.format(
                                "Test %s metric \"%s\" only in before-patch run.",
                                id.first, id.second));
            }
        }

        for (Pair<TestDescription, String> id : after.mTestMetrics.keySet()) {
            if (!mTestMetrics.containsKey(id)) {
                warn(
                        String.format(
                                "Test %s metric \"%s\" only in after-patch run.",
                                id.first, id.second));
            }
        }
    }

    @VisibleForTesting
    void error(String msg) {
        if (mStrictMode) {
            throw new MetricsException(msg);
        } else {
            CLog.e(msg);
        }
    }

    @VisibleForTesting
    void warn(String msg) {
        if (mStrictMode) {
            throw new MetricsException(msg);
        } else {
            CLog.w(msg);
        }
    }

    /**
     * Gets the number of test runs stored in this object.
     *
     * @return number of test runs
     */
    public int getNumRuns() {
        return mNumRuns;
    }

    /**
     * Gets the number of tests stored in this object.
     *
     * @return number of tests
     */
    public int getNumTests() {
        return mNumTests;
    }

    /**
     * Gets all run metrics stored in this object.
     *
     * @return a {@link MultiMap} from test name String to Double
     */
    public MultiMap<String, Double> getRunMetrics() {
        return mRunMetrics;
    }

    /**
     * Gets all test metrics stored in this object.
     *
     * @return a {@link MultiMap} from (TestDescription, test name) pair to Double
     */
    public MultiMap<Pair<TestDescription, String>, Double> getTestMetrics() {
        return mTestMetrics;
    }
}
