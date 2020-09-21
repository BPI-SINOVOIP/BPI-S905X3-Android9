/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.tradefed.result.suite;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogFile;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.testtype.Abi;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.util.FileUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

import java.io.File;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

/** Unit tests for {@link XmlSuiteResultFormatter}. */
@RunWith(JUnit4.class)
public class XmlSuiteResultFormatterTest {
    private XmlSuiteResultFormatter mFormatter;
    private SuiteResultHolder mResultHolder;
    private IInvocationContext mContext;
    private File mResultDir;

    @Before
    public void setUp() throws Exception {
        mFormatter = new XmlSuiteResultFormatter();
        mResultHolder = new SuiteResultHolder();
        mContext = new InvocationContext();
        mResultDir = FileUtil.createTempDir("result-dir");
    }

    @After
    public void tearDown() throws Exception {
        FileUtil.recursiveDelete(mResultDir);
    }

    /** Check that the basic overall structure is good an contains all the information. */
    @Test
    public void testBasicFormat() throws Exception {
        mResultHolder.context = mContext;

        Collection<TestRunResult> runResults = new ArrayList<>();
        runResults.add(createFakeResult("module1", 2, 0));
        runResults.add(createFakeResult("module2", 1, 0));
        mResultHolder.runResults = runResults;

        Map<String, IAbi> modulesAbi = new HashMap<>();
        modulesAbi.put("module1", new Abi("armeabi-v7a", "32"));
        modulesAbi.put("module2", new Abi("armeabi-v7a", "32"));
        mResultHolder.modulesAbi = modulesAbi;

        mResultHolder.completeModules = 2;
        mResultHolder.totalModules = 2;
        mResultHolder.passedTests = 2;
        mResultHolder.failedTests = 0;
        mResultHolder.startTime = 0L;
        mResultHolder.endTime = 10L;
        File res = mFormatter.writeResults(mResultHolder, mResultDir);
        String content = FileUtil.readStringFromFile(res);
        assertXmlContainsNode(content, "Result");
        // Verify that the summary has been populated
        assertXmlContainsNode(content, "Result/Summary");
        assertXmlContainsAttribute(content, "Result/Summary", "pass", "2");
        assertXmlContainsAttribute(content, "Result/Summary", "failed", "0");
        assertXmlContainsAttribute(content, "Result/Summary", "modules_done", "2");
        assertXmlContainsAttribute(content, "Result/Summary", "modules_total", "2");
        // Verify that each module results are available
        assertXmlContainsNode(content, "Result/Module");
        assertXmlContainsAttribute(content, "Result/Module", "name", "module1");
        assertXmlContainsAttribute(content, "Result/Module", "abi", "armeabi-v7a");
        assertXmlContainsAttribute(content, "Result/Module", "runtime", "10");
        assertXmlContainsAttribute(content, "Result/Module", "done", "true");
        assertXmlContainsAttribute(content, "Result/Module", "pass", "2");
        // Verify the test cases that passed are present
        assertXmlContainsNode(content, "Result/Module/TestCase");
        assertXmlContainsAttribute(content, "Result/Module/TestCase", "name", "com.class.module1");
        assertXmlContainsAttribute(
                content, "Result/Module/TestCase/Test", "name", "module1.method0");
        assertXmlContainsAttribute(
                content, "Result/Module/TestCase/Test", "name", "module1.method1");

        assertXmlContainsAttribute(content, "Result/Module", "name", "module2");
        assertXmlContainsAttribute(content, "Result/Module", "pass", "1");
        assertXmlContainsAttribute(content, "Result/Module/TestCase", "name", "com.class.module2");
        assertXmlContainsAttribute(
                content, "Result/Module/TestCase/Test", "name", "module2.method0");
    }

    /** Check that the test failures are properly reported. */
    @Test
    public void testFailuresReporting() throws Exception {
        mResultHolder.context = mContext;

        Collection<TestRunResult> runResults = new ArrayList<>();
        runResults.add(createFakeResult("module1", 2, 1));
        mResultHolder.runResults = runResults;

        Map<String, IAbi> modulesAbi = new HashMap<>();
        modulesAbi.put("module1", new Abi("armeabi-v7a", "32"));
        mResultHolder.modulesAbi = modulesAbi;

        mResultHolder.completeModules = 2;
        mResultHolder.totalModules = 1;
        mResultHolder.passedTests = 2;
        mResultHolder.failedTests = 1;
        mResultHolder.startTime = 0L;
        mResultHolder.endTime = 10L;
        File res = mFormatter.writeResults(mResultHolder, mResultDir);
        String content = FileUtil.readStringFromFile(res);

        assertXmlContainsNode(content, "Result/Module");
        assertXmlContainsAttribute(content, "Result/Module/TestCase", "name", "com.class.module1");
        assertXmlContainsAttribute(
                content, "Result/Module/TestCase/Test", "name", "module1.method0");
        assertXmlContainsAttribute(
                content, "Result/Module/TestCase/Test", "name", "module1.method1");
        // Check that failures are showing in the xml for the test cases
        assertXmlContainsAttribute(
                content, "Result/Module/TestCase/Test", "name", "module1.failed0");
        assertXmlContainsAttribute(
                content, "Result/Module/TestCase/Test/Failure", "message", "module1 failed.");
        assertXmlContainsValue(
                content,
                "Result/Module/TestCase/Test/Failure/StackTrace",
                "module1 failed.\nstack\nstack");
        // Test that we can read back the informations
        SuiteResultHolder holder = mFormatter.parseResults(mResultDir);
        assertEquals(holder.completeModules, mResultHolder.completeModules);
        assertEquals(holder.totalModules, mResultHolder.totalModules);
        assertEquals(holder.passedTests, mResultHolder.passedTests);
        assertEquals(holder.failedTests, mResultHolder.failedTests);
        assertEquals(holder.startTime, mResultHolder.startTime);
        assertEquals(holder.endTime, mResultHolder.endTime);
        assertEquals(
                holder.modulesAbi.get("armeabi-v7a module1"),
                mResultHolder.modulesAbi.get("module1"));
        assertEquals(holder.runResults.size(), mResultHolder.runResults.size());
    }

    /** Check that the logs for each test case are reported. */
    @Test
    public void testLogReporting() throws Exception {
        mResultHolder.context = mContext;

        Collection<TestRunResult> runResults = new ArrayList<>();
        runResults.add(createResultWithLog("armeabi-v7a module1", 1, LogDataType.LOGCAT));
        runResults.add(createResultWithLog("module2", 1, LogDataType.BUGREPORT));
        runResults.add(createResultWithLog("module3", 1, LogDataType.PNG));
        mResultHolder.runResults = runResults;

        Map<String, IAbi> modulesAbi = new HashMap<>();
        modulesAbi.put("armeabi-v7a module1", new Abi("armeabi-v7a", "32"));
        mResultHolder.modulesAbi = modulesAbi;

        mResultHolder.completeModules = 2;
        mResultHolder.totalModules = 2;
        mResultHolder.passedTests = 2;
        mResultHolder.failedTests = 0;
        mResultHolder.startTime = 0L;
        mResultHolder.endTime = 10L;
        File res = mFormatter.writeResults(mResultHolder, mResultDir);
        String content = FileUtil.readStringFromFile(res);
        // One logcat and one bugreport are found in the report
        assertXmlContainsValue(
                content, "Result/Module/TestCase/Test/Logcat", "http:url/armeabi-v7a module1");
        assertXmlContainsValue(
                content, "Result/Module/TestCase/Test/BugReport", "http:url/module2");
        assertXmlContainsValue(
                content, "Result/Module/TestCase/Test/Screenshot", "http:url/module3");

        // Test that we can read back the informations for log files
        SuiteResultHolder holder = mFormatter.parseResults(mResultDir);
        assertEquals(
                holder.modulesAbi.get("armeabi-v7a module1"),
                mResultHolder.modulesAbi.get("armeabi-v7a module1"));
        assertEquals(holder.runResults.size(), mResultHolder.runResults.size());
        for (TestRunResult result : holder.runResults) {
            TestDescription description =
                    new TestDescription(
                            "com.class." + result.getName(), result.getName() + ".method0");
            // Check that we reloaded the logged files.
            assertTrue(
                    result.getTestResults()
                                    .get(description)
                                    .getLoggedFiles()
                                    .get(result.getName() + "log0")
                            != null);
        }
    }

    private TestRunResult createResultWithLog(String runName, int count, LogDataType type) {
        TestRunResult fakeRes = new TestRunResult();
        fakeRes.testRunStarted(runName, count);
        for (int i = 0; i < count; i++) {
            TestDescription description =
                    new TestDescription("com.class." + runName, runName + ".method" + i);
            fakeRes.testStarted(description);
            fakeRes.testLogSaved(
                    runName + "log" + i, new LogFile("path", "http:url/" + runName, type));
            fakeRes.testEnded(description, new HashMap<String, Metric>());
        }
        fakeRes.testRunEnded(10L, new HashMap<String, String>());
        return fakeRes;
    }

    private TestRunResult createFakeResult(String runName, int passed, int failed) {
        TestRunResult fakeRes = new TestRunResult();
        fakeRes.testRunStarted(runName, passed + failed);
        for (int i = 0; i < passed; i++) {
            TestDescription description =
                    new TestDescription("com.class." + runName, runName + ".method" + i);
            fakeRes.testStarted(description);
            fakeRes.testEnded(description, new HashMap<String, Metric>());
        }
        for (int i = 0; i < failed; i++) {
            TestDescription description =
                    new TestDescription("com.class." + runName, runName + ".failed" + i);
            fakeRes.testStarted(description);
            fakeRes.testFailed(description, runName + " failed.\nstack\nstack");
            fakeRes.testEnded(description, new HashMap<String, Metric>());
        }
        fakeRes.testRunEnded(10L, new HashMap<String, Metric>());
        return fakeRes;
    }

    /** Return all XML nodes that match the given xPathExpression. */
    private NodeList getXmlNodes(String xml, String xPathExpression)
            throws XPathExpressionException {

        InputSource inputSource = new InputSource(new StringReader(xml));
        XPath xpath = XPathFactory.newInstance().newXPath();
        return (NodeList) xpath.evaluate(xPathExpression, inputSource, XPathConstants.NODESET);
    }

    /** Assert that the XML contains a node matching the given xPathExpression. */
    private NodeList assertXmlContainsNode(String xml, String xPathExpression)
            throws XPathExpressionException {
        NodeList nodes = getXmlNodes(xml, xPathExpression);
        assertNotNull(
                String.format("XML '%s' returned null for xpath '%s'.", xml, xPathExpression),
                nodes);
        assertTrue(
                String.format(
                        "XML '%s' should have returned at least 1 node for xpath '%s', "
                                + "but returned %s nodes instead.",
                        xml, xPathExpression, nodes.getLength()),
                nodes.getLength() >= 1);
        return nodes;
    }

    /**
     * Assert that the XML contains a node matching the given xPathExpression and that the node has
     * a given value.
     */
    private void assertXmlContainsValue(String xml, String xPathExpression, String value)
            throws XPathExpressionException {
        NodeList nodes = assertXmlContainsNode(xml, xPathExpression);
        boolean found = false;

        for (int i = 0; i < nodes.getLength(); i++) {
            Element element = (Element) nodes.item(i);
            if (element.getTextContent().equals(value)) {
                found = true;
                break;
            }
        }

        assertTrue(
                String.format(
                        "xPath '%s' should contain value '%s' but does not. XML: '%s'",
                        xPathExpression, value, xml),
                found);
    }

    /**
     * Assert that the XML contains a node matching the given xPathExpression and that the node has
     * a given value.
     */
    private void assertXmlContainsAttribute(
            String xml, String xPathExpression, String attributeName, String attributeValue)
            throws XPathExpressionException {
        NodeList nodes = assertXmlContainsNode(xml, xPathExpression);
        boolean found = false;

        for (int i = 0; i < nodes.getLength(); i++) {
            Element element = (Element) nodes.item(i);
            String value = element.getAttribute(attributeName);
            if (attributeValue.equals(value)) {
                found = true;
                break;
            }
        }

        assertTrue(
                String.format(
                        "xPath '%s' should contain attribute '%s' but does not. XML: '%s'",
                        xPathExpression, attributeName, xml),
                found);
    }
}
