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
 * Thrown when a device is no longer available for testing.
 * e.g. the adb connection to the device has been lost, device has stopped responding to commands,
 * etc
 */
@SuppressWarnings("serial")
public class DeviceNotAvailableException extends Exception {

    private String mSerial = null;

    /**
     * Creates a {@link DeviceNotAvailableException}.
     */
    public DeviceNotAvailableException() {
        super();
    }

    /**
     * Creates a {@link DeviceNotAvailableException}.
     *
     * @param msg a descriptive message.
     *
     * @deprecated use {@link #DeviceNotAvailableException(String msg, String serial)} instead
     */
    @Deprecated
    public DeviceNotAvailableException(String msg) {
        super(msg);
    }

    /**
     * Creates a {@link DeviceNotAvailableException}.
     *
     * @param msg a descriptive message.
     * @param serial the serial of the device concerned
     */
    public DeviceNotAvailableException(String msg, String serial) {
        super(msg);
        mSerial = serial;
    }

    /**
     * Creates a {@link DeviceNotAvailableException}.
     *
     * @param msg a descriptive message.
     * @param cause the root {@link Throwable} that caused the device to become unavailable.
     *
     * @deprecated use
     * {@link #DeviceNotAvailableException(String msg, Throwable cause, String serial)} instead
     */
    @Deprecated
    public DeviceNotAvailableException(String msg, Throwable cause) {
        super(msg, cause);
    }

    /**
     * Creates a {@link DeviceNotAvailableException}.
     *
     * @param msg a descriptive message.
     * @param cause the root {@link Throwable} that caused the device to become unavailable.
     * @param serial the serial of the device concerned by the exception
     */
    public DeviceNotAvailableException(String msg, Throwable cause, String serial) {
        super(msg, cause);
        mSerial = serial;
    }

    /**
     * Return Serial of the device associated with exception.
     */
    public String getSerial() {
        return mSerial;
    }
}
