/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.tradefed.util;


/**
 * Thrown when a run operation is interrupted by an external request.
 */
@SuppressWarnings("serial")
public class RunInterruptedException extends RuntimeException {
    /**
     * Creates a {@link RunInterruptedException}.
     */
    public RunInterruptedException() {
        super();
    }

    /**
     * Creates a {@link RunInterruptedException}.
     *
     * @param msg a descriptive message.
     */
    public RunInterruptedException(String msg) {
        super(msg);
    }

    /**
     * Creates a {@link RunInterruptedException}.
     *
     * @param cause the root {@link Throwable} that caused the device to become unavailable.
     */
    public RunInterruptedException(Throwable cause) {
        super(cause);
    }

    /**
     * Creates a {@link RunInterruptedException}.
     *
     * @param msg a descriptive message.
     * @param cause the root {@link Throwable} that caused the device to become unavailable.
     */
    public RunInterruptedException(String msg, Throwable cause) {
        super(msg, cause);
    }
}
