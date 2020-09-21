/*
 * Copyright (C) 2012 The Android Open Source Project
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
 * Thrown if a device fails to boot after being flashed with a build.
 */
@SuppressWarnings("serial")
public class DeviceFailedToBootError extends BuildError {

    /**
     * Constructs a new (@link DeviceFailedToBootError} with a detailed error message.
     *
     * @param reason an error message giving more details about the boot failure
     * @param descriptor the descriptor of the device concerned by the exception
     */
    public DeviceFailedToBootError(String reason, DeviceDescriptor descriptor) {
        super(reason, descriptor);
    }
}
