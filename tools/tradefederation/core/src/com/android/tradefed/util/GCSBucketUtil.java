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

package com.android.tradefed.util;

import com.android.ddmlib.Log;
import com.android.tradefed.log.LogUtil.CLog;

import java.io.File;
import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * File manager to download and upload files from Google Cloud Storage (GCS).
 *
 * This class should NOT be used from the scope of a test (i.e., IRemoteTest).
 */
public class GCSBucketUtil {

    // https://cloud.google.com/storage/docs/gsutil

    private static final String CMD_COPY = "cp";
    private static final String CMD_MAKE_BUCKET = "mb";
    private static final String CMD_REMOVE = "rm";
    private static final String CMD_REMOVE_BUCKET = "rb";
    private static final String CMD_VERSION = "-v";
    private static final String ENV_BOTO_PATH = "BOTO_PATH";
    private static final String ENV_BOTO_CONFIG = "BOTO_CONFIG";
    private static final String FILENAME_STDOUT = "-";
    private static final String FLAG_FORCE = "-f";
    private static final String FLAG_NO_CLOBBER = "-n";
    private static final String FLAG_PARALLEL = "-m";
    private static final String FLAG_PROJECT_ID = "-p";
    private static final String FLAG_RECURSIVE = "-r";
    private static final String GCS_SCHEME = "gs";
    private static final String GSUTIL = "gsutil";

    /**
     * Whether gsutil is verified to be installed
     */
    private static boolean mCheckedGsutil = false;

    /**
     * Number of attempts for gsutil operations.
     *
     * @see RunUtil#runTimedCmdRetry
     */
    private int mAttempts = 1;

    /**
     * Path to the .boto files to use, set via environment variable $BOTO_PATH.
     *
     * @see <a href="https://cloud.google.com/storage/docs/gsutil/commands/config">
     *      gsutil documentation</a>
     */
    private String mBotoPath = null;

    /**
     * Path to the .boto file to use, set via environment variable $BOTO_CONFIG.
     *
     * @see <a href="https://cloud.google.com/storage/docs/gsutil/commands/config">
     *      gsutil documentation</a>
     */
    private String mBotoConfig = null;

    /**
     * Name of the GCS bucket.
     */
    private String mBucketName = null;

    /**
     * Whether to use the "-n" flag to avoid clobbering files.
     */
    private boolean mNoClobber = false;

    /**
     * Whether to use the "-m" flag to parallelize large operations.
     */
    private boolean mParallel = false;

    /**
     * Whether to use the "-r" flag to perform a recursive copy.
     */
    private boolean mRecursive = true;

    /**
     * Retry interval for gsutil operations.
     *
     * @see RunUtil#runTimedCmdRetry
     */
    private long mRetryInterval = 0;

    /**
     * Timeout for gsutil operations.
     *
     * @see RunUtil#runTimedCmdRetry
     */
    private long mTimeoutMs = 0;

    public GCSBucketUtil(String bucketName) {
        setBucketName(bucketName);
    }

    /**
     * Verify that gsutil is installed.
     */
    void checkGSUtil() throws IOException {
        if (mCheckedGsutil) {
            return;
        }

        // N.B. We don't use retry / attempts here, since this doesn't involve any RPC.
        CommandResult res = getRunUtil()
                .runTimedCmd(mTimeoutMs, GSUTIL, CMD_VERSION);

        if (!CommandStatus.SUCCESS.equals(res.getStatus())) {
            throw new IOException(
                    "gsutil is not installed.\n"
                            + "https://cloud.google.com/storage/docs/gsutil for instructions.");
        }

        mCheckedGsutil = true;
    }

    /**
     * Copy a file or directory to or from the bucket.
     *
     * @param source Source file or pattern
     * @param dest Destination file or pattern
     * @return {@link CommandResult} result of the operation.
     */
    public CommandResult copy(String source, String dest) throws IOException {
        checkGSUtil();
        CLog.d("Copying %s => %s", source, dest);

        IRunUtil run = getRunUtil();
        List<String> command = new ArrayList<>();

        command.add(GSUTIL);

        if (mParallel) {
            command.add(FLAG_PARALLEL);
        }

        command.add(CMD_COPY);

        if (mRecursive) {
            command.add(FLAG_RECURSIVE);
        }

        if (mNoClobber) {
            command.add(FLAG_NO_CLOBBER);
        }

        command.add(source);
        command.add(dest);

        String[] commandAsStr = command.toArray(new String[0]);

        CommandResult res = run
                .runTimedCmdRetry(mTimeoutMs, mRetryInterval, mAttempts, commandAsStr);
        if (!CommandStatus.SUCCESS.equals(res.getStatus())) {
            throw new IOException(
                    String.format(
                            "Failed to copy '%s' -> '%s' with %s\nstdout: %s\nstderr: %s",
                            source,
                            dest,
                            res.getStatus(),
                            res.getStdout(),
                            res.getStderr()));
        }
        return res;
    }

    public int getAttempts() {
        return mAttempts;
    }

    public String getBotoConfig() {
        return mBotoConfig;
    }

    public String getBotoPath() {
        return mBotoPath;
    }

    public String getBucketName() {
        return mBucketName;
    }

    public boolean getNoClobber() {
        return mNoClobber;
    }

    public boolean getParallel() {
        return mParallel;
    }

    public boolean getRecursive() {
        return mRecursive;
    }

    public long getRetryInterval() {
        return mRetryInterval;
    }

    protected IRunUtil getRunUtil() {
        IRunUtil run = new RunUtil();

        if (mBotoPath != null) {
            run.setEnvVariable(ENV_BOTO_PATH, mBotoPath);
        }

        if (mBotoConfig != null) {
            run.setEnvVariable(ENV_BOTO_CONFIG, mBotoConfig);
        }

        return run;
    }

    public long getTimeout() {
        return mTimeoutMs;
    }

    /**
     * Retrieve the gs://bucket/path URI
     */
    String getUriForGcsPath(Path path) {
        // N.B. Would just use java.net.URI, but it doesn't allow e.g. underscores,
        // which are valid in GCS bucket names.
        if (!path.isAbsolute()) {
            path = Paths.get("/").resolve(path);
        }
        return String.format("%s://%s%s", GCS_SCHEME, mBucketName, path.toString());
    }

    /**
     * Make the GCS bucket.
     *
     * @return {@link CommandResult} result of the operation.
     * @throws IOException
     */
    public CommandResult makeBucket(String projectId) throws IOException {
        checkGSUtil();
        CLog.d("Making bucket %s for project %s", mBucketName, projectId);

        List<String> command = new ArrayList<>();
        command.add(GSUTIL);
        command.add(CMD_MAKE_BUCKET);

        if (projectId != null) {
            command.add(FLAG_PROJECT_ID);
            command.add(projectId);
        }

        command.add(getUriForGcsPath(Paths.get("/")));

        CommandResult res = getRunUtil()
                .runTimedCmdRetry(mTimeoutMs, mRetryInterval, mAttempts,
                        command.toArray(new String[0]));

        if (!CommandStatus.SUCCESS.equals(res.getStatus())) {
            throw new IOException(
                    String.format(
                            "Failed to create bucket '%s' with %s\nstdout: %s\nstderr: %s",
                            mBucketName,
                            res.getStatus(),
                            res.getStdout(),
                            res.getStderr()));
        }

        return res;
    }

    /**
     * Download a file or directory from a GCS bucket to the current directory.
     *
     * @param bucketPath File path in the GCS bucket
     * @return {@link CommandResult} result of the operation.
     */
    public CommandResult pull(Path bucketPath) throws IOException {
        return copy(getUriForGcsPath(bucketPath), ".");
    }

    /**
     * Download a file or directory from a GCS bucket.
     *
     * @param bucketPath File path in the GCS bucket
     * @param localFile Local destination path
     * @return {@link CommandResult} result of the operation.
     */
    public CommandResult pull(Path bucketPath, File localFile) throws IOException {
        return copy(getUriForGcsPath(bucketPath), localFile.getPath());
    }

    /**
     * Download a file from a GCS bucket, and extract its contents.
     *
     * @param bucketPath File path in the GCS bucket
     * @return String contents of the file
     */
    public String pullContents(Path bucketPath) throws IOException {
        CommandResult res = copy(getUriForGcsPath(bucketPath), FILENAME_STDOUT);
        return res.getStdout();
    }

    /**
     * Upload a local file or directory to a GCS bucket.
     *
     * @param localFile Local file or directory
     * @return {@link CommandResult} result of the operation.
     */
    public CommandResult push(File localFile) throws IOException {
        return push(localFile, Paths.get("/"));
    }

    /**
     * Upload a local file or directory to a GCS bucket with a specific path.
     *
     * @param localFile Local file or directory
     * @param bucketPath File path in the GCS bucket
     * @return {@link CommandResult} result of the operation.
     */
    public CommandResult push(File localFile, Path bucketPath) throws IOException {
        return copy(localFile.getAbsolutePath(), getUriForGcsPath(bucketPath));
    }

    /**
     * Upload a String to a GCS bucket.
     *
     * @param contents File contents, as a string
     * @param bucketPath File path in the GCS bucket
     * @return {@link CommandResult} result of the operation.
     */
    public CommandResult pushString(String contents, Path bucketPath) throws IOException {
        File localFile = null;
        try {
            localFile = FileUtil.createTempFile(mBucketName, null);
            FileUtil.writeToFile(contents, localFile);
            return copy(localFile.getAbsolutePath(), getUriForGcsPath(bucketPath));
        } finally {
            FileUtil.deleteFile(localFile);
        }
    }

    /**
     * Remove a file or directory from the bucket.
     *
     * @param pattern File, directory, or pattern to remove.
     * @param force Whether to ignore failures and continue silently (will not throw)
     */
    public CommandResult remove(String pattern, boolean force) throws IOException {
        checkGSUtil();
        String path = getUriForGcsPath(Paths.get(pattern));
        Log.d("Removing file(s) %s", path);

        List<String> command = new ArrayList<>();
        command.add(GSUTIL);
        command.add(CMD_REMOVE);

        if (mRecursive) {
            command.add(FLAG_RECURSIVE);
        }

        if (force) {
            command.add(FLAG_FORCE);
        }

        command.add(path);

        CommandResult res = getRunUtil()
                .runTimedCmdRetry(mTimeoutMs, mRetryInterval, mAttempts,
                        command.toArray(new String[0]));

        if (!force && !CommandStatus.SUCCESS.equals(res.getStatus())) {
            throw new IOException(
                    String.format(
                            "Failed to remove '%s' with %s\nstdout: %s\nstderr: %s",
                            pattern,
                            res.getStatus(),
                            res.getStdout(),
                            res.getStderr()));
        }
        return res;
    }

    /**
     * Remove a file or directory from the bucket.
     *
     * @param pattern File, directory, or pattern to remove.
     */
    public CommandResult remove(String pattern) throws IOException {
        return remove(pattern, false);
    }

    /**
     * Remove a file or directory from the bucket.
     *
     * @param path Path to remove
     * @param force Whether to fail if the file does not exist
     */
    public CommandResult remove(Path path, boolean force) throws IOException {
        return remove(path.toString(), force);
    }

    /**
     * Remove a file or directory from the bucket.
     *
     * @param path Path to remove
     */
    public CommandResult remove(Path path) throws IOException {
        return remove(path.toString(), false);
    }


    /**
     * Remove the GCS bucket
     *
     * @throws IOException
     */
    public CommandResult removeBucket() throws IOException {
        checkGSUtil();
        Log.d("Removing bucket %s", mBucketName);

        String[] command = {
                GSUTIL,
                CMD_REMOVE_BUCKET,
                getUriForGcsPath(Paths.get("/"))
        };

        CommandResult res = getRunUtil()
                .runTimedCmdRetry(mTimeoutMs, mRetryInterval, mAttempts, command);

        if (!CommandStatus.SUCCESS.equals(res.getStatus())) {
            throw new IOException(
                    String.format(
                            "Failed to remove bucket '%s' with %s\nstdout: %s\nstderr: %s",
                            mBucketName,
                            res.getStatus(),
                            res.getStdout(),
                            res.getStderr()));
        }

        return res;
    }

    public void setAttempts(int attempts) {
        mAttempts = attempts;
    }

    public void setBotoConfig(String botoConfig) {
        mBotoConfig = botoConfig;
    }

    public void setBotoPath(String botoPath) {
        mBotoPath = botoPath;
    }

    public void setBucketName(String bucketName) {
        mBucketName = bucketName;
    }

    public void setNoClobber(boolean noClobber) {
        mNoClobber = noClobber;
    }

    public void setParallel(boolean parallel) {
        mParallel = parallel;
    }

    public void setRecursive(boolean recursive) {
        mRecursive = recursive;
    }

    public void setRetryInterval(long retryInterval) {
        mRetryInterval = retryInterval;
    }

    public void setTimeoutMs(long timeout) {
        mTimeoutMs = timeout;
    }

    public void setTimeout(long timeout, TimeUnit unit) {
        setTimeoutMs(unit.toMillis(timeout));
    }

}
