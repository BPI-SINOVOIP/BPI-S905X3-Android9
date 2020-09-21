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

package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.ITestDevice.RecoveryMode;
import com.android.tradefed.host.IHostOptions;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.IDeviceFlasher.UserDataFlashOption;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import java.util.ArrayList;
import java.util.Collection;
import java.util.concurrent.TimeUnit;

/** A {@link ITargetPreparer} that flashes an image on physical Android hardware. */
public abstract class DeviceFlashPreparer extends BaseTargetPreparer implements ITargetCleaner {

    /**
     * Enum of options for handling the encryption of userdata image
     */
    public static enum EncryptionOptions {ENCRYPT, IGNORE}

    private static final int BOOT_POLL_TIME_MS = 5 * 1000;

    @Option(name = "device-boot-time", description = "max time in ms to wait for device to boot.")
    private long mDeviceBootTime = 5 * 60 * 1000;

    @Option(name = "userdata-flash", description =
        "specify handling of userdata partition.")
    private UserDataFlashOption mUserDataFlashOption = UserDataFlashOption.FLASH;

    @Option(name = "encrypt-userdata", description = "specify if userdata partition should be "
            + "encrypted; defaults to IGNORE, where no actions will be taken.")
    private EncryptionOptions mEncryptUserData = EncryptionOptions.IGNORE;

    @Option(name = "force-system-flash", description =
        "specify if system should always be flashed even if already running desired build.")
    private boolean mForceSystemFlash = false;

    /*
     * A temporary workaround for special builds. Should be removed after changes from build team.
     * Bug: 18078421
     */
    @Option(name = "skip-post-flash-flavor-check", description =
            "specify if system flavor should not be checked after flash")
    private boolean mSkipPostFlashFlavorCheck = false;

    /*
     * Used for update testing
     */
    @Option(name = "skip-post-flash-build-id-check", description =
            "specify if build ID should not be checked after flash")
    private boolean mSkipPostFlashBuildIdCheck = false;

    @Option(name = "wipe-skip-list", description =
        "list of /data subdirectories to NOT wipe when doing UserDataFlashOption.TESTS_ZIP")
    private Collection<String> mDataWipeSkipList = new ArrayList<>();

    /**
     * @deprecated use host-options:concurrent-flasher-limit.
     */
    @Deprecated
    @Option(name = "concurrent-flasher-limit", description =
        "No-op, do not use. Left for backwards compatibility.")
    private Integer mConcurrentFlasherLimit = null;

    @Option(name = "skip-post-flashing-setup",
            description = "whether or not to skip post-flashing setup steps")
    private boolean mSkipPostFlashingSetup = false;

    @Option(name = "wipe-timeout",
            description = "the timeout for the command of wiping user data.", isTimeVal = true)
    private long mWipeTimeout = 4 * 60 * 1000;

    @Option(
        name = "fastboot-flash-option",
        description = "additional options to pass with fastboot flash/update command."
    )
    private Collection<String> mFastbootFlashOptions = new ArrayList<>();

    /**
     * Sets the device boot time
     * <p/>
     * Exposed for unit testing
     */
    void setDeviceBootTime(long bootTime) {
        mDeviceBootTime = bootTime;
    }

    /**
     * Gets the interval between device boot poll attempts.
     * <p/>
     * Exposed for unit testing
     */
    int getDeviceBootPollTimeMs() {
        return BOOT_POLL_TIME_MS;
    }

    /**
     * Gets the {@link IRunUtil} instance to use.
     * <p/>
     * Exposed for unit testing
     */
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /**
     * Getg a reference to the {@link IDeviceManager}
     *
     * Exposed for unit testing
     *
     * @return the {@link IDeviceManager} to use
     */
    IDeviceManager getDeviceManager() {
        return GlobalConfiguration.getDeviceManagerInstance();
    }

    /**
     * Gets the {@link IHostOptions} instance to use.
     * <p/>
     * Exposed for unit testing
     */
    protected IHostOptions getHostOptions() {
        return GlobalConfiguration.getInstance().getHostOptions();
    }

    /**
     * Set the userdata-flash option
     *
     * @param flashOption
     */
    public void setUserDataFlashOption(UserDataFlashOption flashOption) {
        mUserDataFlashOption = flashOption;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            DeviceNotAvailableException, BuildError {
        if (isDisabled()) {
            CLog.i("Skipping device flashing.");
            return;
        }
        CLog.i("Performing setup on %s", device.getSerialNumber());
        if (!(buildInfo instanceof IDeviceBuildInfo)) {
            throw new IllegalArgumentException("Provided buildInfo is not a IDeviceBuildInfo");
        }
        // don't allow interruptions during flashing operations.
        getRunUtil().allowInterrupt(false);
        IDeviceManager deviceManager = getDeviceManager();
        long queueTime = -1;
        long flashingTime = -1;
        long start = -1;
        try {
            IDeviceBuildInfo deviceBuild = (IDeviceBuildInfo)buildInfo;
            checkDeviceProductType(device, deviceBuild);
            device.setRecoveryMode(RecoveryMode.ONLINE);
            IDeviceFlasher flasher = createFlasher(device);
            flasher.setWipeTimeout(mWipeTimeout);
            // only surround fastboot related operations with flashing permit restriction
            try {
                start = System.currentTimeMillis();
                deviceManager.takeFlashingPermit();
                queueTime = System.currentTimeMillis() - start;
                CLog.v("Flashing permit obtained after %ds",
                        TimeUnit.MILLISECONDS.toSeconds((queueTime)));

                flasher.overrideDeviceOptions(device);
                flasher.setUserDataFlashOption(mUserDataFlashOption);
                flasher.setForceSystemFlash(mForceSystemFlash);
                flasher.setDataWipeSkipList(mDataWipeSkipList);
                if (flasher instanceof FastbootDeviceFlasher) {
                    ((FastbootDeviceFlasher) flasher).setFlashOptions(mFastbootFlashOptions);
                }
                preEncryptDevice(device, flasher);
                start = System.currentTimeMillis();
                flasher.flash(device, deviceBuild);
            } finally {
                flashingTime = System.currentTimeMillis() - start;
                deviceManager.returnFlashingPermit();
                // report flashing status
                CommandStatus status = flasher.getSystemFlashingStatus();
                if (status == null) {
                    CLog.i("Skipped reporting metrics because system partitions were not flashed.");
                } else {
                    reportFlashMetrics(buildInfo.getBuildBranch(), buildInfo.getBuildFlavor(),
                            buildInfo.getBuildId(), device.getSerialNumber(), queueTime,
                            flashingTime, status);
                }
            }
            // only want logcat captured for current build, delete any accumulated log data
            device.clearLogcat();
            if (mSkipPostFlashingSetup) {
                return;
            }
            // Temporary re-enable interruptable since the critical flashing operation is over.
            getRunUtil().allowInterrupt(true);
            device.waitForDeviceOnline();
            // device may lose date setting if wiped, update with host side date in case anything on
            // device side malfunction with an invalid date
            if (device.enableAdbRoot()) {
                device.setDate(null);
            }
            // Disable interrupt for encryption operation.
            getRunUtil().allowInterrupt(false);
            checkBuild(device, deviceBuild);
            postEncryptDevice(device, flasher);
            // Once critical operation is done, we re-enable interruptable
            getRunUtil().allowInterrupt(true);
            try {
                device.setRecoveryMode(RecoveryMode.AVAILABLE);
                device.waitForDeviceAvailable(mDeviceBootTime);
            } catch (DeviceUnresponsiveException e) {
                // assume this is a build problem
                throw new DeviceFailedToBootError(String.format(
                        "Device %s did not become available after flashing %s",
                        device.getSerialNumber(), deviceBuild.getDeviceBuildId()),
                        device.getDeviceDescriptor());
            }
            device.postBootSetup();
        } finally {
            // Allow interruption at the end no matter what.
            getRunUtil().allowInterrupt(true);
        }
    }

    /**
     * Possible check before flashing to ensure the device is as expected compare to the build info.
     *
     * @param device the {@link ITestDevice} to flash.
     * @param deviceBuild the {@link IDeviceBuildInfo} used to flash.
     * @throws BuildError
     * @throws DeviceNotAvailableException
     */
    protected void checkDeviceProductType(ITestDevice device, IDeviceBuildInfo deviceBuild)
            throws BuildError, DeviceNotAvailableException {
        // empty of purpose
    }

    /**
     * Verifies the expected build matches the actual build on device after flashing
     * @throws DeviceNotAvailableException
     */
    private void checkBuild(ITestDevice device, IDeviceBuildInfo deviceBuild)
            throws DeviceNotAvailableException {
        // Need to use deviceBuild.getDeviceBuildId instead of getBuildId because the build info
        // could be an AppBuildInfo and return app build id. Need to be more explicit that we
        // check for the device build here.
        if (!mSkipPostFlashBuildIdCheck) {
            checkBuildAttribute(deviceBuild.getDeviceBuildId(), device.getBuildId(),
                    device.getSerialNumber());
        }
        if (!mSkipPostFlashFlavorCheck) {
            checkBuildAttribute(deviceBuild.getDeviceBuildFlavor(), device.getBuildFlavor(),
                    device.getSerialNumber());
        }
        // TODO: check bootloader and baseband versions too
    }

    private void checkBuildAttribute(String expectedBuildAttr, String actualBuildAttr,
            String serial) throws DeviceNotAvailableException {
        if (expectedBuildAttr == null || actualBuildAttr == null ||
                !expectedBuildAttr.equals(actualBuildAttr)) {
            // throw DNAE - assume device hardware problem - we think flash was successful but
            // device is not running right bits
            throw new DeviceNotAvailableException(
                    String.format("Unexpected build after flashing. Expected %s, actual %s",
                            expectedBuildAttr, actualBuildAttr), serial);
        }
    }

    /**
     * Create {@link IDeviceFlasher} to use. Subclasses can override
     * @throws DeviceNotAvailableException
     */
    protected abstract IDeviceFlasher createFlasher(ITestDevice device)
            throws DeviceNotAvailableException;

    /**
     * Handle encrypting of the device pre-flash.
     *
     * @see #postEncryptDevice(ITestDevice, IDeviceFlasher)
     * @param device
     * @throws DeviceNotAvailableException
     * @throws TargetSetupError if the device could not be encrypted or unlocked.
     */
    private void preEncryptDevice(ITestDevice device, IDeviceFlasher flasher)
            throws DeviceNotAvailableException, TargetSetupError {
        switch (mEncryptUserData) {
            case IGNORE:
                return;
            case ENCRYPT:
                if (!device.isEncryptionSupported()) {
                    throw new TargetSetupError("Encryption is not supported",
                            device.getDeviceDescriptor());
                }
                if (!device.isDeviceEncrypted()) {
                    switch(flasher.getUserDataFlashOption()) {
                        case TESTS_ZIP: // Intentional fall through.
                        case WIPE_RM:
                            // a new filesystem will not be created by the flasher, but the userdata
                            // partition is expected to be cleared anyway, so we encrypt the device
                            // with wipe
                            if (!device.encryptDevice(false)) {
                                throw new TargetSetupError("Failed to encrypt device",
                                        device.getDeviceDescriptor());
                            }
                            if (!device.unlockDevice()) {
                                throw new TargetSetupError("Failed to unlock device",
                                        device.getDeviceDescriptor());
                            }
                            break;
                        case RETAIN:
                            // original filesystem must be retained, so we encrypt in place
                            if (!device.encryptDevice(true)) {
                                throw new TargetSetupError("Failed to encrypt device",
                                        device.getDeviceDescriptor());
                            }
                            if (!device.unlockDevice()) {
                                throw new TargetSetupError("Failed to unlock device",
                                        device.getDeviceDescriptor());
                            }
                            break;
                        default:
                            // Do nothing, userdata will be encrypted post-flash.
                    }
                }
                break;
            default:
                // should not get here
                return;
        }
    }

    /**
     * Handle encrypting of the device post-flash.
     * <p>
     * This method handles encrypting the device after a flash in cases where a flash would undo any
     * encryption pre-flash, such as when the device is flashed or wiped.
     * </p>
     *
     * @see #preEncryptDevice(ITestDevice, IDeviceFlasher)
     * @param device
     * @throws DeviceNotAvailableException
     * @throws TargetSetupError If the device could not be encrypted or unlocked.
     */
    private void postEncryptDevice(ITestDevice device, IDeviceFlasher flasher)
            throws DeviceNotAvailableException, TargetSetupError {
        switch (mEncryptUserData) {
            case IGNORE:
                return;
            case ENCRYPT:
                if (!device.isEncryptionSupported()) {
                    throw new TargetSetupError("Encryption is not supported",
                            device.getDeviceDescriptor());
                }
                switch(flasher.getUserDataFlashOption()) {
                    case FLASH:
                        if (!device.encryptDevice(true)) {
                            throw new TargetSetupError("Failed to encrypt device",
                                    device.getDeviceDescriptor());
                        }
                        break;
                    case WIPE: // Intentional fall through.
                    case FORCE_WIPE:
                        // since the device was just wiped, encrypt with wipe
                        if (!device.encryptDevice(false)) {
                            throw new TargetSetupError("Failed to encrypt device",
                                    device.getDeviceDescriptor());
                        }
                        break;
                    default:
                        // Do nothing, userdata was encrypted pre-flash.
                }
                if (!device.unlockDevice()) {
                    throw new TargetSetupError("Failed to unlock device",
                            device.getDeviceDescriptor());
                }
                break;
            default:
                // should not get here
                return;
        }
    }

    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (isDisabled()) {
            CLog.i("Skipping device flashing tearDown.");
            return;
        }
        if (mEncryptUserData == EncryptionOptions.ENCRYPT
                && mUserDataFlashOption != UserDataFlashOption.RETAIN) {
            if (e instanceof DeviceNotAvailableException) {
                CLog.e("Device was encrypted but now unavailable. may need manual cleanup");
            } else if (device.isDeviceEncrypted()) {
                if (!device.unencryptDevice()) {
                    throw new RuntimeException("Failed to unencrypt device");
                }
            }
        }
    }

    /**
     * Reports device flashing timing data to metrics backend
     * @param branch the branch where the device build originated from
     * @param buildFlavor the build flavor of the device build
     * @param buildId the build number of the device build
     * @param serial the serial number of device
     * @param queueTime the time spent waiting for a flashing limit to become available
     * @param flashingTime the time spent in flashing device image zip
     * @param flashingStatus the execution status of flashing command
     */
    protected void reportFlashMetrics(String branch, String buildFlavor, String buildId,
            String serial, long queueTime, long flashingTime, CommandStatus flashingStatus) {
        // no-op as default implementation
    }
}
