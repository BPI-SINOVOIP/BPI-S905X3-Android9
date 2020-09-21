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

package com.android.cts.devicepolicy;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

/**
 * Helper to simulate user activity on the device.
 */
class UserActivityEmulator {
    private final int mWidth;
    private final int mHeight;
    private final ITestDevice mDevice;

    public UserActivityEmulator(ITestDevice device) throws DeviceNotAvailableException {
        // Figure out screen size. Output is something like "Physical size: 1440x2880".
        mDevice = device;
        final String output = mDevice.executeShellCommand("wm size");
        final String[] sizes = output.split(" ")[2].split("x");
        mWidth = Integer.valueOf(sizes[0].trim());
        mHeight = Integer.valueOf(sizes[1].trim());
    }

    public void tapScreenCenter() throws DeviceNotAvailableException {
        mDevice.executeShellCommand(String.format("input tap %d %d", mWidth / 2, mHeight / 2));
    }
}
