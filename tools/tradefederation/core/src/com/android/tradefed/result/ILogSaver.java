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

import com.android.tradefed.invoker.IInvocationContext;

import java.io.IOException;
import java.io.InputStream;

/**
 * Classes which implement this interface provide methods for storing logs to a central location.
 * <p>
 * A {@link ILogSaver} is declared in the configuration and is responsible for storing logs to a
 * central location. It also exposes methods so {@link ILogSaverListener}s may save additional files
 * to the same location.
 * </p>
 */
public interface ILogSaver {

    /**
     * Reports the start of the test invocation.
     * <p>
     * Will be automatically called by the TradeFederation framework before
     * {@link ITestInvocationListener#invocationStarted(IInvocationContext)} is called.
     * </p>
     *
     * @param context information about the invocation.
     */
    public void invocationStarted(IInvocationContext context);

    /**
     * Reports that the invocation has terminated, whether successfully or due to some error
     * condition.
     * <p>
     * Will be automatically called by the TradeFederation framework after
     * {@link ITestInvocationListener#invocationEnded(long)} is called.
     * </p>
     *
     * @param elapsedTime the elapsed time of the invocation in ms
     */
    public void invocationEnded(long elapsedTime);

    /**
     * Save the log data.
     * <p>
     * Will be automatically called by the TradeFederation framework whenever
     * {@link ITestInvocationListener#testLog(String, LogDataType, InputStreamSource)} is called. It
     * may also be used as a helper method to save additional log data.
     * </p><p>
     * Depending on the implementation and policy, the logs may be saved in a compressed form. Logs
     * may also be stored in a location inaccessable to Tradefed.
     * </p>
     *
     * @param dataName a {@link String} descriptive name of the data. e.g. "device_logcat"
     * @param dataType the {@link LogDataType} of the file.
     * @param dataStream the {@link InputStream} of the data.
     * @return the {@link LogFile} containing the path and URL of the saved file.
     * @throws IOException if log file could not be generated
     */
    public LogFile saveLogData(String dataName, LogDataType dataType, InputStream dataStream)
            throws IOException;

    /**
     * A helper method to save the log data unmodified.
     *
     * <p>Logs may be stored in a location inaccessible to Tradefed.
     *
     * @param dataName a {@link String} descriptive name of the data. e.g. "device_logcat".
     * @param type a {@link LogDataType} containing the type and the extension of the file
     * @param dataStream the {@link InputStream} of the data.
     * @return the {@link LogFile} containing the path and URL of the saved file.
     * @throws IOException if log file could not be generated
     */
    public LogFile saveLogDataRaw(String dataName, LogDataType type, InputStream dataStream)
            throws IOException;

    /**
     * Get the {@link LogFile} containing the path and/or URL of the directory where logs are saved.
     *
     * @return The {@link LogFile}.
     */
    public LogFile getLogReportDir();
}
