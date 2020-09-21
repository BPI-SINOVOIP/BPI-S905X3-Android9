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

/**
 * Allows for {@link ITestInvocationListener}s to listen for when log files are saved.
 * <p>
 * This allows for multiple {@link ITestInvocationListener}s to use the same saved log file when
 * generating reports, and avoids having each listener save the file individually when
 * {@link ITestInvocationListener#testLog(String, LogDataType, InputStreamSource)} is called.
 * </p><p>
 * Classes implementing this interface should be aware that
 * {@link #testLogSaved(String, LogDataType, InputStreamSource, LogFile)} will be called whenever
 * {@link ITestInvocationListener#testLog(String, LogDataType, InputStreamSource)} is called.
 * </p><p>
 * This class also passes the global {@link ILogSaver} instance so {@link ITestInvocationListener}s
 * can save additional files in the same location.
 */
public interface ILogSaverListener extends ITestInvocationListener {

    /**
     * Called when the test log is saved.
     * <p>
     * Should be used in place of
     * {@link ITestInvocationListener#testLog(String, LogDataType, InputStreamSource)}.
     * </p>
     * @param dataName a {@link String} descriptive name of the data. e.g. "device_logcat". Note
     * dataName may not be unique per invocation. ie implementers must be able to handle multiple
     * calls with same dataName
     * @param dataType the {@link LogDataType} of the data
     * @param dataStream the {@link InputStreamSource} of the data. Implementers should call
     * createInputStream to start reading the data, and ensure to close the resulting InputStream
     * when complete.
     * @param logFile the {@link LogFile} containing the meta data of the saved file.
     */
    public void testLogSaved(String dataName, LogDataType dataType, InputStreamSource dataStream,
            LogFile logFile);

    /**
     * In some cases, log must be strongly associated with a test cases, but the opportunity to do
     * so on the direct {@link #testLogSaved(String, LogDataType, InputStreamSource, LogFile)}
     * callback is not possible. Thus, this callback allows to provide a strong association
     * explicitly.
     *
     * @param dataName The name of the data
     * @param logFile the {@link LogFile} that was logged before and should be associated with the
     *     test case.
     */
    public default void logAssociation(String dataName, LogFile logFile) {
        // Do nothing by default
    }

    /**
     * Set the {@link ILogSaver} to allow the implementor to save files.
     *
     * @param logSaver the {@link ILogSaver}
     */
    public void setLogSaver(ILogSaver logSaver);
}
