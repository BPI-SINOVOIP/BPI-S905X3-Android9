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
package com.android.tradefed.result;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.command.FatalHostError;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/**
 * Save logs to a file system.
 */
@OptionClass(alias = "file-system-log-saver")
public class FileSystemLogSaver implements ILogSaver {

    private static final int BUFFER_SIZE = 64 * 1024;

    @Option(name = "log-file-path", description = "root file system path to store log files.")
    private File mRootReportDir = new File(System.getProperty("java.io.tmpdir"));

    @Option(name = "log-file-url", description =
            "root http url of log files. Assumes files placed in log-file-path are visible via " +
            "this url.")
    private String mReportUrl = null;

    @Option(name = "log-retention-days", description =
            "the number of days to keep saved log files.")
    private Integer mLogRetentionDays = null;

    @Option(name = "compress-files", description =
            "whether to compress files which are not already compressed")
    private boolean mCompressFiles = true;

    private File mLogReportDir = null;

    /**
     * A counter to control access to methods which modify this class's directories. Acting as a
     * non-blocking reentrant lock, this int blocks access to sharded child invocations from
     * attempting to create or delete directories.
     */
    private int mShardingLock = 0;

    /**
     * {@inheritDoc}
     *
     * <p>Also, create a unique file system directory under {@code
     * report-dir/[branch/]build-id/test-tag/unique_dir} for saving logs. If the creation of the
     * directory fails, will write logs to a temporary directory on the local file system.
     */
    @Override
    public void invocationStarted(IInvocationContext context) {
        // Create log directory on first build info
        IBuildInfo info = context.getBuildInfos().get(0);
        synchronized (this) {
            if (mShardingLock == 0) {
                mLogReportDir = createLogReportDir(info, mRootReportDir, mLogRetentionDays);
            }
            mShardingLock++;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationEnded(long elapsedTime) {
        // no clean up needed.
        synchronized (this) {
            --mShardingLock;
            if (mShardingLock < 0) {
                CLog.w(
                        "Sharding lock exited more times than entered, possible "
                                + "unbalanced invocationStarted/Ended calls");
            }
        }
    }

    /**
     * {@inheritDoc}
     * <p>
     * Will zip and save the log file if {@link LogDataType#isCompressed()} returns false for
     * {@code dataType} and {@code compressed-files} is set, otherwise, the stream will be saved
     * uncompressed.
     * </p>
     */
    @Override
    public LogFile saveLogData(String dataName, LogDataType dataType, InputStream dataStream)
            throws IOException {
        if (!mCompressFiles || dataType.isCompressed()) {
            File log = saveLogDataInternal(dataName, dataType.getFileExt(), dataStream);
            return new LogFile(log.getAbsolutePath(), getUrl(log), dataType);
        }
        BufferedInputStream bufferedDataStream = null;
        ZipOutputStream outputStream = null;
        // add underscore to end of data name to make generated name more readable
        final String saneDataName = sanitizeFilename(dataName);
        File log = FileUtil.createTempFile(saneDataName + "_", "." + LogDataType.ZIP.getFileExt(),
                mLogReportDir);

        boolean setPerms = FileUtil.chmodGroupRWX(log);
        if (!setPerms) {
            CLog.w(String.format("Failed to set dir %s to be group accessible.", log));
        }

        try {
            bufferedDataStream = new BufferedInputStream(dataStream);
            outputStream = new ZipOutputStream(new BufferedOutputStream(new FileOutputStream(log),
                    BUFFER_SIZE));
            outputStream.putNextEntry(new ZipEntry(saneDataName + "." + dataType.getFileExt()));
            StreamUtil.copyStreams(bufferedDataStream, outputStream);
            CLog.d("Saved log file %s", log.getAbsolutePath());
            return new LogFile(log.getAbsolutePath(), getUrl(log), true, dataType, log.length());
        } finally {
            StreamUtil.close(bufferedDataStream);
            StreamUtil.close(outputStream);
        }
    }

    /** {@inheritDoc} */
    @Override
    public LogFile saveLogDataRaw(String dataName, LogDataType dataType, InputStream dataStream)
            throws IOException {
        File log = saveLogDataInternal(dataName, dataType.getFileExt(), dataStream);
        return new LogFile(log.getAbsolutePath(), getUrl(log), dataType);
    }

    private File saveLogDataInternal(String dataName, String ext, InputStream dataStream)
            throws IOException {
        final String saneDataName = sanitizeFilename(dataName);
        // add underscore to end of data name to make generated name more readable
        File log = FileUtil.createTempFile(saneDataName + "_", "." + ext, mLogReportDir);

        boolean setPerms = FileUtil.chmodGroupRWX(log);
        if (!setPerms) {
            CLog.w(String.format("Failed to set dir %s to be group accessible.", log));
        }

        FileUtil.writeToFile(dataStream, log);
        CLog.d("Saved raw log file %s", log.getAbsolutePath());
        return log;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public LogFile getLogReportDir() {
        return new LogFile(mLogReportDir.getAbsolutePath(), getUrl(mLogReportDir), LogDataType.DIR);
    }

    /**
     * A helper method to create an invocation directory unique for saving logs.
     * <p>
     * Create a unique file system directory with the structure
     * {@code report-dir/[branch/]build-id/test-tag/unique_dir} for saving logs.  If the creation
     * of the directory fails, will write logs to a temporary directory on the local file system.
     * </p>
     *
     * @param buildInfo the {@link IBuildInfo}
     * @param reportDir the {@link File} for the report directory.
     * @param logRetentionDays how many days logs should be kept for. If {@code null}, then no log
     * retention file is writen.
     * @return The directory created.
     */
    private File createLogReportDir(IBuildInfo buildInfo, File reportDir,
            Integer logRetentionDays) {
        File logReportDir;
        // now create unique directory within the buildDir
        try {
            File buildDir = createBuildDir(buildInfo, reportDir);
            logReportDir = FileUtil.createTempDir("inv_", buildDir);
        } catch (IOException e) {
            CLog.e("Unable to create unique directory in %s. Attempting to use tmp dir instead",
                    reportDir.getAbsolutePath());
            CLog.e(e);
            // try to create one in a tmp location instead
            logReportDir = createTempDir();
        }

        boolean setPerms = FileUtil.chmodGroupRWX(logReportDir);
        if (!setPerms) {
            CLog.w(String.format("Failed to set dir %s to be group accessible.", logReportDir));
        }

        if (logRetentionDays != null && logRetentionDays > 0) {
            new RetentionFileSaver().writeRetentionFile(logReportDir, logRetentionDays);
        }
        CLog.d("Using log file directory %s", logReportDir.getAbsolutePath());
        return logReportDir;
    }

    /**
     * A helper method to get or create a build directory based on the build info of the invocation.
     * <p>
     * Create a unique file system directory with the structure
     * {@code report-dir/[branch/]build-id/test-tag} for saving logs.
     * </p>
     *
     * @param buildInfo the {@link IBuildInfo}
     * @param reportDir the {@link File} for the report directory.
     * @return The directory where invocations for the same build should be saved.
     * @throws IOException if the directory could not be created because a file with the same name
     * exists or there are no permissions to write to it.
     */
    private File createBuildDir(IBuildInfo buildInfo, File reportDir) throws IOException {
        List<String> pathSegments = new ArrayList<String>();
        if (buildInfo.getBuildBranch() != null) {
            pathSegments.add(buildInfo.getBuildBranch());
        }
        pathSegments.add(buildInfo.getBuildId());
        pathSegments.add(buildInfo.getTestTag());
        File buildReportDir = FileUtil.getFileForPath(reportDir,
                pathSegments.toArray(new String[] {}));

        // if buildReportDir already exists and is a directory - use it.
        if (buildReportDir.exists()) {
            if (buildReportDir.isDirectory()) {
                return buildReportDir;
            } else {
                final String msg = String.format("Cannot create build-specific output dir %s. " +
                        "File already exists.", buildReportDir.getAbsolutePath());
                CLog.w(msg);
                throw new IOException(msg);
            }
        } else {
            if (FileUtil.mkdirsRWX(buildReportDir)) {
                return buildReportDir;
            } else {
                final String msg = String.format("Cannot create build-specific output dir %s. " +
                        "Failed to create directory.", buildReportDir.getAbsolutePath());
                CLog.w(msg);
                throw new IOException(msg);
            }
        }
    }

    /**
     * A helper method to create a temp directory for an invocation.
     */
    private File createTempDir() {
        try {
            return FileUtil.createTempDir("inv_");
        } catch (IOException e) {
            // Abort tradefed if a temp directory cannot be created
            throw new FatalHostError("Cannot create tmp directory.", e);
        }
    }

    /**
     * A helper function that translates a string into something that can be used as a filename
     */
    private static String sanitizeFilename(String name) {
        return name.replace(File.separatorChar, '_');
    }

    /**
     * A helper method that returns a URL for a given {@link File}.
     *
     * @param file the {@link File} of the log.
     * @return The report directory path replaced with the report-url and path separators normalized
     * (for Windows), or {@code null} if the report-url is not set, report-url ends with /,
     * report-dir ends with {@link File#separator}, or the file is not in the report directory.
     */
    private String getUrl(File file) {
        if (mReportUrl == null) {
            return null;
        }

        final String filePath = file.getAbsolutePath();
        final String reportPath = mRootReportDir.getAbsolutePath();

        if (reportPath.endsWith(File.separator)) {
            CLog.w("Cannot create URL. getAbsolutePath() returned %s which ends with %s",
                    reportPath, File.separator);
            return null;
        }

        // Log file starts with the mReportDir path, so do a simple replacement.
        if (filePath.startsWith(reportPath)) {
            String relativePath = filePath.substring(reportPath.length());
            // relativePath should start with /, drop the / from the url if it exists.
            String url = mReportUrl;
            if (url.endsWith("/")) {
                url =  url.substring(0, url.length() - 1);
            }
            // FIXME: Sanitize the URL.
            return String.format("%s%s", url, relativePath.replace(File.separator, "/"));
        }

        return null;
    }

    /**
     * Set the report directory. Exposed for unit testing.
     */
    void setReportDir(File reportDir) {
        mRootReportDir = reportDir;
    }

    /**
     * Set the log retentionDays. Exposed for unit testing.
     */
    void setLogRetentionDays(int logRetentionDays) {
        mLogRetentionDays = logRetentionDays;
    }
}
