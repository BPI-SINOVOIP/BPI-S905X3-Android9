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

package com.android.tradefed.util;

import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.MetricsXMLResultReporter;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.metricregression.Metrics;
import com.android.tradefed.util.MetricsXmlParser.ParseException;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import com.google.common.collect.ImmutableSet;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/** Simple unit tests for {@link MetricsXmlParser}. */
@RunWith(JUnit4.class)
public class MetricsXmlParserTest {

    @Spy private MetricsXMLResultReporter mResultReporter;
    @Mock private Metrics mMetrics;
    private ByteArrayOutputStream mOutputStream;

    @Before
    public void setUp() throws Exception {
        mOutputStream = new ByteArrayOutputStream();
        MockitoAnnotations.initMocks(this);
        OptionSetter optionSetter = new OptionSetter(mResultReporter);
        optionSetter.setOptionValue("metrics-folder", "/tmp");
        doReturn(mOutputStream).when(mResultReporter).createOutputStream();
        doReturn("ignore").when(mResultReporter).getTimeStamp();
    }

    /** Test behavior when data to parse is empty */
    @Test
    public void testEmptyParse() {
        try {
            MetricsXmlParser.parse(
                    mMetrics, Collections.emptySet(), new ByteArrayInputStream(new byte[0]));
            fail("ParseException not thrown");
        } catch (ParseException e) {
            // expected
        }
        Mockito.verifyZeroInteractions(mMetrics);
    }

    /** Simple success test for xml parsing */
    @Test
    public void testSimpleParse() throws ParseException {
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo());
        context.setTestTag("stub");
        mResultReporter.invocationStarted(context);
        mResultReporter.testRunStarted("run", 3);
        final TestDescription testId0 = new TestDescription("Test", "pass1");
        mResultReporter.testStarted(testId0);
        mResultReporter.testEnded(testId0, new HashMap<String, Metric>());
        final TestDescription testId1 = new TestDescription("Test", "pass2");
        mResultReporter.testStarted(testId1);
        mResultReporter.testEnded(testId1, new HashMap<String, Metric>());
        final TestDescription testId2 = new TestDescription("Test", "pass3");
        mResultReporter.testStarted(testId2);
        mResultReporter.testEnded(testId2, new HashMap<String, Metric>());
        mResultReporter.testRunEnded(3, new HashMap<String, Metric>());
        mResultReporter.invocationEnded(5);

        MetricsXmlParser.parse(
                mMetrics, Collections.emptySet(), new ByteArrayInputStream(getOutput()));
        verify(mMetrics).setNumTests(3);
        verify(mMetrics).addRunMetric("time", "5");
        verify(mMetrics, times(0)).addTestMetric(any(), anyString(), anyString());
        Mockito.verifyNoMoreInteractions(mMetrics);
    }

    /** Test parsing a comprehensive document containing run metrics and test metrics */
    @Test
    public void testParse() throws ParseException {
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo());
        context.setTestTag("stub");
        mResultReporter.invocationStarted(context);
        mResultReporter.testRunStarted("run", 2);

        final TestDescription testId0 = new TestDescription("Test", "pass1");
        mResultReporter.testStarted(testId0);
        Map<String, String> testMetrics0 = new HashMap<>();
        testMetrics0.put("metric1", "1.1");
        mResultReporter.testEnded(testId0, TfMetricProtoUtil.upgradeConvert(testMetrics0));

        final TestDescription testId1 = new TestDescription("Test", "pass2");
        mResultReporter.testStarted(testId1);
        Map<String, String> testMetrics1 = new HashMap<>();
        testMetrics1.put("metric2", "5.5");
        mResultReporter.testEnded(testId1, TfMetricProtoUtil.upgradeConvert(testMetrics1));

        Map<String, String> runMetrics = new HashMap<>();
        runMetrics.put("metric3", "8.8");
        mResultReporter.testRunEnded(3, TfMetricProtoUtil.upgradeConvert(runMetrics));
        mResultReporter.invocationEnded(5);

        MetricsXmlParser.parse(
                mMetrics, Collections.emptySet(), new ByteArrayInputStream(getOutput()));

        verify(mMetrics).setNumTests(2);
        verify(mMetrics).addRunMetric("metric3", "8.8");
        verify(mMetrics).addTestMetric(testId0, "metric1", "1.1");
        verify(mMetrics).addTestMetric(testId1, "metric2", "5.5");
    }

    /** Test parsing a document with blacklist metrics */
    @Test
    public void testParseBlacklist() throws ParseException {
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo());
        context.setTestTag("stub");
        mResultReporter.invocationStarted(context);
        mResultReporter.testRunStarted("run", 3);

        final TestDescription testId0 = new TestDescription("Test", "pass1");
        mResultReporter.testStarted(testId0);
        Map<String, String> testMetrics0 = new HashMap<>();
        testMetrics0.put("metric1", "1.1");
        mResultReporter.testEnded(testId0, TfMetricProtoUtil.upgradeConvert(testMetrics0));

        final TestDescription testId1 = new TestDescription("Test", "pass2");
        mResultReporter.testStarted(testId1);
        Map<String, String> testMetrics1 = new HashMap<>();
        testMetrics1.put("metric2", "5.5");
        mResultReporter.testEnded(testId1, TfMetricProtoUtil.upgradeConvert(testMetrics1));

        Map<String, String> runMetrics = new HashMap<>();
        runMetrics.put("metric3", "8.8");
        mResultReporter.testRunEnded(3, TfMetricProtoUtil.upgradeConvert(runMetrics));
        mResultReporter.invocationEnded(5);

        Set<String> blacklist = ImmutableSet.of("metric1", "metric3");

        MetricsXmlParser.parse(mMetrics, blacklist, new ByteArrayInputStream(getOutput()));

        verify(mMetrics, times(0)).addRunMetric("metric3", "8.8");
        verify(mMetrics, times(0)).addTestMetric(testId0, "metric1", "1.1");
        verify(mMetrics).addTestMetric(testId1, "metric2", "5.5");
    }

    /** Gets the output produced, stripping it of extraneous whitespace characters. */
    private byte[] getOutput() {
        return mOutputStream.toByteArray();
    }
}
