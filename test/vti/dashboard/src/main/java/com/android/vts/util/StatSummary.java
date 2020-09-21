/*
 * Copyright (c) 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.vts.util;

import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;

/** Helper object for storing statistics. */
public class StatSummary {
    private String label;
    private double min;
    private double max;
    private double mean;
    private double sumSq;
    private int n;
    private VtsProfilingRegressionMode regression_mode;

    /**
     * Create a statistical summary.
     *
     * <p>Sets the label, min, max, mean, sum of squared error, n, and mode as provided.
     *
     * @param label The (String) label to assign to the summary.
     * @param min The minimum observed value.
     * @param max The maximum observed value.
     * @param mean The average observed value.
     * @param sumSq The sum of squared error.
     * @param n The number of values observed.
     * @param mode The VtsProfilingRegressionMode to use when analyzing performance.
     */
    public StatSummary(
            String label,
            double min,
            double max,
            double mean,
            double sumSq,
            int n,
            VtsProfilingRegressionMode mode) {
        this.label = label;
        this.min = min;
        this.max = max;
        this.mean = mean;
        this.sumSq = sumSq;
        this.n = n;
        this.regression_mode = mode;
    }

    /**
     * Initializes the statistical summary.
     *
     * <p>Sets the label as provided. Initializes the mean, variance, and n (number of values seen)
     * to 0.
     *
     * @param label The (String) label to assign to the summary.
     * @param mode The VtsProfilingRegressionMode to use when analyzing performance.
     */
    public StatSummary(String label, VtsProfilingRegressionMode mode) {
        this(label, Double.MAX_VALUE, Double.MIN_VALUE, 0, 0, 0, mode);
    }

    /**
     * Update the mean and variance using Welford's single-pass method.
     *
     * @param value The observed value in the stream.
     */
    public void updateStats(double value) {
        n += 1;
        double oldMean = mean;
        mean = oldMean + (value - oldMean) / n;
        sumSq = sumSq + (value - mean) * (value - oldMean);
        if (value < min) min = value;
        if (value > max) max = value;
    }

    /**
     * Combine the mean and variance with another StatSummary.
     *
     * @param stat The StatSummary to combine with.
     */
    public void merge(StatSummary stat) {
        double delta = stat.getMean() - mean;
        int newN = n + stat.getCount();
        sumSq = sumSq + stat.getSumSq() + delta / newN * delta * n * stat.getCount();
        double recipN = 1.0 / newN;
        mean = n * recipN * mean + stat.getCount() * recipN * stat.getMean();
        n = newN;
    }

    /**
     * Gets the best case of the stream.
     *
     * @return The min or max.
     */
    public double getBestCase() {
        switch (regression_mode) {
            case VTS_REGRESSION_MODE_DECREASING:
                return getMax();
            default:
                return getMin();
        }
    }

    /**
     * Gets the calculated min of the stream.
     *
     * @return The min.
     */
    public double getMin() {
        return min;
    }

    /**
     * Gets the calculated max of the stream.
     *
     * @return The max.
     */
    public double getMax() {
        return max;
    }

    /**
     * Gets the calculated mean of the stream.
     *
     * @return The mean.
     */
    public double getMean() {
        return mean;
    }

    /**
     * Gets the calculated sum of squared error of the stream.
     *
     * @return The sum of squared error.
     */
    public double getSumSq() {
        return sumSq;
    }

    /**
     * Gets the calculated standard deviation of the stream.
     *
     * @return The standard deviation.
     */
    public double getStd() {
        return Math.sqrt(sumSq / (n - 1));
    }

    /**
     * Gets the number of elements that have passed through the stream.
     *
     * @return Number of elements.
     */
    public int getCount() {
        return n;
    }

    /**
     * Gets the label for the summarized statistics.
     *
     * @return The (string) label.
     */
    public String getLabel() {
        return label;
    }

    /**
     * Gets the regression mode.
     *
     * @return The VtsProfilingRegressionMode value.
     */
    public VtsProfilingRegressionMode getRegressionMode() {
        return regression_mode;
    }
}
