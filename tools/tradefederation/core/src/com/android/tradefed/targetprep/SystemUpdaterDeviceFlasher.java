/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CommandStatus;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

/**
 * A {@link IDeviceFlasher} that relies on the system updater to install a
 * system image bundled in a OTA update package. In particular, this
 * implementation doesn't rely on fastboot.
 */
public class SystemUpdaterDeviceFlasher implements IDeviceFlasher {

    private ITestsZipInstaller mTestsZipInstaller = new DefaultTestsZipInstaller();

    private UserDataFlashOption mFlashOption = UserDataFlashOption.TESTS_ZIP;

    private boolean mForceSystemFlash;

    private Collection<String> mDataWipeSkipList = new ArrayList<String>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void setFlashingResourcesRetriever(IFlashingResourcesRetriever retriever) {
        // ignore
    }

    /**
     * {@inheritDoc}
     * <p>
     * This implementation assumes the device image file returned by
     * {@link IDeviceBuildInfo#getDeviceImageFile()} is an OTA update zip. It's
     * not safe to use this updater in a context where this interpretation
     * doesn't hold.
     *
     * @throws DeviceNotAvailableException
     * @throws TargetSetupError
     */
    @Override
    public void flash(ITestDevice device, IDeviceBuildInfo deviceBuild)
            throws DeviceNotAvailableException, TargetSetupError {
        CLog.i("Flashing device %s  with build %s", device.getSerialNumber(),
            deviceBuild.getDeviceBuildId());

        // TODO could add a check for bootloader versions and install
        // the one produced by the build server @ the current build if the one
        // on the target is different

        if (installUpdate(device, deviceBuild)) {
            if (mFlashOption == UserDataFlashOption.TESTS_ZIP) {
                mTestsZipInstaller.pushTestsZipOntoData(device, deviceBuild);
                CLog.d("rebooting after installing tests");
                device.reboot();
            }
        }
    }

    private boolean installUpdate(ITestDevice device, IDeviceBuildInfo deviceBuild)
            throws DeviceNotAvailableException, TargetSetupError {
        // FIXME same high level logic as in
        // FastbootDeviceFlasher#checkAndFlashSystem, could be de-duped
        if (!mForceSystemFlash && deviceBuild.getDeviceBuildId().equals(device.getBuildId())) {
            CLog.i("System is already version %s, skipping install" , device.getBuildId());
            // reboot
            return false;
        }
        CLog.i("Flashing system %s on device %s", deviceBuild.getDeviceBuildId(),
            device.getSerialNumber());
        File otaPackageFile = deviceBuild.getOtaPackageFile();
        if (otaPackageFile == null) {
            throw new TargetSetupError("No OTA package file present for build "
                    + deviceBuild.getDeviceBuildId(), device.getDeviceDescriptor());
        }
        if (!device.pushFile(otaPackageFile, "/cache/update.zip")) {
            throw new TargetSetupError("Could not push OTA file to the target.",
                    device.getDeviceDescriptor());
        }
        String commands =
                "echo --update_package > /cache/recovery/command &&" +
                // FIXME would need to be "CACHE:" instead of "/cache/" for
                // eclair devices
                "echo /cache/update.zip >> /cache/recovery/command";
        device.executeShellCommand(commands);
        device.rebootIntoRecovery();
        device.waitForDeviceAvailable();
        return true;
    }

    // friendly visibility for mock during testing
    void setTestsZipInstaller(ITestsZipInstaller testsZipInstaller) {
        mTestsZipInstaller = testsZipInstaller;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void overrideDeviceOptions(ITestDevice device) {
        // ignore
    }

    /**
     * {@inheritDoc}
     * <p>
     * This implementation only supports {@link IDeviceFlasher.UserDataFlashOption#TESTS_ZIP}
     * and {@link IDeviceFlasher.UserDataFlashOption#RETAIN} as a valid options
     */
    @Override
    public void setUserDataFlashOption(UserDataFlashOption flashOption) {
        List<UserDataFlashOption> supported = Arrays.asList(
                UserDataFlashOption.TESTS_ZIP, UserDataFlashOption.RETAIN);
        if (!supported.contains(flashOption)) {
            throw new IllegalArgumentException(flashOption
                    + " not supported. This implementation only supports flashing "
                    + supported.toString());
        }
        mFlashOption = flashOption;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public UserDataFlashOption getUserDataFlashOption() {
        return mFlashOption;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setForceSystemFlash(boolean forceSystemFlash) {
        mForceSystemFlash = forceSystemFlash;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDataWipeSkipList(Collection<String> dataWipeSkipList) {
        if (dataWipeSkipList != null) {
            mDataWipeSkipList = dataWipeSkipList;
        }
        if (mTestsZipInstaller != null) {
            mTestsZipInstaller.setDataWipeSkipList(mDataWipeSkipList);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setWipeTimeout(long timeout) {
        // ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public CommandStatus getSystemFlashingStatus() {
        // system partition flashing status is not applicable to this flasher
        return null;
    }
}
