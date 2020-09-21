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
package com.android.compatibility.common.tradefed.result.suite;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.compatibility.common.util.DeviceInfo;
import com.android.compatibility.common.util.ResultHandler;
import com.android.compatibility.common.util.ResultUploader;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ILogSaverListener;
import com.android.tradefed.result.ITestSummaryListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogFile;
import com.android.tradefed.result.LogFileSaver;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.result.TestSummary;
import com.android.tradefed.result.suite.IFormatterGenerator;
import com.android.tradefed.result.suite.SuiteResultReporter;
import com.android.tradefed.result.suite.XmlFormattedGeneratorReporter;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.ZipUtil;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.stream.StreamSource;

/**
 * Extension of {@link XmlFormattedGeneratorReporter} and {@link SuiteResultReporter} to handle
 * Compatibility specific format and operations.
 */
public class CertificationSuiteResultReporter extends XmlFormattedGeneratorReporter
        implements ILogSaverListener, ITestSummaryListener {

    public static final String LATEST_LINK_NAME = "latest";
    public static final String SUMMARY_FILE = "invocation_summary.txt";
    public static final String FAILURE_REPORT_NAME = "test_result_failures_suite.html";
    public static final String FAILURE_XSL_FILE_NAME = "compatibility_failures.xsl";

    public static final String BUILD_FINGERPRINT = "build_fingerprint";

    @Option(name = "result-server", description = "Server to publish test results.")
    private String mResultServer;

    @Option(name = "disable-result-posting", description = "Disable result posting into report server.")
    private boolean mDisableResultPosting = false;

    @Option(name = "include-test-log-tags", description = "Include test log tags in report.")
    private boolean mIncludeTestLogTags = false;

    @Option(name = "use-log-saver", description = "Also saves generated result with log saver")
    private boolean mUseLogSaver = false;

    @Option(name = "compress-logs", description = "Whether logs will be saved with compression")
    private boolean mCompressLogs = true;

    public static final String INCLUDE_HTML_IN_ZIP = "html-in-zip";
    @Option(name = INCLUDE_HTML_IN_ZIP,
            description = "Whether failure summary report is included in the zip fie.")
    private boolean mIncludeHtml = false;

    private CompatibilityBuildHelper mBuildHelper;

    /** The directory containing the results */
    private File mResultDir = null;
    /** The directory containing the logs */
    private File mLogDir = null;

    private ResultUploader mUploader;

    private LogFileSaver mTestLogSaver;
    /** Log saver to receive when files are logged */
    private ILogSaver mLogSaver;

    private String mReferenceUrl;

    private Map<String, String> mLoggedFiles;

    private static final String[] RESULT_RESOURCES = {
        "compatibility_result.css",
        "compatibility_result.xsd",
        "compatibility_result.xsl",
        "logo.png"
    };

    public CertificationSuiteResultReporter() {
        super();
        mLoggedFiles = new LinkedHashMap<>();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public final void invocationStarted(IInvocationContext context) {
        super.invocationStarted(context);

        if (mBuildHelper == null) {
            mBuildHelper = new CompatibilityBuildHelper(getPrimaryBuildInfo());
        }
        if (mResultDir == null) {
            initializeResultDirectories();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLog(String name, LogDataType type, InputStreamSource stream) {
        if (name.endsWith(DeviceInfo.FILE_SUFFIX)) {
            // Handle device info file case
            testLogDeviceInfo(name, stream);
            return;
        }
        try {
            File logFile = null;
            if (mCompressLogs) {
                try (InputStream inputStream = stream.createInputStream()) {
                    logFile = mTestLogSaver.saveAndGZipLogData(name, type, inputStream);
                }
            } else {
                try (InputStream inputStream = stream.createInputStream()) {
                    logFile = mTestLogSaver.saveLogData(name, type, inputStream);
                }
            }
            CLog.d("Saved logs for %s in %s", name, logFile.getAbsolutePath());
        } catch (IOException e) {
            CLog.e("Failed to write log for %s", name);
            CLog.e(e);
        }
    }

    /** Write device-info files to the result, invoked only by the master result reporter */
    private void testLogDeviceInfo(String name, InputStreamSource stream) {
        try {
            File ediDir = new File(mResultDir, DeviceInfo.RESULT_DIR_NAME);
            ediDir.mkdirs();
            File ediFile = new File(ediDir, name);
            if (!ediFile.exists()) {
                // only write this file to the results if not already present
                FileUtil.writeToFile(stream.createInputStream(), ediFile);
            }
        } catch (IOException e) {
            CLog.w("Failed to write device info %s to result", name);
            CLog.e(e);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLogSaved(String dataName, LogDataType dataType, InputStreamSource dataStream,
            LogFile logFile) {
        if (mIncludeTestLogTags) {
            switch (dataType) {
                case BUGREPORT:
                case LOGCAT:
                case PNG:
                    mLoggedFiles.put(dataName, logFile.getUrl());
                    break;
                default:
                    // Do nothing
                    break;
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void putSummary(List<TestSummary> summaries) {
        for (TestSummary summary : summaries) {
            if (mReferenceUrl == null && summary.getSummary().getString() != null) {
                mReferenceUrl = summary.getSummary().getString();
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setLogSaver(ILogSaver saver) {
        mLogSaver = saver;
    }

    /**
     * Create directory structure where results and logs will be written.
     */
    private void initializeResultDirectories() {
        CLog.d("Initializing result directory");

        try {
            mResultDir = mBuildHelper.getResultDir();
            if (mResultDir != null) {
                mResultDir.mkdirs();
            }
        } catch (FileNotFoundException e) {
            throw new RuntimeException(e);
        }

        if (mResultDir == null) {
            throw new RuntimeException("Result Directory was not created");
        }
        if (!mResultDir.exists()) {
            throw new RuntimeException("Result Directory was not created: " +
                    mResultDir.getAbsolutePath());
        }

        CLog.d("Results Directory: %s", mResultDir.getAbsolutePath());

        mUploader = new ResultUploader(mResultServer, mBuildHelper.getSuiteName());
        try {
            mLogDir = new File(mBuildHelper.getLogsDir(),
                    CompatibilityBuildHelper.getDirSuffix(mBuildHelper.getStartTime()));
        } catch (FileNotFoundException e) {
            CLog.e(e);
        }
        if (mLogDir != null && mLogDir.mkdirs()) {
            CLog.d("Created log dir %s", mLogDir.getAbsolutePath());
        }
        if (mLogDir == null || !mLogDir.exists()) {
            throw new IllegalArgumentException(String.format("Could not create log dir %s",
                    mLogDir.getAbsolutePath()));
        }
        if (mTestLogSaver == null) {
            mTestLogSaver = new LogFileSaver(mLogDir);
        }
    }

    @Override
    public IFormatterGenerator createFormatter() {
        return new CertificationResultXml(mBuildHelper.getSuiteName(),
                mBuildHelper.getSuiteVersion(),
                mBuildHelper.getSuitePlan(),
                mBuildHelper.getSuiteBuild(),
                mReferenceUrl,
                getLogUrl());
    }

    @Override
    public void preFormattingSetup(IFormatterGenerator formater) {
        super.preFormattingSetup(formater);
        // Log the summary
        TestSummary summary = getSummary();
        try {
            File summaryFile = new File(mResultDir, SUMMARY_FILE);
            FileUtil.writeToFile(summary.getSummary().toString(), summaryFile);
        } catch (IOException e) {
            CLog.e("Failed to save the summary.");
            CLog.e(e);
        }

        copyDynamicConfigFiles();
        copyFormattingFiles(mResultDir, mBuildHelper.getSuiteName());
    }

    @Override
    public File createResultDir() throws IOException {
        return mResultDir;
    }

    @Override
    public void postFormattingStep(File resultDir, File reportFile) {
        super.postFormattingStep(resultDir,reportFile);

        createChecksum(resultDir, getRunResults(),
                getPrimaryBuildInfo().getBuildAttributes().get(BUILD_FINGERPRINT));

        File failureReport = null;
        if (mIncludeHtml) {
            // Create the html report before the zip file.
            failureReport = createFailureReport(reportFile);
        }
        File zippedResults = zipResults(mResultDir);
        // TODO: calculate results checksum file
        if (!mIncludeHtml) {
            // Create failure report after zip file so extra data is not uploaded
            failureReport = createFailureReport(reportFile);
        }
        try {
            if (failureReport.exists()) {
                CLog.i("Test Result: %s", failureReport.getCanonicalPath());
            } else {
                CLog.i("Test Result: %s", reportFile.getCanonicalPath());
            }
            Path latestLink = createLatestLinkDirectory(mResultDir.toPath());
            if (latestLink != null) {
                CLog.i("Latest results link: " + latestLink.toAbsolutePath());
            }

            latestLink = createLatestLinkDirectory(mLogDir.toPath());
            if (latestLink != null) {
                CLog.i("Latest logs link: " + latestLink.toAbsolutePath());
            }

            saveLog(reportFile, zippedResults);
        } catch (IOException e) {
            CLog.e("Error when handling the post processing of results file:");
            CLog.e(e);
        }

        uploadResult(reportFile);
    }

    /**
     * Return the path in which log saver persists log files or null if
     * logSaver is not enabled.
     */
    private String getLogUrl() {
        if (!mUseLogSaver || mLogSaver == null) {
            return null;
        }

        return mLogSaver.getLogReportDir().getUrl();
    }

    /**
     * Update the "latest" symlink to the newest result directory. CTS specific.
     */
    private Path createLatestLinkDirectory(Path directory) {
        Path link = null;

        Path parent = directory.getParent();

        if (parent != null) {
            link = parent.resolve(LATEST_LINK_NAME);
            try {
                // if latest already exists, we have to remove it before creating
                Files.deleteIfExists(link);
                Files.createSymbolicLink(link, directory);
            } catch (IOException ioe) {
                CLog.e("Exception while attempting to create 'latest' link to: [%s]",
                    directory);
                CLog.e(ioe);
                return null;
            } catch (UnsupportedOperationException uoe) {
                CLog.e("Failed to create 'latest' symbolic link - unsupported operation");
                return null;
            }
        }
        return link;
    }

    /**
     * move the dynamic config files to the results directory
     */
    private void copyDynamicConfigFiles() {
        File configDir = new File(mResultDir, "config");
        if (!configDir.mkdir()) {
            CLog.w("Failed to make dynamic config directory \"%s\" in the result",
                    configDir.getAbsolutePath());
        }

        Set<String> uniqueModules = new HashSet<>();
        // Check each build of the invocation, in case of multi-device invocation.
        for (IBuildInfo buildInfo : getInvocationContext().getBuildInfos()) {
            CompatibilityBuildHelper helper = new CompatibilityBuildHelper(buildInfo);
            Map<String, File> dcFiles = helper.getDynamicConfigFiles();
            for (String moduleName : dcFiles.keySet()) {
                File srcFile = dcFiles.get(moduleName);
                if (!uniqueModules.contains(moduleName)) {
                    // have not seen config for this module yet, copy into result
                    File destFile = new File(configDir, moduleName + ".dynamic");
                    try {
                        FileUtil.copyFile(srcFile, destFile);
                        uniqueModules.add(moduleName); // Add to uniqueModules if copy succeeds
                    } catch (IOException e) {
                        CLog.w("Failure when copying config file \"%s\" to \"%s\" for module %s",
                                srcFile.getAbsolutePath(), destFile.getAbsolutePath(), moduleName);
                        CLog.e(e);
                    }
                }
                FileUtil.deleteFile(srcFile);
            }
        }
    }

    /**
     * Copy the xml formatting files stored in this jar to the results directory. CTS specific.
     *
     * @param resultsDir
     */
    private void copyFormattingFiles(File resultsDir, String suiteName) {
        for (String resultFileName : RESULT_RESOURCES) {
            InputStream configStream = CertificationResultXml.class.getResourceAsStream(
                    String.format("/report/%s-%s", suiteName, resultFileName));
            if (configStream == null) {
                // If suite specific files are not available, fallback to common.
                configStream = CertificationResultXml.class.getResourceAsStream(
                    String.format("/report/%s", resultFileName));
            }
            if (configStream != null) {
                File resultFile = new File(resultsDir, resultFileName);
                try {
                    FileUtil.writeToFile(configStream, resultFile);
                } catch (IOException e) {
                    CLog.w("Failed to write %s to file", resultFileName);
                }
            } else {
                CLog.w("Failed to load %s from jar", resultFileName);
            }
        }
    }

    /**
     * When enabled, save log data using log saver
     */
    private void saveLog(File resultFile, File zippedResults) throws IOException {
        if (!mUseLogSaver) {
            return;
        }

        FileInputStream fis = null;
        LogFile logFile = null;
        try {
            fis = new FileInputStream(resultFile);
            logFile = mLogSaver.saveLogData("log-result", LogDataType.XML, fis);
            CLog.d("Result XML URL: %s", logFile.getUrl());
        } catch (IOException ioe) {
            CLog.e("error saving XML with log saver");
            CLog.e(ioe);
        } finally {
            StreamUtil.close(fis);
        }
        // Save the full results folder.
        if (zippedResults != null) {
            FileInputStream zipResultStream = null;
            try {
                zipResultStream = new FileInputStream(zippedResults);
                logFile = mLogSaver.saveLogData("results", LogDataType.ZIP, zipResultStream);
                CLog.d("Result zip URL: %s", logFile.getUrl());
            } finally {
                StreamUtil.close(zipResultStream);
            }
        }
    }

    /**
     * Zip the contents of the given results directory. CTS specific.
     *
     * @param resultsDir
     */
    private static File zipResults(File resultsDir) {
        File zipResultFile = null;
        try {
            // create a file in parent directory, with same name as resultsDir
            zipResultFile = new File(resultsDir.getParent(), String.format("%s.zip",
                    resultsDir.getName()));
            ZipUtil.createZip(resultsDir, zipResultFile);
        } catch (IOException e) {
            CLog.w("Failed to create zip for %s", resultsDir.getName());
        }
        return zipResultFile;
    }

    /**
     * When enabled, upload the result to a server. CTS specific.
     */
    private void uploadResult(File resultFile) {
        if (mResultServer != null && !mResultServer.trim().isEmpty() && !mDisableResultPosting) {
            try {
                CLog.d("Result Server: %d", mUploader.uploadResult(resultFile, mReferenceUrl));
            } catch (IOException ioe) {
                CLog.e("IOException while uploading result.");
                CLog.e(ioe);
            }
        }
    }

    /**
     * Generate html report listing an failed tests. CTS specific.
     */
    private File createFailureReport(File inputXml) {
        File failureReport = new File(inputXml.getParentFile(), FAILURE_REPORT_NAME);
        try (InputStream xslStream = ResultHandler.class.getResourceAsStream(
                String.format("/report/%s", FAILURE_XSL_FILE_NAME));
             OutputStream outputStream = new FileOutputStream(failureReport)) {

            Transformer transformer = TransformerFactory.newInstance().newTransformer(
                    new StreamSource(xslStream));
            transformer.transform(new StreamSource(inputXml), new StreamResult(outputStream));
        } catch (IOException | TransformerException ignored) { }
        return failureReport;
    }

    /**
     * Generates a checksum files based on the results.
     */
    private void createChecksum(File resultDir, Collection<TestRunResult> results,
            String buildFingerprint) {
        CertificationChecksumHelper.tryCreateChecksum(resultDir, results, buildFingerprint);
    }
}
