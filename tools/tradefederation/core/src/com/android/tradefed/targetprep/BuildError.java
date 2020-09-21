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
 * Thrown if the provided build fails to run.
 */
@SuppressWarnings("serial")
public class BuildError extends Exception {

    private DeviceDescriptor mDescriptor = null;

    /**
     * Constructs a new (@link BuildError} with a detailed error message.
     *
     * @param reason an error message giving more details on the build error
     * @param descriptor the descriptor of the device concerned
     */
    public BuildError(String reason, DeviceDescriptor descriptor) {
        super(reason + " " + descriptor);
        mDescriptor = descriptor;
    }

    /**
     * Constructs a new (@link BuildError} with a detailed error message.
     *
     * @param reason an error message giving more details on the build error
     * @deprecated use {@link #BuildError(String, DeviceDescriptor)} instead.
     */
    @Deprecated
    public BuildError(String reason) {
        super(reason);
    }

    /**
     * Return the descriptor of the device associated with exception.
     */
    public DeviceDescriptor getDeviceDescriptor() {
        return mDescriptor;
    }
}
