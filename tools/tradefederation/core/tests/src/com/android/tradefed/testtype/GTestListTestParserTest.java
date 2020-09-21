/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.tradefed.testtype;

import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;

import org.easymock.EasyMock;

import java.util.HashMap;
import java.util.List;

/**
 * Test {@link GTestListTestParser}
 */
public class GTestListTestParserTest extends GTestParserTestBase {

    /**
     * Tests the parser for a test run output with 1 class and 23 tests.
     */
    @SuppressWarnings("unchecked")
    public void testParseSimpleList() throws Exception {
        String[] contents =  readInFile(GTEST_LIST_FILE_1);
        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mockRunListener.testRunStarted(TEST_MODULE_NAME, 23);
        // 11 passing test cases in this run
        for (int i = 0; i < 23; ++i) {
            mockRunListener.testStarted((TestDescription) EasyMock.anyObject());
            mockRunListener.testEnded(
                    (TestDescription) EasyMock.anyObject(),
                    (HashMap<String, Metric>) EasyMock.anyObject());
        }
        mockRunListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mockRunListener);
        GTestListTestParser parser = new GTestListTestParser(TEST_MODULE_NAME, mockRunListener);
        parser.processNewLines(contents);
        parser.flush();
        EasyMock.verify(mockRunListener);
        verifyTestDescriptions(parser.mTests, 1);
    }

    /**
     * Tests the parser for a test run output with 29 classes and 127 tests.
     */
    @SuppressWarnings("unchecked")
    public void testParseMultiClassList() throws Exception {
        String[] contents =  readInFile(GTEST_LIST_FILE_2);
        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mockRunListener.testRunStarted(TEST_MODULE_NAME, 127);
        // 11 passing test cases in this run
        for (int i = 0; i < 127; ++i) {
            mockRunListener.testStarted((TestDescription) EasyMock.anyObject());
            mockRunListener.testEnded(
                    (TestDescription) EasyMock.anyObject(),
                    (HashMap<String, Metric>) EasyMock.anyObject());
        }
        mockRunListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mockRunListener);
        GTestListTestParser parser = new GTestListTestParser(TEST_MODULE_NAME, mockRunListener);
        parser.processNewLines(contents);
        parser.flush();
        EasyMock.verify(mockRunListener);
        verifyTestDescriptions(parser.mTests, 29);
    }

    /**
     * Tests the parser against a malformed list of tests.
     */
    public void testParseMalformedList() throws Exception {
        String[] contents =  readInFile(GTEST_LIST_FILE_3);
        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        GTestListTestParser parser = new GTestListTestParser(TEST_MODULE_NAME, mockRunListener);
        try {
            parser.processNewLines(contents);
            parser.flush();
            fail("Expected IllegalStateException not thrown");
        } catch (IllegalStateException ise) {
            // expected
        }
    }

    /** Tests that test cases with special characters like "/" are still parsed properly. */
    public void testParseSimpleList_withSpecialChar() throws Exception {
        String[] contents = readInFile(GTEST_LIST_FILE_4);
        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mockRunListener.testRunStarted(TEST_MODULE_NAME, 2);
        TestDescription test1 =
                new TestDescription(
                        "SPM/AAudioOutputStreamCallbackTest",
                        "SD/AAudioStreamBuilderDirectionTest");
        mockRunListener.testStarted(test1);
        mockRunListener.testEnded(
                EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());

        TestDescription test2 =
                new TestDescription(
                        "SPM/AAudioOutputStreamCallbackTest",
                        "testPlayback/SHARED__0__LOW_LATENCY");
        mockRunListener.testStarted(test2);
        mockRunListener.testEnded(
                EasyMock.eq(test2), (HashMap<String, Metric>) EasyMock.anyObject());

        mockRunListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mockRunListener);
        GTestListTestParser parser = new GTestListTestParser(TEST_MODULE_NAME, mockRunListener);
        parser.processNewLines(contents);
        parser.flush();
        EasyMock.verify(mockRunListener);
        verifyTestDescriptions(parser.mTests, 1);
    }

    private void verifyTestDescriptions(List<TestDescription> tests, int classesExpected)
            throws Exception {
        int classesFound = 0;
        String lastClass = "notaclass";
        for (TestDescription test : tests) {
            String className = test.getClassName();
            String methodName = test.getTestName();
            assertFalse(String.format("Class name %s improperly formatted", className),
                    className.matches("^.*\\.$")); // should not end with '.'
            assertFalse(String.format("Method name %s improperly formatted", methodName),
                    methodName.matches("^\\s+.*")); // should not begin with whitespace
            if (!className.equals(lastClass)) {
                lastClass = className;
                classesFound++;
            }
        }
        assertEquals(classesExpected, classesFound);
    }
}
