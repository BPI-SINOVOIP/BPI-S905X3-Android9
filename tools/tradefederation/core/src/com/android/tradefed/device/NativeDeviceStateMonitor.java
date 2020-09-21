/*
 * Copyright (C) 2016 The Android Open Source Project
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
import com.android.ddmlib.CollectingOutputReceiver;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.ShellCommandUnresponsiveException;
import com.android.ddmlib.TimeoutException;
import com.android.tradefed.device.IDeviceManager.IFastbootListener;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

/**
 * Helper class for monitoring the state of a {@link IDevice} with no framework support.
 */
public class NativeDeviceStateMonitor implements IDeviceStateMonitor {

    static final String BOOTCOMPLETE_PROP = "dev.bootcomplete";

    private IDevice mDevice;
    private TestDeviceState mDeviceState;

    /** the time in ms to wait between 'poll for responsiveness' attempts */
    private static final long CHECK_POLL_TIME = 3 * 1000;
    protected static final long MAX_CHECK_POLL_TIME = 30 * 1000;
    /** the maximum operation time in ms for a 'poll for responsiveness' command */
    protected static final int MAX_OP_TIME = 10 * 1000;

    /** The  time in ms to wait for a device to be online. */
    private long mDefaultOnlineTimeout = 1 * 60 * 1000;

    /** The  time in ms to wait for a device to available. */
    private long mDefaultAvailableTimeout = 6 * 60 * 1000;

    private List<DeviceStateListener> mStateListeners;
    private IDeviceManager mMgr;
    private final boolean mFastbootEnabled;

    protected static final String PERM_DENIED_ERROR_PATTERN = "Permission denied";

    public NativeDeviceStateMonitor(IDeviceManager mgr, IDevice device,
            boolean fastbootEnabled) {
        mMgr = mgr;
        mDevice = device;
        mStateListeners = new ArrayList<DeviceStateListener>();
        mDeviceState = TestDeviceState.getStateByDdms(device.getState());
        mFastbootEnabled = fastbootEnabled;
    }

    /**
     * Get the {@link RunUtil} instance to use.
     * <p/>
     * Exposed for unit testing.
     */
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /**
     * Set the time in ms to wait for a device to be online in {@link #waitForDeviceOnline()}.
     */
    @Override
    public void setDefaultOnlineTimeout(long timeoutMs) {
        mDefaultOnlineTimeout = timeoutMs;
    }

    /**
     * Set the time in ms to wait for a device to be available in {@link #waitForDeviceAvailable()}.
     */
    @Override
    public void setDefaultAvailableTimeout(long timeoutMs) {
        mDefaultAvailableTimeout = timeoutMs;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IDevice waitForDeviceOnline(long waitTime) {
        if (waitForDeviceState(TestDeviceState.ONLINE, waitTime)) {
            return getIDevice();
        }
        return null;
    }

    /**
     * @return {@link IDevice} associate with the state monitor
     */
    protected IDevice getIDevice() {
        synchronized (mDevice) {
            return mDevice;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getSerialNumber() {
        return getIDevice().getSerialNumber();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IDevice waitForDeviceOnline() {
        return waitForDeviceOnline(mDefaultOnlineTimeout);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean waitForDeviceNotAvailable(long waitTime) {
        IFastbootListener listener = new StubFastbootListener();
        if (mFastbootEnabled) {
            mMgr.addFastbootListener(listener);
        }
        boolean result = waitForDeviceState(TestDeviceState.NOT_AVAILABLE, waitTime);
        if (mFastbootEnabled) {
            mMgr.removeFastbootListener(listener);
        }
        return result;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean waitForDeviceInRecovery(long waitTime) {
        return waitForDeviceState(TestDeviceState.RECOVERY, waitTime);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean waitForDeviceShell(final long waitTime) {
        CLog.i("Waiting %d ms for device %s shell to be responsive", waitTime,
                getSerialNumber());
        long startTime = System.currentTimeMillis();
        int counter = 1;
        while (System.currentTimeMillis() - startTime < waitTime) {
            final CollectingOutputReceiver receiver = createOutputReceiver();
            final String cmd = "ls /system/bin/adb";
            try {
                getIDevice().executeShellCommand(cmd, receiver, MAX_OP_TIME, TimeUnit.MILLISECONDS);
                String output = receiver.getOutput();
                if (output.contains("/system/bin/adb")) {
                    return true;
                }
            } catch (IOException | AdbCommandRejectedException |
                    ShellCommandUnresponsiveException e) {
                CLog.i("%s failed:", cmd);
                CLog.e(e);
            } catch (TimeoutException e) {
                CLog.i("%s failed: timeout", cmd);
                CLog.e(e);
            }
            getRunUtil().sleep(Math.min(getCheckPollTime() * counter, MAX_CHECK_POLL_TIME));
            counter++;
        }
        CLog.w("Device %s shell is unresponsive", getSerialNumber());
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IDevice waitForDeviceAvailable(final long waitTime) {
        // A device is currently considered "available" if and only if four events are true:
        // 1. Device is online aka visible via DDMS/adb
        // 2. Device has dev.bootcomplete flag set
        // 3. Device's package manager is responsive (may be inop)
        // 4. Device's external storage is mounted
        //
        // The current implementation waits for each event to occur in sequence.
        //
        // it will track the currently elapsed time and fail if it is
        // greater than waitTime

        long startTime = System.currentTimeMillis();
        IDevice device = waitForDeviceOnline(waitTime);
        if (device == null) {
            return null;
        }
        long elapsedTime = System.currentTimeMillis() - startTime;
        if (!waitForBootComplete(waitTime - elapsedTime)) {
            return null;
        }
        elapsedTime = System.currentTimeMillis() - startTime;
        if (!postOnlineCheck(waitTime - elapsedTime)) {
            return null;
        }
        return device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IDevice waitForDeviceAvailable() {
        return waitForDeviceAvailable(mDefaultAvailableTimeout);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean waitForBootComplete(final long waitTime) {
        CLog.i("Waiting %d ms for device %s boot complete", waitTime, getSerialNumber());
        int counter = 1;
        long startTime = System.currentTimeMillis();
        final String cmd = "getprop " + BOOTCOMPLETE_PROP;
        while ((System.currentTimeMillis() - startTime) < waitTime) {
            try {
                String bootFlag = getIDevice().getSystemProperty("dev.bootcomplete").get();
                if ("1".equals(bootFlag)) {
                    return true;
                }
            } catch (InterruptedException e) {
                CLog.i("%s on device %s failed: %s", cmd, getSerialNumber(), e.getMessage());
                // exit the loop for InterruptedException
                break;
            } catch (ExecutionException e) {
                CLog.i("%s on device %s failed: %s", cmd, getSerialNumber(), e.getMessage());
            }
            getRunUtil().sleep(Math.min(getCheckPollTime() * counter, MAX_CHECK_POLL_TIME));
            counter++;
        }
        CLog.w("Device %s did not boot after %d ms", getSerialNumber(), waitTime);
        return false;
    }

    /**
     * Additional checks to be done on an Online device
     *
     * @param waitTime time in ms to wait before giving up
     * @return <code>true</code> if checks are successful before waitTime expires.
     * <code>false</code> otherwise
     */
    protected boolean postOnlineCheck(final long waitTime) {
        return waitForStoreMount(waitTime);
    }

    /**
     * Waits for the device's external store to be mounted.
     *
     * @param waitTime time in ms to wait before giving up
     * @return <code>true</code> if external store is mounted before waitTime expires.
     * <code>false</code> otherwise
     */
    protected boolean waitForStoreMount(final long waitTime) {
        CLog.i("Waiting %d ms for device %s external store", waitTime, getSerialNumber());
        long startTime = System.currentTimeMillis();
        int counter = 1;
        while (System.currentTimeMillis() - startTime < waitTime) {
            final CollectingOutputReceiver receiver = createOutputReceiver();
            final CollectingOutputReceiver bitBucket = new CollectingOutputReceiver();
            final long number = getCurrentTime();
            final String externalStore = getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);

            final String testFile = String.format("'%s/%d'", externalStore, number);
            final String testString = String.format("number %d one", number);
            final String writeCmd = String.format("echo '%s' > %s", testString, testFile);
            final String checkCmd = String.format("cat %s", testFile);
            final String cleanupCmd = String.format("rm %s", testFile);
            String cmd = null;
            if (externalStore != null) {
                try {
                    cmd = writeCmd;
                    getIDevice().executeShellCommand(writeCmd, bitBucket,
                            MAX_OP_TIME, TimeUnit.MILLISECONDS);
                    cmd = checkCmd;
                    getIDevice().executeShellCommand(checkCmd, receiver,
                            MAX_OP_TIME, TimeUnit.MILLISECONDS);
                    cmd = cleanupCmd;
                    getIDevice().executeShellCommand(cleanupCmd, bitBucket,
                            MAX_OP_TIME, TimeUnit.MILLISECONDS);

                    String output = receiver.getOutput();
                    CLog.v("%s returned %s", checkCmd, output);
                    if (output.contains(testString)) {
                        return true;
                    } else if (output.contains(PERM_DENIED_ERROR_PATTERN)) {
                        CLog.w("Device %s mount check returned Permision Denied, "
                                + "issue with mounting.", getSerialNumber());
                        return false;
                    }
                } catch (IOException | AdbCommandRejectedException |
                        ShellCommandUnresponsiveException e) {
                    CLog.i("%s on device %s failed:", cmd, getSerialNumber());
                    CLog.e(e);
                } catch (TimeoutException e) {
                    CLog.i("%s on device %s failed: timeout", cmd, getSerialNumber());
                    CLog.e(e);
                }
            } else {
                CLog.w("Failed to get external store mount point for %s", getSerialNumber());
            }
            getRunUtil().sleep(Math.min(getCheckPollTime() * counter, MAX_CHECK_POLL_TIME));
            counter++;
        }
        CLog.w("Device %s external storage is not mounted after %d ms",
                getSerialNumber(), waitTime);
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getMountPoint(String mountName) {
        String mountPoint = getIDevice().getMountPoint(mountName);
        if (mountPoint != null) {
            return mountPoint;
        }
        // cached mount point is null - try querying directly
        CollectingOutputReceiver receiver = createOutputReceiver();
        try {
            getIDevice().executeShellCommand("echo $" + mountName, receiver);
            return receiver.getOutput().trim();
        } catch (IOException e) {
            return null;
        } catch (TimeoutException e) {
            return null;
        } catch (AdbCommandRejectedException e) {
            return null;
        } catch (ShellCommandUnresponsiveException e) {
            return null;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public TestDeviceState getDeviceState() {
        return mDeviceState;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean waitForDeviceBootloader(long time) {
        if (!mFastbootEnabled) {
            return false;
        }
        long startTime = System.currentTimeMillis();
        // ensure fastboot state is updated at least once
        waitForDeviceBootloaderStateUpdate();
        long elapsedTime = System.currentTimeMillis() - startTime;
        IFastbootListener listener = new StubFastbootListener();
        mMgr.addFastbootListener(listener);
        long waitTime = time - elapsedTime;
        if (waitTime < 0) {
            // wait at least 200ms
            waitTime = 200;
        }
        boolean result =  waitForDeviceState(TestDeviceState.FASTBOOT, waitTime);
        mMgr.removeFastbootListener(listener);
        return result;
    }

    @Override
    public void waitForDeviceBootloaderStateUpdate() {
        if (!mFastbootEnabled) {
            return;
        }
        IFastbootListener listener = new NotifyFastbootListener();
        synchronized (listener) {
            mMgr.addFastbootListener(listener);
            try {
                listener.wait();
            } catch (InterruptedException e) {
                CLog.w("wait for device bootloader state update interrupted");
                throw new RuntimeException(e);
            } finally {
                mMgr.removeFastbootListener(listener);
            }
        }
    }

    private boolean waitForDeviceState(TestDeviceState state, long time) {
        String deviceSerial = getSerialNumber();
        if (getDeviceState() == state) {
            CLog.i("Device %s is already %s", deviceSerial, state);
            return true;
        }
        CLog.i("Waiting for device %s to be %s; it is currently %s...", deviceSerial,
                state, getDeviceState());
        DeviceStateListener listener = new DeviceStateListener(state);
        addDeviceStateListener(listener);
        synchronized (listener) {
            try {
                listener.wait(time);
            } catch (InterruptedException e) {
                CLog.w("wait for device state interrupted");
                throw new RuntimeException(e);
            } finally {
                removeDeviceStateListener(listener);
            }
        }
        return getDeviceState().equals(state);
    }

    /**
     * @param listener
     */
    private void removeDeviceStateListener(DeviceStateListener listener) {
        synchronized (mStateListeners) {
            mStateListeners.remove(listener);
        }
    }

    /**
     * @param listener
     */
    private void addDeviceStateListener(DeviceStateListener listener) {
        synchronized (mStateListeners) {
            mStateListeners.add(listener);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setState(TestDeviceState deviceState) {
        mDeviceState = deviceState;
        // create a copy of listeners to prevent holding mStateListeners lock when notifying
        // and to protect from list modification when iterating
        Collection<DeviceStateListener> listenerCopy = new ArrayList<DeviceStateListener>(
                mStateListeners.size());
        synchronized (mStateListeners) {
            listenerCopy.addAll(mStateListeners);
        }
        for (DeviceStateListener listener: listenerCopy) {
            listener.stateChanged(deviceState);
        }
    }

    @Override
    public void setIDevice(IDevice newDevice) {
        IDevice currentDevice = mDevice;
        if (!getIDevice().equals(newDevice)) {
            synchronized (currentDevice) {
                mDevice = newDevice;
            }
        }
    }

    private static class DeviceStateListener {
        private final TestDeviceState mExpectedState;

        public DeviceStateListener(TestDeviceState expectedState) {
            mExpectedState = expectedState;
        }

        public void stateChanged(TestDeviceState newState) {
            if (mExpectedState.equals(newState)) {
                synchronized (this) {
                    notify();
                }
            }
        }
    }

    /**
     * An empty implementation of {@link IFastbootListener}
     */
    private static class StubFastbootListener implements IFastbootListener {
        @Override
        public void stateUpdated() {
            // ignore
        }
    }

    /**
     * A {@link IFastbootListener} that notifies when a status update has been received.
     */
    private static class NotifyFastbootListener implements IFastbootListener {
        @Override
        public void stateUpdated() {
            synchronized (this) {
                notify();
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isAdbTcp() {
        return mDevice.getSerialNumber().contains(":");
    }

    /**
     * Exposed for testing
     * @return {@link CollectingOutputReceiver}
     */
    protected CollectingOutputReceiver createOutputReceiver() {
        return new CollectingOutputReceiver();
    }

    /**
     * Exposed for testing
     */
    protected long getCheckPollTime() {
        return CHECK_POLL_TIME;
    }

    /**
     * Exposed for testing
     */
    protected long getCurrentTime() {
        return System.currentTimeMillis();
    }
}
