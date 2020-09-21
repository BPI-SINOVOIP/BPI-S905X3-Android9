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
package com.android.tradefed.testtype;

import com.android.tradefed.device.INativeDevice;

/**
 * Interface for object that needs a reference to a {@link INativeDevice}.
 */
public interface INativeDeviceTest {

    /**
     * Inject the device under test.
     *
     * @param device the {@link INativeDevice} to use
     */
    public void setDevice(INativeDevice device);

    /**
     * Get the device under test.
     *
     * @return the {@link INativeDevice}
     */
    public INativeDevice getDevice();
}