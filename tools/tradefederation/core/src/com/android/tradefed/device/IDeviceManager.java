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
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.util.IRunUtil;

import java.io.PrintWriter;
import java.util.List;

/**
 * Interface for managing the set of available devices for testing.
 */
public interface IDeviceManager {
    /**
     * A listener for fastboot state changes.
     */
    public static interface IFastbootListener {
        /**
         * Callback when fastboot state has been updated for all devices.
         */
        public void stateUpdated();
    }

    /**
     * Initialize the device manager. This must be called once and only once before any other
     * methods are called.
     */
    public void init();

    /**
     * Initialize the device manager with a device filter. This filter can be used to instruct
     * the DeviceManager to ignore certain connected devices.
     *
     * @param globalDeviceFilter the device filter
     */
    public void init(IDeviceSelection globalDeviceFilter, List<IDeviceMonitor> deviceMonitors);

    /**
     * Request a physical device for testing
     *
     * @return a {@link ITestDevice} for testing, or <code>null</code> if one is not available
     */
    public ITestDevice allocateDevice();

    /**
     * Request a device for testing that meets certain criteria.
     *
     * @param options the {@link IDeviceSelection} the device should meet.
     * @return a {@link ITestDevice} for testing, or <code>null</code> if one
     *         is not available
     */
    public ITestDevice allocateDevice(IDeviceSelection options);

    /**
     * Rudely allocate a device, even if its not currently available.
     * <p/>
     * Will have no effect if device is already allocated.
     *
     * @param serial the device serial to allocate
     * @return the {@link ITestDevice}, or <code>null</code> if it could not be allocated
     */
    public ITestDevice forceAllocateDevice(String serial);

    /**
     * Return a device to the pool
     * <p/>
     * Attempts to return a device that hasn't been previously allocated will be ignored.
     *
     * @param device the {@link ITestDevice} to free
     * @param state the {@link com.android.tradefed.device.FreeDeviceState}. Used to control if
     *        device is returned to available device pool.
     */
    public void freeDevice(ITestDevice device, FreeDeviceState state);

    /**
     * Helper method to launch emulator.
     * <p/>
     * Will launch the emulator as specified by the caller
     *
     * @param device the placeholder {@link ITestDevice} representing allocated emulator device
     * @param bootTimeout the time in ms to wait for the emulator to boot
     * @param runUtil
     * @param emulatorArgs command line arguments to launch the emulator
     * @throws DeviceNotAvailableException if emulator fails to boot or come online
     */
    void launchEmulator(ITestDevice device, long bootTimeout, IRunUtil runUtil,
            List<String> emulatorArgs) throws DeviceNotAvailableException;

    /**
     * Shut down the given emulator.
     * <p/>
     * Blocks until emulator disappears from adb. Will have no effect if emulator is already not
     * available.
     *
     * @param device the {@link ITestDevice} representing emulator to shut down
     * @throws DeviceNotAvailableException if emulator fails to shut down
     */
    public void killEmulator(ITestDevice device) throws DeviceNotAvailableException;

    /**
     * Connect to a device with adb-over-tcp
     * <p/>
     * This method allocates a new device, which should eventually be freed via
     * {@link #disconnectFromTcpDevice(ITestDevice)}
     * <p/>
     * The returned {@link ITestDevice} will be online, but may not be responsive.
     * <p/>
     * Note that performing action such as a reboot on a tcp connected device, will sever the
     * tcp connection to the device, and result in a {@link DeviceNotAvailableException}
     *
     * @param ipAndPort the original ip address and port of the device to connect to
     * @return the {@link ITestDevice} or <code>null</code> if a tcp connection could not be formed
     */
    public ITestDevice connectToTcpDevice(String ipAndPort);

    /**
     * Disconnect from an adb-over-tcp connected device.
     * <p/>
     * Switches the device back to usb mode, and frees it.
     *
     * @param tcpDevice the device currently in tcp mode, previously allocated via
     *            {@link #connectToTcpDevice(String)}
     * @return <code>true</code> if switch to usb mode was successful
     */
    public boolean disconnectFromTcpDevice(ITestDevice tcpDevice);

    /**
     * A helper method that switches the given usb device to adb-over-tcp mode, and then connects to
     * it via {@link #connectToTcpDevice(String)}.
     *
     * @param usbDevice the device currently in usb mode
     * @return the newly allocated {@link ITestDevice} in tcp mode or <code>null</code> if a tcp
     *         connection could not be formed
     * @throws DeviceNotAvailableException if the connection with <var>usbDevice</var> was lost and
     *         could not be recovered
     */
    public ITestDevice reconnectDeviceToTcp(ITestDevice usbDevice)
            throws DeviceNotAvailableException;

    /**
     * Stops device monitoring services, and terminates the ddm library.
     * <p/>
     * This must be called upon application termination.
     *
     * @see AndroidDebugBridge#terminate()
     */
    public void terminate();

    /** Stops the device recovery thread. */
    public void terminateDeviceRecovery();

    /** Stop the Device Monitors. */
    public void terminateDeviceMonitor();

    /** Like {@link #terminate()}, but attempts to forcefully shut down adb as well. */
    public void terminateHard();

    /** Stop adb bridge and services depend on adb connections. */
    public void stopAdbBridge();

    /**
     * Restart (if {@link #stopAdbBridge()} was called) adb bridge and services depend on adb
     * connections.
     */
    public void restartAdbBridge();

    /**
     * Returns a map of all known devices and their state
     *
     * @return a list of device serials and their {@link
     *     com.android.tradefed.device.DeviceAllocationState}
     */
    public List<DeviceDescriptor> listAllDevices();

    /**
     * Output a user-friendly description containing list of known devices, their state, and
     * values for commonly used {@link IDeviceSelection} options.
     *
     * @param printWriter the {@link PrintWriter} to output the description to
     */
    public void displayDevicesInfo(PrintWriter printWriter);

    /**
     * Informs the manager that a listener is interested in fastboot state changes.
     * <p/>
     * Currently a {@link IDeviceManager} will only monitor devices in fastboot if there are one or
     * more active listeners.
     * <p/>
     * TODO: this is a bit of a hack - find a better solution
     *
     * @param listener
     */
    public void addFastbootListener(IFastbootListener listener);

    /**
     * Informs the manager that a listener is no longer interested in fastboot state changes.
     * @param listener
     */
    public void removeFastbootListener(IFastbootListener listener);

    /**
     * Determine if given serial represents a null device
     */
    public boolean isNullDevice(String serial);

    /**
     * Determine if given serial represents a emulator
     */
    public boolean isEmulator(String serial);

    /**
     * Adds a {@link IDeviceMonitor}
     */
    public void addDeviceMonitor(IDeviceMonitor mon);

    /**
     * Removes a previously added {@link IDeviceMonitor}. Has no effect if mon has not been added.
     */
    public void removeDeviceMonitor(IDeviceMonitor mon);

    /**
     * Returns the path to the fastboot binary path to use.
     */
    public String getFastbootPath();

    /**
     * Wait until a first physical device is connected. If a device was connected before, it
     * returns directly True. If no device was added, it returns false after timeout.
     *
     * @param timeout time to wait in millisecond before returning false.
     */
    public boolean waitForFirstDeviceAdded(long timeout);

    /**
     * Get the number of available flashing permits
     *
     * @return Number of available flashing permits or Integer.MAX_VALUE if not available.
     */
    public int getAvailableFlashingPermits();

    /**
     * Request permission to flash.  If the number of concurrent flashers is limited, this will
     * wait in line in order to remain under the flash limit count.
     */
    public void takeFlashingPermit();

    /**
     * Restore a flashing permit that we acquired previously
     */
    public void returnFlashingPermit();

    /** Get the adb version currently in use by the device manager. */
    public String getAdbVersion();
}
