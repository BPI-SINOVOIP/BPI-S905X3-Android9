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
 * Thrown when there's no device to execute a given command.
 */
@SuppressWarnings("serial")
public class NoDeviceException extends Exception {
    /**
     * Creates a {@link NoDeviceException}.
     */
    public NoDeviceException() {
        super();
    }

    /**
     * Creates a {@link NoDeviceException}.
     *
     * @param msg a descriptive message.
     */
    public NoDeviceException(String msg) {
        super(msg);
    }

    /**
     * Creates a {@link NoDeviceException}.
     *
     * @param msg a descriptive message.
     * @param cause the root {@link Throwable} that caused the device to become unavailable.
     */
    public NoDeviceException(String msg, Throwable cause) {
        super(msg, cause);
    }
}
