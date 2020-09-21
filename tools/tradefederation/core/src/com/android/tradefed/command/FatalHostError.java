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
package com.android.tradefed.command;

/**
 * An exception that indicates a fatal unrecoverable error has occurred on the host machine
 * running TradeFederation, and that the TradeFederation instance should be shut down.
 */
@SuppressWarnings("serial")
public class FatalHostError extends RuntimeException {

    /**
     * Creates a {@link FatalHostError}.
     *
     * @param msg the detailed message
     * @param cause the original cause of the fatal host error.
     *
     * @see RuntimeException#RuntimeException(String, Throwable)
     */
    public FatalHostError(String msg, Throwable cause) {
        super(msg, cause);
    }

    /**
     * Creates a {@link FatalHostError}.
     *
     * @param msg the detailed message
     *
     * @see RuntimeException#RuntimeException(String)
     */
    public FatalHostError(String msg) {
        super(msg);
    }
}
