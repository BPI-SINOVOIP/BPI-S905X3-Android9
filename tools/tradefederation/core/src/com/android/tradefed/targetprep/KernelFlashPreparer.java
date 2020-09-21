/*
 * Copyright (C) 2012 The Android Open Source Project
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
import com.android.tradefed.build.KernelDeviceBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import java.io.File;
import java.io.IOException;

/** A {@link ITargetPreparer} that flashes a kernel on the device. */
public class KernelFlashPreparer extends BaseTargetPreparer {

    /**
     * {@inheritDoc}
     * <p>
     * Expects a {@link KernelDeviceBuildInfo} that returns non-null values for
     * {@link KernelDeviceBuildInfo#getMkbootimgFile()} and
     * {@link KernelDeviceBuildInfo#getRamdiskFile()}.
     * </p>
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        if (!(buildInfo instanceof KernelDeviceBuildInfo)) {
            throw new IllegalArgumentException("Provided buildInfo is not a KernelDeviceBuildInfo");
        }

        KernelDeviceBuildInfo kernelBuildInfo = (KernelDeviceBuildInfo) buildInfo;
        CLog.i("Flashing device %s running %s with kernel %s", device.getSerialNumber(),
                device.getBuildId(), kernelBuildInfo.getSha1());

        if (kernelBuildInfo.getRamdiskFile() == null) {
            throw new TargetSetupError("Missing ramdisk file", device.getDeviceDescriptor());
        }
        if (kernelBuildInfo.getMkbootimgFile() == null) {
            throw new TargetSetupError("Missing mkbootimg file", device.getDeviceDescriptor());
        }
        if (kernelBuildInfo.getKernelFile() == null) {
            throw new TargetSetupError("Missing kernel file", device.getDeviceDescriptor());
        }

        kernelBuildInfo.getMkbootimgFile().setExecutable(true);

        File boot = null;
        try {
            boot = createBootImage(kernelBuildInfo.getMkbootimgFile(),
                    kernelBuildInfo.getKernelFile(), kernelBuildInfo.getRamdiskFile());
        } catch (IOException e) {
            throw new TargetSetupError("Could not create boot image", e,
                    device.getDeviceDescriptor());
        }

        try {
            device.rebootIntoBootloader();
            CLog.d("fastboot flash boot %s", boot.getAbsolutePath());
            device.executeFastbootCommand("flash", "boot", boot.getAbsolutePath());
        } finally {
            FileUtil.deleteFile(boot);
        }

        try {
            device.reboot();
        } catch (DeviceUnresponsiveException e) {
            // assume this is a build problem
            throw new BuildError(String.format("Device %s did not become available after " +
                    "flashing kernel", device.getSerialNumber()), device.getDeviceDescriptor());
        }
        device.postBootSetup();
    }

    /**
     * Creates a boot.img file using mkbootimg, ramdisk, and kernel files.
     * <p>
     * Runs {@code mkbootimg --kernel (filename) --ramdisk (filename) -o (filename)} to create a
     * boot.img image file. Exposed for unit testing.
     * </p>
     *
     * @param mkbootimg the mkbootimg executable
     * @param kernel the kernel file
     * @param ramdisk the ramdisk file
     * @return a boot.img {@link File} for the build and kernel build
     * @throws IOException if there was an exception making the boot image
     */
    File createBootImage(File mkbootimg, File kernel, File ramdisk) throws IOException {
        CLog.d("Create boot.img from %s and %s", kernel.getAbsolutePath(),
                ramdisk.getAbsolutePath());
        String bootPath = getBootImgPath();
        try {
            String[] cmd = {mkbootimg.getAbsolutePath(), "--kernel", kernel.getAbsolutePath(),
                    "--ramdisk", ramdisk.getAbsolutePath(), "-o", bootPath};
            CommandResult result = getRunUtil().runTimedCmd(30 * 1000, cmd);
            if (result.getStatus() != CommandStatus.SUCCESS) {
                CLog.e("mkbootimg failed. Command status was %s", result.getStatus());
                FileUtil.deleteFile(new File(bootPath));
                throw new IOException();
            }
        } catch (IOException e) {
            FileUtil.deleteFile(new File(bootPath));
            throw e;
        } catch (RuntimeException e) {
            FileUtil.deleteFile(new File(bootPath));
            throw e;
        }
        return new File(bootPath);
    }

    /**
     * Get an {@link IRunUtil} instance. Exposed for unit testing.
     */
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /**
     * Create a path for the boot image using {@link FileUtil#createTempFile(String, String)}.
     * Exposed for unit testing.
     */
    String getBootImgPath() throws IOException {
        File bootImg = FileUtil.createTempFile("boot", ".img");
        String bootImgPath = bootImg.getAbsolutePath();
        FileUtil.deleteFile(bootImg);
        return bootImgPath;
    }
}
