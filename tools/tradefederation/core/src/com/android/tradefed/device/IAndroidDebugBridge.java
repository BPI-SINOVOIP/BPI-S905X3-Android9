/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.android.ddmlib.AndroidDebugBridge;
import com.android.ddmlib.AndroidDebugBridge.IDeviceChangeListener;
import com.android.ddmlib.IDevice;

/**
 * Interface definition for {@link com.android.ddmlib.AndroidDebugBridge} methods used in this
 * package.
 * <p/>
 * Exposed so use of {@link com.android.ddmlib.AndroidDebugBridge} can be mocked out in unit tests.
 */
public interface IAndroidDebugBridge {

    /**
     * Wrapper for {@link AndroidDebugBridge#getDevices()}.
     */
    IDevice[] getDevices();

    /**
     * Wrapper for {@link AndroidDebugBridge#addDeviceChangeListener(IDeviceChangeListener)}
     */
    void addDeviceChangeListener(IDeviceChangeListener listener);

    /**
     * Wrapper for {@link AndroidDebugBridge#removeDeviceChangeListener(IDeviceChangeListener)}
     */
    void removeDeviceChangeListener(IDeviceChangeListener listener);

    /**
     * Wrapper for {@link AndroidDebugBridge#init(boolean)} and
     * {@link AndroidDebugBridge#createBridge(String, boolean)}
     */
    void init(boolean clientSupport, String adbOsLocation);

    /**
     * Returns the adb full version of the adb location provided, or null if anything goes wrong.
     */
    String getAdbVersion(String adbOsLocation);

    /**
     * Wrapper for {@link AndroidDebugBridge#terminate()}
     */
    void terminate();

    /**
     * Wrapper for {@link AndroidDebugBridge#disconnectBridge()}
     */
    void disconnectBridge();

}
