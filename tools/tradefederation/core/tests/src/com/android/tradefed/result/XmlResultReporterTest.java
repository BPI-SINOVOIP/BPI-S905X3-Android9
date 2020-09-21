/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import junit.framework.TestCase;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.util.HashMap;

/**
 * Unit tests for {@link XmlResultReporter}.
 */
public class XmlResultReporterTest extends TestCase {
    private static final String PATH = "path";
    private static final String URL = "url";

    private XmlResultReporter mResultReporter;
    private ByteArrayOutputStream mOutputStream;
    private ILogSaver mMockLogSaver;

    class MockLogSaver implements ILogSaver {
        @Override
        public LogFile saveLogData(String dataName, LogDataType dataType,
                InputStream dataStream) {
            return new LogFile(PATH, URL, dataType);
        }

        @Override
        public LogFile saveLogDataRaw(String dataName, LogDataType type, InputStream dataStream) {
            return new LogFile(PATH, URL, type);
        }

        @Override
        public LogFile getLogReportDir() {
            return new LogFile(PATH, URL, LogDataType.DIR);
        }

        @Override
        public void invocationStarted(IInvocationContext context) {
            // Ignore
        }

        @Override
        public void invocationEnded(long elapsedTime) {
            // Ignore
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mMockLogSaver = new MockLogSaver();

        mResultReporter = new XmlResultReporter() {
            @Override
            ByteArrayOutputStream createOutputStream() {
                mOutputStream = new ByteArrayOutputStream();
                return mOutputStream;
            }

            @Override
            String getTimestamp() {
                return "ignore";
            }
        };
        mResultReporter.setLogSaver(mMockLogSaver);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
    }

    /**
     * A simple test to ensure expected output is generated for test run with no tests.
     */
    public void testEmptyGeneration() {
        final String expectedOutput = "<?xml version='1.0' encoding='UTF-8' ?>" +
            "<testsuite name=\"test\" tests=\"0\" failures=\"0\" errors=\"0\" time=\"1\" " +
            "timestamp=\"ignore\" hostname=\"localhost\"> " +
            "<properties />" +
            "</testsuite>";
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo("1", "test"));
        context.setTestTag("test");
        mResultReporter.invocationStarted(context);
        mResultReporter.invocationEnded(1);
        assertEquals(expectedOutput, getOutput());
    }

    /**
     * A simple test to ensure expected output is generated for test run with a single passed test.
     */
    public void testSinglePass() {
        HashMap<String, Metric> emptyMap = new HashMap<>();
        final TestDescription testId = new TestDescription("FooTest", "testFoo");
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo());
        context.setTestTag("stub");
        mResultReporter.invocationStarted(context);
        mResultReporter.testRunStarted("run", 1);
        mResultReporter.testStarted(testId);
        mResultReporter.testEnded(testId, emptyMap);
        mResultReporter.testRunEnded(3, emptyMap);
        mResultReporter.invocationEnded(1);
        String output =  getOutput();
        // TODO: consider doing xml based compare
        assertTrue(output.contains("tests=\"1\" failures=\"0\" errors=\"0\""));
        final String testCaseTag = String.format("<testcase name=\"%s\" classname=\"%s\"",
                testId.getTestName(), testId.getClassName());
        assertTrue(output.contains(testCaseTag));
    }

    /**
     * A simple test to ensure expected output is generated for test run with a single failed test.
     */
    public void testSingleFail() {
        HashMap<String, Metric> emptyMap = new HashMap<>();
        final TestDescription testId = new TestDescription("FooTest", "testFoo");
        final String trace = "this is a trace";
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo());
        context.setTestTag("stub");
        mResultReporter.invocationStarted(context);
        mResultReporter.testRunStarted("run", 1);
        mResultReporter.testStarted(testId);
        mResultReporter.testFailed(testId, trace);
        mResultReporter.testEnded(testId, emptyMap);
        mResultReporter.testRunEnded(3, emptyMap);
        mResultReporter.invocationEnded(1);
        String output =  getOutput();
        // TODO: consider doing xml based compare
        assertTrue(output.contains("tests=\"1\" failures=\"1\" errors=\"0\""));
        final String testCaseTag = String.format("<testcase name=\"%s\" classname=\"%s\"",
                testId.getTestName(), testId.getClassName());
        assertTrue(output.contains(testCaseTag));
        final String failureTag = String.format("<failure>%s</failure>", trace);
        assertTrue(output.contains(failureTag));
    }

    /**
     * Gets the output produced, stripping it of extraneous whitespace characters.
     */
    private String getOutput() {
        String output = mOutputStream.toString();
        // ignore newlines and tabs whitespace
        output = output.replaceAll("[\\r\\n\\t]", "");
        // replace two ws chars with one
        return output.replaceAll("  ", " ");
    }
}
