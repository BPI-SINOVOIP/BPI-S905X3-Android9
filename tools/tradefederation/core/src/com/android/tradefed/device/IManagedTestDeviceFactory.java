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

import com.android.ddmlib.IDevice;

/**
 * Creator interface for {@link IManagedTestDevice}s
 */
public interface IManagedTestDeviceFactory {

    /**
     * Create a {@link IManagedTestDevice} based on the {@link IDevice} passed to it.
     *
     * @param stubDevice that will define the type of device created
     * @return a IManagedTestDevice created base on the IDevice
     */
    IManagedTestDevice createDevice(IDevice stubDevice);

    /**
     * Enable or not fastboot support for the device created.
     * @param enable value set the support.
     */
    public void setFastbootEnabled(boolean enable);
}
