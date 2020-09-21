/*
 * Copyright (C) 2009 The Android Open Source Project
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

import com.android.ddmlib.Log;
import com.android.ddmlib.Log.LogLevel;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.StreamUtil;

import org.kxml2.io.KXmlSerializer;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Map;
import java.util.TimeZone;

/**
 * Writes JUnit results to an XML files in a format consistent with
 * Ant's XMLJUnitResultFormatter.
 * <p/>
 * Unlike Ant's formatter, this class does not report the execution time of
 * tests.
 * <p/>
 * Collects all test info in memory, then dumps to file when invocation is complete.
 * <p/>
 * Ported from dalvik runner XmlReportPrinter.
 * <p/>
 * Result files will be stored in path constructed via [--output-file-path]/[build_id]
 */
@OptionClass(alias = "xml")
public class XmlResultReporter extends CollectingTestListener implements ILogSaverListener {

    private static final String LOG_TAG = "XmlResultReporter";

    private static final String TEST_RESULT_FILE_PREFIX = "test_result_";

    private static final String TESTSUITE = "testsuite";
    private static final String TESTCASE = "testcase";
    private static final String ERROR = "error";
    private static final String FAILURE = "failure";
    private static final String ATTR_NAME = "name";
    private static final String ATTR_TIME = "time";
    private static final String ATTR_ERRORS = "errors";
    private static final String ATTR_FAILURES = "failures";
    private static final String ATTR_TESTS = "tests";
    //private static final String ATTR_TYPE = "type";
    //private static final String ATTR_MESSAGE = "message";
    private static final String PROPERTIES = "properties";
    private static final String ATTR_CLASSNAME = "classname";
    private static final String TIMESTAMP = "timestamp";
    private static final String HOSTNAME = "hostname";

    /** the XML namespace */
    private static final String NS = null;

    private ILogSaver mLogSaver;

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationEnded(long elapsedTime) {
        super.invocationEnded(elapsedTime);
        generateSummary(elapsedTime);
    }

    @Override
    public void testFailed(TestDescription test, String trace) {
        super.testFailed(test, trace);
        CLog.d("%s : %s", test, trace);
    }

    /**
     * Creates a report file and populates it with the report data from the completed tests.
     */
    private void generateSummary(long elapsedTime) {
        String timestamp = getTimestamp();

        ByteArrayOutputStream outputStream = null;
        InputStream inputStream = null;

        try {
            outputStream = createOutputStream();
            KXmlSerializer serializer = new KXmlSerializer();
            serializer.setOutput(outputStream, "UTF-8");
            serializer.startDocument("UTF-8", null);
            serializer.setFeature(
                    "http://xmlpull.org/v1/doc/features.html#indent-output", true);
            // TODO: insert build info
            printTestResults(serializer, timestamp, elapsedTime);
            serializer.endDocument();

            inputStream = new ByteArrayInputStream(outputStream.toByteArray());
            LogFile log = mLogSaver.saveLogData(TEST_RESULT_FILE_PREFIX, LogDataType.XML,
                    inputStream);

            String msg = String.format("XML test result file generated at %s. Total tests %d, " +
                    "Failed %d", log.getPath(), getNumTotalTests(), getNumAllFailedTests());
            Log.logAndDisplay(LogLevel.INFO, LOG_TAG, msg);
        } catch (IOException e) {
            Log.e(LOG_TAG, "Failed to generate report data");
            // TODO: consider throwing exception
        } finally {
            StreamUtil.close(outputStream);
            StreamUtil.close(inputStream);
        }
    }

    /**
     * Return the current timestamp as a {@link String}.
     */
    String getTimestamp() {
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss");
        TimeZone gmt = TimeZone.getTimeZone("UTC");
        dateFormat.setTimeZone(gmt);
        dateFormat.setLenient(true);
        String timestamp = dateFormat.format(new Date());
        return timestamp;
    }

    /**
     * Creates the output stream to use for test results. Exposed for mocking.
     */
    ByteArrayOutputStream createOutputStream() {
        return new ByteArrayOutputStream();
    }

    void printTestResults(KXmlSerializer serializer, String timestamp, long elapsedTime)
            throws IOException {
        serializer.startTag(NS, TESTSUITE);
        serializer.attribute(NS, ATTR_NAME, getInvocationContext().getTestTag());
        serializer.attribute(NS, ATTR_TESTS, Integer.toString(getNumTotalTests()));
        serializer.attribute(
                NS, ATTR_FAILURES, Integer.toString(getNumTestsInState(TestStatus.FAILURE)));
        serializer.attribute(NS, ATTR_ERRORS, "0");
        serializer.attribute(NS, ATTR_TIME, Long.toString(elapsedTime));
        serializer.attribute(NS, TIMESTAMP, timestamp);
        serializer.attribute(NS, HOSTNAME, "localhost");
        serializer.startTag(NS, PROPERTIES);
        serializer.endTag(NS, PROPERTIES);

        for (TestRunResult runResult : getRunResults()) {
            // TODO: add test run summaries as TESTSUITES ?
            Map<TestDescription, TestResult> testResults = runResult.getTestResults();
            for (Map.Entry<TestDescription, TestResult> testEntry : testResults.entrySet()) {
                print(serializer, testEntry.getKey(), testEntry.getValue());
            }
        }

        serializer.endTag(NS, TESTSUITE);
    }

    void print(KXmlSerializer serializer, TestDescription testId, TestResult testResult)
            throws IOException {

        serializer.startTag(NS, TESTCASE);
        serializer.attribute(NS, ATTR_NAME, testId.getTestName());
        serializer.attribute(NS, ATTR_CLASSNAME, testId.getClassName());
        serializer.attribute(NS, ATTR_TIME, "0");

        if (!TestStatus.PASSED.equals(testResult.getStatus())) {
            String result = testResult.getStatus().equals(TestStatus.FAILURE) ? FAILURE : ERROR;
            serializer.startTag(NS, result);
            // TODO: get message of stack trace ?
//            String msg = testResult.getStackTrace();
//            if (msg != null && msg.length() > 0) {
//                serializer.attribute(ns, ATTR_MESSAGE, msg);
//            }
           // TODO: get class name of stackTrace exception
            //serializer.attribute(ns, ATTR_TYPE, testId.getClassName());
            String stackText = sanitize(testResult.getStackTrace());
            serializer.text(stackText);
            serializer.endTag(NS, result);
        }

        serializer.endTag(NS, TESTCASE);
     }

    /**
     * Returns the text in a format that is safe for use in an XML document.
     */
    private String sanitize(String text) {
        return text.replace("\0", "<\\0>");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
        // Ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLogSaved(String dataName, LogDataType dataType, InputStreamSource dataStream,
            LogFile logFile) {
        Log.logAndDisplay(LogLevel.INFO, LOG_TAG, String.format("Saved %s log to %s", dataName,
                logFile.getPath()));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setLogSaver(ILogSaver logSaver) {
        mLogSaver = logSaver;
    }
}
