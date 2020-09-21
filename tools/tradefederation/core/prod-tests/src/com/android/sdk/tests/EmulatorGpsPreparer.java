/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.sdk.tests;

import com.android.ddmlib.EmulatorConsole;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.BaseTargetPreparer;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.TargetSetupError;

import org.junit.Assert;

/** Injects specific gps location into emulator. */
public class EmulatorGpsPreparer extends BaseTargetPreparer {

    // default gps location for the gps test
    // TODO: make these options rather than hardcoding
    private static final double LONGITUDE = -122.08345770835876;
    private static final double LATITUDE = 37.41991859119417;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        Assert.assertTrue("device is not a emulator", device.getIDevice().isEmulator());
        EmulatorConsole console = EmulatorConsole.getConsole(device.getIDevice());
        console.sendLocation(LONGITUDE, LATITUDE, 0);
    }
}
