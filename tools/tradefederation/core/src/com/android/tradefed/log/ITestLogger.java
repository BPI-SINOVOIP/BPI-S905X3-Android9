/*
 * Copyright (C) 2016 The Android Open Source Project
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

import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;

/**
 * An entity that can perform logging of data streams of various types.
 */
public interface ITestLogger {

    /**
     * Provides the associated log or debug data from the test invocation.
     * <p/>
     * Must be called before {@link ITestInvocationListener#invocationFailed(Throwable)} or
     * {@link ITestInvocationListener#invocationEnded(long)}
     * <p/>
     * The TradeFederation framework will automatically call this method, providing the host log
     * and if applicable, the device logcat.
     *
     * @param dataName a {@link String} descriptive name of the data. e.g. "device_logcat". Note
     *            dataName may not be unique per invocation. ie implementers must be able to handle
     *            multiple calls with same dataName
     * @param dataType the {@link LogDataType} of the data
     * @param dataStream the {@link InputStreamSource} of the data. Implementers should call
     *        createInputStream to start reading the data, and ensure to close the resulting
     *        InputStream when complete. Callers should ensure the source of the data remains
     *        present and accessible until the testLog method completes.
     */
    default public void testLog(String dataName, LogDataType dataType,
            InputStreamSource dataStream) { }

}
