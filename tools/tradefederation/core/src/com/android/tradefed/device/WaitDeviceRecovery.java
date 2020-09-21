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

import com.android.ddmlib.AdbCommandRejectedException;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.Log;
import com.android.ddmlib.TimeoutException;
import com.android.tradefed.config.Option;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import java.io.IOException;
import java.util.concurrent.ExecutionException;

/**
 * A simple implementation of a {@link IDeviceRecovery} that waits for device to be online and
 * respond to simple commands.
 */
public class WaitDeviceRecovery implements IDeviceRecovery {

    private static final String LOG_TAG = "WaitDeviceRecovery";

    /** the time in ms to wait before beginning recovery attempts */
    protected static final long INITIAL_PAUSE_TIME = 5 * 1000;

    /**
     * The number of attempts to check if device is in bootloader.
     * <p/>
     * Exposed for unit testing
     */
    public static final int BOOTLOADER_POLL_ATTEMPTS = 3;

    // TODO: add a separate configurable timeout per operation
    @Option(name="online-wait-time",
            description="maximum time in ms to wait for device to come online.")
    protected long mOnlineWaitTime = 60 * 1000;
    @Option(name="device-wait-time",
            description="maximum time in ms to wait for a single device recovery command.")
    protected long mWaitTime = 4 * 60 * 1000;

    @Option(name="bootloader-wait-time",
            description="maximum time in ms to wait for device to be in fastboot.")
    protected long mBootloaderWaitTime = 30 * 1000;

    @Option(name="shell-wait-time",
            description="maximum time in ms to wait for device shell to be responsive.")
    protected long mShellWaitTime = 30 * 1000;

    @Option(name="fastboot-wait-time",
            description="maximum time in ms to wait for a fastboot command result.")
    protected long mFastbootWaitTime = 30 * 1000;

    @Option(name = "min-battery-after-recovery",
            description = "require a min battery level after successful recovery, " +
                          "default to 0 for ignoring.")
    protected int mRequiredMinBattery = 0;

    @Option(name = "disable-unresponsive-reboot",
            description = "If this is set, we will not attempt to reboot an unresponsive device" +
            "that is in userspace.  Note that this will have no effect if the device is in " +
            "fastboot or is expected to be in fastboot.")
    protected boolean mDisableUnresponsiveReboot = false;

    private String mFastbootPath = "fastboot";

    /**
     * Get the {@link RunUtil} instance to use.
     * <p/>
     * Exposed for unit testing.
     */
    protected IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /**
     * Sets the maximum time in ms to wait for a single device recovery command.
     */
    void setWaitTime(long waitTime) {
        mWaitTime = waitTime;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setFastbootPath(String fastbootPath) {
        mFastbootPath = fastbootPath;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void recoverDevice(IDeviceStateMonitor monitor, boolean recoverUntilOnline)
            throws DeviceNotAvailableException {
        // device may have just gone offline
        // sleep a small amount to give ddms state a chance to settle
        // TODO - see if there is better way to handle this
        Log.i(LOG_TAG, String.format("Pausing for %d for %s to recover",
                INITIAL_PAUSE_TIME, monitor.getSerialNumber()));
        getRunUtil().sleep(INITIAL_PAUSE_TIME);

        // ensure bootloader state is updated
        monitor.waitForDeviceBootloaderStateUpdate();

        if (monitor.getDeviceState().equals(TestDeviceState.FASTBOOT)) {
            Log.i(LOG_TAG, String.format(
                    "Found device %s in fastboot but expected online. Rebooting...",
                    monitor.getSerialNumber()));
            // TODO: retry if failed
            getRunUtil().runTimedCmd(mFastbootWaitTime, mFastbootPath, "-s",
                    monitor.getSerialNumber(), "reboot");
        }

        // wait for device online
        IDevice device = monitor.waitForDeviceOnline(mOnlineWaitTime);
        if (device == null) {
            handleDeviceNotAvailable(monitor, recoverUntilOnline);
            // function returning implies that recovery is successful, check battery level here
            checkMinBatteryLevel(getDeviceAfterRecovery(monitor));
            return;
        }
        // occasionally device is erroneously reported as online - double check that we can shell
        // into device
        if (!monitor.waitForDeviceShell(mShellWaitTime)) {
            // treat this as a not available device
            handleDeviceNotAvailable(monitor, recoverUntilOnline);
            checkMinBatteryLevel(getDeviceAfterRecovery(monitor));
            return;
        }

        if (!recoverUntilOnline) {
            if (monitor.waitForDeviceAvailable(mWaitTime) == null) {
                // device is online but not responsive
                handleDeviceUnresponsive(device, monitor);
            }
        }
        // do a final check here when all previous if blocks are skipped or the last
        // handleDeviceUnresponsive was successful
        checkMinBatteryLevel(getDeviceAfterRecovery(monitor));
    }

    private IDevice getDeviceAfterRecovery(IDeviceStateMonitor monitor)
            throws DeviceNotAvailableException {
        IDevice device = monitor.waitForDeviceOnline(mOnlineWaitTime);
        if (device == null) {
            throw new DeviceNotAvailableException(
                    "Device still not online after successful recovery", monitor.getSerialNumber());
        }
        return device;
    }

    /**
     * Checks if device battery level meets min requirement
     * @param device
     * @throws DeviceNotAvailableException if battery level cannot be read or lower than min
     */
    protected void checkMinBatteryLevel(IDevice device) throws DeviceNotAvailableException {
        if (mRequiredMinBattery <= 0) {
            // don't do anything if check is not required
            return;
        }
        try {
            Integer level = device.getBattery().get();
            if (level == null) {
                // can't read battery level but we are requiring a min, reject
                // device
                throw new DeviceNotAvailableException(
                        "Cannot read battery level but a min is required",
                        device.getSerialNumber());
            } else if (level < mRequiredMinBattery) {
                throw new DeviceNotAvailableException(String.format(
                        "After recovery, device battery level %d is lower than required minimum %d",
                        level, mRequiredMinBattery), device.getSerialNumber());
            }
            return;
        } catch (InterruptedException | ExecutionException e) {
            throw new DeviceNotAvailableException("exception while reading battery level", e,
                    device.getSerialNumber());
        }
    }

    /**
     * Handle situation where device is online but unresponsive.
     * @param monitor
     * @throws DeviceNotAvailableException
     */
    protected void handleDeviceUnresponsive(IDevice device, IDeviceStateMonitor monitor)
            throws DeviceNotAvailableException {
        if (!mDisableUnresponsiveReboot) {
            Log.i(LOG_TAG, String.format(
                    "Device %s unresponsive. Rebooting...", monitor.getSerialNumber()));
            rebootDevice(device);
            IDevice newdevice = monitor.waitForDeviceOnline(mOnlineWaitTime);
            if (newdevice == null) {
                handleDeviceNotAvailable(monitor, false);
                return;
            }
            if (monitor.waitForDeviceAvailable(mWaitTime) != null) {
                return;
            }
        }
        // If no reboot was done, waitForDeviceAvailable has already been checked.
        throw new DeviceUnresponsiveException(String.format(
                "Device %s is online but unresponsive", monitor.getSerialNumber()),
                monitor.getSerialNumber());
    }

    /**
     * Handle situation where device is not available.
     *
     * @param monitor the {@link IDeviceStateMonitor}
     * @param recoverTillOnline if true this method should return if device is online, and not
     * check for responsiveness
     * @throws DeviceNotAvailableException
     */
    protected void handleDeviceNotAvailable(IDeviceStateMonitor monitor, boolean recoverTillOnline)
            throws DeviceNotAvailableException {
        throw new DeviceNotAvailableException(String.format("Could not find device %s",
                monitor.getSerialNumber()), monitor.getSerialNumber());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void recoverDeviceBootloader(final IDeviceStateMonitor monitor)
            throws DeviceNotAvailableException {
        // device may have just gone offline
        // wait a small amount to give device state a chance to settle
        // TODO - see if there is better way to handle this
        Log.i(LOG_TAG, String.format("Pausing for %d for %s to recover",
                INITIAL_PAUSE_TIME, monitor.getSerialNumber()));
        getRunUtil().sleep(INITIAL_PAUSE_TIME);

        // poll and wait for device to return to valid state
        long pollTime = mBootloaderWaitTime / BOOTLOADER_POLL_ATTEMPTS;
        for (int i=0; i < BOOTLOADER_POLL_ATTEMPTS; i++) {
            if (monitor.waitForDeviceBootloader(pollTime)) {
                handleDeviceBootloaderUnresponsive(monitor);
                // passed above check, abort
                return;
            } else if (monitor.getDeviceState() == TestDeviceState.ONLINE) {
                handleDeviceOnlineExpectedBootloader(monitor);
                return;
            }
        }
        handleDeviceBootloaderNotAvailable(monitor);
    }

    /**
     * Handle condition where device is online, but should be in bootloader state.
     * <p/>
     * If this method
     * @param monitor
     * @throws DeviceNotAvailableException
     */
    protected void handleDeviceOnlineExpectedBootloader(final IDeviceStateMonitor monitor)
            throws DeviceNotAvailableException {
        Log.i(LOG_TAG, String.format("Found device %s online but expected fastboot.",
            monitor.getSerialNumber()));
        // call waitForDeviceOnline to get handle to IDevice
        IDevice device = monitor.waitForDeviceOnline(mOnlineWaitTime);
        if (device == null) {
            handleDeviceBootloaderNotAvailable(monitor);
            return;
        }
        rebootDeviceIntoBootloader(device);
        if (!monitor.waitForDeviceBootloader(mBootloaderWaitTime)) {
            throw new DeviceNotAvailableException(String.format(
                    "Device %s not in bootloader after reboot", monitor.getSerialNumber()),
                    monitor.getSerialNumber());
        }
    }

    /**
     * @param monitor
     * @throws DeviceNotAvailableException
     */
    protected void handleDeviceBootloaderUnresponsive(IDeviceStateMonitor monitor)
            throws DeviceNotAvailableException {
        CLog.i("Found device %s in fastboot but potentially unresponsive.",
                monitor.getSerialNumber());
        // TODO: retry reboot
        getRunUtil().runTimedCmd(mFastbootWaitTime, mFastbootPath, "-s", monitor.getSerialNumber(),
                "reboot-bootloader");
        // wait for device to reboot
        monitor.waitForDeviceNotAvailable(20*1000);
        if (!monitor.waitForDeviceBootloader(mBootloaderWaitTime)) {
            throw new DeviceNotAvailableException(String.format(
                    "Device %s not in bootloader after reboot", monitor.getSerialNumber()),
                    monitor.getSerialNumber());
        }
        // running a meaningless command just to see whether the device is responsive.
        CommandResult result = getRunUtil().runTimedCmd(mFastbootWaitTime, mFastbootPath, "-s",
                monitor.getSerialNumber(), "getvar", "product");
        if (result.getStatus().equals(CommandStatus.TIMED_OUT)) {
            throw new DeviceNotAvailableException(String.format(
                    "Device %s is in fastboot but unresponsive", monitor.getSerialNumber()),
                    monitor.getSerialNumber());
        }
    }

    /**
     * Reboot device into bootloader.
     *
     * @param device the {@link IDevice} to reboot.
     */
    protected void rebootDeviceIntoBootloader(IDevice device) {
        try {
            device.reboot("bootloader");
        } catch (IOException e) {
            Log.w(LOG_TAG, String.format("failed to reboot %s: %s", device.getSerialNumber(),
                    e.getMessage()));
        } catch (TimeoutException e) {
            Log.w(LOG_TAG, String.format("failed to reboot %s: timeout", device.getSerialNumber()));
        } catch (AdbCommandRejectedException e) {
            Log.w(LOG_TAG, String.format("failed to reboot %s: %s", device.getSerialNumber(),
                    e.getMessage()));
        }
    }

    /**
     * Reboot device into bootloader.
     *
     * @param device the {@link IDevice} to reboot.
     */
    protected void rebootDevice(IDevice device) {
        try {
            device.reboot(null);
        } catch (IOException e) {
            Log.w(LOG_TAG, String.format("failed to reboot %s: %s", device.getSerialNumber(),
                    e.getMessage()));
        } catch (TimeoutException e) {
            Log.w(LOG_TAG, String.format("failed to reboot %s: timeout", device.getSerialNumber()));
        } catch (AdbCommandRejectedException e) {
            Log.w(LOG_TAG, String.format("failed to reboot %s: %s", device.getSerialNumber(),
                    e.getMessage()));
        }
    }

    /**
     * Handle situation where device is not available when expected to be in bootloader.
     *
     * @param monitor the {@link IDeviceStateMonitor}
     * @throws DeviceNotAvailableException
     */
    protected void handleDeviceBootloaderNotAvailable(final IDeviceStateMonitor monitor)
            throws DeviceNotAvailableException {
        throw new DeviceNotAvailableException(String.format(
                "Could not find device %s in bootloader", monitor.getSerialNumber()),
                monitor.getSerialNumber());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void recoverDeviceRecovery(IDeviceStateMonitor monitor)
            throws DeviceNotAvailableException {
        throw new DeviceNotAvailableException("device recovery not implemented",
                monitor.getSerialNumber());
    }
}
