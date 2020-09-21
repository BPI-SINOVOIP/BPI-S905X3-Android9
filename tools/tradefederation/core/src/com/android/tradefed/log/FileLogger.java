/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.tradefed.log;

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.SnapshotInputStreamSource;
import com.android.tradefed.util.SizeLimitedOutputStream;
import com.android.tradefed.util.StreamUtil;

import java.io.IOException;
import java.io.InputStream;
import java.util.Collection;
import java.util.HashSet;

/**
 * A {@link ILeveledLogOutput} that directs log messages to a file and to stdout.
 */
@OptionClass(alias = "file")
public class FileLogger implements ILeveledLogOutput {
    private static final String TEMP_FILE_PREFIX = "tradefed_log_";
    private static final String TEMP_FILE_SUFFIX = ".txt";

    @Option(name = "log-level", description = "the minimum log level to log.")
    private LogLevel mLogLevel = LogLevel.DEBUG;

    @Option(name = "log-level-display", shortName = 'l',
            description = "the minimum log level to display on stdout.",
            importance = Importance.ALWAYS)
    private LogLevel mLogLevelDisplay = LogLevel.ERROR;

    @Option(name = "log-tag-display", description = "Always display given tags logs on stdout")
    private Collection<String> mLogTagsDisplay = new HashSet<String>();

    @Option(name = "max-log-size", description = "maximum allowable size of tmp log data in mB.")
    private long mMaxLogSizeMbytes = 20;

    private SizeLimitedOutputStream mLogStream;

    /**
     * Adds tags to the log-tag-display list
     *
     * @param tags collection of tags to add
     */
    void addLogTagsDisplay(Collection<String> tags) {
        mLogTagsDisplay.addAll(tags);
    }

    /** Returns the collection of tags to always display on stdout. */
    Collection<String> getLogTagsDisplay() {
        return mLogTagsDisplay;
    }

    public FileLogger() {
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void init() throws IOException {
        init(TEMP_FILE_PREFIX, TEMP_FILE_SUFFIX);
    }

    /**
     * Alternative to {@link #init()} where we can specify the file name and suffix.
     *
     * @param logPrefix the file name where to log without extension.
     * @param fileSuffix the extension of the file where to log.
     */
    protected void init(String logPrefix, String fileSuffix) {
        mLogStream =
                new SizeLimitedOutputStream(mMaxLogSizeMbytes * 1024 * 1024, logPrefix, fileSuffix);
    }

    /**
     * Creates a new {@link FileLogger} with the same log level settings as the current object.
     * <p/>
     * Does not copy underlying log file content (ie the clone's log data will be written to a new
     * file.)
     */
    @Override
    public ILeveledLogOutput clone()  {
        FileLogger logger = new FileLogger();
        OptionCopier.copyOptionsNoThrow(this, logger);
        return logger;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void printAndPromptLog(LogLevel logLevel, String tag, String message) {
        internalPrintLog(logLevel, tag, message, true /* force print to stdout */);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void printLog(LogLevel logLevel, String tag, String message) {
        internalPrintLog(logLevel, tag, message, false /* don't force stdout */);
    }

    /**
     * A version of printLog(...) which can be forced to print to stdout, even if the log level
     * isn't above the urgency threshold.
     */
    private void internalPrintLog(LogLevel logLevel, String tag, String message,
            boolean forceStdout) {
        String outMessage = LogUtil.getLogFormatString(logLevel, tag, message);
        if (forceStdout
                || logLevel.getPriority() >= mLogLevelDisplay.getPriority()
                || mLogTagsDisplay.contains(tag)) {
            System.out.print(outMessage);
        }
        try {
            writeToLog(outMessage);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * Writes given message to log.
     * <p/>
     * Exposed for unit testing.
     *
     * @param outMessage the entry to write to log
     * @throws IOException
     */
    void writeToLog(String outMessage) throws IOException {
        if (mLogStream != null) {
            mLogStream.write(outMessage.getBytes());
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public LogLevel getLogLevel() {
        return mLogLevel;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setLogLevel(LogLevel logLevel) {
        mLogLevel = logLevel;
    }

    /**
     * Sets the log level filtering for stdout.
     *
     * @param logLevel the minimum {@link LogLevel} to display
     */
    void setLogLevelDisplay(LogLevel logLevel) {
        mLogLevelDisplay = logLevel;
    }

    /**
     * Gets the log level filtering for stdout.
     *
     * @return the current {@link LogLevel}
     */
    LogLevel getLogLevelDisplay() {
        return mLogLevelDisplay;
    }

    /** Returns the max log size of the log in MBytes. */
    public long getMaxLogSizeMbytes() {
        return mMaxLogSizeMbytes;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public InputStreamSource getLog() {
        if (mLogStream != null) {
            try {
                // create a InputStream from log file
                mLogStream.flush();
                return new SnapshotInputStreamSource("FileLogger", mLogStream.getData());
            } catch (IOException e) {
                System.err.println("Failed to get log");
                e.printStackTrace();
            }
        }
        return new ByteArrayInputStreamSource(new byte[0]);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void closeLog() {
        doCloseLog();
    }

    /**
     * Flushes stream and closes log file.
     * <p/>
     * Exposed for unit testing.
     */
    void doCloseLog() {
        SizeLimitedOutputStream stream = mLogStream;
        mLogStream = null;
        StreamUtil.flushAndCloseStream(stream);
        if (stream != null) {
            stream.delete();
        }
    }

    /**
     * Dump the contents of the input stream to this log
     *
     * @param inputStream
     * @throws IOException
     */
    void dumpToLog(InputStream inputStream) throws IOException {
        if (mLogStream != null) {
            StreamUtil.copyStreams(inputStream, mLogStream);
        }
    }
}
