/*
 * Copyright (C) 2013 The Android Open Source Project
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

import com.android.ddmlib.testrunner.XmlTestRunListener;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.xml.AbstractXmlParser;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import java.text.NumberFormat;
import java.util.HashMap;

/**
 * Parser that extracts test result data from JUnit results stored in ant's XMLJUnitResultFormatter
 * and forwards it to a ITestInvocationListener.
 * <p/>
 * @see XmlTestRunListener
 */
public class JUnitXmlParser extends AbstractXmlParser {
    private final ITestInvocationListener mTestListener;

    /**
     * Parses the xml format. Expected tags/attributes are
     *
     * testsuite name="runname" tests="X"
     *     testcase classname="FooTest" name="testMethodName"
     *         failure type="blah" message="exception message"
     *              stack
     */
    private class JUnitXmlHandler extends DefaultHandler {

        /**
         * The total number of tests in the suite that errored. An errored test is one that had an
         * unanticipated problem, for example an unchecked throwable; or a problem with the
         * implementation of the test. optional.
         */
        private static final String ERROR_TAG = "error";
        /**
         * The total number of tests in the suite that failed. A failure is a test which the code
         * has explicitly failed by using the mechanisms for that purpose. e.g., via an
         * assertEquals. optional.
         */
        private static final String FAILURE_TAG = "failure";

        private static final String SKIPPED_TAG = "skipped";
        private static final String TESTSUITE_TAG = "testsuite";
        private static final String TESTCASE_TAG = "testcase";
        private static final String TIME_TAG = "time";
        private TestDescription mCurrentTest = null;
        private StringBuffer mFailureContent = null;
        private long mRunTime = 0L;

        /**
        * {@inheritDoc}
        */
        @Override
        public void startElement(String uri, String localName, String name, Attributes attributes)
                throws SAXException {
            mFailureContent = null;
            if (TESTSUITE_TAG.equalsIgnoreCase(name)) {
                // top level tag - maps to a test run in TF terminology
                String testSuiteName = getMandatoryAttribute(name, "name", attributes);
                String testCountString = getMandatoryAttribute(name, "tests", attributes);
                mRunTime = getTimeAttribute(name, TIME_TAG, attributes);
                int testCount = Integer.parseInt(testCountString);
                mTestListener.testRunStarted(testSuiteName, testCount);
            }
            if (TESTCASE_TAG.equalsIgnoreCase(name)) {
                // start of description of an individual test method - extract out test name and
                // store it
                String testClassName = getMandatoryAttribute(name, "classname", attributes);
                String methodName = getMandatoryAttribute(name, "name", attributes);
                mCurrentTest = new TestDescription(testClassName, methodName);
                mTestListener.testStarted(mCurrentTest);
            }
            if (SKIPPED_TAG.equalsIgnoreCase(name)) {
                if (mCurrentTest != null) {
                    mTestListener.testIgnored(mCurrentTest);
                }
            }
            if (FAILURE_TAG.equalsIgnoreCase(name) || ERROR_TAG.equalsIgnoreCase(name)) {
                // current testcase has a failure - extract out message and type and store it
                // detailed stack is CDATA, will be extracted in characters() callback
                mFailureContent = new StringBuffer();
                String message = attributes.getValue("message");
                String type = attributes.getValue("type");
                if (message != null) {
                    mFailureContent.append(message);
                    mFailureContent.append("\n");
                }
                if (type != null) {
                    mFailureContent.append(type);
                    mFailureContent.append("\n");
                }
            }
        }

        /**
        * Called when parsing CDATA content of a tag
        */
        @Override
        public void characters(char[] data, int offset, int len) {
            // if currently parsing a failure, add stack data to content
            if (mFailureContent != null) {
                mFailureContent.append(data, offset, len);
            }
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void endElement(String uri, String localName, String name) {
            if (TESTSUITE_TAG.equalsIgnoreCase(name)) {
                mTestListener.testRunEnded(mRunTime, new HashMap<String, Metric>());
            }
            if (TESTCASE_TAG.equalsIgnoreCase(name)) {
                mTestListener.testEnded(mCurrentTest, new HashMap<String, Metric>());
            }
            if (FAILURE_TAG.equalsIgnoreCase(name) || ERROR_TAG.equalsIgnoreCase(name)) {
                mTestListener.testFailed(mCurrentTest,
                        mFailureContent.toString());
            }
            mFailureContent = null;
        }

        /**
         * Retrieves an attributes value.
         *
         * @throws SAXException if attribute is not present
         */
        String getMandatoryAttribute(String tagName, String attrName, Attributes attributes)
                throws SAXException {
            String value = attributes.getValue(attrName);
            if (value == null) {
                throw new SAXException(String.format(
                        "Malformed XML, could not find '%s' attribute in '%s'", attrName, tagName));
            }
            return value;
        }

        /**
         * Parse the time attributes from the xml, and put it in milliseconds instead of seconds.
         */
        Long getTimeAttribute(String tagName, String attrName, Attributes attributes)
                throws SAXException {
            String value = attributes.getValue(attrName);
            if (value == null) {
                throw new SAXException(
                        String.format(
                                "Malformed XML, could not find '%s' attribute in '%s'",
                                attrName, tagName));
            }
            NumberFormat f = NumberFormat.getInstance();
            Number n;
            try {
                n = f.parse(value);
            } catch (java.text.ParseException e) {
                throw new SAXException(
                        String.format(
                                "Malformed XML, attribute '%s' in '%s' is not a time: '%s'",
                                attrName, tagName, value));
            }
            return (long) (n.floatValue() * 1000L);
        }
    }

    /**
     * Create a JUnitXmlParser
     *
     * @param listener the {@link ITestInvocationListener} to forward results to
     */
    public JUnitXmlParser(ITestInvocationListener listener) {
        mTestListener = listener;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected DefaultHandler createXmlHandler() {
        return new JUnitXmlHandler();
    }
}
