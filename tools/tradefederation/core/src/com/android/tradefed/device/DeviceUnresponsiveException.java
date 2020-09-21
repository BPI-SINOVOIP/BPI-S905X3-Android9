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
package com.android.tradefed.device;


/**
 * A specialization of {@link DeviceNotAvailableException} that indicates device is visible to
 * adb, but is unresponsive (ie commands timing out, won't boot, etc)
 */
@SuppressWarnings("serial")
public class DeviceUnresponsiveException extends DeviceNotAvailableException {
    /**
     * Creates a {@link DeviceUnresponsiveException}.
     */
    public DeviceUnresponsiveException() {
        super();
    }

    /**
     * Creates a {@link DeviceUnresponsiveException}.
     *
     * @param msg a descriptive message.
     *
     * @deprecated use {@link #DeviceUnresponsiveException(String msg, String serial)} instead
     */
    @Deprecated
    public DeviceUnresponsiveException(String msg) {
        super(msg);
    }

    /**
     * Creates a {@link DeviceUnresponsiveException}.
     *
     * @param msg a descriptive message.
     */
    public DeviceUnresponsiveException(String msg, String serial) {
        super(msg, serial);
    }

    /**
     * Creates a {@link DeviceUnresponsiveException}.
     *
     * @param msg a descriptive message.
     * @param cause the root {@link Throwable} that caused the device to become unavailable.
     *
     * @deprecated use
     * {@link #DeviceUnresponsiveException(String msg, Throwable cause, String serial)} instead
     */
    @Deprecated
    public DeviceUnresponsiveException(String msg, Throwable cause) {
        super(msg, cause);
    }

    /**
     * Creates a {@link DeviceUnresponsiveException}.
     *
     * @param msg a descriptive message.
     * @param cause the root {@link Throwable} that caused the device to become unavailable.
     */
    public DeviceUnresponsiveException(String msg, Throwable cause, String serial) {
        super(msg, cause, serial);
    }
}
