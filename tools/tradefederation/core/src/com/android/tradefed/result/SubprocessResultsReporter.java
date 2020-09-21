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
package com.android.tradefed.result;

import com.android.tradefed.config.Option;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.SubprocessEventHelper.BaseTestEventInfo;
import com.android.tradefed.util.SubprocessEventHelper.FailedTestEventInfo;
import com.android.tradefed.util.SubprocessEventHelper.InvocationFailedEventInfo;
import com.android.tradefed.util.SubprocessEventHelper.InvocationStartedEventInfo;
import com.android.tradefed.util.SubprocessEventHelper.LogAssociationEventInfo;
import com.android.tradefed.util.SubprocessEventHelper.TestEndedEventInfo;
import com.android.tradefed.util.SubprocessEventHelper.TestLogEventInfo;
import com.android.tradefed.util.SubprocessEventHelper.TestModuleStartedEventInfo;
import com.android.tradefed.util.SubprocessEventHelper.TestRunEndedEventInfo;
import com.android.tradefed.util.SubprocessEventHelper.TestRunFailedEventInfo;
import com.android.tradefed.util.SubprocessEventHelper.TestRunStartedEventInfo;
import com.android.tradefed.util.SubprocessEventHelper.TestStartedEventInfo;
import com.android.tradefed.util.SubprocessTestResultsParser;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.json.JSONObject;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.HashMap;

/**
 * Implements {@link ITestInvocationListener} to be specified as a result_reporter and forward from
 * the subprocess the results of tests, test runs, test invocations.
 */
public class SubprocessResultsReporter
        implements ITestInvocationListener, ILogSaverListener, AutoCloseable {

    @Option(name = "subprocess-report-file", description = "the file where to log the events.")
    private File mReportFile = null;

    @Option(name = "subprocess-report-port", description = "the port where to connect to send the"
            + "events.")
    private Integer mReportPort = null;

    @Option(name = "output-test-log", description = "Option to report test logs to parent process.")
    private boolean mOutputTestlog = false;

    private Socket mReportSocket = null;
    private PrintWriter mPrintWriter = null;

    private boolean mPrintWarning = true;

    /** {@inheritDoc} */
    @Override
    public void testAssumptionFailure(TestDescription testId, String trace) {
        FailedTestEventInfo info =
                new FailedTestEventInfo(testId.getClassName(), testId.getTestName(), trace);
        printEvent(SubprocessTestResultsParser.StatusKeys.TEST_ASSUMPTION_FAILURE, info);
    }

    /** {@inheritDoc} */
    @Override
    public void testEnded(TestDescription testId, HashMap<String, Metric> metrics) {
        testEnded(testId, System.currentTimeMillis(), metrics);
    }

    /** {@inheritDoc} */
    @Override
    public void testEnded(TestDescription testId, long endTime, HashMap<String, Metric> metrics) {
        // TODO: transfer the proto metrics instead of string metrics
        TestEndedEventInfo info =
                new TestEndedEventInfo(
                        testId.getClassName(),
                        testId.getTestName(),
                        endTime,
                        TfMetricProtoUtil.compatibleConvert(metrics));
        printEvent(SubprocessTestResultsParser.StatusKeys.TEST_ENDED, info);
    }

    /** {@inheritDoc} */
    @Override
    public void testFailed(TestDescription testId, String reason) {
        FailedTestEventInfo info =
                new FailedTestEventInfo(testId.getClassName(), testId.getTestName(), reason);
        printEvent(SubprocessTestResultsParser.StatusKeys.TEST_FAILED, info);
    }

    /** {@inheritDoc} */
    @Override
    public void testIgnored(TestDescription testId) {
        BaseTestEventInfo info = new BaseTestEventInfo(testId.getClassName(), testId.getTestName());
        printEvent(SubprocessTestResultsParser.StatusKeys.TEST_IGNORED, info);
    }

    /** {@inheritDoc} */
    @Override
    public void testRunEnded(long time, HashMap<String, Metric> runMetrics) {
        // TODO: Transfer the full proto instead of just Strings.
        TestRunEndedEventInfo info =
                new TestRunEndedEventInfo(time, TfMetricProtoUtil.compatibleConvert(runMetrics));
        printEvent(SubprocessTestResultsParser.StatusKeys.TEST_RUN_ENDED, info);
    }

    /** {@inheritDoc} */
    @Override
    public void testRunFailed(String reason) {
        TestRunFailedEventInfo info = new TestRunFailedEventInfo(reason);
        printEvent(SubprocessTestResultsParser.StatusKeys.TEST_RUN_FAILED, info);
    }

    @Override
    public void testRunStarted(String runName, int testCount) {
        TestRunStartedEventInfo info = new TestRunStartedEventInfo(runName, testCount);
        printEvent(SubprocessTestResultsParser.StatusKeys.TEST_RUN_STARTED, info);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStopped(long arg0) {
        // ignore
    }

    /** {@inheritDoc} */
    @Override
    public void testStarted(TestDescription testId) {
        testStarted(testId, System.currentTimeMillis());
    }

    /** {@inheritDoc} */
    @Override
    public void testStarted(TestDescription testId, long startTime) {
        TestStartedEventInfo info =
                new TestStartedEventInfo(testId.getClassName(), testId.getTestName(), startTime);
        printEvent(SubprocessTestResultsParser.StatusKeys.TEST_STARTED, info);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationStarted(IInvocationContext context) {
        InvocationStartedEventInfo info =
                new InvocationStartedEventInfo(context.getTestTag(), System.currentTimeMillis());
        printEvent(SubprocessTestResultsParser.StatusKeys.INVOCATION_STARTED, info);
    }

    /** {@inheritDoc} */
    @Override
    public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
        if (!mOutputTestlog || (mReportPort == null && mReportFile == null)) {
            return;
        }
        if (dataStream != null && dataStream.size() != 0) {
            File tmpFile = null;
            try {
                // put 'subprocess' in front to identify the files.
                tmpFile =
                        FileUtil.createTempFile(
                                "subprocess-" + dataName, "." + dataType.getFileExt());
                FileUtil.writeToFile(dataStream.createInputStream(), tmpFile);
                TestLogEventInfo info = new TestLogEventInfo(dataName, dataType, tmpFile);
                printEvent(SubprocessTestResultsParser.StatusKeys.TEST_LOG, info);
            } catch (IOException e) {
                CLog.e(e);
                FileUtil.deleteFile(tmpFile);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void testLogSaved(
            String dataName, LogDataType dataType, InputStreamSource dataStream, LogFile logFile) {
        // Do nothing, we are not passing the testLogSaved information to the parent process.
    }

    /** {@inheritDoc} */
    @Override
    public void setLogSaver(ILogSaver logSaver) {
        // Do nothing, this result_reporter does not need the log saver.
    }

    /** {@inheritDoc} */
    @Override
    public void logAssociation(String dataName, LogFile logFile) {
        LogAssociationEventInfo info = new LogAssociationEventInfo(dataName, logFile);
        printEvent(SubprocessTestResultsParser.StatusKeys.LOG_ASSOCIATION, info);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationEnded(long elapsedTime) {
        // ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationFailed(Throwable cause) {
        InvocationFailedEventInfo info = new InvocationFailedEventInfo(cause);
        printEvent(SubprocessTestResultsParser.StatusKeys.INVOCATION_FAILED, info);
    }

    /** {@inheritDoc} */
    @Override
    public void testModuleStarted(IInvocationContext moduleContext) {
        TestModuleStartedEventInfo info = new TestModuleStartedEventInfo(moduleContext);
        printEvent(SubprocessTestResultsParser.StatusKeys.TEST_MODULE_STARTED, info);
    }

    /** {@inheritDoc} */
    @Override
    public void testModuleEnded() {
        printEvent(SubprocessTestResultsParser.StatusKeys.TEST_MODULE_ENDED, new JSONObject());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public TestSummary getSummary() {
        return null;
    }

    /**
     * Helper to print the event key and then the json object.
     */
    public void printEvent(String key, Object event) {
        if (mReportFile != null) {
            if (mReportFile.canWrite()) {
                try {
                    try (FileWriter fw = new FileWriter(mReportFile, true)) {
                        String eventLog = String.format("%s %s\n", key, event.toString());
                        fw.append(eventLog);
                        fw.flush();
                    }
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
            } else {
                throw new RuntimeException(
                        String.format("report file: %s is not writable",
                                mReportFile.getAbsolutePath()));
            }
        }
        if(mReportPort != null) {
            try {
                if (mReportSocket == null) {
                    mReportSocket = new Socket("localhost", mReportPort.intValue());
                    mPrintWriter = new PrintWriter(mReportSocket.getOutputStream(), true);
                }
                if (!mReportSocket.isConnected()) {
                    throw new RuntimeException("Reporter Socket is not connected");
                }
                String eventLog = String.format("%s %s\n", key, event.toString());
                mPrintWriter.print(eventLog);
                mPrintWriter.flush();
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
        if (mReportFile == null && mReportPort == null) {
            if (mPrintWarning) {
                // Only print the warning the first time.
                mPrintWarning = false;
                CLog.w("No report file or socket has been configured, skipping this reporter.");
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void close() {
        StreamUtil.close(mReportSocket);
        StreamUtil.close(mPrintWriter);
    }
}
