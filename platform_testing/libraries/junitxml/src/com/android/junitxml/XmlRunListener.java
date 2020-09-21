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

package com.android.junitxml;

import org.junit.Ignore;
import org.junit.runner.Description;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunListener;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Text;

import java.io.BufferedWriter;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.Properties;
import java.util.Set;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

/**
 * {@link RunListener} to write JUnit4 test results to XML in a format adapted from the schema used
 * by Ant in {@code org.apache.tools.ant.taskdefs.optional.junit.XMLJUnitResultFormatter}.
 */
public class XmlRunListener extends RunListener implements XmlConstants {

    private static final double ONE_SECOND = 1000.0;

    private static final String TESTCASE_NAME_UNKNOWN = "unknown";

    private Document mDocument;

    private Element mRootElement;

    private final Hashtable<Description, Element> mTestElements = new Hashtable<>();

    private final Set<Description> mFailedTests = new HashSet<>();

    private final Set<Description> mErrorTests = new HashSet<>();

    private final Set<Description> mSkippedTests = new HashSet<>();

    private final Set<Description> mIgnoredTests = new HashSet<>();

    private final Hashtable<Description, Long> mTestStarts = new Hashtable<>();

    private OutputStream mOutputStream;

    private long mStartTime;

    private static DocumentBuilder getDocumentBuilder() {
        try {
            return DocumentBuilderFactory.newInstance().newDocumentBuilder();
        } catch (final Exception exc) {
            throw new ExceptionInInitializerError(exc);
        }
    }

    public XmlRunListener(OutputStream out, String suiteName) {
        mDocument = getDocumentBuilder().newDocument();
        mRootElement = mDocument.createElement(ELEMENT_TESTSUITE);
        mOutputStream = out;
        startTestSuite(suiteName);
    }

    private void startTestSuite(String suiteName) {
        mRootElement.setAttribute(ATTR_TESTSUITE_NAME, suiteName);

        SimpleDateFormat simpleDateFormat = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss");
        String timestamp = simpleDateFormat.format(new Date());
        mRootElement.setAttribute(ATTR_TESTSUITE_TIME, timestamp);

        mRootElement.setAttribute(ATTR_TESTSUITE_HOSTNAME, getHostname());

        Element propsElement = mDocument.createElement(ELEMENT_PROPERTIES);
        mRootElement.appendChild(propsElement);
        mStartTime = System.currentTimeMillis();
        final Properties props = System.getProperties();
        if (props != null) {
            for (Object name : props.keySet()) {
                Element propElement = mDocument.createElement(ELEMENT_PROPERTY);
                propElement.setAttribute(ATTR_PROPERTY_NAME, (String) name);
                propElement.setAttribute(ATTR_PROPERTY_VALUE, props.getProperty((String) name));
                propsElement.appendChild(propElement);
            }
        }
    }

    private String getHostname() {
        String hostname = "localhost";
        try {
            InetAddress localHost = InetAddress.getLocalHost();
            if (localHost != null) {
                hostname = localHost.getHostName();
            }
        } catch (UnknownHostException e) {
            // fall back to default 'localhost'
        }
        return hostname;
    }

    public void endTestSuite() throws IOException {
        mRootElement.setAttribute(ATTR_TESTSUITE_TESTS, "" + mTestStarts.size());
        mRootElement.setAttribute(ATTR_TESTSUITE_FAILURES, "" + mFailedTests.size());
        mRootElement.setAttribute(ATTR_TESTSUITE_ERRORS, "" + mErrorTests.size());
        mRootElement.setAttribute(ATTR_TESTSUITE_SKIPPED, "" + mSkippedTests.size());

        mRootElement.setAttribute(
                ATTR_TESTSUITE_TIME,
                "" + ((System.currentTimeMillis() - mStartTime) / ONE_SECOND));
        if (mOutputStream != null) {
            Writer writer = null;
            try {
                writer = new BufferedWriter(new OutputStreamWriter(mOutputStream, "UTF8"));

                Transformer transformer;

                transformer = TransformerFactory.newInstance().newTransformer();
                javax.xml.transform.Result output = new StreamResult(writer);
                Source input = new DOMSource(mRootElement);
                transformer.setOutputProperty(OutputKeys.INDENT, "yes");
                transformer.transform(input, output);

            } catch (final IOException | TransformerException exc) {
                throw new IOException("Unable to write log file", exc);
            } finally {
                if (writer != null) {
                    try {
                        writer.flush();
                    } catch (final IOException ex) {
                        // ignore
                    }
                    if (mOutputStream != System.out && mOutputStream != System.err) {
                        writer.close();
                    }
                }
            }
        }
    }

    @SuppressWarnings("ThrowableResultOfMethodCallIgnored")
    @Override
    public void testFailure(Failure failure) throws Exception {
        Description description = failure.getDescription();
        testFinished(description);

        if (failure.getException() instanceof AssertionError) {
            formatError(ELEMENT_FAILURE, failure);
            mFailedTests.add(description);
        } else {
            formatError(ELEMENT_ERROR, failure);
            mErrorTests.add(description);
        }
    }

    private void formatError(String type, Failure failure) throws Exception {
        final Element failureOrError = mDocument.createElement(type);
        Element currentTest;
        if (failure.getDescription() != null) {
            currentTest = mTestElements.get(failure.getDescription());
        } else {
            currentTest = mRootElement;
        }

        currentTest.appendChild(failureOrError);

        final String message = failure.getMessage();
        if (message != null && message.length() > 0) {
            failureOrError.setAttribute(ATTR_FAILURE_MESSAGE, message);
        }
        failureOrError.setAttribute(
                ATTR_FAILURE_TYPE,
                failure.getDescription().getClassName());

        final String stackTrace = failure.getTrace();
        final Text trace = mDocument.createTextNode(stackTrace);
        failureOrError.appendChild(trace);
    }

    @Override
    public void testFinished(Description description) throws Exception {
        if (!mTestStarts.containsKey(description)) {
            testStarted(description);
        }

        Element currentTest;
        if (!mFailedTests.contains(description) && !mErrorTests.contains(description)
                && !mSkippedTests.contains(description) && !mIgnoredTests
                .contains(description)) {
            currentTest = mDocument.createElement(ELEMENT_TESTCASE);
            final String displayName = description.getDisplayName();
            currentTest.setAttribute(
                    ATTR_TESTCASE_NAME,
                    displayName == null ? TESTCASE_NAME_UNKNOWN : displayName);
            // a TestSuite can contain Tests from multiple classes,
            // even tests with the same name - disambiguate them.
            currentTest.setAttribute(ATTR_TESTCASE_CLASSNAME, description.getClassName());
            mRootElement.appendChild(currentTest);
            mTestElements.put(description, currentTest);

        } else {
            currentTest = mTestElements.get(description);
        }

        final long l = mTestStarts.get(description);
        currentTest.setAttribute(
                ATTR_TESTCASE_TIME, "" + ((System.currentTimeMillis() - l) / ONE_SECOND));
    }

    @Override
    public void testStarted(Description description) throws Exception {
        mTestStarts.put(description, System.currentTimeMillis());
    }

    @Override
    public void testIgnored(Description description) throws Exception {
        Ignore ignoreAnnotation = description.getAnnotation(Ignore.class);
        formatSkip(description, ignoreAnnotation != null ? ignoreAnnotation.value() : null);
        mIgnoredTests.add(description);
    }

    @Override
    public void testAssumptionFailure(Failure failure) {
        try {
            formatSkip(failure.getDescription(), failure.getMessage());
        } catch (Exception e) {
            e.printStackTrace();
        }
        mSkippedTests.add(failure.getDescription());
    }

    private void formatSkip(Description description, String message) throws Exception {
        testFinished(description);

        final Element skippedElement = mDocument.createElement(ELEMENT_SKIPPED);
        if (message != null) {
            skippedElement.setAttribute(ATTR_SKIPPED_MESSAGE, message);
        }

        Element currentTest = mTestElements.get(description);
        currentTest.appendChild(skippedElement);
    }
}
