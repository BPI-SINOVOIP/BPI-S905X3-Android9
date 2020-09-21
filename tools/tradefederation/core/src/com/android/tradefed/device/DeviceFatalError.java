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
 * Thrown when a fatal error has occurred with device, and it should no longer be used for testing.
 *
 * This should typically be used when the device is still visible through adb, but it is in some
 * known-to-be unrecoverable state. Or if an "interesting" exception condition happened, that
 * requires human inspection while device is in the current state.
 */
public class DeviceFatalError extends Exception {

    private static final long serialVersionUID = -7928528651742852301L;

    /**
     * Creates a {@link DeviceFatalError}.
     *
     * @param msg a descriptive error message of the error
     */
    public DeviceFatalError(String msg) {
        super(msg);
    }
}
