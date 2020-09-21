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
package com.android.tradefed.device;

/**
 * Thrown when a device is not able to connect to network for testing.
 * This usually gets thrown if a device fails to reconnect to wifi after reboot.
 */
@SuppressWarnings("serial")
public class NetworkNotAvailableException extends RuntimeException {
    /**
     * Creates a {@link NetworkNotAvailableException}.
     */
    public NetworkNotAvailableException() {
        super();
    }

    /**
     * Creates a {@link NetworkNotAvailableException}.
     *
     * @param msg a descriptive message.
     */
    public NetworkNotAvailableException(String msg) {
        super(msg);
    }

    /**
     * Creates a {@link NetworkNotAvailableException}.
     *
     * @param msg a descriptive message.
     * @param cause the root {@link Throwable} that caused the connection failure.
     */
    public NetworkNotAvailableException(String msg, Throwable cause) {
        super(msg, cause);
    }
}
