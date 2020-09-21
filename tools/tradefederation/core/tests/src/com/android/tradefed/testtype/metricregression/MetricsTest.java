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
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.Pair;

import com.google.common.primitives.Doubles;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Unit tests for {@link Metrics}. */
@RunWith(JUnit4.class)
public class MetricsTest {

    private Metrics mMetrics;

    @Before
    public void setUp() throws Exception {
        mMetrics = spy(new Metrics(false));
    }

    @Test
    public void testAddRunMetrics() {
        Map<String, List<String>> data = new HashMap<>();
        data.put("metric1", Arrays.asList("1.0", "1.1", "1.2"));
        data.put("metric2", Arrays.asList("2.0", "2.1", "2.2"));
        data.forEach((k, v) -> v.forEach(e -> mMetrics.addRunMetric(k, e)));
        assertEquals(Doubles.asList(1.0, 1.1, 1.2), mMetrics.getRunMetrics().get("metric1"));
        assertEquals(Doubles.asList(2.0, 2.1, 2.2), mMetrics.getRunMetrics().get("metric2"));
    }

    @Test
    public void testAddTestMetrics() {
        TestDescription id1 = new TestDescription("class", "test1");
        Arrays.asList("1.0", "1.1", "1.2").forEach(e -> mMetrics.addTestMetric(id1, "metric1", e));
        TestDescription id2 = new TestDescription("class", "test2");
        Arrays.asList("2.0", "2.1", "2.2").forEach(e -> mMetrics.addTestMetric(id2, "metric1", e));
        Arrays.asList("3.0", "3.1", "3.2").forEach(e -> mMetrics.addTestMetric(id2, "metric2", e));

        assertEquals(
                Doubles.asList(1.0, 1.1, 1.2),
                mMetrics.getTestMetrics().get(new Pair<>(id1, "metric1")));
        assertEquals(
                Doubles.asList(2.0, 2.1, 2.2),
                mMetrics.getTestMetrics().get(new Pair<>(id2, "metric1")));
        assertEquals(
                Doubles.asList(3.0, 3.1, 3.2),
                mMetrics.getTestMetrics().get(new Pair<>(id2, "metric2")));
    }

    @Test
    public void testValidate() {
        Map<String, List<String>> data = new HashMap<>();
        data.put("metric1", Arrays.asList("1.0", "1.1", "1.2"));
        data.put("metric2", Arrays.asList("2.0", "2.1"));
        data.forEach((k, v) -> v.forEach(e -> mMetrics.addRunMetric(k, e)));
        TestDescription id1 = new TestDescription("class", "test1");
        Arrays.asList("1.0", "1.1", "1.2").forEach(e -> mMetrics.addTestMetric(id1, "metric1", e));
        TestDescription id2 = new TestDescription("class", "test2");
        Arrays.asList("2.0", "2.1", "2.2").forEach(e -> mMetrics.addTestMetric(id2, "metric1", e));
        Arrays.asList("3.0", "3.1").forEach(e -> mMetrics.addTestMetric(id2, "metric2", e));
        mMetrics.validate(3);
        verify(mMetrics, times(2)).error(anyString());
    }

    @Test
    public void testCrossValidate() {
        Metrics other = new Metrics(false);
        Arrays.asList("1.0", "1.1", "1.2")
                .forEach(
                        e -> {
                            mMetrics.addRunMetric("metric1", e);
                            other.addRunMetric("metric1", e);
                        });
        Arrays.asList("2.0", "2.1", "2.2").forEach(e -> mMetrics.addRunMetric("metric2", e));
        Arrays.asList("2.0", "2.1", "2.2").forEach(e -> other.addRunMetric("metric5", e));
        TestDescription id1 = new TestDescription("class", "test1");
        Arrays.asList("1.0", "1.1", "1.2")
                .forEach(
                        e -> {
                            mMetrics.addTestMetric(id1, "metric1", e);
                            other.addTestMetric(id1, "metric1", e);
                        });
        Arrays.asList("3.0", "3.1", "3.3").forEach(e -> mMetrics.addTestMetric(id1, "metric6", e));
        TestDescription id2 = new TestDescription("class", "test2");
        Arrays.asList("2.0", "2.1", "2.2")
                .forEach(
                        e -> {
                            mMetrics.addTestMetric(id2, "metric1", e);
                            other.addTestMetric(id2, "metric1", e);
                        });
        Arrays.asList("3.0", "3.1", "3.3").forEach(e -> other.addTestMetric(id2, "metric2", e));
        mMetrics.crossValidate(other);
        verify(mMetrics, times(1)).warn("Run metric \"metric2\" only in before-patch run.");
        verify(mMetrics, times(1)).warn("Run metric \"metric5\" only in after-patch run.");
        verify(mMetrics, times(1))
                .warn(
                        String.format(
                                "Test %s metric \"metric6\" only in before-patch run.",
                                id1.toString()));
        verify(mMetrics, times(1))
                .warn(
                        String.format(
                                "Test %s metric \"metric2\" only in after-patch run.",
                                id2.toString()));
    }
}
