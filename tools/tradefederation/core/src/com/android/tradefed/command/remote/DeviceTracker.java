/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.tradefed.command.remote;

import com.android.tradefed.device.ITestDevice;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Hashtable;
import java.util.Map;
import java.util.concurrent.Future;

/**
 * Singleton class that tracks devices that have been remotely allocated.
 */
class DeviceTracker {

    /**
    * Use on demand holder idiom
    * @see <a href="http://en.wikipedia.org/wiki/Singleton_pattern#The_solution_of_Bill_Pugh">
    * http://en.wikipedia.org/wiki/Singleton_pattern#The_solution_of_Bill_Pugh</a>
    */
    private static class SingletonHolder {
        public static final DeviceTracker cInstance = new DeviceTracker();
    }

    public static DeviceTracker getInstance() {
        return SingletonHolder.cInstance;
    }

    // private constructor - don't allow instantiation
    private DeviceTracker() {
    }

    // use Hashtable since its thread-safe
    private Map<String, ITestDevice> mAllocatedDeviceMap = new Hashtable<String, ITestDevice>();

    // TODO: consider merging these maps
    private Map<String, ExecCommandTracker> mDeviceLastCommandMap =
            new Hashtable<String, ExecCommandTracker>();

    /**
     * Mark given device as remotely allocated.
     */
    public void allocateDevice(ITestDevice d) {
        mAllocatedDeviceMap.put(d.getSerialNumber(), d);
    }

    /**
     * Mark given device serial as freed and clear the command result if any.
     *
     * @return the corresponding {@link ITestDevice} or <code>null</code> if device with given
     *         serial cannot be found
     */
    public ITestDevice freeDevice(String serial) {
        mDeviceLastCommandMap.remove(serial);
        return mAllocatedDeviceMap.remove(serial);
    }

    /**
     * Mark all remotely allocated devices as freed.
     *
     * @return a {@link Collection} of all remotely allocated devices
     */
    public Collection<ITestDevice> freeAll() {
        Collection<ITestDevice> devices = new ArrayList<ITestDevice>(mAllocatedDeviceMap.values());
        mAllocatedDeviceMap.clear();
        mDeviceLastCommandMap.clear();
        return devices;
    }

    /**
     * Return a previously allocated device that matches given serial.
     *
     * @param serial
     * @return the {@link ITestDevice} or <code>null</code> if it cannot be found
     */
    public ITestDevice getDeviceForSerial(String serial) {
        return mAllocatedDeviceMap.get(serial);
    }

    /**
     * Retrieve the last {@link Future} command result for given device.
     * @param deviceSerial
     * @return the {@link Future} or <code>null</code> if no result exists for device. Note results
     * are cleared on {@link #freeDevice(String)}.
     */
    public ExecCommandTracker getLastCommandResult(String deviceSerial) {
        return mDeviceLastCommandMap.get(deviceSerial);
    }

    /**
     * Sets the command result tracker for given device.
     *
     * @param deviceSerial
     * @param tracker
     */
    public void setCommandTracker(String deviceSerial, ExecCommandTracker tracker) {
        mDeviceLastCommandMap.put(deviceSerial, tracker);
    }
}
