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

import com.android.ddmlib.Log.ILogOutput;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.result.InputStreamSource;

import java.io.BufferedInputStream;
import java.io.IOException;

/**
 * Classes which implement this interface provides methods that deal with outputting log
 * messages.
 */
public interface ILeveledLogOutput extends ILogOutput {

    /**
     * Initialize the log, creating any required IO resources.
     */
    public void init() throws IOException;

    /**
     * Gets the minimum log level to display.
     *
     * @return the current {@link LogLevel}
     */
    public LogLevel getLogLevel();

    /**
     * Sets the minimum log level to display.
     *
     * @param logLevel the {@link LogLevel} to display
     */
    public void setLogLevel(LogLevel logLevel);

    /**
     * Grabs a snapshot stream of the log data.
     * <p/>
     * Must not be called after {@link ILeveledLogOutput#closeLog()}.
     * <p/>
     * The returned stream is not guaranteed to have optimal performance. Callers may wish to
     * wrap result in a {@link BufferedInputStream}.
     *
     * @return a {@link InputStreamSource} of the log data
     * @throws IllegalStateException if called when log has been closed.
     */
    public InputStreamSource getLog();

    /**
     * Closes the log and performs any cleanup before closing, as necessary.
     */
    public void closeLog();

    /**
     * @return a {@link ILeveledLogOutput}
     */
    public ILeveledLogOutput clone();
}
