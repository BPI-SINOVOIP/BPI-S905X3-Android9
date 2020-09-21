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
package com.android.tradefed.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ILogSaverListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogFile;
import com.android.tradefed.result.TestDescription;

import org.easymock.Capture;
import org.easymock.EasyMock;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.HashMap;
import java.util.Vector;

/** Unit Tests for {@link SubprocessTestResultsParser} */
@RunWith(JUnit4.class)
public class SubprocessTestResultsParserTest {

    private static final String TEST_TYPE_DIR = "testdata";
    private static final String SUBPROC_OUTPUT_FILE_1 = "subprocess1.txt";
    private static final String SUBPROC_OUTPUT_FILE_2 = "subprocess2.txt";

    /**
     * Helper to read a file from the res/testdata directory and return its contents as a String[]
     *
     * @param filename the name of the file (without the extension) in the res/testdata directory
     * @return a String[] of the
     */
    private String[] readInFile(String filename) {
        Vector<String> fileContents = new Vector<String>();
        try {
            InputStream gtestResultStream1 = getClass().getResourceAsStream(File.separator +
                    TEST_TYPE_DIR + File.separator + filename);
            BufferedReader reader = new BufferedReader(new InputStreamReader(gtestResultStream1));
            String line = null;
            while ((line = reader.readLine()) != null) {
                fileContents.add(line);
            }
        }
        catch (NullPointerException e) {
            CLog.e("Gest output file does not exist: " + filename);
        }
        catch (IOException e) {
            CLog.e("Unable to read contents of gtest output file: " + filename);
        }
        return fileContents.toArray(new String[fileContents.size()]);
    }

    /** Tests the parser for cases of test failed, ignored, assumption failure */
    @Test
    public void testParse_randomEvents() throws Exception {
        String[] contents = readInFile(SUBPROC_OUTPUT_FILE_1);
        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mockRunListener.testRunStarted("arm64-v8a CtsGestureTestCases", 4);
        mockRunListener.testStarted((TestDescription) EasyMock.anyObject(), EasyMock.anyLong());
        EasyMock.expectLastCall().times(4);
        mockRunListener.testEnded(
                (TestDescription) EasyMock.anyObject(),
                EasyMock.anyLong(),
                (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(4);
        mockRunListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
        mockRunListener.testIgnored((TestDescription) EasyMock.anyObject());
        EasyMock.expectLastCall();
        mockRunListener.testFailed(
                (TestDescription) EasyMock.anyObject(), (String) EasyMock.anyObject());
        EasyMock.expectLastCall();
        mockRunListener.testAssumptionFailure(
                (TestDescription) EasyMock.anyObject(), (String) EasyMock.anyObject());
        EasyMock.expectLastCall();
        EasyMock.replay(mockRunListener);
        File tmp = FileUtil.createTempFile("sub", "unit");
        SubprocessTestResultsParser resultParser = null;
        try {
            resultParser =
                    new SubprocessTestResultsParser(mockRunListener, new InvocationContext());
            resultParser.processNewLines(contents);
            EasyMock.verify(mockRunListener);
        } finally {
            StreamUtil.close(resultParser);
            FileUtil.deleteFile(tmp);
        }
    }

    /** Tests the parser for cases of test starting without closing. */
    @Test
    public void testParse_invalidEventOrder() throws Exception {
        String[] contents =  readInFile(SUBPROC_OUTPUT_FILE_2);
        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mockRunListener.testRunStarted("arm64-v8a CtsGestureTestCases", 4);
        mockRunListener.testStarted((TestDescription) EasyMock.anyObject(), EasyMock.anyLong());
        EasyMock.expectLastCall().times(4);
        mockRunListener.testEnded(
                (TestDescription) EasyMock.anyObject(),
                EasyMock.anyLong(),
                (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(3);
        mockRunListener.testRunFailed((String)EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
        mockRunListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
        mockRunListener.testIgnored((TestDescription) EasyMock.anyObject());
        EasyMock.expectLastCall();
        mockRunListener.testAssumptionFailure(
                (TestDescription) EasyMock.anyObject(), (String) EasyMock.anyObject());
        EasyMock.expectLastCall();
        EasyMock.replay(mockRunListener);
        File tmp = FileUtil.createTempFile("sub", "unit");
        SubprocessTestResultsParser resultParser = null;
        try {
            resultParser =
                    new SubprocessTestResultsParser(mockRunListener, new InvocationContext());
            resultParser.processNewLines(contents);
            EasyMock.verify(mockRunListener);
        } finally {
            StreamUtil.close(resultParser);
            FileUtil.deleteFile(tmp);
        }
    }

    /** Tests the parser for cases of test starting without closing. */
    @Test
    public void testParse_testNotStarted() throws Exception {
        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mockRunListener.testRunStarted("arm64-v8a CtsGestureTestCases", 4);
        mockRunListener.testEnded(
                (TestDescription) EasyMock.anyObject(),
                EasyMock.anyLong(),
                (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
        EasyMock.replay(mockRunListener);
        File tmp = FileUtil.createTempFile("sub", "unit");
        SubprocessTestResultsParser resultParser = null;
        try {
            resultParser =
                    new SubprocessTestResultsParser(mockRunListener, new InvocationContext());
            String startRun =
                    "TEST_RUN_STARTED {\"testCount\":4,\"runName\":\"arm64-v8a "
                            + "CtsGestureTestCases\"}\n";
            FileUtil.writeToFile(startRun, tmp, true);
            String testEnded =
                    "03-22 14:04:02 E/SubprocessResultsReporter: TEST_ENDED "
                            + "{\"end_time\":1489160958359,\"className\":\"android.gesture.cts."
                            + "GestureLibraryTest\",\"testName\":\"testGetGestures\",\"extra\":\""
                            + "data\"}\n";
            FileUtil.writeToFile(testEnded, tmp, true);
            resultParser.parseFile(tmp);
            EasyMock.verify(mockRunListener);
        } finally {
            StreamUtil.close(resultParser);
            FileUtil.deleteFile(tmp);
        }
    }

    /** Tests the parser for a cases when there is no start/end time stamp. */
    @Test
    public void testParse_noTimeStamp() throws Exception {
        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mockRunListener.testRunStarted("arm64-v8a CtsGestureTestCases", 4);
        mockRunListener.testStarted(EasyMock.anyObject());
        mockRunListener.testEnded(
                (TestDescription) EasyMock.anyObject(),
                (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
        EasyMock.replay(mockRunListener);
        File tmp = FileUtil.createTempFile("sub", "unit");
        SubprocessTestResultsParser resultParser = null;
        try {
            resultParser =
                    new SubprocessTestResultsParser(mockRunListener, new InvocationContext());
            String startRun = "TEST_RUN_STARTED {\"testCount\":4,\"runName\":\"arm64-v8a "
                    + "CtsGestureTestCases\"}\n";
            FileUtil.writeToFile(startRun, tmp, true);
            String testStarted =
                    "03-22 14:04:02 E/SubprocessResultsReporter: TEST_STARTED "
                            + "{\"className\":\"android.gesture.cts."
                            + "GestureLibraryTest\",\"testName\":\"testGetGestures\"}\n";
            FileUtil.writeToFile(testStarted, tmp, true);
            String testEnded =
                    "03-22 14:04:02 E/SubprocessResultsReporter: TEST_ENDED "
                            + "{\"className\":\"android.gesture.cts."
                            + "GestureLibraryTest\",\"testName\":\"testGetGestures\",\"extra\":\""
                            + "data\"}\n";
            FileUtil.writeToFile(testEnded, tmp, true);
            resultParser.parseFile(tmp);
            EasyMock.verify(mockRunListener);
        } finally {
            StreamUtil.close(resultParser);
            FileUtil.deleteFile(tmp);
        }
    }

    /** Test injecting an invocation failure and verify the callback is called. */
    @Test
    public void testParse_invocationFailed() throws Exception {
        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        Capture<Throwable> cap = new Capture<Throwable>();
        mockRunListener.invocationFailed((EasyMock.capture(cap)));
        EasyMock.replay(mockRunListener);
        File tmp = FileUtil.createTempFile("sub", "unit");
        SubprocessTestResultsParser resultParser = null;
        try {
            resultParser =
                    new SubprocessTestResultsParser(mockRunListener, new InvocationContext());
            String cause = "com.android.tradefed.targetprep."
                    + "TargetSetupError: Not all target preparation steps completed\n\tat "
                    + "com.android.compatibility.common.tradefed.targetprep."
                    + "ApkInstrumentationPreparer.run(ApkInstrumentationPreparer.java:88)\n";
            String startRun = "03-23 11:50:12 E/SubprocessResultsReporter: "
                    + "INVOCATION_FAILED {\"cause\":\"com.android.tradefed.targetprep."
                    + "TargetSetupError: Not all target preparation steps completed\\n\\tat "
                    + "com.android.compatibility.common.tradefed.targetprep."
                    + "ApkInstrumentationPreparer.run(ApkInstrumentationPreparer.java:88)\\n\"}\n";
            FileUtil.writeToFile(startRun, tmp, true);
            resultParser.parseFile(tmp);
            EasyMock.verify(mockRunListener);
            String expected = cap.getValue().getMessage();
            assertEquals(cause, expected);
        } finally {
            StreamUtil.close(resultParser);
            FileUtil.deleteFile(tmp);
        }
    }

    /** Report results when received from socket. */
    @Test
    public void testParser_receiveFromSocket() throws Exception {
        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mockRunListener.testRunStarted("arm64-v8a CtsGestureTestCases", 4);
        mockRunListener.testEnded(
                (TestDescription) EasyMock.anyObject(),
                EasyMock.anyLong(),
                (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
        EasyMock.replay(mockRunListener);
        SubprocessTestResultsParser resultParser = null;
        Socket socket = null;
        try {
            resultParser =
                    new SubprocessTestResultsParser(mockRunListener, true, new InvocationContext());
            socket = new Socket("localhost", resultParser.getSocketServerPort());
            if (!socket.isConnected()) {
                fail("socket did not connect");
            }
            PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
            String startRun = "TEST_RUN_STARTED {\"testCount\":4,\"runName\":\"arm64-v8a "
                    + "CtsGestureTestCases\"}\n";
            out.print(startRun);
            out.flush();
            String testEnded =
                    "03-22 14:04:02 E/SubprocessResultsReporter: TEST_ENDED "
                            + "{\"end_time\":1489160958359,\"className\":\"android.gesture.cts."
                            + "GestureLibraryTest\",\"testName\":\"testGetGestures\",\"extra\":\""
                            + "data\"}\n";
            out.print(testEnded);
            out.flush();
            StreamUtil.close(socket);
            assertTrue(resultParser.joinReceiver(500));
            EasyMock.verify(mockRunListener);
        } finally {
            StreamUtil.close(resultParser);
            StreamUtil.close(socket);
        }
    }

    /** When the receiver thread fails to join then an exception is thrown. */
    @Test
    public void testParser_failToJoin() throws Exception {
        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        EasyMock.replay(mockRunListener);
        SubprocessTestResultsParser resultParser = null;
        try {
            resultParser =
                    new SubprocessTestResultsParser(mockRunListener, true, new InvocationContext());
            assertFalse(resultParser.joinReceiver(50));
            EasyMock.verify(mockRunListener);
        } finally {
            StreamUtil.close(resultParser);
        }
    }

    /** Tests the parser receiving event on updating test tag. */
    @Test
    public void testParse_testTag() throws Exception {
        final String subTestTag = "test_tag_in_subprocess";
        InvocationContext context = new InvocationContext();
        context.setTestTag("stub");

        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        EasyMock.replay(mockRunListener);
        File tmp = FileUtil.createTempFile("sub", "unit");
        SubprocessTestResultsParser resultParser = null;
        try {
            resultParser = new SubprocessTestResultsParser(mockRunListener, false, context);
            String testTagEvent =
                    String.format(
                            "INVOCATION_STARTED {\"testTag\": \"%s\",\"start_time\":250}",
                            subTestTag);
            FileUtil.writeToFile(testTagEvent, tmp, true);
            resultParser.parseFile(tmp);
            EasyMock.verify(mockRunListener);
            assertEquals(subTestTag, context.getTestTag());
            assertEquals(250l, resultParser.getStartTime().longValue());
        } finally {
            StreamUtil.close(resultParser);
            FileUtil.deleteFile(tmp);
        }
    }

    /** Tests the parser should not overwrite the test tag in parent process if it's already set. */
    @Test
    public void testParse_testTagNotOverwrite() throws Exception {
        final String subTestTag = "test_tag_in_subprocess";
        final String parentTestTag = "test_tag_in_parent_process";
        InvocationContext context = new InvocationContext();
        context.setTestTag(parentTestTag);

        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        EasyMock.replay(mockRunListener);
        File tmp = FileUtil.createTempFile("sub", "unit");
        SubprocessTestResultsParser resultParser = null;
        try {
            resultParser = new SubprocessTestResultsParser(mockRunListener, false, context);
            String testTagEvent = String.format("TEST_TAG %s", subTestTag);
            FileUtil.writeToFile(testTagEvent, tmp, true);
            resultParser.parseFile(tmp);
            EasyMock.verify(mockRunListener);
            assertEquals(parentTestTag, context.getTestTag());
        } finally {
            StreamUtil.close(resultParser);
            FileUtil.deleteFile(tmp);
        }
    }

    /** Test that module start and end is properly parsed when reported. */
    @Test
    public void testParse_moduleStarted_end() throws Exception {
        ITestInvocationListener mockRunListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mockRunListener.testModuleStarted(EasyMock.anyObject());
        mockRunListener.testModuleEnded();
        EasyMock.replay(mockRunListener);
        IInvocationContext fakeModuleContext = new InvocationContext();
        File tmp = FileUtil.createTempFile("sub", "unit");
        SubprocessTestResultsParser resultParser = null;
        File serializedModule = null;
        try {
            serializedModule = SerializationUtil.serialize(fakeModuleContext);
            resultParser =
                    new SubprocessTestResultsParser(mockRunListener, new InvocationContext());
            String moduleStart =
                    String.format(
                            "TEST_MODULE_STARTED {\"moduleContextFileName\":\"%s\"}\n",
                            serializedModule.getAbsolutePath());
            FileUtil.writeToFile(moduleStart, tmp, true);
            String moduleEnd = "TEST_MODULE_ENDED {}\n";
            FileUtil.writeToFile(moduleEnd, tmp, true);

            resultParser.parseFile(tmp);
            EasyMock.verify(mockRunListener);
        } finally {
            StreamUtil.close(resultParser);
            FileUtil.deleteFile(tmp);
            FileUtil.deleteFile(serializedModule);
        }
    }

    /** Test that logAssociation event is properly passed and parsed. */
    @Test
    public void testParse_logAssociation() throws Exception {
        ILogSaverListener mockRunListener = EasyMock.createMock(ILogSaverListener.class);
        Capture<LogFile> capture = new Capture<>();
        mockRunListener.logAssociation(EasyMock.eq("dataname"), EasyMock.capture(capture));
        EasyMock.replay(mockRunListener);
        LogFile logFile = new LogFile("path", "url", LogDataType.TEXT);
        File serializedLogFile = null;
        File tmp = FileUtil.createTempFile("sub", "unit");
        SubprocessTestResultsParser resultParser = null;
        try {
            serializedLogFile = SerializationUtil.serialize(logFile);
            resultParser =
                    new SubprocessTestResultsParser(mockRunListener, new InvocationContext());
            String logAssocation =
                    String.format(
                            "LOG_ASSOCIATION {\"loggedFile\":\"%s\",\"dataName\":\"dataname\"}\n",
                            serializedLogFile.getAbsolutePath());
            FileUtil.writeToFile(logAssocation, tmp, true);
            resultParser.parseFile(tmp);
            EasyMock.verify(mockRunListener);
        } finally {
            StreamUtil.close(resultParser);
            FileUtil.deleteFile(serializedLogFile);
            FileUtil.deleteFile(tmp);
        }
        LogFile received = capture.getValue();
        assertEquals(logFile.getPath(), received.getPath());
        assertEquals(logFile.getUrl(), received.getUrl());
        assertEquals(logFile.getType(), received.getType());
    }
}
