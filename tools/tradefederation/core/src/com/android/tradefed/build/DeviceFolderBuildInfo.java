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
import java.util.Collection;
import java.util.ArrayList;
import java.util.List;

/**
 * A {@link IDeviceBuildInfo} that also contains other build artifacts contained in a directory on
 * the local filesystem.
 */
public class DeviceFolderBuildInfo extends BuildInfo implements IDeviceBuildInfo, IFolderBuildInfo {

    private static final long serialVersionUID = BuildSerializedVersion.VERSION;
    private IDeviceBuildInfo mDeviceBuild;
    private IFolderBuildInfo mFolderBuild;

    /**
     * @see DeviceBuildInfo#DeviceBuildInfo(String, String)
     */
    public DeviceFolderBuildInfo(String buildId, String buildName) {
        super(buildId, buildName);
    }

    /**
     * @see DeviceBuildInfo#DeviceBuildInfo()
     */
    public DeviceFolderBuildInfo() {
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getDeviceBuildId() {
        return mDeviceBuild.getDeviceBuildId();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getDeviceBuildFlavor() {
        return mDeviceBuild.getDeviceBuildFlavor();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getDeviceImageFile() {
        return mDeviceBuild.getDeviceImageFile();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getDeviceImageVersion() {
        return mDeviceBuild.getDeviceImageVersion();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDeviceImageFile(File deviceImageFile, String version) {
        mDeviceBuild.setDeviceImageFile(deviceImageFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getUserDataImageFile() {
        return mDeviceBuild.getUserDataImageFile();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getUserDataImageVersion() {
        return mDeviceBuild.getUserDataImageVersion();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUserDataImageFile(File userDataFile, String version) {
        mDeviceBuild.setUserDataImageFile(userDataFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getTestsDir() {
        return mDeviceBuild.getTestsDir();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getTestsDirVersion() {
        return mDeviceBuild.getTestsDirVersion();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTestsDir(File testsDir, String version) {
        mDeviceBuild.setTestsDir(testsDir, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getBasebandImageFile() {
        return mDeviceBuild.getBasebandImageFile();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getBasebandVersion() {
        return mDeviceBuild.getBasebandVersion();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBasebandImage(File basebandFile, String version) {
        mDeviceBuild.setBasebandImage(basebandFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getBootloaderImageFile() {
        return mDeviceBuild.getBootloaderImageFile();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getBootloaderVersion() {
        return mDeviceBuild.getBootloaderVersion();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBootloaderImageFile(File bootloaderImgFile, String version) {
        mDeviceBuild.setBootloaderImageFile(bootloaderImgFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getOtaPackageFile() {
        return mDeviceBuild.getOtaPackageFile();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getOtaPackageVersion() {
        return mDeviceBuild.getOtaPackageVersion();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setOtaPackageFile(File otaFile, String version) {
        mDeviceBuild.setOtaPackageFile(otaFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getMkbootimgFile() {
        return mDeviceBuild.getMkbootimgFile();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getMkbootimgVersion() {
        return mDeviceBuild.getMkbootimgVersion();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setMkbootimgFile(File mkbootimg, String version) {
        mDeviceBuild.setMkbootimgFile(mkbootimg, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getRamdiskFile() {
        return mDeviceBuild.getRamdiskFile();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getRamdiskVersion() {
        return mDeviceBuild.getRamdiskVersion();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setRamdiskFile(File ramdisk, String version) {
        mDeviceBuild.setRamdiskFile(ramdisk, version);
    }

    /** {@inheritDoc} */
    @Override
    public Collection<VersionedFile> getFiles() {
        Collection<VersionedFile> rootFiles = super.getFiles();
        Collection<VersionedFile> deviceFiles = mDeviceBuild.getFiles();
        List<VersionedFile> combinedFiles = new ArrayList<VersionedFile>();
        combinedFiles.addAll(rootFiles);
        combinedFiles.addAll(deviceFiles);
        return combinedFiles;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getRootDir() {
        return mFolderBuild.getRootDir();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setRootDir(File rootDir) {
        mFolderBuild.setRootDir(rootDir);
    }

    /**
     * @param folderBuild
     */
    public void setFolderBuild(IFolderBuildInfo folderBuild) {
        mFolderBuild = folderBuild;
    }

    /**
     * @param deviceBuild
     */
    public void setDeviceBuild(IDeviceBuildInfo deviceBuild) {
        mDeviceBuild = deviceBuild;
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public void cleanUp() {
        mDeviceBuild.cleanUp();
        mFolderBuild.cleanUp();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo clone() {
        DeviceFolderBuildInfo copy = new DeviceFolderBuildInfo(getBuildId(), getBuildTargetName());
        copy.addAllBuildAttributes(this);
        IDeviceBuildInfo deviceBuildClone = (IDeviceBuildInfo)mDeviceBuild.clone();
        copy.setDeviceBuild(deviceBuildClone);
        IFolderBuildInfo folderBuildClone = (IFolderBuildInfo)mFolderBuild.clone();
        copy.setFolderBuild(folderBuildClone);

        return copy;
    }
}
