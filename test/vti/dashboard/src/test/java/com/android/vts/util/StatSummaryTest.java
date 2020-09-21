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

import static org.junit.Assert.*;

import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import java.util.Random;
import org.junit.Before;
import org.junit.Test;

public class StatSummaryTest {
    private static double threshold = 0.0000000001;
    private StatSummary test;

    @Before
    public void setUp() {
        test = new StatSummary("label", VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DECREASING);
    }

    /** Test computation of average. */
    @Test
    public void testAverage() {
        int n = 1000;
        double mean = (n - 1) / 2.0;
        for (int i = 0; i < n; i++) {
            test.updateStats(i);
        }
        assertEquals(n, test.getCount(), threshold);
        assertEquals(mean, test.getMean(), threshold);
    }

    /** Test computation of minimum. */
    @Test
    public void testMin() {
        double min = Double.MAX_VALUE;
        int n = 1000;
        Random rand = new Random();
        for (int i = 0; i < n; i++) {
            double value = rand.nextInt(1000);
            if (value < min) min = value;
            test.updateStats(value);
        }
        assertEquals(n, test.getCount(), threshold);
        assertEquals(min, test.getMin(), threshold);
    }

    /** Test computation of maximum. */
    @Test
    public void testMax() {
        double max = Double.MIN_VALUE;
        int n = 1000;
        Random rand = new Random();
        for (int i = 0; i < n; i++) {
            double value = rand.nextInt(1000);
            if (value > max) max = value;
            test.updateStats(value);
        }
        assertEquals(max, test.getMax(), threshold);
    }

    /** Test computation of standard deviation. */
    @Test
    public void testStd() {
        int n = 1000;
        double[] values = new double[n];
        Random rand = new Random();
        double sum = 0.0;
        for (int i = 0; i < n; i++) {
            values[i] = rand.nextInt(1000);
            sum += values[i];
            test.updateStats(values[i]);
        }
        double mean = sum / n;
        double sumSq = 0;
        for (int i = 0; i < n; i++) {
            sumSq += (values[i] - mean) * (values[i] - mean);
        }
        double std = Math.sqrt(sumSq / (n - 1));
        assertEquals(std, test.getStd(), threshold);
    }

    /** Test computation of standard deviation. */
    @Test
    public void testMerge() {
        StatSummary test2 =
                new StatSummary("label", VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DECREASING);
        StatSummary all =
                new StatSummary("label", VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DECREASING);
        int n1 = 1000;
        int n2 = 2000;
        Random rand = new Random();
        for (int i = 0; i < n1; i++) {
            int value = rand.nextInt(1000);
            test.updateStats(value);
            all.updateStats(value);
        }
        for (int i = 0; i < n2; i++) {
            int value = rand.nextInt(1000);
            test2.updateStats(value);
            all.updateStats(value);
        }

        test.merge(test2);
        assertEquals(all.getCount(), test.getCount());
        assertEquals(all.getStd(), test.getStd(), threshold);
        assertEquals(all.getMean(), test.getMean(), threshold);
    }
}
