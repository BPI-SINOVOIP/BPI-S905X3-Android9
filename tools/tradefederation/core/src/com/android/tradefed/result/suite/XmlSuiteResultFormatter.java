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

import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogFile;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.testtype.Abi;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.suite.TestFailureListener;
import com.android.tradefed.util.AbiUtils;
import com.android.tradefed.util.StreamUtil;

import com.google.common.base.Joiner;
import com.google.common.base.Strings;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;
import org.xmlpull.v1.XmlSerializer;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Date;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Utility class to save a suite run as an XML. TODO: Remove all the special Compatibility Test
 * format work around to get the same format.
 */
public class XmlSuiteResultFormatter implements IFormatterGenerator {

    private static final String ENCODING = "UTF-8";
    private static final String TYPE = "org.kxml2.io.KXmlParser,org.kxml2.io.KXmlSerializer";
    public static final String NS = null;

    public static final String TEST_RESULT_FILE_NAME = "test_result_suite.xml";

    // XML constants
    private static final String ABI_ATTR = "abi";
    private static final String BUGREPORT_TAG = "BugReport";
    private static final String BUILD_TAG = "Build";
    private static final String CASE_TAG = "TestCase";
    private static final String COMMAND_LINE_ARGS = "command_line_args";
    private static final String DEVICES_ATTR = "devices";
    private static final String DONE_ATTR = "done";
    private static final String END_DISPLAY_TIME_ATTR = "end_display";
    private static final String END_TIME_ATTR = "end";
    private static final String FAILED_ATTR = "failed";
    private static final String FAILURE_TAG = "Failure";
    private static final String HOST_NAME_ATTR = "host_name";
    private static final String JAVA_VENDOR_ATTR = "java_vendor";
    private static final String JAVA_VERSION_ATTR = "java_version";
    private static final String LOGCAT_TAG = "Logcat";

    private static final String METRIC_TAG = "Metric";
    private static final String MESSAGE_ATTR = "message";
    private static final String MODULE_TAG = "Module";
    private static final String MODULES_DONE_ATTR = "modules_done";
    private static final String MODULES_TOTAL_ATTR = "modules_total";
    private static final String NAME_ATTR = "name";
    private static final String OS_ARCH_ATTR = "os_arch";
    private static final String OS_NAME_ATTR = "os_name";
    private static final String OS_VERSION_ATTR = "os_version";
    private static final String PASS_ATTR = "pass";

    private static final String RESULT_ATTR = "result";
    private static final String RESULT_TAG = "Result";
    private static final String RUNTIME_ATTR = "runtime";
    private static final String SCREENSHOT_TAG = "Screenshot";
    private static final String SKIPPED_ATTR = "skipped";
    private static final String STACK_TAG = "StackTrace";
    private static final String START_DISPLAY_TIME_ATTR = "start_display";
    private static final String START_TIME_ATTR = "start";

    private static final String SUMMARY_TAG = "Summary";
    private static final String TEST_TAG = "Test";
    private static final String TOTAL_TESTS_ATTR = "total_tests";

    private static final String LOG_FILE_NAME_ATTR = "file_name";

    /**
     * Allows to add some attributes to the <Result> tag via {@code serializer.attribute}.
     *
     * @param serializer
     * @throws IOException
     */
    public void addSuiteAttributes(XmlSerializer serializer)
            throws IllegalArgumentException, IllegalStateException, IOException {
        // Default implementation does nothing
    }

    /**
     * Reverse operation from {@link #addSuiteAttributes(XmlSerializer)}.
     *
     * @param parser The parser where to read the attributes from.
     * @param context The {@link IInvocationContext} where to put the attributes.
     * @throws XmlPullParserException
     */
    public void parseSuiteAttributes(XmlPullParser parser, IInvocationContext context)
            throws XmlPullParserException {
        // Default implementation does nothing
    }

    /**
     * Allows to add some attributes to the <Build> tag via {@code serializer.attribute}.
     *
     * @param serializer
     * @param holder
     * @throws IllegalArgumentException
     * @throws IllegalStateException
     * @throws IOException
     */
    public void addBuildInfoAttributes(XmlSerializer serializer, SuiteResultHolder holder)
            throws IllegalArgumentException, IllegalStateException, IOException {
        // Default implementation does nothing
    }

    /**
     * Reverse operation from {@link #addBuildInfoAttributes(XmlSerializer, SuiteResultHolder)}.
     *
     * @param parser The parser where to read the attributes from.
     * @param context The {@link IInvocationContext} where to put the attributes.
     * @throws XmlPullParserException
     */
    public void parseBuildInfoAttributes(XmlPullParser parser, IInvocationContext context)
            throws XmlPullParserException {
        // Default implementation does nothing
    }

    /**
     * Write the invocation results in an xml format.
     *
     * @param holder a {@link SuiteResultHolder} holding all the info required for the xml
     * @param resultDir the result directory {@link File} where to put the results.
     * @return a {@link File} pointing to the xml output file.
     */
    @Override
    public File writeResults(SuiteResultHolder holder, File resultDir) throws IOException {
        File resultFile = new File(resultDir, TEST_RESULT_FILE_NAME);
        OutputStream stream = new FileOutputStream(resultFile);
        XmlSerializer serializer = null;
        try {
            serializer = XmlPullParserFactory.newInstance(TYPE, null).newSerializer();
        } catch (XmlPullParserException e) {
            StreamUtil.close(stream);
            throw new IOException(e);
        }
        serializer.setOutput(stream, ENCODING);
        serializer.startDocument(ENCODING, false);
        serializer.setFeature("http://xmlpull.org/v1/doc/features.html#indent-output", true);
        serializer.processingInstruction(
                "xml-stylesheet type=\"text/xsl\" href=\"compatibility_result.xsl\"");
        serializer.startTag(NS, RESULT_TAG);
        serializer.attribute(NS, START_TIME_ATTR, String.valueOf(holder.startTime));
        serializer.attribute(NS, END_TIME_ATTR, String.valueOf(holder.endTime));
        serializer.attribute(NS, START_DISPLAY_TIME_ATTR, toReadableDateString(holder.startTime));
        serializer.attribute(NS, END_DISPLAY_TIME_ATTR, toReadableDateString(holder.endTime));
        serializer.attribute(
                NS,
                COMMAND_LINE_ARGS,
                Strings.nullToEmpty(
                        holder.context.getAttributes().getUniqueMap().get(COMMAND_LINE_ARGS)));

        addSuiteAttributes(serializer);

        // Device Info
        Map<Integer, List<String>> serialsShards = holder.context.getShardsSerials();
        String deviceList = "";
        if (serialsShards.isEmpty()) {
            deviceList = Joiner.on(",").join(holder.context.getSerials());
        } else {
            for (List<String> list : serialsShards.values()) {
                deviceList += Joiner.on(",").join(list);
            }
        }
        serializer.attribute(NS, DEVICES_ATTR, deviceList);

        // Host Info
        String hostName = "";
        try {
            hostName = InetAddress.getLocalHost().getHostName();
        } catch (UnknownHostException ignored) {
        }
        serializer.attribute(NS, HOST_NAME_ATTR, hostName);
        serializer.attribute(NS, OS_NAME_ATTR, System.getProperty("os.name"));
        serializer.attribute(NS, OS_VERSION_ATTR, System.getProperty("os.version"));
        serializer.attribute(NS, OS_ARCH_ATTR, System.getProperty("os.arch"));
        serializer.attribute(NS, JAVA_VENDOR_ATTR, System.getProperty("java.vendor"));
        serializer.attribute(NS, JAVA_VERSION_ATTR, System.getProperty("java.version"));

        // Build Info
        serializer.startTag(NS, BUILD_TAG);
        for (String key : holder.context.getAttributes().keySet()) {
            serializer.attribute(
                    NS, key, Joiner.on(",").join(holder.context.getAttributes().get(key)));
        }
        addBuildInfoAttributes(serializer, holder);
        serializer.endTag(NS, BUILD_TAG);

        // Summary
        serializer.startTag(NS, SUMMARY_TAG);
        serializer.attribute(NS, PASS_ATTR, Long.toString(holder.passedTests));
        serializer.attribute(NS, FAILED_ATTR, Long.toString(holder.failedTests));
        serializer.attribute(NS, MODULES_DONE_ATTR, Integer.toString(holder.completeModules));
        serializer.attribute(NS, MODULES_TOTAL_ATTR, Integer.toString(holder.totalModules));
        serializer.endTag(NS, SUMMARY_TAG);

        // Results
        for (TestRunResult module : holder.runResults) {
            serializer.startTag(NS, MODULE_TAG);
            // To be compatible of CTS strip the abi from the module name when available.
            if (holder.modulesAbi.get(module.getName()) != null) {
                String moduleAbi = holder.modulesAbi.get(module.getName()).getName();
                String moduleNameStripped = module.getName().replace(moduleAbi + " ", "");
                serializer.attribute(NS, NAME_ATTR, moduleNameStripped);
                serializer.attribute(NS, ABI_ATTR, moduleAbi);
            } else {
                serializer.attribute(NS, NAME_ATTR, module.getName());
            }
            serializer.attribute(NS, RUNTIME_ATTR, String.valueOf(module.getElapsedTime()));
            serializer.attribute(NS, DONE_ATTR, Boolean.toString(module.isRunComplete()));
            serializer.attribute(
                    NS, PASS_ATTR, Integer.toString(module.getNumTestsInState(TestStatus.PASSED)));
            serializer.attribute(NS, TOTAL_TESTS_ATTR, Integer.toString(module.getNumTests()));

            serializeTestCases(serializer, module.getTestResults());
            serializer.endTag(NS, MODULE_TAG);
        }
        serializer.endDocument();
        return resultFile;
    }

    private static void serializeTestCases(
            XmlSerializer serializer, Map<TestDescription, TestResult> results)
            throws IllegalArgumentException, IllegalStateException, IOException {
        // We reformat into the same format as the ResultHandler from CTS to be compatible for now.
        Map<String, Map<String, TestResult>> format = new LinkedHashMap<>();
        for (Entry<TestDescription, TestResult> cr : results.entrySet()) {
            if (format.get(cr.getKey().getClassName()) == null) {
                format.put(cr.getKey().getClassName(), new LinkedHashMap<>());
            }
            Map<String, TestResult> methodResult = format.get(cr.getKey().getClassName());
            methodResult.put(cr.getKey().getTestName(), cr.getValue());
        }

        for (String className : format.keySet()) {
            serializer.startTag(NS, CASE_TAG);
            serializer.attribute(NS, NAME_ATTR, className);
            for (Entry<String, TestResult> individualResult : format.get(className).entrySet()) {
                TestStatus status = individualResult.getValue().getStatus();
                if (status == null) {
                    continue; // test was not executed, don't report
                }
                serializer.startTag(NS, TEST_TAG);
                serializer.attribute(NS, RESULT_ATTR, getTestStatusCompatibilityString(status));
                serializer.attribute(NS, NAME_ATTR, individualResult.getKey());
                if (TestStatus.IGNORED.equals(status)) {
                    serializer.attribute(NS, SKIPPED_ATTR, Boolean.toString(true));
                }

                handleTestFailure(serializer, individualResult.getValue().getStackTrace());

                HandleLoggedFiles(serializer, individualResult);

                for (Entry<String, String> metric :
                        individualResult.getValue().getMetrics().entrySet()) {
                    serializer.startTag(NS, METRIC_TAG);
                    serializer.attribute(NS, metric.getKey(), metric.getValue());
                    serializer.endTag(NS, METRIC_TAG);
                }
                serializer.endTag(NS, TEST_TAG);
            }
            serializer.endTag(NS, CASE_TAG);
        }
    }

    private static void handleTestFailure(XmlSerializer serializer, String fullStack)
            throws IllegalArgumentException, IllegalStateException, IOException {
        if (fullStack != null) {
            String message;
            int index = fullStack.indexOf('\n');
            if (index < 0) {
                // Trace is a single line, just set the message to be the same as the stacktrace.
                message = fullStack;
            } else {
                message = fullStack.substring(0, index);
            }
            serializer.startTag(NS, FAILURE_TAG);

            serializer.attribute(NS, MESSAGE_ATTR, message);
            serializer.startTag(NS, STACK_TAG);
            serializer.text(fullStack);
            serializer.endTag(NS, STACK_TAG);

            serializer.endTag(NS, FAILURE_TAG);
        }
    }

    /** Add files captured by {@link TestFailureListener} on test failures. */
    private static void HandleLoggedFiles(
            XmlSerializer serializer, Entry<String, TestResult> testResult)
            throws IllegalArgumentException, IllegalStateException, IOException {
        Map<String, LogFile> loggedFiles = testResult.getValue().getLoggedFiles();
        if (loggedFiles == null || loggedFiles.isEmpty()) {
            return;
        }
        for (String key : loggedFiles.keySet()) {
            switch (loggedFiles.get(key).getType()) {
                case BUGREPORT:
                    serializer.startTag(NS, BUGREPORT_TAG);
                    serializer.attribute(NS, LOG_FILE_NAME_ATTR, key);
                    serializer.text(loggedFiles.get(key).getUrl());
                    serializer.endTag(NS, BUGREPORT_TAG);
                    break;
                case LOGCAT:
                    serializer.startTag(NS, LOGCAT_TAG);
                    serializer.attribute(NS, LOG_FILE_NAME_ATTR, key);
                    serializer.text(loggedFiles.get(key).getUrl());
                    serializer.endTag(NS, LOGCAT_TAG);
                    break;
                case PNG:
                case JPEG:
                    serializer.startTag(NS, SCREENSHOT_TAG);
                    serializer.attribute(NS, LOG_FILE_NAME_ATTR, key);
                    serializer.text(loggedFiles.get(key).getUrl());
                    serializer.endTag(NS, SCREENSHOT_TAG);
                    break;
                default:
                    break;
            }
        }
    }

    /**
     * Return the given time as a {@link String} suitable for displaying.
     *
     * <p>Example: Fri Aug 20 15:13:03 PDT 2010
     *
     * @param time the epoch time in ms since midnight Jan 1, 1970
     */
    private static String toReadableDateString(long time) {
        SimpleDateFormat dateFormat = new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy");
        return dateFormat.format(new Date(time));
    }

    /** Convert our test status to a format compatible with CTS backend. */
    private static String getTestStatusCompatibilityString(TestStatus status) {
        switch (status) {
            case PASSED:
                return "pass";
            case FAILURE:
                return "fail";
            default:
                return status.toString();
        }
    }

    /** {@inheritDoc} */
    @Override
    public SuiteResultHolder parseResults(File resultDir) throws IOException {
        File resultFile = new File(resultDir, TEST_RESULT_FILE_NAME);
        if (!resultFile.exists()) {
            CLog.d("Could not find %s for loading the results.", resultFile.getAbsolutePath());
            return null;
        }
        SuiteResultHolder invocation = new SuiteResultHolder();
        IInvocationContext context = new InvocationContext();
        try {
            XmlPullParserFactory factory = XmlPullParserFactory.newInstance();
            XmlPullParser parser = factory.newPullParser();
            parser.setInput(new FileReader(resultFile));

            parser.nextTag();
            parser.require(XmlPullParser.START_TAG, NS, RESULT_TAG);
            invocation.startTime = (Long.valueOf(parser.getAttributeValue(NS, START_TIME_ATTR)));
            invocation.endTime = (Long.valueOf(parser.getAttributeValue(NS, END_TIME_ATTR)));
            context.addInvocationAttribute(
                    COMMAND_LINE_ARGS, parser.getAttributeValue(NS, COMMAND_LINE_ARGS));
            parseSuiteAttributes(parser, context);

            String deviceList = parser.getAttributeValue(NS, DEVICES_ATTR);
            int i = 0;
            // TODO: Fix to correctly handle the number of device per shard.
            for (String device : deviceList.split(",")) {
                context.addSerialsFromShard(i, Arrays.asList(device));
                i++;
            }

            parser.nextTag();
            parser.require(XmlPullParser.START_TAG, NS, BUILD_TAG);

            for (int index = 0; index < parser.getAttributeCount(); index++) {
                String key = parser.getAttributeName(i);
                String value = parser.getAttributeValue(NS, key);
                // TODO: Handle list of values that are comma separated.
                context.addInvocationAttribute(key, value);
            }
            parseBuildInfoAttributes(parser, context);

            parser.nextTag();
            parser.require(XmlPullParser.END_TAG, NS, BUILD_TAG);

            parser.nextTag();
            parser.require(XmlPullParser.START_TAG, NS, SUMMARY_TAG);

            invocation.completeModules =
                    Integer.parseInt(parser.getAttributeValue(NS, MODULES_DONE_ATTR));
            invocation.totalModules =
                    Integer.parseInt(parser.getAttributeValue(NS, MODULES_TOTAL_ATTR));
            invocation.passedTests = Integer.parseInt(parser.getAttributeValue(NS, PASS_ATTR));
            invocation.failedTests = Integer.parseInt(parser.getAttributeValue(NS, FAILED_ATTR));

            parser.nextTag();
            parser.require(XmlPullParser.END_TAG, NS, SUMMARY_TAG);

            Collection<TestRunResult> results = new ArrayList<>();
            Map<String, IAbi> moduleAbis = new HashMap<>();
            // Module level information parsing
            handleModuleLevel(parser, results, moduleAbis);
            parser.require(XmlPullParser.END_TAG, NS, RESULT_TAG);
            invocation.runResults = results;
            invocation.modulesAbi = moduleAbis;
        } catch (XmlPullParserException e) {
            CLog.e(e);
            return null;
        }

        invocation.context = context;
        return invocation;
    }

    /**
     * Handle the parsing and replay of all the information inside a module (class, method,
     * failures).
     */
    private void handleModuleLevel(
            XmlPullParser parser, Collection<TestRunResult> results, Map<String, IAbi> moduleAbis)
            throws IOException, XmlPullParserException {
        while (parser.nextTag() == XmlPullParser.START_TAG) {
            parser.require(XmlPullParser.START_TAG, NS, MODULE_TAG);
            TestRunResult module = new TestRunResult();
            results.add(module);
            String name = parser.getAttributeValue(NS, NAME_ATTR);
            String abi = parser.getAttributeValue(NS, ABI_ATTR);
            String moduleId = name;
            if (abi != null) {
                moduleId = AbiUtils.createId(abi, name);
                moduleAbis.put(moduleId, new Abi(abi, AbiUtils.getBitness(abi)));
            }
            long moduleElapsedTime = Long.parseLong(parser.getAttributeValue(NS, RUNTIME_ATTR));
            boolean moduleDone = Boolean.parseBoolean(parser.getAttributeValue(NS, DONE_ATTR));
            int totalTests = Integer.parseInt(parser.getAttributeValue(NS, TOTAL_TESTS_ATTR));
            module.testRunStarted(moduleId, totalTests);
            // TestCase level information parsing
            while (parser.nextTag() == XmlPullParser.START_TAG) {
                parser.require(XmlPullParser.START_TAG, NS, CASE_TAG);
                String className = parser.getAttributeValue(NS, NAME_ATTR);
                // Test level information parsing
                handleTestCaseLevel(parser, module, className);
                parser.require(XmlPullParser.END_TAG, NS, CASE_TAG);
            }
            module.testRunEnded(moduleElapsedTime, new HashMap<String, Metric>());
            module.setRunComplete(moduleDone);
            parser.require(XmlPullParser.END_TAG, NS, MODULE_TAG);
        }
    }

    /** Parse and replay all the individual test cases level (method) informations. */
    private void handleTestCaseLevel(
            XmlPullParser parser, TestRunResult currentModule, String className)
            throws IOException, XmlPullParserException {
        while (parser.nextTag() == XmlPullParser.START_TAG) {
            parser.require(XmlPullParser.START_TAG, NS, TEST_TAG);
            String methodName = parser.getAttributeValue(NS, NAME_ATTR);
            TestDescription description = new TestDescription(className, methodName);
            currentModule.testStarted(description);
            while (parser.nextTag() == XmlPullParser.START_TAG) { // Failure level
                if (parser.getName().equals(FAILURE_TAG)) {
                    String failure = parser.getAttributeValue(NS, MESSAGE_ATTR);
                    if (parser.nextTag() == XmlPullParser.START_TAG) {
                        parser.require(XmlPullParser.START_TAG, NS, STACK_TAG);
                        failure = parser.nextText();
                        parser.require(XmlPullParser.END_TAG, NS, STACK_TAG);
                        parser.nextTag();
                    }
                    currentModule.testFailed(description, failure);
                    parser.require(XmlPullParser.END_TAG, NS, FAILURE_TAG);
                }
                parseLoggedFiles(parser, currentModule);
            }
            currentModule.testEnded(description, new HashMap<String, Metric>());
            parser.require(XmlPullParser.END_TAG, NS, TEST_TAG);
        }
    }

    /** Add files captured by {@link TestFailureListener} on test failures. */
    private static void parseLoggedFiles(XmlPullParser parser, TestRunResult currentModule)
            throws XmlPullParserException, IOException {
        if (parser.getName().equals(BUGREPORT_TAG)) {
            parseSingleFiles(parser, currentModule, BUGREPORT_TAG, LogDataType.BUGREPORTZ);
        } else if (parser.getName().equals(LOGCAT_TAG)) {
            parseSingleFiles(parser, currentModule, LOGCAT_TAG, LogDataType.LOGCAT);
        } else if (parser.getName().equals(SCREENSHOT_TAG)) {
            parseSingleFiles(parser, currentModule, SCREENSHOT_TAG, LogDataType.PNG);
        }
    }

    private static void parseSingleFiles(
            XmlPullParser parser, TestRunResult currentModule, String tagName, LogDataType type)
            throws XmlPullParserException, IOException {
        String name = parser.getAttributeValue(NS, LOG_FILE_NAME_ATTR);
        String logFileUrl = parser.nextText();
        currentModule.testLogSaved(name, new LogFile(logFileUrl, logFileUrl, type));
        parser.require(XmlPullParser.END_TAG, NS, tagName);
    }
}
