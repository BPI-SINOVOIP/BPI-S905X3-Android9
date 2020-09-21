/*
 * Copyright (C) 2017 The Android Open Source Project
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Before;
import org.junit.Test;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;

import java.io.ByteArrayOutputStream;
import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link MetricsXMLResultReporter}. */
public class MetricsXMLResultReporterTest {
    @Spy private MetricsXMLResultReporter mResultReporter;
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

    /** A test to ensure expected output is generated for test run with no tests. */
    @Test
    public void testEmptyGeneration() {
        final String expectedOutput =
                "<?xml version='1.0' encoding='UTF-8' ?><testsuite "
                        + "name=\"test\" tests=\"0\" failures=\"0\" time=\"1\" "
                        + "timestamp=\"ignore\" />";
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo("1", "test"));
        context.setTestTag("test");
        mResultReporter.invocationStarted(context);
        mResultReporter.invocationEnded(1);
        assertEquals(expectedOutput, getOutput());
    }

    /**
     * A test to ensure expected output is generated for test run with a single passed test with one
     * pair of run metric.
     */
    @Test
    public void testRunMetrics() {
        Map<String, String> map = new HashMap<>();
        map.put("metric-1", "1.0");
        final TestDescription testId = new TestDescription("FooTest", "testFoo");
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo());
        context.setTestTag("stub");
        mResultReporter.invocationStarted(context);
        mResultReporter.testRunStarted("run", 1);
        mResultReporter.testStarted(testId);
        mResultReporter.testEnded(testId, TfMetricProtoUtil.upgradeConvert(map));
        mResultReporter.testRunEnded(3, TfMetricProtoUtil.upgradeConvert(map));
        mResultReporter.invocationEnded(1);
        String output = getOutput();
        assertTrue(output.contains("<runmetric name=\"metric-1\" value=\"1.0\" />"));
        final String testCaseTag =
                String.format(
                        "<testcase testname=\"%s\" classname=\"%s\"",
                        testId.getTestName(), testId.getClassName());
        assertTrue(output.contains(testCaseTag));
    }

    /**
     * A test to ensure expected output is generated for test run with a single passed test with one
     * pair of test metric.
     */
    @Test
    public void testTestMetrics() {
        Map<String, String> map = new HashMap<>();
        map.put("metric-1", "1.0");
        final TestDescription testId = new TestDescription("FooTest", "testFoo");
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo());
        context.setTestTag("stub");
        mResultReporter.invocationStarted(context);
        mResultReporter.testRunStarted("run", 1);
        mResultReporter.testStarted(testId);
        mResultReporter.testEnded(testId, TfMetricProtoUtil.upgradeConvert(map));
        mResultReporter.testRunEnded(3, new HashMap<String, Metric>());
        mResultReporter.invocationEnded(1);
        String output = getOutput();
        assertTrue(output.contains("<testmetric name=\"metric-1\" value=\"1.0\" />"));
        final String testCaseTag =
                String.format(
                        "<testcase testname=\"%s\" classname=\"%s\"",
                        testId.getTestName(), testId.getClassName());
        assertTrue(output.contains(testCaseTag));
    }

    /** A test to ensure expected output is generated for test run with a single failed test. */
    @Test
    public void testTestFailMetrics() {
        Map<String, String> map = new HashMap<>();
        map.put("metric-1", "1.0");
        final TestDescription testId = new TestDescription("FooTest", "testFoo");
        final String trace = "sample trace";
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo());
        context.setTestTag("stub");
        mResultReporter.invocationStarted(context);
        mResultReporter.testRunStarted("run", 1);
        mResultReporter.testStarted(testId);
        mResultReporter.testFailed(testId, trace);
        mResultReporter.testEnded(testId, TfMetricProtoUtil.upgradeConvert(map));
        mResultReporter.testRunEnded(3, new HashMap<String, Metric>());
        mResultReporter.invocationEnded(1);
        String output = getOutput();
        assertTrue(output.contains("tests=\"1\" failures=\"1\""));
        final String testCaseTag =
                String.format(
                        "<testcase testname=\"%s\" classname=\"%s\"",
                        testId.getTestName(), testId.getClassName());
        assertTrue(output.contains(testCaseTag));
        final String failureTag = String.format("<FAILURE>%s</FAILURE>", trace);
        assertTrue(output.contains(failureTag));
    }

    /** Gets the output produced, stripping it of extraneous whitespace characters. */
    private String getOutput() {
        return mOutputStream
                .toString()
                // ignore newlines and tabs whitespace
                .replaceAll("[\\r\\n\\t]", "")
                // replace two ws chars with one
                .replaceAll("\\s+", " ");
    }
}
