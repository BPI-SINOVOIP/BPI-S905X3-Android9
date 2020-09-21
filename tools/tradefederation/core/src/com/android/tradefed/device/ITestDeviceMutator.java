/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * An interface allowing manipulation of the {@link IManagedTestDevice}.
 * Note that {@link IManagedTestDevice} is not public, {@link ITestDevice} is cast to
 * {@link IManagedTestDevice}.
 * <b>This is a rather dangerous manipulation and not recommended if you have no need for this.</b>
 */
public interface ITestDeviceMutator {


   /**
    * Changes the {@link IDevice} held by {@link ITestDevice}
    * @param testDevice
    * @param device
    */
    public void setIDevice(ITestDevice testDevice, IDevice device);

    /**
     * Sets if Fastboot is enabled or not for a given {@link ITestDevice}.
     * @param testDevice
     * @param fastbootEnabled
     */
    public void setFastbootEnabled(ITestDevice testDevice, boolean fastbootEnabled);
}
