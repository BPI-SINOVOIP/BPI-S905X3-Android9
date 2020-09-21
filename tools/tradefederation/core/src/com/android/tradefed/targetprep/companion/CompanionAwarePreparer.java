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

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.BaseTargetPreparer;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.TargetSetupError;

/**
 * An {@link ITargetPreparer} that encapsulates the logic of getting an already allocated companion
 * device.
 *
 * <p>Note: that this must be used in conjunction with a {@link CompanionAllocator}.
 */
public abstract class CompanionAwarePreparer extends BaseTargetPreparer {

    /**
     * Retrieves the {@link ITestDevice} instance of companion device allocated for the primary
     * {@link ITestDevice}
     * @param primary
     * @return the {@link ITestDevice} instance of companion device allocated
     * @throws TargetSetupError if no companion device has been allocated for the primary device
     */
    protected ITestDevice getCompanion(ITestDevice primary) throws TargetSetupError {
        ITestDevice companionDevice = getCompanionDeviceTracker().getCompanionDevice(primary);
        if (companionDevice == null) {
            throw new TargetSetupError(String.format("no companion device allocated for %s",
                    primary.getSerialNumber()), primary.getDeviceDescriptor());
        }
        return companionDevice;
    }

    private CompanionDeviceTracker getCompanionDeviceTracker() {
        return CompanionDeviceTracker.getInstance();
    }
}
