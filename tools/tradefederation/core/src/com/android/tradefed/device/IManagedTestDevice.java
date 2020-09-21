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

import com.android.ddmlib.IDevice;

/**
 * A ITestDevice whose lifecycle is managed.
 */
public interface IManagedTestDevice extends ITestDevice {

    /**
     * Container for a response to a {@link IManagedTestDevice#handleAllocationEvent(DeviceEvent)}
     * call
     */
    static class DeviceEventResponse {
        /** the new state of the device */
        final DeviceAllocationState allocationState;
        /** true if state changed as a result of device event */
        final boolean stateChanged;

        DeviceEventResponse(DeviceAllocationState s, boolean b) {
            allocationState = s;
            stateChanged = b;
        }
    }

    /**
     * Update the IDevice associated with this ITestDevice.
     * <p/>
     * The new IDevice must refer the same physical device as the current reference. This method
     * will be called if DDMS has allocated a new IDevice
     *
     * @param device the {@link IDevice}
     */
    public void setIDevice(IDevice device);

    /**
     * Update the device's state.
     *
     * @param deviceState the {@link TestDeviceState}
     */
    public void setDeviceState(TestDeviceState deviceState);

    /**
     * Set the fastboot option for the device. Should be set when device is first
     * allocated.
     *
     * @param fastbootEnabled whether fastboot is available for the device or not
     */
    public void setFastbootEnabled(boolean fastbootEnabled);

    /**
     * Return if fastboot is available for the device.
     */
    public boolean isFastbootEnabled();

    /**
     * Sets the path to the fastboot binary that should be used.
     * Still requires {@link #isFastbootEnabled()} to be true, to have fastboot functions enabled.
     */
    public void setFastbootPath(String fastbootPath);

    /**
     * Returns the path of the fastboot binary being used.
     * Still requires {@link #isFastbootEnabled()} to be true, to have fastboot functions enabled.
     */
    public String getFastbootPath();

    /**
     * Invoke recovery on the device.
     *
     * @throws DeviceNotAvailableException if recovery was not successful
     */
    public void recoverDevice() throws DeviceNotAvailableException;

    /**
     * Sets the {@link Process}, when this device is an emulator.
     */
    public void setEmulatorProcess(Process p);

    /**
     * Return the {@link Process} corresponding to this emulator.
     *
     * @return the {@link Process} or <code>null</code>
     */
    public Process getEmulatorProcess();

    /**
     * Return the current allocation state of device
     */
    public DeviceAllocationState getAllocationState();

    /**
     * Process the given {@link com.android.tradefed.device.DeviceEvent}. May transition device
     * to new state. Will inform the {@link IDeviceMonitor} of any state transitions.
     */
    public DeviceEventResponse handleAllocationEvent(DeviceEvent event);

    /**
     * Return the {@link IDeviceStateMonitor} associated with device.
     */
    public IDeviceStateMonitor getMonitor();

    /**
     * Returns the MAC address of the device, null if it fails to query from the device.
     */
    public String getMacAddress();

    /** Return the SIM card state or null if device is not available. */
    public String getSimState();

    /** Return the SIM card operator or null if device is not available. */
    public String getSimOperator();
}
