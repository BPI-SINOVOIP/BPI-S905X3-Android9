/*
 * Copyright (C) 2017 The Android Open Source Project
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
import com.android.tradefed.log.ILogRegistry.EventType;

import org.json.JSONObject;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

/** TF History Logger, special log that contains only some specific events. */
public class HistoryLogger extends FileLogger {
    private static final String TEMP_FILE_PREFIX = "tradefed_history_log_";
    private static final String TEMP_FILE_SUFFIX = ".txt";

    public HistoryLogger() {}

    /** Initialize the log, creating any required IO resources. */
    @Override
    public void init() throws IOException {
        init(TEMP_FILE_PREFIX, TEMP_FILE_SUFFIX);
    }

    /** {@inheritDoc} */
    @Override
    public void printAndPromptLog(LogLevel logLevel, String tag, String message) {
        throw new UnsupportedOperationException(
                "printAndPromptLog is not supported by HistoryLogger");
    }

    /** {@inheritDoc} */
    @Override
    public void printLog(LogLevel logLevel, String tag, String message) {
        throw new UnsupportedOperationException("printLog is not supported by HistoryLogger");
    }

    /**
     * Call this method to log an event from a type with the associated information in the map.
     *
     * @param event the {@link EventType} of the event to log.
     * @param args the map of arguments to be added to the log entry to get more details on the
     *     event.
     */
    public void logEvent(LogLevel logLevel, EventType event, Map<String, String> args) {
        StringBuilder message = new StringBuilder();
        message.append(event);
        message.append(": ");
        if (args == null) {
            args = new HashMap<>();
        }
        JSONObject formattedArgs = new JSONObject(args);
        message.append(formattedArgs.toString());
        message.append("\n");
        internalPrintLog(logLevel, message.toString());
    }

    /** internal version of printLog(...) which prints if the event is above the urgency level. */
    private void internalPrintLog(LogLevel logLevel, String message) {
        if (logLevel.getPriority() >= getLogLevelDisplay().getPriority()) {
            System.out.print(message);
        }
        try {
            writeToLog(message);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public ILeveledLogOutput clone() {
        FileLogger logger = new HistoryLogger();
        logger.setLogLevelDisplay(getLogLevelDisplay());
        logger.setLogLevel(getLogLevel());
        logger.addLogTagsDisplay(getLogTagsDisplay());
        return logger;
    }
}
