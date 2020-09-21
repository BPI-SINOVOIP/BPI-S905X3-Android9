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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.metricregression.DetectRegression.TableRow;
import com.android.tradefed.util.MultiMap;

import com.google.common.primitives.Doubles;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.ArgumentCaptor;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

/** Unit tests for {@link DetectRegression}. */
@RunWith(JUnit4.class)
public class DetectRegressionTest {

    private static final double EPS = 0.0001;

    @Test
    public void testCalcMean() {
        Map<Double, double[]> data = new HashMap<>();
        data.put(2.5, new double[] {1, 2, 3, 4});
        data.put(
                4.5,
                new double[] {
                    -11, 22, 5, 4, 2.5,
                });
        data.forEach(
                (k, v) -> {
                    assertTrue(equal(k, DetectRegression.calcMean(Doubles.asList(v))));
                });
    }

    @Test
    public void testCalcStdDev() {
        Map<Double, double[]> data = new HashMap<>();
        data.put(36.331000536732, new double[] {12.3, 56.7, 45.6, 124, 56});
        data.put(119.99906922093, new double[] {123.4, 22.5, 5.67, 4.56, 2.5, 333});
        data.forEach(
                (k, v) -> {
                    assertTrue(equal(k, DetectRegression.calcStdDev(Doubles.asList(v))));
                });
    }

    @Test
    public void testDetectRegression() {
        List<List<Double>> befores =
                Arrays.stream(
                                new double[][] {
                                    {3, 3, 3, 3, 3},
                                    {3, 5, 3, 5, 4},
                                    {3, 4, 3, 4, 3},
                                    {1, 2, 3, 2, 1},
                                    {-1, -2, -3, 0, 1, 2, 3},
                                    {5, 6, 5, 6, 6, 5, 7},
                                })
                        .map(Doubles::asList)
                        .collect(Collectors.toList());
        List<List<Double>> afters =
                Arrays.stream(
                                new double[][] {
                                    {3, 3, 3, 3, 3},
                                    {2, 3, 4, 5, 6},
                                    {2, 5, 2, 5, 2},
                                    {10, 11, 12, 13, 14},
                                    {-10, -20, -30, 0, 10, 20, 30},
                                    {5, 6, 5, 6, 6, 5, 700},
                                })
                        .map(Doubles::asList)
                        .collect(Collectors.toList());
        boolean[] result = {false, false, true, true, true, false};

        for (int i = 0; i < result.length; i++) {
            assertEquals(
                    result[i], DetectRegression.computeRegression(befores.get(i), afters.get(i)));
        }
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testRunRegressionDetection() {
        DetectRegression detector = spy(DetectRegression.class);
        doNothing().when(detector).logResult(any(), any(), any(), any());
        TestDescription id1 = new TestDescription("class", "test1");
        TestDescription id2 = new TestDescription("class", "test2");
        Metrics before = new Metrics(false);
        Arrays.asList("3.0", "3.0", "3.0", "3.0", "3.0")
                .forEach(e -> before.addRunMetric("metric-1", e));
        Arrays.asList("3.1", "3.3", "3.1", "3.2", "3.3")
                .forEach(e -> before.addRunMetric("metric-2", e));
        Arrays.asList("5.1", "5.2", "5.1", "5.2", "5.1")
                .forEach(e -> before.addRunMetric("metric-3", e));
        Arrays.asList("3.0", "3.0", "3.0", "3.0", "3.0")
                .forEach(e -> before.addTestMetric(id1, "metric-4", e));
        Arrays.asList("3.1", "3.3", "3.1", "3.2", "3.3")
                .forEach(e -> before.addTestMetric(id2, "metric-5", e));
        Arrays.asList("5.1", "5.2", "5.1", "5.2", "5.1")
                .forEach(e -> before.addTestMetric(id2, "metric-6", e));

        Metrics after = new Metrics(false);
        Arrays.asList("3.0", "3.0", "3.0", "3.0", "3.0")
                .forEach(e -> after.addRunMetric("metric-1", e));
        Arrays.asList("3.2", "3.2", "3.2", "3.2", "3.2")
                .forEach(e -> after.addRunMetric("metric-2", e));
        Arrays.asList("8.1", "8.2", "8.1", "8.2", "8.1")
                .forEach(e -> after.addRunMetric("metric-3", e));
        Arrays.asList("3.0", "3.0", "3.0", "3.0", "3.0")
                .forEach(e -> after.addTestMetric(id1, "metric-4", e));
        Arrays.asList("3.2", "3.2", "3.2", "3.2", "3.2")
                .forEach(e -> after.addTestMetric(id2, "metric-5", e));
        Arrays.asList("8.1", "8.2", "8.1", "8.2", "8.1")
                .forEach(e -> after.addTestMetric(id2, "metric-6", e));

        ArgumentCaptor<List<TableRow>> runResultCaptor = ArgumentCaptor.forClass(List.class);
        ArgumentCaptor<MultiMap<String, TableRow>> testResultCaptor =
                ArgumentCaptor.forClass(MultiMap.class);
        detector.runRegressionDetection(before, after);
        verify(detector, times(1))
                .logResult(
                        eq(before),
                        eq(after),
                        runResultCaptor.capture(),
                        testResultCaptor.capture());

        List<TableRow> runResults = runResultCaptor.getValue();
        assertEquals(1, runResults.size());
        assertEquals("metric-3", runResults.get(0).name);

        MultiMap<String, TableRow> testResults = testResultCaptor.getValue();
        assertEquals(1, testResults.size());
        assertEquals(1, testResults.get(id2.toString()).size());
        assertEquals("metric-6", testResults.get(id2.toString()).get(0).name);
    }

    private boolean equal(double d1, double d2) {
        return Math.abs(d1 - d2) < EPS;
    }
}
