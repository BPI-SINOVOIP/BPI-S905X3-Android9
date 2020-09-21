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
 * Default implementation of {@link ITestDeviceMutator}
 */
public class TestDeviceMutator implements ITestDeviceMutator {

    /**
     * {@inheritDoc}
     */
    @Override
    public void setIDevice(ITestDevice testDevice, IDevice device) {
       ((IManagedTestDevice)testDevice).setIDevice(device);
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public void setFastbootEnabled(ITestDevice testDevice, boolean fastbootEnabled) {
        ((IManagedTestDevice)testDevice).setFastbootEnabled(fastbootEnabled);
    }

}
