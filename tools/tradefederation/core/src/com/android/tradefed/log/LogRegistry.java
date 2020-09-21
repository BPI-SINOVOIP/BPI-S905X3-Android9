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

import com.android.ddmlib.Log;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.Collection;
import java.util.Date;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Map;

/**
 * A {@link ILogRegistry} implementation that multiplexes and manages different loggers,
 * using the appropriate one based on the {@link ThreadGroup} of the thread making the call.
 * <p/>
 * Note that the registry hashes on the ThreadGroup in which a thread belongs. If a thread is
 * spawned with its own explicitly-supplied ThreadGroup, it will not inherit the parent thread's
 * logger, and thus will need to register its own logger with the LogRegistry if it wants to log
 * output.
 */
public class LogRegistry implements ILogRegistry {
    private static final String LOG_TAG = "LogRegistry";
    private static final String GLOBAL_LOG_PREFIX = "tradefed_global_log_";
    private static final String HISTORY_LOG_PREFIX = "tradefed_history_log_";
    private static LogRegistry mLogRegistry = null;
    private Map<ThreadGroup, ILeveledLogOutput> mLogTable = new Hashtable<>();
    private FileLogger mGlobalLogger;
    private HistoryLogger mHistoryLogger;

    /**
     * Package-private constructor; callers should use {@link #getLogRegistry} to get an instance of
     * the {@link LogRegistry}.
     */
    LogRegistry() {
        try {
            mGlobalLogger = new FileLogger();
            mGlobalLogger.init();
        } catch (IOException e) {
            System.err.println("Failed to create global logger");
            throw new IllegalStateException(e);
        }
        try {
            mHistoryLogger = new HistoryLogger();
            mHistoryLogger.init();
        } catch (IOException e) {
            System.err.println("Failed to create history logger");
            throw new IllegalStateException(e);
        }
    }

    /**
     * Get the {@link LogRegistry} instance
     * <p/>
     *
     * @return a {@link LogRegistry} that can be used to register, get, write to, and close logs
     */
    public static ILogRegistry getLogRegistry() {
        if (mLogRegistry == null) {
            mLogRegistry = new LogRegistry();
        }

        return mLogRegistry;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setGlobalLogDisplayLevel(LogLevel logLevel) {
        mGlobalLogger.setLogLevelDisplay(logLevel);
        mHistoryLogger.setLogLevel(logLevel);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setGlobalLogTagDisplay(Collection<String> logTagsDisplay) {
        mGlobalLogger.addLogTagsDisplay(logTagsDisplay);
        mHistoryLogger.addLogTagsDisplay(logTagsDisplay);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public LogLevel getGlobalLogDisplayLevel() {
        return mGlobalLogger.getLogLevelDisplay();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void registerLogger(ILeveledLogOutput log) {
        synchronized (mLogTable) {
            ILeveledLogOutput oldValue = mLogTable.put(getCurrentThreadGroup(), log);
            if (oldValue != null) {
                Log.e(LOG_TAG, "Registering a new logger when one already exists for this thread!");
                oldValue.closeLog();
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void unregisterLogger() {
        ThreadGroup currentThreadGroup = getCurrentThreadGroup();
        if (currentThreadGroup != null) {
            synchronized (mLogTable) {
                mLogTable.remove(currentThreadGroup);
            }
        } else {
            printLog(LogLevel.ERROR, LOG_TAG, "Unregistering when thread has no logger "
                    + "registered.");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dumpToGlobalLog(ILeveledLogOutput log) {
        try (InputStreamSource source = log.getLog();
                InputStream stream = source.createInputStream()) {
            mGlobalLogger.dumpToLog(stream);
        } catch (IOException e) {
            System.err.println("Failed to dump log");
            e.printStackTrace();
        }
    }

    /**
     * Gets the current thread Group.
     * <p/>
     * Exposed so unit tests can mock
     *
     * @return the ThreadGroup that the current thread belongs to
     */
    ThreadGroup getCurrentThreadGroup() {
        return Thread.currentThread().getThreadGroup();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void printLog(LogLevel logLevel, String tag, String message) {
        ILeveledLogOutput log = getLogger();
        LogLevel currentLogLevel = log.getLogLevel();
        if (logLevel.getPriority() >= currentLogLevel.getPriority()) {
            log.printLog(logLevel, tag, message);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void printAndPromptLog(LogLevel logLevel, String tag, String message) {
        getLogger().printAndPromptLog(logLevel, tag, message);
    }

    /**
     * Gets the underlying logger associated with this thread.
     *
     * @return the logger for this thread, or null if one has not been registered.
     */
    ILeveledLogOutput getLogger() {
        synchronized (mLogTable) {
            ILeveledLogOutput log = mLogTable.get(getCurrentThreadGroup());
            if (log == null) {
                // If there's no logger set for this thread, use global logger
                log = mGlobalLogger;
            }
            return log;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void closeAndRemoveAllLogs() {
        synchronized (mLogTable) {
            Collection<ILeveledLogOutput> allLogs = mLogTable.values();
            Iterator<ILeveledLogOutput> iter = allLogs.iterator();
            while (iter.hasNext()) {
                ILeveledLogOutput log = iter.next();
                log.closeLog();
                iter.remove();
            }
        }
        saveGlobalLog();
        mGlobalLogger.closeLog();
        // TODO: enable saving the history log when TF close
        // saveHistoryToDirLog();
        mHistoryLogger.closeLog();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void saveGlobalLog() {
        saveGlobalLogToDir(null);
    }

    /** {@inheritDoc} */
    @Override
    public void logEvent(LogLevel logLevel, EventType event, Map<String, String> args) {
        // We always add the time of the event
        SimpleDateFormat sdfDate = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS Z");
        args.put("time", sdfDate.format(new Date()));
        mHistoryLogger.logEvent(logLevel, event, args);
    }

    /**
     * Save the global log data to a file in the specified directory.
     *
     * @param dir directory to save file, can be null, file will be saved in tmp directory.
     */
    private void saveGlobalLogToDir(File dir) {
        try (InputStreamSource globalLog = mGlobalLogger.getLog()) {
            saveLog(GLOBAL_LOG_PREFIX, globalLog, dir);
        }
    }

    /**
     * Save the history log data to a file in the specified directory.
     *
     * @param dir directory to save file, can be null, file will be saved in tmp directory.
     */
    private void saveHistoryLogToDir(File dir) {
        try (InputStreamSource globalLog = mHistoryLogger.getLog()) {
            saveLog(HISTORY_LOG_PREFIX, globalLog, dir);
        }
    }

    /**
     * Save log data to a temporary file
     *
     * @param filePrefix the file name prefix
     * @param logData the textual log data
     */
    private void saveLog(String filePrefix, InputStreamSource logData, File parentdir) {
        try {
            File tradefedLog = FileUtil.createTempFile(filePrefix, ".txt", parentdir);
            FileUtil.writeToFile(logData.createInputStream(), tradefedLog);
            System.out.println(String.format("Saved log to %s", tradefedLog.getAbsolutePath()));
        } catch (IOException e) {
            // ignore
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dumpLogs() {
        dumpLogsToDir(null);
    }

    /**
     * Save the log data to files in the specified directory.
     *
     * @param dir directory to save file, can be null, file will be saved in tmp directory.
     */
    public void dumpLogsToDir(File dir) {
        synchronized (mLogTable) {
            for (Map.Entry<ThreadGroup, ILeveledLogOutput> logEntry : mLogTable.entrySet()) {
                // use thread group name as file name - assume its descriptive
                String filePrefix = String.format("%s_log_", logEntry.getKey().getName());
                try (InputStreamSource logSource = logEntry.getValue().getLog()) {
                    saveLog(filePrefix, logSource, dir);
                }
            }
        }
        // save history log
        saveHistoryLogToDir(dir);
        // save global log last
        saveGlobalLogToDir(dir);
    }
}
