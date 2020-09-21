/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tradefed.targetprep.companion;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceSelectionOptions;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.BaseTargetPreparer;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.ITargetCleaner;
import com.android.tradefed.targetprep.TargetSetupError;

/**
 * Base class that takes care of allocating and freeing companion device
 *
 * <p>{@link #getCompanionDeviceSelectionOptions()} should be implemented to describe the criteria
 * needed to allocate the companion device
 */
public abstract class CompanionAllocator extends BaseTargetPreparer implements ITargetCleaner {

    /**
     * Sets up the device.
     * <p>
     * Internal implementation of this method will request a companion device, and allocate it.
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        ITestDevice companionDevice = getCompanionDeviceTracker().allocateCompanionDevice(
                device, getCompanionDeviceSelectionOptions());
        if (companionDevice == null) {
            throw new TargetSetupError(String.format("failed to allocate companion device for %s",
                    device.getSerialNumber()), device.getDeviceDescriptor());
        }
    }

    /**
     * Describe the {@link DeviceSelectionOptions} for the companion device
     */
    protected abstract DeviceSelectionOptions getCompanionDeviceSelectionOptions();

    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        getCompanionDeviceTracker().freeCompanionDevice(device);
    }

    private CompanionDeviceTracker getCompanionDeviceTracker() {
        return CompanionDeviceTracker.getInstance();
    }
}
