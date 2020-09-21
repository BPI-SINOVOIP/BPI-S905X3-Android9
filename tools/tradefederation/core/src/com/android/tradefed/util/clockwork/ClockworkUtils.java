/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.tradefed.util.clockwork;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import java.util.List;
import java.util.Map;

/** A clockwork utility for sharing multi-device logic */
public class ClockworkUtils {
    /**
     * Helper method to share multi-device setup that returns an ITestDevice for the companion and
     * fills the deviceInfos list with watches
     *
     * @param deviceInfos device infos provided
     * @param deviceList device list to fill
     * @return {@link ITestDevice} companion device
     */
    public ITestDevice setUpMultiDevice(
            Map<ITestDevice, IBuildInfo> deviceInfos, List<ITestDevice> deviceList) {
        ITestDevice companion = null;
        if (deviceInfos.size() < 2) {
            throw new RuntimeException(
                    "there should be at least two devices for clockwork multiple device test");
        }
        for (Map.Entry<ITestDevice, IBuildInfo> entry : deviceInfos.entrySet()) {
            try {
                String deviceBuildPropertyString =
                        entry.getKey().getProperty("ro.build.characteristics");
                if (deviceBuildPropertyString.contains("watch")) {
                    deviceList.add(entry.getKey());
                } else {
                    if (companion != null) {
                        throw new RuntimeException(
                                "there should be only one companion in the test");
                    }
                    companion = entry.getKey();
                }
            } catch (DeviceNotAvailableException e) {
                throw new RuntimeException(
                        "device not available, cannot get device build property to determine "
                                + "companion/watch device");
            }
        }
        if (companion == null) {
            throw new RuntimeException("no companion device found in the test");
        }
        return companion;
    }
}
