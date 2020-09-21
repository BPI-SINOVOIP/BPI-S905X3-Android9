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
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

/**
 * Parses the 'xml output mode' results of native tests using GTest that run from shell,
 * and informs a ITestRunListener of the results.
 */
public class GTestXmlResultParser {

    private final static String TEST_SUITE_TAG = "testsuite";
    private final static String TEST_CASE_TAG = "testcase";

    private final String mTestRunName;
    private int mNumTestsRun = 0;
    private int mNumTestsExpected = 0;
    private long mTotalRunTime = 0;
    private final Collection<ITestInvocationListener> mTestListeners;

    /**
     * Creates the GTestXmlResultParser.
     *
     * @param testRunName the test run name to provide to {@link
     *     ITestInvocationListener#testRunStarted(String, int)}
     * @param listeners informed of test results as the tests are executing
     */
    public GTestXmlResultParser(String testRunName, Collection<ITestInvocationListener> listeners) {
        mTestRunName = testRunName;
        mTestListeners = new ArrayList<>(listeners);
    }

    /**
     * Creates the GTestXmlResultParser for a single listener.
     *
     * @param testRunName the test run name to provide to {@link
     *     ITestInvocationListener#testRunStarted(String, int)}
     * @param listener informed of test results as the tests are executing
     */
    public GTestXmlResultParser(String testRunName, ITestInvocationListener listener) {
        mTestRunName = testRunName;
        mTestListeners = new ArrayList<>();
        if (listener != null) {
            mTestListeners.add(listener);
        }
    }

    /**
     * Parse the xml results
     * @param f {@link File} containing the outputed xml
     * @param output The output collected from the execution run to complete the logs if necessary
     */
    public void parseResult(File f, CollectingOutputReceiver output) {
        DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
        Document result = null;
        try {
            DocumentBuilder db = dbf.newDocumentBuilder();
            db.setErrorHandler(new DefaultHandler());
            result = db.parse(f);
        } catch (SAXException | IOException | ParserConfigurationException e) {
            reportTestRunStarted();
            for (ITestInvocationListener listener : mTestListeners) {
                String errorMessage = String.format("Failed to get an xml output from tests,"
                        + " it probably crashed");
                if (output != null) {
                    errorMessage += "\nlogs:\n" + output.getOutput();
                    CLog.e(errorMessage);
                }
                listener.testRunFailed(errorMessage);
                listener.testRunEnded(mTotalRunTime, new HashMap<String, Metric>());
            }
            return;
        }
        Element rootNode = result.getDocumentElement();
        // Parse root node: "testsuites" for generic infos.
        getTestSuitesInfo(rootNode);
        reportTestRunStarted();
        // Iterate other "testsuite" for each test results.
        NodeList testSuiteList = rootNode.getElementsByTagName(TEST_SUITE_TAG);
        if (testSuiteList != null && testSuiteList.getLength() > 0) {
            for (int i = 0; i < testSuiteList.getLength() ; i++) {
                NodeList testcasesList =
                        ((Element)testSuiteList.item(i)).getElementsByTagName(TEST_CASE_TAG);
                // Iterate other the test cases in the test suite.
                if (testcasesList != null && testcasesList.getLength() > 0) {
                    for (int j = 0 ; j < testcasesList.getLength(); j++) {
                        processTestResult((Element)testcasesList.item(j));
                    }
                }
            }
        }

        if (mNumTestsExpected > mNumTestsRun) {
            for (ITestInvocationListener listener : mTestListeners) {
                listener.testRunFailed(
                        String.format("Test run incomplete. Expected %d tests, received %d",
                        mNumTestsExpected, mNumTestsRun));
            }
        }
        for (ITestInvocationListener listener : mTestListeners) {
            listener.testRunEnded(mTotalRunTime, new HashMap<String, Metric>());
        }
    }

    private void getTestSuitesInfo(Element rootNode) {
        mNumTestsExpected = Integer.parseInt(rootNode.getAttribute("tests"));
        mTotalRunTime = (long) (Double.parseDouble(rootNode.getAttribute("time")) * 1000d);
    }

    /**
     * Reports the start of a test run, and the total test count, if it has not been previously
     * reported.
     */
    private void reportTestRunStarted() {
        for (ITestInvocationListener listener : mTestListeners) {
            listener.testRunStarted(mTestRunName, mNumTestsExpected);
        }
    }

    /**
     * Processes and informs listener when we encounter a tag indicating that a test has started.
     *
     * @param testcase Raw log output of the form classname.testname, with an optional time (x ms)
     */
    private void processTestResult(Element testcase) {
        String classname = testcase.getAttribute("classname");
        String testname = testcase.getAttribute("name");
        String runtime = testcase.getAttribute("time");
        ParsedTestInfo parsedResults = new ParsedTestInfo(classname, testname, runtime);
        TestDescription testId =
                new TestDescription(parsedResults.mTestClassName, parsedResults.mTestName);
        mNumTestsRun++;
        for (ITestInvocationListener listener : mTestListeners) {
            listener.testStarted(testId);
        }

        // If there is a failure tag report failure
        if (testcase.getElementsByTagName("failure").getLength() != 0) {
            String trace = ((Element)testcase.getElementsByTagName("failure").item(0))
                    .getAttribute("message");
            if (!trace.contains("Failed")) {
                // For some reason, the alternative GTest format doesn't specify Failed in the
                // trace and error doesn't show properly in reporter, so adding it here.
                trace += "\nFailed";
            }
            for (ITestInvocationListener listener : mTestListeners) {
                listener.testFailed(testId, trace);
            }
        }

        Map<String, String> map = new HashMap<>();
        map.put("runtime", parsedResults.mTestRunTime);
        for (ITestInvocationListener listener : mTestListeners) {
            listener.testEnded(testId, TfMetricProtoUtil.upgradeConvert(map));
        }
    }

    /** Internal helper struct to store parsed test info. */
    private static class ParsedTestInfo {
        String mTestName = null;
        String mTestClassName = null;
        String mTestRunTime = null;

        public ParsedTestInfo(String testName, String testClassName, String testRunTime) {
            mTestName = testName;
            mTestClassName = testClassName;
            mTestRunTime = testRunTime;
        }
    }
}
