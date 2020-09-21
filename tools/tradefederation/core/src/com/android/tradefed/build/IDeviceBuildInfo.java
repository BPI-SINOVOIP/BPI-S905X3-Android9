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

package com.android.tradefed.build;

import java.io.File;

/**
 * A {@link IBuildInfo} that represents a complete Android device build and (optionally) its tests.
 */
public interface IDeviceBuildInfo extends IBuildInfo {

    /**
     * Returns the unique identifier of platform build under test. Should never be null. Defaults to
     * {@link #UNKNOWN_BUILD_ID}.
     */
    public String getDeviceBuildId();

    /**
     * Optional method to return the type of the platform build being tested.
     */
    public String getDeviceBuildFlavor();

    /**
     * Get the local device image zip file.
     */
    public File getDeviceImageFile();

    /**
     * Get the local device image zip version.
     */
    public String getDeviceImageVersion();

    /**
     * Set the device system image file to use.
     *
     * @param deviceImageFile
     */
    public void setDeviceImageFile(File deviceImageFile, String version);

    /**
     * Get the local test userdata image file.
     */
    public File getUserDataImageFile();

    /**
     * Get the local test userdata image version.
     */
    public String getUserDataImageVersion();

    /**
     * Set the user data image file to use.
     *
     * @param userDataFile
     */
    public void setUserDataImageFile(File userDataFile, String version);

    /**
     * Get the local path to the extracted tests.zip file contents.
     */
    public File getTestsDir();

    /**
     * Get the extracted tests.zip version.
     */
    public String getTestsDirVersion();

    /**
     * Set local path to the extracted tests.zip file contents.
     *
     * @param testsZipFile
     */
    public void setTestsDir(File testsZipFile, String version);

    /**
     * Get the local baseband image file.
     */
    public File getBasebandImageFile();

    /**
     * Get the baseband version.
     */
    public String getBasebandVersion();

    /**
     * Set the baseband image for the device build.
     *
     * @param basebandFile the baseband image {@link File}
     * @param version the version of the baseband
     */
    public void setBasebandImage(File basebandFile, String version);

    /**
     * Get the local bootloader image file.
     */
    public File getBootloaderImageFile();

    /**
     * Get the bootloader version.
     */
    public String getBootloaderVersion();

    /**
     * Set the bootloader image for the device build.
     *
     * @param bootloaderImgFile the bootloader image {@link File}
     * @param version the version of the bootloader
     */
    public void setBootloaderImageFile(File bootloaderImgFile, String version);

    /**
     * Get the device OTA package zip file.
     */
    public File getOtaPackageFile();

    /**
     * Get the device OTA package zip version.
     */
    public String getOtaPackageVersion();

    /**
     * Set the device OTA package zip file.
     */
    public void setOtaPackageFile(File otaFile, String version);

    /**
     * Gets the mkbootimg file used to create the kernel image.
     */
    public File getMkbootimgFile();

    /**
     * Gets the mkbootimg version.
     */
    public String getMkbootimgVersion();

    /**
     * Sets the mkbootimg file used to create the kernel image.
     */
    public void setMkbootimgFile(File mkbootimg, String version);

    /**
     * Gets the ramdisk file used to create the kernel image.
     */
    public File getRamdiskFile();

    /**
     * Gets the ramdisk version.
     */
    public String getRamdiskVersion();

    /**
     * Gets the ramdisk file used to create the kernel image.
     */
    public void setRamdiskFile(File ramdisk, String version);

    /**
     * Removes all temporary files.
     */
    @Override
    public void cleanUp();

}
