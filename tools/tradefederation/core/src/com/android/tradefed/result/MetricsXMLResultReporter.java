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

import com.android.ddmlib.Log;
import com.android.ddmlib.Log.LogLevel;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;

import com.google.common.annotations.VisibleForTesting;

import org.kxml2.io.KXmlSerializer;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.Map;
import java.util.TimeZone;

/**
 * MetricsXMLResultReporter writes test metrics and run metrics to an XML file in a folder specified
 * by metrics-folder parameter at the invocationEnded phase of the test. The XML file will be piped
 * into an algorithm to detect regression.
 *
 * <p>All k-v paris in run metrics map will be formatted into: <runmetric name="name" value="value"
 * /> and placed under <testsuite/> tag
 *
 * <p>All k-v paris in run metrics map will be formatted into: <testmetric name="name" value="value"
 * /> and placed under <testcase/> tag, a tag nested under <testsuite/>.
 *
 * <p>A sample XML format: <testsuite name="suite" tests="1" failures="0" time="10"
 * timestamp="2017-01-01T01:00:00"> <runmetric name="sample" value="1.0" /> <testcase
 * testname="test" classname="classname" time="2"> <testmetric name="sample" value="1.0" />
 * </testcase> </testsuite>
 */
@OptionClass(alias = "metricsreporter")
public class MetricsXMLResultReporter extends CollectingTestListener {

    private static final String TAG = "MetricsXMLResultReporter";
    private static final String METRICS_PREFIX = "metrics-";
    private static final String TAG_TESTSUITE = "testsuite";
    private static final String TAG_TESTCASE = "testcase";
    private static final String TAG_RUN_METRIC = "runmetric";
    private static final String TAG_TEST_METRIC = "testmetric";
    private static final String ATTR_NAME = "name";
    private static final String ATTR_VALUE = "value";
    private static final String ATTR_TESTNAME = "testname";
    private static final String ATTR_TIME = "time";
    private static final String ATTR_FAILURES = "failures";
    private static final String ATTR_TESTS = "tests";
    private static final String ATTR_CLASSNAME = "classname";
    private static final String ATTR_TIMESTAMP = "timestamp";
    /** the XML namespace */
    private static final String NS = null;

    @Option(name = "metrics-folder", description = "The folder to save metrics files")
    private File mFolder;

    private File mLog;

    @Override
    public void invocationEnded(long elapsedTime) {
        super.invocationEnded(elapsedTime);
        if (mFolder == null) {
            Log.w(TAG, "metrics-folder not specified, unable to record metrics");
            return;
        }
        generateResults(elapsedTime);
    }

    private void generateResults(long elapsedTime) {
        String timestamp = getTimeStamp();
        OutputStream os = null;

        try {
            os = createOutputStream();
            if (os == null) {
                return;
            }
            KXmlSerializer serializer = new KXmlSerializer();
            serializer.setOutput(os, "UTF-8");
            serializer.startDocument("UTF-8", null);
            serializer.setFeature("http://xmlpull.org/v1/doc/features.html#indent-output", true);
            printRunResults(serializer, timestamp, elapsedTime);
            serializer.endDocument();
            if (mLog != null) {
                String msg =
                        String.format(
                                Locale.US,
                                "XML metrics report generated at %s. "
                                        + "Total tests %d, Failed %d",
                                mLog.getPath(),
                                getNumTotalTests(),
                                getNumAllFailedTests());
                Log.logAndDisplay(LogLevel.INFO, TAG, msg);
            }
        } catch (IOException e) {
            Log.e(TAG, "Failed to generate XML metric report");
            throw new RuntimeException(e);
        } finally {
            StreamUtil.close(os);
        }
    }

    private void printRunResults(KXmlSerializer serializer, String timestamp, long elapsedTime)
            throws IOException {
        serializer.startTag(NS, TAG_TESTSUITE);
        serializer.attribute(NS, ATTR_NAME, getInvocationContext().getTestTag());
        serializer.attribute(NS, ATTR_TESTS, Integer.toString(getNumTotalTests()));
        serializer.attribute(NS, ATTR_FAILURES, Integer.toString(getNumAllFailedTests()));
        serializer.attribute(NS, ATTR_TIME, Long.toString(elapsedTime));
        serializer.attribute(NS, ATTR_TIMESTAMP, timestamp);

        for (TestRunResult runResult : getRunResults()) {
            printRunMetrics(serializer, runResult.getRunMetrics());
            Map<TestDescription, TestResult> testResults = runResult.getTestResults();
            for (TestDescription test : testResults.keySet()) {
                printTestResults(serializer, test, testResults.get(test));
            }
        }

        serializer.endTag(NS, TAG_TESTSUITE);
    }

    private void printTestResults(
            KXmlSerializer serializer, TestDescription testId, TestResult testResult)
            throws IOException {
        serializer.startTag(NS, TAG_TESTCASE);
        serializer.attribute(NS, ATTR_TESTNAME, testId.getTestName());
        serializer.attribute(NS, ATTR_CLASSNAME, testId.getClassName());
        long elapsedTime = testResult.getEndTime() - testResult.getStartTime();
        serializer.attribute(NS, ATTR_TIME, Long.toString(elapsedTime));

        printTestMetrics(serializer, testResult.getMetrics());

        if (!TestStatus.PASSED.equals(testResult.getStatus())) {
            String result = testResult.getStatus().name();
            serializer.startTag(NS, result);
            String stackText = sanitize(testResult.getStackTrace());
            serializer.text(stackText);
            serializer.endTag(NS, result);
        }

        serializer.endTag(NS, TAG_TESTCASE);
    }

    private void printRunMetrics(KXmlSerializer serializer, Map<String, String> metrics)
            throws IOException {
        for (String key : metrics.keySet()) {
            serializer.startTag(NS, TAG_RUN_METRIC);
            serializer.attribute(NS, ATTR_NAME, key);
            serializer.attribute(NS, ATTR_VALUE, metrics.get(key));
            serializer.endTag(NS, TAG_RUN_METRIC);
        }
    }

    private void printTestMetrics(KXmlSerializer serializer, Map<String, String> metrics)
            throws IOException {
        for (String key : metrics.keySet()) {
            serializer.startTag(NS, TAG_TEST_METRIC);
            serializer.attribute(NS, ATTR_NAME, key);
            serializer.attribute(NS, ATTR_VALUE, metrics.get(key));
            serializer.endTag(NS, TAG_TEST_METRIC);
        }
    }

    @VisibleForTesting
    public OutputStream createOutputStream() throws IOException {
        if (!mFolder.exists() && !mFolder.mkdirs()) {
            throw new IOException(String.format("Unable to create metrics directory: %s", mFolder));
        }
        mLog = FileUtil.createTempFile(METRICS_PREFIX, ".xml", mFolder);
        return new BufferedOutputStream(new FileOutputStream(mLog));
    }

    /** Returns the text in a format that is safe for use in an XML document. */
    private String sanitize(String text) {
        return text.replace("\0", "<\\0>");
    }

    /** Return the current timestamp as a {@link String}. */
    @VisibleForTesting
    public String getTimeStamp() {
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss");
        dateFormat.setTimeZone(TimeZone.getTimeZone("UTC"));
        dateFormat.setLenient(true);
        return dateFormat.format(new Date());
    }
}
