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
 * Provides facilities for monitoring the state of a {@link IDevice}.
 */
public interface IDeviceStateMonitor {

    /**
     * Waits for device to be online.
     * <p/>
     * Note: this method will return once device is visible via DDMS. It does not guarantee that the
     * device is actually responsive to adb commands - use {@link #waitForDeviceAvailable()}
     * instead.
     *
     * @param time the maximum time in ms to wait
     *
     * @return the {@link IDevice} if device becomes online before time expires. <code>null</code>
     * otherwise.
     */
    public IDevice waitForDeviceOnline(long time);

    /**
     * Waits for device to be online using standard boot timeout.
     * <p/>
     * Note: this method will return once device is visible via DDMS. It does not guarantee that the
     * device is actually responsive to adb commands - use {@link #waitForDeviceAvailable()}
     * instead.
     *
     * @return the {@link IDevice} if device becomes online before time expires. <code>null</code>
     * otherwise.
     */
    public IDevice waitForDeviceOnline();

    /**
     * Blocks until the device's boot complete flag is set
     *
     * @param waitTime the amount in ms to wait
     */
    public boolean waitForBootComplete(final long waitTime);

    /**
     * Waits for device to be responsive to a basic adb shell command.
     *
     * @param waitTime the time in ms to wait
     * @return <code>true</code> if device becomes responsive before <var>waitTime</var> elapses.
     */
    public boolean waitForDeviceShell(final long waitTime);

    /**
     * Waits for the device to be responsive and available for testing. Currently this means that
     * the package manager and external storage are available.
     *
     * @param waitTime the time in ms to wait
     * @return the {@link IDevice} if device becomes online before time expires. <code>null</code>
     * otherwise.
     */
    public IDevice waitForDeviceAvailable(final long waitTime);

    /**
     * Waits for the device to be responsive and available for testing.
     * <p/>
     * Equivalent to {@link #waitForDeviceAvailable(long)}, but uses default device
     * boot timeout.
     *
     * @return the {@link IDevice} if device becomes online before time expires. <code>null</code>
     * otherwise.
     */
    public IDevice waitForDeviceAvailable();

    /**
     * Waits for the device to be in bootloader.
     *
     * @param waitTime the maximum time in ms to wait
     *
     * @return <code>true</code> if device is in bootloader before time expires
     */
    public boolean waitForDeviceBootloader(long waitTime);

    /**
     * Waits for device bootloader state to be refreshed
     */
    public void waitForDeviceBootloaderStateUpdate();

    /**
     * Waits for the device to be not available
     *
     * @param waitTime the maximum time in ms to wait
     *
     * @return <code>true</code> if device becomes unavailable
     */
    public boolean waitForDeviceNotAvailable(long waitTime);

    /**
     * Waits for the device to be in the 'adb recovery' state
     *
     * @param waitTime the maximum time in ms to wait
     * @return True if the device is in Recovery before the timeout, False otherwise.
     */
    public boolean waitForDeviceInRecovery(long waitTime);

    /**
     * Gets the serial number of the device.
     */
    public String getSerialNumber();

    /**
     * Gets the device state.
     *
     * @return the {@link TestDeviceState} of device
     */
    public TestDeviceState getDeviceState();

    /**
     * Sets the device current state.
     *
     * @param deviceState
     */
    public void setState(TestDeviceState deviceState);

    /**
     * Returns a mount point.
     * <p/>
     * Queries the device directly if the cached info in {@link IDevice} is not available.
     * <p/>
     * TODO: move this behavior to {@link IDevice#getMountPoint(String)}
     *
     * @param mountName the name of the mount point
     * @return the mount point or <code>null</code>
     * @see IDevice#getMountPoint(String)
     */
    public String getMountPoint(String mountName);

    /**
     * Updates the current IDevice.
     *
     * @param device
     *
     * @see IManagedTestDevice#setIDevice(IDevice)
     */
    public void setIDevice(IDevice device);

    /**
     * @return <code>true</code> if device is connected to adb via tcp
     */
    public boolean isAdbTcp();

    /**
     * Set the time in ms to wait for a device to be online in {@link #waitForDeviceOnline()}.
     */
    public void setDefaultOnlineTimeout(long timeoutMs);

    /**
     * Set the time in ms to wait for a device to be available in {@link #waitForDeviceAvailable()}.
     */
    public void setDefaultAvailableTimeout(long timeoutMs);

}
