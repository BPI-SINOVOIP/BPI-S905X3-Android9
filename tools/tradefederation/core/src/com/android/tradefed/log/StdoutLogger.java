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
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.InputStreamSource;

import java.io.IOException;

/**
 * A {@link ILeveledLogOutput} that directs log messages to stdout.
 */
@OptionClass(alias = "stdout")
public class StdoutLogger implements ILeveledLogOutput {

    @Option(name="log-level", description="minimum log level to display.",
            importance = Importance.ALWAYS)
    private LogLevel mLogLevel = LogLevel.INFO;

    /**
     * {@inheritDoc}
     */
    @Override
    public void printAndPromptLog(LogLevel logLevel, String tag, String message) {
        printLog(logLevel, tag, message);

    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void printLog(LogLevel logLevel, String tag, String message) {
        LogUtil.printLog(logLevel, tag, message);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setLogLevel(LogLevel logLevel) {
        mLogLevel = logLevel;
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
    public void closeLog() {
        // ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public InputStreamSource getLog() {
        // not supported - return empty stream
        return new ByteArrayInputStreamSource(new byte[0]);
    }

    @Override
    public ILeveledLogOutput clone()  {
        return new StdoutLogger();
    }

    @Override
    public void init() throws IOException {
        // ignore
    }
}
