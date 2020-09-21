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

package com.android.monkey;

import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.RunUtil;

import java.io.File;
import java.io.IOException;

/**
 * A utility class that encapsulates details of calling post-processing scripts to generate monkey
 * ANR reports.
 */
public class AnrReportGenerator {

    private static final long REPORT_GENERATION_TIMEOUT = 30 * 1000; // 30s

    private File mCachedMonkeyLog = null;
    private File mCachedBugreport = null;

    private final String mReportScriptPath;
    private final String mReportBasePath;
    private final String mReportUrlPrefix;
    private final String mReportPath;
    private final String mDeviceSerial;

    private String mBuildId = null;
    private String mBuildFlavor = null;

    /**
     * Constructs the instance with details of report script and output location information. See
     * matching options on {@link MonkeyBase} for more info.
     */
    public AnrReportGenerator(String reportScriptPath, String reportBasePath,
            String reportUrlPrefix, String reportPath,
            String buildId, String buildFlavor, String deviceSerial) {
        mReportScriptPath = reportScriptPath;
        mReportBasePath = reportBasePath;
        mReportUrlPrefix = reportUrlPrefix;
        mReportPath = reportPath;
        mBuildId = buildId;
        mBuildFlavor = buildFlavor;
        mDeviceSerial = deviceSerial;

        if (mReportBasePath == null || mReportPath == null || mReportScriptPath == null
                || mReportUrlPrefix == null) {
            throw new IllegalArgumentException("ANR post-processing enabled but missing "
                    + "required parameters!");
        }
    }

    /**
     * Return the storage sub path based on build info. The path will not include trailing path
     * separator.
     */
    private String getPerBuildStoragePath() {
        if (mBuildId == null) {
            mBuildId = "-1";
        }
        if (mBuildFlavor == null) {
            mBuildFlavor = "unknown_flavor";
        }
        return String.format("%s/%s", mBuildId, mBuildFlavor);
    }

    /**
     * Sets bugreport information for ANR post-processing script
     * @param bugreportStream
     */
    public void setBugReportInfo(InputStreamSource bugreportStream) throws IOException {
        if (mCachedBugreport != null) {
            CLog.w("A bugreport for this invocation already existed at %s, overriding anyways",
                    mCachedBugreport.getAbsolutePath());
        }
        mCachedBugreport = FileUtil.createTempFile("monkey-anr-report-bugreport", ".txt");
        FileUtil.writeToFile(bugreportStream.createInputStream(), mCachedBugreport);
    }

    /**
     * Sets monkey log information for ANR post-processing script
     * @param monkeyLogStream
     */
    public void setMonkeyLogInfo(InputStreamSource monkeyLogStream) throws IOException {
        if (mCachedMonkeyLog != null) {
            CLog.w("A monkey log for this invocation already existed at %s, overriding anyways",
                    mCachedMonkeyLog.getAbsolutePath());
        }
        mCachedMonkeyLog = FileUtil.createTempFile("monkey-anr-report-monkey-log", ".txt");
        FileUtil.writeToFile(monkeyLogStream.createInputStream(), mCachedMonkeyLog);
    }

    public boolean genereateAnrReport(ITestLogger logger) {
        if (mCachedMonkeyLog == null || mCachedBugreport == null) {
            CLog.w("Cannot generate report: bugreport or monkey log not populated yet.");
            return false;
        }
        // generate monkey report and log it
        File reportPath = new File(mReportBasePath,
                String.format("%s/%s", mReportPath, getPerBuildStoragePath()));
        if (reportPath.exists()) {
            if (!reportPath.isDirectory()) {
                CLog.w("The expected report storage path is not a directory: %s",
                        reportPath.getAbsolutePath());
                return false;
            }
        } else {
            if (!reportPath.mkdirs()) {
                CLog.w("Failed to create report storage directory: %s",
                        reportPath.getAbsolutePath());
                return false;
            }
        }
        // now we should have the storage path, calculate the HTML report path
        // HTML report file should be named as:
        // monkey-anr-report-<device serial>-<random string>.html
        // under the pre-constructed base report storage path
        File htmlReport = null;
        try {
            htmlReport = FileUtil.createTempFile(
                    String.format("monkey-anr-report-%s-", mDeviceSerial), ".html",
                    reportPath);
        } catch (IOException ioe) {
            CLog.e("Error getting place holder file for HTML report.");
            CLog.e(ioe);
            return false;
        }
        // now ready to call the script
        String htmlReportPath = htmlReport.getAbsolutePath();
        String command[] = {
                mReportScriptPath, "--monkey", mCachedMonkeyLog.getAbsolutePath(), "--html",
                htmlReportPath, mCachedBugreport.getAbsolutePath()
        };
        CommandResult cr = RunUtil.getDefault().runTimedCmdSilently(REPORT_GENERATION_TIMEOUT,
                command);
        if (cr.getStatus() == CommandStatus.SUCCESS) {
            // Test log the generated HTML report
            try (InputStreamSource source = new FileInputStreamSource(htmlReport)) {
                logger.testLog("monkey-anr-report", LogDataType.HTML, source);
            }
            // Clean up and declare success!
            FileUtil.deleteFile(htmlReport);
            return true;
        } else {
            CLog.w(cr.getStderr());
            return false;
        }
    }

    public void cleanTempFiles() {
        FileUtil.deleteFile(mCachedBugreport);
        FileUtil.deleteFile(mCachedMonkeyLog);
    }
}
