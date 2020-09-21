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
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.BluetoothUtils;

import java.util.Set;

/**
 * A {@link CompanionAwarePreparer} that verifies BT bonding between primary and companion devices
 */
public class CheckPairingPreparer extends CompanionAwarePreparer {

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        String primaryMac = BluetoothUtils.getBluetoothMac(device);
        String companionMac = BluetoothUtils.getBluetoothMac(getCompanion(device));
        Set<String> primaryBonded = BluetoothUtils.getBondedDevices(device);
        Set<String> companionBonded = BluetoothUtils.getBondedDevices(getCompanion(device));
        boolean primaryHasCompanion = primaryBonded.contains(companionMac);
        boolean companionHasPrimary = companionBonded.contains(primaryMac);
        if (!primaryHasCompanion || !companionHasPrimary) {
            throw new TargetSetupError(String.format(
                    "device bonding error: primaryHasCompanion=%s, companionHasPrimary=%s",
                    primaryHasCompanion, companionHasPrimary), device.getDeviceDescriptor());
        }
    }
}
