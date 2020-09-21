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
import com.android.ddmlib.IDevice;
import com.android.ddmlib.IDevice.DeviceState;
import com.android.ddmlib.ShellCommandUnresponsiveException;
import com.android.ddmlib.TimeoutException;
import com.android.tradefed.device.DeviceManager.FastbootDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import java.io.IOException;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Factory to create the different kind of devices that can be monitored by Tf
 */
public class ManagedTestDeviceFactory implements IManagedTestDeviceFactory {

    private static final String IPADDRESS_PATTERN =
            "((^([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
            "([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
            "([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
            "([01]?\\d\\d?|2[0-4]\\d|25[0-5]))|(localhost)){1}";

    protected boolean mFastbootEnabled;
    protected IDeviceManager mDeviceManager;
    protected IDeviceMonitor mAllocationMonitor;
    protected static final String CHECK_PM_CMD = "ls %s";
    protected static final String EXPECTED_RES = "/system/bin/pm";
    protected static final String EXPECTED_ERROR = "No such file or directory";
    protected static final long FRAMEWORK_CHECK_SLEEP_MS = 500;
    protected static final int FRAMEWORK_CHECK_MAX_RETRY = 3;

    public ManagedTestDeviceFactory(boolean fastbootEnabled, IDeviceManager deviceManager,
            IDeviceMonitor allocationMonitor) {
        mFastbootEnabled = fastbootEnabled;
        mDeviceManager = deviceManager;
        mAllocationMonitor = allocationMonitor;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IManagedTestDevice createDevice(IDevice idevice) {
        IManagedTestDevice testDevice = null;
        if (idevice instanceof TcpDevice || isTcpDeviceSerial(idevice.getSerialNumber())) {
            // Special device for Tcp device for custom handling.
            testDevice = new RemoteAndroidDevice(idevice,
                    new DeviceStateMonitor(mDeviceManager, idevice, mFastbootEnabled),
                    mAllocationMonitor);
            testDevice.setDeviceState(TestDeviceState.NOT_AVAILABLE);
        } else if (!checkFrameworkSupport(idevice)) {
            // Brillo device instance tier 1 (no framework support)
            testDevice = new NativeDevice(idevice,
                    new NativeDeviceStateMonitor(mDeviceManager, idevice, mFastbootEnabled),
                    mAllocationMonitor);
        } else {
            // Default to-go device is Android full stack device.
            testDevice = new TestDevice(idevice,
                    new DeviceStateMonitor(mDeviceManager, idevice, mFastbootEnabled),
                    mAllocationMonitor);
        }

        if (idevice instanceof FastbootDevice) {
            testDevice.setDeviceState(TestDeviceState.FASTBOOT);
        } else if (idevice instanceof StubDevice) {
            testDevice.setDeviceState(TestDeviceState.NOT_AVAILABLE);
        }
        testDevice.setFastbootEnabled(mFastbootEnabled);
        testDevice.setFastbootPath(mDeviceManager.getFastbootPath());
        return testDevice;
    }

    /**
     * Helper that return true if device has framework support.
     */
    protected boolean checkFrameworkSupport(IDevice idevice) {
        if (idevice instanceof StubDevice) {
            // Assume stub device should go to the default full framework support for
            // backward compatibility
            return true;
        }
        final long timeout = 60 * 1000;
        try {
            for (int i = 0; i < FRAMEWORK_CHECK_MAX_RETRY; i++) {
                CollectingOutputReceiver receiver = createOutputReceiver();
                if (!DeviceState.ONLINE.equals(idevice.getState())) {
                    // Device will be 'unavailable' and recreated in DeviceManager so no need to
                    // check.
                    CLog.w("Device state is not Online, assuming Framework support for now.");
                    return true;
                }
                String cmd = String.format(CHECK_PM_CMD, EXPECTED_RES);
                idevice.executeShellCommand(cmd, receiver, timeout, TimeUnit.MILLISECONDS);
                String output = receiver.getOutput().trim();
                // It can only be one of the expected output or an exception if device offline
                // otherwise we retry
                if (EXPECTED_RES.equals(output)) {
                    return true;
                }
                if (output.contains(EXPECTED_ERROR)) {
                    CLog.i("No support for Framework, creating a native device. "
                            + "output: %s", receiver.getOutput());
                    return false;
                }
                getRunUtil().sleep(FRAMEWORK_CHECK_SLEEP_MS);
            }
            CLog.w("Could not determine the framework support for '%s' after retries, assuming "
                    + "framework support.", idevice.getSerialNumber());
        } catch (TimeoutException | AdbCommandRejectedException | ShellCommandUnresponsiveException
                | IOException e) {
            CLog.w("Exception during checkFrameworkSupport, assuming True: '%s' with device: %s",
                    e.getMessage(), idevice.getSerialNumber());
            CLog.e(e);
        }
        // We default to support for framework to get same behavior as before.
        return true;
    }

    /**
     * Return the default {@link IRunUtil} instance.
     * Exposed for testing.
     */
    protected IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /**
     * Create a {@link CollectingOutputReceiver}.
     * Exposed for testing.
     */
    protected CollectingOutputReceiver createOutputReceiver() {
        return new CollectingOutputReceiver();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setFastbootEnabled(boolean enable) {
        mFastbootEnabled = enable;
    }

    /**
     * Helper to device if it's a serial from a remotely connected device.
     * serial format of tcp device is <ip or locahost>:<port>
     */
    protected boolean isTcpDeviceSerial(String serial) {
        final String remotePattern = IPADDRESS_PATTERN + "(:)([0-9]{2,5})(\\b)";
        Pattern pattern = Pattern.compile(remotePattern);
        Matcher match = pattern.matcher(serial.trim());
        if (match.find()) {
            return true;
        }
        return false;
    }
}
