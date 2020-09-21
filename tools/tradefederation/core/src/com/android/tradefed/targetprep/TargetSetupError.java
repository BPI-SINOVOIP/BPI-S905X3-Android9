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
package com.android.tradefed.targetprep;

import com.android.tradefed.command.remote.DeviceDescriptor;

/**
 * A fatal error occurred while preparing the target for testing.
 */
public class TargetSetupError extends Exception {

    private static final long serialVersionUID = 2202987086655357201L;

    private DeviceDescriptor mDescriptor = null;

    /**
     * Constructs a new (@link TargetSetupError} with a meaningful error message.
     *
     * @param reason a error message describing the cause of the error
     * @deprecated use {@link #TargetSetupError(String, DeviceDescriptor)} instead.
     */
    @Deprecated
    public TargetSetupError(String reason) {
        super(reason);
    }

    /**
     * Constructs a new (@link TargetSetupError} with a meaningful error message.
     *
     * @param reason a error message describing the cause of the error
     * @param descriptor the descriptor of the device concerned
     */
    public TargetSetupError(String reason, DeviceDescriptor descriptor) {
        this(reason, null, descriptor);
    }

    /**
     * Constructs a new (@link TargetSetupError} with a meaningful error message, and a
     * cause.
     *
     * @param reason a detailed error message.
     * @param cause a {@link Throwable} capturing the original cause of the TargetSetupError
     * @deprecated use {@link #TargetSetupError(String, Throwable, DeviceDescriptor)} instead.
     */
    @Deprecated
    public TargetSetupError(String reason, Throwable cause) {
        super(reason, cause);
    }

    /**
     * Constructs a new (@link TargetSetupError} with a meaningful error message, and a
     * cause.
     *
     * @param reason a detailed error message.
     * @param cause a {@link Throwable} capturing the original cause of the TargetSetupError
     * @param descriptor the descriptor of the device concerned
     */
    public TargetSetupError(String reason, Throwable cause, DeviceDescriptor descriptor) {
        super(reason + " " + descriptor, cause);
        mDescriptor = descriptor;
    }

    /**
     * Return the descriptor of the device associated with exception.
     */
    public DeviceDescriptor getDeviceDescriptor() {
        return mDescriptor;
    }
}
