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

import com.android.tradefed.build.BuildInfoKey.BuildInfoFileKey;

import java.io.File;

/**
 * A {@link IBuildInfo} that represents a complete Android device build and (optionally) its tests.
 */
public class DeviceBuildInfo extends BuildInfo implements IDeviceBuildInfo {

    private static final long serialVersionUID = BuildSerializedVersion.VERSION;

    private static final String[] FILE_NOT_TO_CLONE =
            new String[] {
                BuildInfoFileKey.TESTDIR_IMAGE.getFileKey(),
                BuildInfoFileKey.HOST_LINKED_DIR.getFileKey(),
                BuildInfoFileKey.TARGET_LINKED_DIR.getFileKey()
            };

    public DeviceBuildInfo() {
        super();
    }

    public DeviceBuildInfo(String buildId, String buildTargetName) {
        super(buildId, buildTargetName);
    }

    /**
     * @deprecated use the constructor without test-tag instead. test-tag is no longer a mandatory
     * option for build info.
     */
    @Deprecated
    public DeviceBuildInfo(String buildId, String testTag, String buildTargetName) {
        super(buildId, testTag, buildTargetName);
    }

    public DeviceBuildInfo(BuildInfo buildInfo) {
        super(buildInfo);
    }

    /**
     * {@inheritDoc}
     *
     * @return {@link #getDeviceImageVersion()} if not {@code null}, else
     * {@link IBuildInfo#UNKNOWN_BUILD_ID}
     *
     * @see #getDeviceImageVersion()
     */
    @Override
    public String getDeviceBuildId() {
        String buildId = getDeviceImageVersion();
        return buildId == null ? UNKNOWN_BUILD_ID : buildId;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getDeviceBuildFlavor() {
        return getBuildFlavor();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getDeviceImageFile() {
        return getFile(BuildInfoFileKey.DEVICE_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getDeviceImageVersion() {
        return getVersion(BuildInfoFileKey.DEVICE_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDeviceImageFile(File deviceImageFile, String version) {
        setFile(BuildInfoFileKey.DEVICE_IMAGE, deviceImageFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getUserDataImageFile() {
        return getFile(BuildInfoFileKey.USERDATA_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getUserDataImageVersion() {
        return getVersion(BuildInfoFileKey.USERDATA_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUserDataImageFile(File userDataFile, String version) {
        setFile(BuildInfoFileKey.USERDATA_IMAGE, userDataFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getTestsDir() {
        return getFile(BuildInfoFileKey.TESTDIR_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getTestsDirVersion() {
        return getVersion(BuildInfoFileKey.TESTDIR_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTestsDir(File testsDir, String version) {
        setFile(BuildInfoFileKey.TESTDIR_IMAGE, testsDir, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getBasebandImageFile() {
        return getFile(BuildInfoFileKey.BASEBAND_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getBasebandVersion() {
        return getVersion(BuildInfoFileKey.BASEBAND_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBasebandImage(File basebandFile, String version) {
        setFile(BuildInfoFileKey.BASEBAND_IMAGE, basebandFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getBootloaderImageFile() {
        return getFile(BuildInfoFileKey.BOOTLOADER_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getBootloaderVersion() {
        return getVersion(BuildInfoFileKey.BOOTLOADER_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBootloaderImageFile(File bootloaderImgFile, String version) {
        setFile(BuildInfoFileKey.BOOTLOADER_IMAGE, bootloaderImgFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getOtaPackageFile() {
        return getFile(BuildInfoFileKey.OTA_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getOtaPackageVersion() {
        return getVersion(BuildInfoFileKey.OTA_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setOtaPackageFile(File otaFile, String version) {
        setFile(BuildInfoFileKey.OTA_IMAGE, otaFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getMkbootimgFile() {
        return getFile(BuildInfoFileKey.MKBOOTIMG_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getMkbootimgVersion() {
        return getVersion(BuildInfoFileKey.MKBOOTIMG_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setMkbootimgFile(File mkbootimg, String version) {
        setFile(BuildInfoFileKey.MKBOOTIMG_IMAGE, mkbootimg, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getRamdiskFile() {
        return getFile(BuildInfoFileKey.RAMDISK_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getRamdiskVersion() {
        return getVersion(BuildInfoFileKey.RAMDISK_IMAGE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setRamdiskFile(File ramdisk, String version) {
        setFile(BuildInfoFileKey.RAMDISK_IMAGE, ramdisk, version);
    }

    /** {@inheritDoc} */
    @Override
    protected boolean applyBuildProperties(
            VersionedFile origFileConsidered, IBuildInfo build, IBuildInfo receiver) {
        // If the no copy on sharding is set, that means the tests dir will be shared and should
        // not be copied.
        if (getProperties().contains(BuildInfoProperties.DO_NOT_COPY_ON_SHARDING)) {
            for (String name : FILE_NOT_TO_CLONE) {
                if (origFileConsidered.getFile().equals(build.getFile(name))) {
                    receiver.setFile(
                            name, origFileConsidered.getFile(), origFileConsidered.getVersion());
                    return true;
                }
            }
        }
        return false;
    }
}
