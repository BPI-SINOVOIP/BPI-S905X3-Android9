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
package com.android.tradefed.device;

/**
 * Thrown when a device is no longer reachable via it's transport type. e.g. if the device is no
 * longer visible via USB, or TCP/IP connection
 */
public class DeviceDisconnectedException extends DeviceNotAvailableException {

    private static final long serialVersionUID = 1L;

    /**
     * Creates a {@link DeviceDisconnectedException}.
     */
    public DeviceDisconnectedException() {
        super();
    }

    /**
     * Creates a {@link DeviceDisconnectedException}.
     *
     * @param msg a descriptive message.
     *
     * @deprecated use {@link #DeviceDisconnectedException(String msg, String serial)} instead
     */
    @Deprecated
    public DeviceDisconnectedException(String msg) {
        super(msg);
    }

    /**
     * Creates a {@link DeviceDisconnectedException}.
     *
     * @param msg a descriptive message.
     */
    public DeviceDisconnectedException(String msg, String serial) {
        super(msg, serial);
    }

    /**
     * Creates a {@link DeviceDisconnectedException}.
     *
     * @param msg a descriptive message.
     * @param cause the root {@link Throwable} that caused the device to become unavailable.
     *
     * @deprecated use
     * {@link #DeviceDisconnectedException(String msg, Throwable cause, String serial)} instead
     */
    @Deprecated
    public DeviceDisconnectedException(String msg, Throwable cause) {
        super(msg, cause);
    }

    /**
     * Creates a {@link DeviceDisconnectedException}.
     *
     * @param msg a descriptive message.
     * @param cause the root {@link Throwable} that caused the device to become disconnected.
     */
    public DeviceDisconnectedException(String msg, Throwable cause, String serial) {
        super(msg, cause, serial);
    }

}
