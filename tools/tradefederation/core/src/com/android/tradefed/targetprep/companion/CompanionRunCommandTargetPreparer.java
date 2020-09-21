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
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.RunCommandTargetPreparer;
import com.android.tradefed.targetprep.TargetSetupError;

/**
 * A {@link ITargetPreparer} that runs specified commands on the allocated companion device
 */
@OptionClass(alias = "companion-run-command")
public class CompanionRunCommandTargetPreparer extends RunCommandTargetPreparer {

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            DeviceNotAvailableException {
        // get companion device first
        ITestDevice companion = CompanionDeviceTracker.getInstance().getCompanionDevice(device);
        if (companion == null) {
            throw new TargetSetupError(String.format("no companion device allocated for %s",
                    device.getSerialNumber()), device.getDeviceDescriptor());
        }
        super.setUp(companion, buildInfo);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        // get companion device first
        ITestDevice companion = CompanionDeviceTracker.getInstance().getCompanionDevice(device);
        if (companion != null) {
            super.tearDown(companion, buildInfo, e);
        }
    }
}
