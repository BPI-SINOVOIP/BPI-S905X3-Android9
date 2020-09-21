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
package com.android.fastboot.tests;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceDisconnectedException;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IManagedTestDevice;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.ITestDevice.RecoveryMode;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.DeviceFlashPreparer;
import com.android.tradefed.targetprep.IDeviceFlasher.UserDataFlashOption;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.io.File;
import java.util.HashMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

/**
 * Flashes the device as part of the test, report device post-flash status as test results.
 * TODO: Add more fastboot test validation step.
 */
public class FastbootTest implements IRemoteTest, IDeviceTest, IBuildReceiver {

    private static final String FASTBOOT_TEST = FastbootTest.class.getName();
    /** the desired recentness of battery level **/
    private static final long BATTERY_FRESHNESS_MS = 30 * 1000;
    private static final long INVALID_TIME_DURATION = -1;
    private static final String INITIAL_BOOT_TIME = "initial-boot";
    private static final String ONLINE_TIME = "online";

    @Option(name = "device-boot-time", description = "Max time in ms to wait for"
            + " device to boot.", isTimeVal = true)
    private long mDeviceBootTimeMs = 5 * 60 * 1000;

    @Option(name = "userdata-flash", description = "Specify handling of userdata partition.")
    private UserDataFlashOption mUserDataFlashOption = UserDataFlashOption.WIPE;

    @Option(name = "concurrent-flasher-limit", description = "The maximum number of concurrent"
            + " flashers (may be useful to avoid memory constraints)")
    private Integer mConcurrentFlasherLimit = null;

    @Option(name = "skip-battery-check", description = "If true, the battery reading test will"
            + " be skipped.")
    private boolean mSkipBatteryCheck = false;

    @Option(name = "flasher-class", description = "The Flasher class (implementing "
            + "DeviceFlashPreparer) to be used for the fastboot test", mandatory = true)
    private String mFlasherClass;

    private IBuildInfo mBuildInfo;
    private ITestDevice mDevice;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        long start = System.currentTimeMillis();
        listener.testRunStarted(FASTBOOT_TEST, 1);
        String originalFastbootpath = ((IManagedTestDevice)mDevice).getFastbootPath();
        try {
            testFastboot(listener);
        } finally {
            // reset fastboot path
            ((IManagedTestDevice)mDevice).setFastbootPath(originalFastbootpath);
            listener.testRunEnded(
                    System.currentTimeMillis() - start, new HashMap<String, Metric>());
        }
    }

    /**
     * Flash the device and calculate the time to bring the device online and first boot.
     * @param listener
     * @throws DeviceNotAvailableException
     */
    private void testFastboot(ITestInvocationListener listener) throws DeviceNotAvailableException {
        HashMap<String, Metric> result = new HashMap<>();
        TestDescription firstBootTestId =
                new TestDescription(
                        String.format("%s.%s", FASTBOOT_TEST, FASTBOOT_TEST), FASTBOOT_TEST);
        listener.testStarted(firstBootTestId);
        DeviceFlashPreparer flasher = loadFlashPreparerClass();
        long bootStart = INVALID_TIME_DURATION;
        long onlineTime = INVALID_TIME_DURATION;
        long bootTime = INVALID_TIME_DURATION;
        try {
            if (flasher == null) {
                throw new RuntimeException(String.format("Could not find flasher %s",
                        mFlasherClass));
            }
            try {
                OptionSetter setter = new OptionSetter(flasher);
                // duping and passing parameters down to flasher
                setter.setOptionValue("device-boot-time", Long.toString(mDeviceBootTimeMs));
                setter.setOptionValue("userdata-flash", mUserDataFlashOption.toString());
                if (mConcurrentFlasherLimit != null) {
                    setter.setOptionValue("concurrent-flasher-limit",
                            mConcurrentFlasherLimit.toString());
                }
                // always to skip because the test needs to detect device online
                // and available state individually
                setter.setOptionValue("skip-post-flashing-setup", "true");
                setter.setOptionValue("force-system-flash", "true");
            } catch (ConfigurationException ce) {
                // this really shouldn't happen, but if it does, it'll indicate a setup problem
                // so this should be exposed, even at the expense of categorizing the build as
                // having a critical failure
                throw new RuntimeException("failed to set options for flasher", ce);
            }

            File fastboot = getFastbootFile(mBuildInfo);
            if (fastboot == null) {
                listener.testFailed(firstBootTestId,
                        "Couldn't find the fastboot binary in build info.");
                return;
            }
            // Set the fastboot path for device
            ((IManagedTestDevice)mDevice).setFastbootPath(fastboot.getAbsolutePath());

            // flash it!
            CLog.v("Flashing device %s", mDevice.getSerialNumber());
            try {
                flasher.setUp(mDevice, mBuildInfo);
                // we are skipping post boot setup so this is the start of boot process
                bootStart = System.currentTimeMillis();
            } catch (TargetSetupError | BuildError e) {
                // setUp() may throw DeviceNotAvailableException, TargetSetupError and BuildError.
                // DNAE is allowed to get thrown here so that a tool failure is triggered and build
                // maybe retried; the other 2x types are also rethrown as RuntimeException's for
                // the same purpose. In general, these exceptions reflect flashing or infra related
                // flakiness, so retrying is a reasonable mitigation.
                throw new RuntimeException("Exception during device flashing", e);
            }
            // check if device is online after flash, i.e. if adb is broken
            CLog.v("Waiting for device %s online", mDevice.getSerialNumber());
            mDevice.setRecoveryMode(RecoveryMode.ONLINE);
            try {
                mDevice.waitForDeviceOnline();
                onlineTime = System.currentTimeMillis() - bootStart;
            } catch (DeviceNotAvailableException dnae) {
                CLog.e("Device not online after flashing");
                CLog.e(dnae);
                listener.testRunFailed("Device not online after flashing");
                throw new DeviceDisconnectedException("Device not online after flashing",
                        mDevice.getSerialNumber());
            }
            // check if device can be fully booted, i.e. if application framework won't boot
            CLog.v("Waiting for device %s boot complete", mDevice.getSerialNumber());
            mDevice.setRecoveryMode(RecoveryMode.AVAILABLE);
            try {
                mDevice.waitForDeviceAvailable(mDeviceBootTimeMs);
                bootTime = System.currentTimeMillis() - bootStart;
            } catch (DeviceNotAvailableException dnae) {
                CLog.e("Device %s not available after flashing", mDevice.getSerialNumber());
                CLog.e(dnae);
                // only report as test failure, not test run failure because we were able to run the
                // test until the end, despite the failure verdict, and the device is returned to
                // the pool in a useable state
                listener.testFailed(firstBootTestId, "Device not available after flashing");
                return;
            }
            CLog.v("Device %s boot complete", mDevice.getSerialNumber());
            if (mSkipBatteryCheck) {
                // If we skip the battery Check, we can return directly after boot complete.
                return;
            }
            // We check if battery level are readable as non root to ensure that device is usable.
            mDevice.disableAdbRoot();
            try {
                Future<Integer> batteryFuture = mDevice.getIDevice()
                        .getBattery(BATTERY_FRESHNESS_MS, TimeUnit.MILLISECONDS);
                // get cached value or wait up to 500ms for battery level query
                Integer level = batteryFuture.get(500, TimeUnit.MILLISECONDS);
                CLog.d("Battery level value reading is: '%s'", level);
                if (level == null) {
                    listener.testFailed(firstBootTestId, "Reading of battery level is wrong.");
                    return;
                }
            } catch (InterruptedException | ExecutionException |
                    java.util.concurrent.TimeoutException e) {
                CLog.e("Failed to query battery level for %s", mDevice.getSerialNumber());
                CLog.e(e);
                listener.testFailed(firstBootTestId, "Failed to query battery level.");
                return;
            } finally {
                mDevice.enableAdbRoot();
            }
        } finally {
            CLog.d("Device online time: %dms, initial boot time: %dms", onlineTime, bootTime);
            if (onlineTime != INVALID_TIME_DURATION) {
                result.put(
                        ONLINE_TIME, TfMetricProtoUtil.stringToMetric(Long.toString(onlineTime)));
            }
            if (bootTime != INVALID_TIME_DURATION) {
                result.put(
                        INITIAL_BOOT_TIME,
                        TfMetricProtoUtil.stringToMetric(Long.toString(bootTime)));
            }
            listener.testEnded(firstBootTestId, result);
        }
    }

    /**
     * Attempt to load the class implementing {@link DeviceFlashPreparer} based on the option
     * flasher-class, allows anybody to tests fastboot using their own implementation of it.
     */
    private DeviceFlashPreparer loadFlashPreparerClass() {
        try {
            Class<?> flasherClass = Class.forName(mFlasherClass);
            Object flasherObject = flasherClass.newInstance();
            if (flasherObject instanceof DeviceFlashPreparer) {
                return (DeviceFlashPreparer)flasherObject;
            } else {
                CLog.e("Loaded class '%s' is not an instance of DeviceFlashPreparer.",
                        flasherObject);
                return null;
            }
        } catch (ClassNotFoundException | InstantiationException | IllegalAccessException e) {
            CLog.e(e);
            return null;
        }
    }

    /**
     * Helper to find the fastboot file as part of the buildinfo file list.
     */
    private File getFastbootFile(IBuildInfo buildInfo) {
        File fastboot = buildInfo.getFile("fastboot");
        if (fastboot == null) {
            return null;
        }
        FileUtil.chmodGroupRWX(fastboot);
        return fastboot;
    }
}
