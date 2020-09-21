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
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.targetprep.TestAppInstallSetup;

/**
 * A {@link ITargetPreparer} that installs one or more apps from a
 * {@link IDeviceBuildInfo#getTestsDir()} folder onto an allocated companion device.
 * <p>
 * Note that this implies the test artifact in question in built into primary device build but also
 * suitable for companion device
 */
@OptionClass(alias = "companion-tests-zip-app")
public class CompanionTestAppInstallSetup extends TestAppInstallSetup {
    ITestDevice mCompanion;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            DeviceNotAvailableException {
        // get companion device first
        mCompanion = CompanionDeviceTracker.getInstance().getCompanionDevice(device);
        if (mCompanion == null) {
            throw new TargetSetupError(String.format("no companion device allocated for %s",
                    device.getSerialNumber()), device.getDeviceDescriptor());
        }
        super.setUp(mCompanion, buildInfo);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        super.tearDown(mCompanion, buildInfo, e);
    }
}
