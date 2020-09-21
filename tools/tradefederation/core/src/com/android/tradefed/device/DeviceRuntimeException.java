/*
 * Copyright (C) 2017 The Android Open Source Project
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
 * Thrown when a device action did not results in the expected results.
 *
 * <p>For example: 'pm list users' is vastly expected to return the list of users, failure to do so
 * should be raised as a DeviceRuntimeException since something went very wrong.
 */
public class DeviceRuntimeException extends RuntimeException {
    private static final long serialVersionUID = -7928528651742852301L;

    /**
     * Creates a {@link DeviceRuntimeException}.
     *
     * @param msg a descriptive error message of the error.
     */
    public DeviceRuntimeException(String msg) {
        super(msg);
    }

    /**
     * Creates a {@link DeviceRuntimeException}.
     *
     * @param t {@link Throwable} that should be wrapped in {@link DeviceRuntimeException}.
     */
    public DeviceRuntimeException(Throwable t) {
        super(t);
    }

    /**
     * Creates a {@link DeviceRuntimeException}.
     *
     * @param msg a descriptive error message of the error
     * @param t {@link Throwable} that should be wrapped in {@link DeviceRuntimeException}.
     */
    public DeviceRuntimeException(String msg, Throwable t) {
        super(msg, t);
    }
}
