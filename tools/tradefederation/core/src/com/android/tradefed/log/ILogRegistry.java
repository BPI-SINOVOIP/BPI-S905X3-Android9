/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.ddmlib.Log.ILogOutput;
import com.android.ddmlib.Log.LogLevel;

import java.util.Collection;
import java.util.Map;

/**
 * An interface for a {@link ILogOutput} singleton logger that multiplexes and manages different
 * loggers.
 */
public interface ILogRegistry extends ILogOutput {

    /** Events that are useful to be logged */
    public enum EventType {
        DEVICE_CONNECTED,
        DEVICE_CONNECTED_OFFLINE,
        DEVICE_DISCONNECTED,
        INVOCATION_START,
        INVOCATION_END,
        HEAP_MEMORY,
        SHARD_POLLER_EARLY_TERMINATION,
        MODULE_DEVICE_NOT_AVAILABLE,
    }

    /**
     * Set the log level display for the global log
     *
     * @param logLevel the {@link LogLevel} to use
     */
    public void setGlobalLogDisplayLevel(LogLevel logLevel);

    /**
     * Set the log tags to display for the global log
     */
    public void setGlobalLogTagDisplay(Collection<String> logTagsDisplay);

    /**
     * Returns current log level display for the global log
     *
     * @return logLevel the {@link LogLevel} to use
     */
    public LogLevel getGlobalLogDisplayLevel();

    /**
     * Registers the logger as the instance to use for the current thread.
     */
    public void registerLogger(ILeveledLogOutput log);

    /**
     * Unregisters the current logger in effect for the current thread.
     */
    public void unregisterLogger();

    /**
     * Dumps the entire contents of a {@link ILeveledLogOutput} logger to the global log.
     * <p/>
     * This is useful in scenarios where you know the logger's output won't be saved permanently,
     * yet you want the contents to be saved somewhere and not lost.
     *
     * @param log
     */
    public void dumpToGlobalLog(ILeveledLogOutput log);

    /**
     * Closes and removes all logs being managed by this LogRegistry.
     */
    public void closeAndRemoveAllLogs();

    /** Saves all the global loggers contents to tmp files. */
    public void saveGlobalLog();

    /**
     * Call this method to log an event from a type with the associated information in the map. Time
     * of the event is automatically added.
     *
     * @param logLevel the {@link LogLevel} to be printed.
     * @param event the {@link ILogRegistry.EventType} of the event to log.
     * @param args the map of arguments to be added to the log entry to get more details on the
     *     event.
     */
    public void logEvent(LogLevel logLevel, EventType event, Map<String, String> args);

    /** Diagnosis method to dump all logs to files. */
    public void dumpLogs();

}
