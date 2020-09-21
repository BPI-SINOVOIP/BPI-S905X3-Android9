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

import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.device.DeviceSelectionOptions;
import com.android.tradefed.device.FreeDeviceState;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.TestDeviceState;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.HashMap;
import java.util.Map;

/**
 * A class for allocating and freeing companion devices
 */
public class CompanionDeviceTracker {

    private static CompanionDeviceTracker sInst = null;

    private Map<ITestDevice, ITestDevice> mDeviceMapping = null;

    private CompanionDeviceTracker() {
        mDeviceMapping = new HashMap<ITestDevice, ITestDevice>();
    }

    /**
     * Retrieves singleton instance of the tracker
     */
    public static CompanionDeviceTracker getInstance() {
        if (sInst == null) {
            sInst = new CompanionDeviceTracker();
        }
        return sInst;
    }

    /**
     * Allocate a companion device based on selection criteria.
     *
     * @param device the primary device. used to identify the companion device
     * @param opt selection criteria
     * @return the device allocated or <code>null</code> if none available
     */
    public ITestDevice allocateCompanionDevice(ITestDevice device, DeviceSelectionOptions opt) {
        ITestDevice companion = getDeviceManager().allocateDevice(opt);
        if (companion != null) {
            if (mDeviceMapping.containsKey(device)) {
                CLog.w("device %s already has an allocated companion %s",
                        device.getSerialNumber(), mDeviceMapping.get(companion).getSerialNumber());
            }
            CLog.i("allocated companion device %s for primary device %s",
                    companion.getSerialNumber(), device.getSerialNumber());
            mDeviceMapping.put(device, companion);
        }
        return companion;
    }

    /**
     * Free the companion device as identified by the primary device
     * @param device the primary device whose corresponding companion device should be freed
     * @throws IllegalStateException if no companion devices
     */
    public void freeCompanionDevice(ITestDevice device) {
        if (!mDeviceMapping.containsKey(device)) {
            CLog.w("primary device %s has no tracked companion device", device.getSerialNumber());
            return;
        }
        ITestDevice companion = mDeviceMapping.remove(device);
        FreeDeviceState deviceState = FreeDeviceState.AVAILABLE;
        if (!TestDeviceState.ONLINE.equals(companion.getDeviceState())) {
            //If the device is offline at the end of the test
            deviceState = FreeDeviceState.UNAVAILABLE;
        }
        getDeviceManager().freeDevice(companion, deviceState);
        CLog.i("freed companion device %s for primary device %s",
                companion.getSerialNumber(), device.getSerialNumber());
    }

    /**
     * Retrieve the allocated companion device as identified by the primary device
     * @param device the primary device that the companion device is allocated with
     * @return the companion device or <code>null</code> if not found
     */
    public ITestDevice getCompanionDevice(ITestDevice device) {
        return mDeviceMapping.get(device);
    }

    private IDeviceManager getDeviceManager() {
        return GlobalConfiguration.getDeviceManagerInstance();
    }

}
