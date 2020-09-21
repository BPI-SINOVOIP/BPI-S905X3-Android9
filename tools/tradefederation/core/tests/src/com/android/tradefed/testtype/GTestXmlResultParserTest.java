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

import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.FileUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;

/** Unit tests for {@link GTestXmlResultParser} */
public class GTestXmlResultParserTest extends TestCase {
    private static final String TEST_TYPE_DIR = "testtype";
    private static final String TEST_MODULE_NAME = "module";
    private static final String GTEST_OUTPUT_FILE_1 = "gtest_output1.xml";
    private static final String GTEST_OUTPUT_FILE_2 = "gtest_output2.xml";
    private static final String GTEST_OUTPUT_FILE_3 = "gtest_output3.xml";
    private static final String GTEST_OUTPUT_FILE_4 = "gtest_output4.xml";
    private static final String GTEST_OUTPUT_FILE_5 = "gtest_output5.xml";

    /**
     * Helper to read a file from the res/testtype directory and return the associated {@link File}
     *
     * @param filename the name of the file (without the extension) in the res/testtype directory
     * @return a File to the output
     */
    private File readInFile(String filename) {
        InputStream gtest_output = null;
        File res = null;
        FileOutputStream out = null;
        try {
            gtest_output = getClass().getResourceAsStream(File.separator +
                    TEST_TYPE_DIR + File.separator + filename);
            res = FileUtil.createTempFile("unit_gtest_", ".xml") ;
            out = new FileOutputStream(res);
            byte[] buffer = new byte[1024];
            int byteRead;
            while ((byteRead = gtest_output.read(buffer)) != -1) {
                out.write(buffer, 0, byteRead);
            }
            out.close();
        }
        catch (NullPointerException | IOException e) {
            CLog.e("Gest output file does not exist: " + filename);
        }
        return res;
    }

    /**
     * Tests the parser for a simple test run output with 6 tests.
     */
    @SuppressWarnings("unchecked")
    public void testParseSimpleFile() throws Exception {
        File contents =  readInFile(GTEST_OUTPUT_FILE_1);
        try {
            ITestInvocationListener mockRunListener =
                    EasyMock.createMock(ITestInvocationListener.class);
            mockRunListener.testRunStarted(TEST_MODULE_NAME, 6);
            // 6 passing test cases in this run
            for (int i=0; i<6; ++i) {
                mockRunListener.testStarted((TestDescription) EasyMock.anyObject());
                mockRunListener.testEnded(
                        (TestDescription) EasyMock.anyObject(),
                        (HashMap<String, Metric>) EasyMock.anyObject());
            }
            mockRunListener.testRunEnded(
                    EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
            EasyMock.replay(mockRunListener);
            GTestXmlResultParser resultParser =
                    new GTestXmlResultParser(TEST_MODULE_NAME, mockRunListener);
            resultParser.parseResult(contents, null);
            EasyMock.verify(mockRunListener);
        } finally {
            FileUtil.deleteFile(contents);
        }
    }

    /**
     * Tests the parser for a run with 84 tests.
     */
    @SuppressWarnings("unchecked")
    public void testParseLargerFile() throws Exception {
        File contents =  readInFile(GTEST_OUTPUT_FILE_2);
        try {
            ITestInvocationListener mockRunListener =
                    EasyMock.createMock(ITestInvocationListener.class);
            mockRunListener.testRunStarted(TEST_MODULE_NAME, 84);
            // 84 passing test cases in this run
            for (int i=0; i<84; ++i) {
                mockRunListener.testStarted((TestDescription) EasyMock.anyObject());
                mockRunListener.testEnded(
                        (TestDescription) EasyMock.anyObject(),
                        (HashMap<String, Metric>) EasyMock.anyObject());
            }
            mockRunListener.testRunEnded(
                    EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
            EasyMock.replay(mockRunListener);
            GTestXmlResultParser resultParser =
                    new GTestXmlResultParser(TEST_MODULE_NAME, mockRunListener);
            resultParser.parseResult(contents, null);
            EasyMock.verify(mockRunListener);
        } finally {
            FileUtil.deleteFile(contents);
        }
    }

    /**
     * Tests the parser for a run with test failures.
     */
    @SuppressWarnings("unchecked")
    public void testParseWithFailures() throws Exception {
        String expectedMessage = "Message\nFailed";

        File contents =  readInFile(GTEST_OUTPUT_FILE_3);
        try {
            ITestInvocationListener mockRunListener =
                    EasyMock.createMock(ITestInvocationListener.class);
            mockRunListener.testRunStarted(TEST_MODULE_NAME, 7);
            // 6 passing test cases in this run
            for (int i=0; i<6; ++i) {
                mockRunListener.testStarted((TestDescription) EasyMock.anyObject());
                mockRunListener.testEnded(
                        (TestDescription) EasyMock.anyObject(),
                        (HashMap<String, Metric>) EasyMock.anyObject());
            }
            // 1 failed test
            mockRunListener.testStarted((TestDescription) EasyMock.anyObject());
            mockRunListener.testFailed(
                    (TestDescription) EasyMock.anyObject(), EasyMock.eq(expectedMessage));
            mockRunListener.testEnded(
                    (TestDescription) EasyMock.anyObject(),
                    (HashMap<String, Metric>) EasyMock.anyObject());
            mockRunListener.testRunEnded(
                    EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
            EasyMock.replay(mockRunListener);
            GTestXmlResultParser resultParser =
                    new GTestXmlResultParser(TEST_MODULE_NAME, mockRunListener);
            resultParser.parseResult(contents, null);
            EasyMock.verify(mockRunListener);
        } finally {
            FileUtil.deleteFile(contents);
        }
    }

    /**
     * Tests the parser for a run with a bad file, as if the test hadn't outputed.
     */
    @SuppressWarnings("unchecked")
    public void testParseWithEmptyFile() throws Exception {
        String expected = "Failed to get an xml output from tests, it probably crashed";

        File contents = FileUtil.createTempFile("test", ".xml");
        try {
            ITestInvocationListener mockRunListener =
                    EasyMock.createMock(ITestInvocationListener.class);
            mockRunListener.testRunStarted(TEST_MODULE_NAME, 0);
            mockRunListener.testRunFailed(expected);
            mockRunListener.testRunEnded(
                    EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
            EasyMock.replay(mockRunListener);
            GTestXmlResultParser resultParser =
                    new GTestXmlResultParser(TEST_MODULE_NAME, mockRunListener);
            resultParser.parseResult(contents, null);
            EasyMock.verify(mockRunListener);
        } finally {
            FileUtil.deleteFile(contents);
        }
    }

    /**
     * Tests the parser for a simple test run output with 6 tests but report expected 7.
     */
    @SuppressWarnings("unchecked")
    public void testParseUnexpectedNumberTest() throws Exception {
        String expected = "Test run incomplete. Expected 7 tests, received 6";
        File contents =  readInFile(GTEST_OUTPUT_FILE_4);
        try {
            ITestInvocationListener mockRunListener =
                    EasyMock.createMock(ITestInvocationListener.class);
            mockRunListener.testRunStarted(TEST_MODULE_NAME, 7);
            // 6 passing test cases in this run
            for (int i=0; i<6; ++i) {
                mockRunListener.testStarted((TestDescription) EasyMock.anyObject());
                mockRunListener.testEnded(
                        (TestDescription) EasyMock.anyObject(),
                        (HashMap<String, Metric>) EasyMock.anyObject());
            }
            mockRunListener.testRunFailed(expected);
            mockRunListener.testRunEnded(
                    EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
            EasyMock.replay(mockRunListener);
            GTestXmlResultParser resultParser =
                    new GTestXmlResultParser(TEST_MODULE_NAME, mockRunListener);
            resultParser.parseResult(contents, null);
            EasyMock.verify(mockRunListener);
        } finally {
            FileUtil.deleteFile(contents);
        }
    }

    /**
     * Tests the parser for a simple test run output with 6 tests but a bad xml tag so some tests
     * won't be parsed.
     */
    @SuppressWarnings("unchecked")
    public void testParseSimpleFile_badXmltag() throws Exception {
        String expected = "Test run incomplete. Expected 6 tests, received 3";
        File contents =  readInFile(GTEST_OUTPUT_FILE_5);
        try {
            ITestInvocationListener mockRunListener =
                    EasyMock.createMock(ITestInvocationListener.class);
            mockRunListener.testRunStarted(TEST_MODULE_NAME, 6);
            // 6 passing test cases in this run
            for (int i=0; i<3; ++i) {
                mockRunListener.testStarted((TestDescription) EasyMock.anyObject());
                mockRunListener.testEnded(
                        (TestDescription) EasyMock.anyObject(),
                        (HashMap<String, Metric>) EasyMock.anyObject());
            }
            mockRunListener.testRunFailed(expected);
            mockRunListener.testRunEnded(
                    EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
            EasyMock.replay(mockRunListener);
            GTestXmlResultParser resultParser =
                    new GTestXmlResultParser(TEST_MODULE_NAME, mockRunListener);
            resultParser.parseResult(contents, null);
            EasyMock.verify(mockRunListener);
        } finally {
            FileUtil.deleteFile(contents);
        }
    }

    /**
     * Tests the parser for a run with a bad file, with Collector output to get some logs.
     */
    @SuppressWarnings("unchecked")
    public void testParseWithEmptyFile_AdditionalOutput() throws Exception {
        final String exec_log = "EXECUTION LOG";
        CollectingOutputReceiver fake = new CollectingOutputReceiver() {
            @Override
            public String getOutput() {
                return exec_log;
            }
        };
        String expected = "Failed to get an xml output from tests, it probably crashed\nlogs:\n"
                + exec_log;

        File contents = FileUtil.createTempFile("test", ".xml");
        try {
            ITestInvocationListener mockRunListener =
                    EasyMock.createMock(ITestInvocationListener.class);
            mockRunListener.testRunStarted(TEST_MODULE_NAME, 0);
            mockRunListener.testRunFailed(expected);
            mockRunListener.testRunEnded(
                    EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
            EasyMock.replay(mockRunListener);
            GTestXmlResultParser resultParser =
                    new GTestXmlResultParser(TEST_MODULE_NAME, mockRunListener);
            resultParser.parseResult(contents, fake);
            EasyMock.verify(mockRunListener);
        } finally {
            FileUtil.deleteFile(contents);
        }
    }
}
