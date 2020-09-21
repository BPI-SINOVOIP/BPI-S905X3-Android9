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
package com.android.tradefed.build;

import java.io.File;

/**
 * A {@link IBuildInfo} that represents a kernel build paired with a complete Android build.
 */
public class KernelDeviceBuildInfo extends BuildInfo implements IDeviceBuildInfo,
        IKernelBuildInfo {
    private static final long serialVersionUID = BuildSerializedVersion.VERSION;
    private IDeviceBuildInfo mDeviceBuild = new DeviceBuildInfo();
    private IKernelBuildInfo mKernelBuild = new KernelBuildInfo();

    /**
     * Creates a {@link KernelBuildInfo}.
     */
    public KernelDeviceBuildInfo() {
        super();
    }

    /**
     * Creates a {@link KernelBuildInfo}.
     *
     * @param buildId the build id as a combination of the kernel build id and the device build id
     * @param buildName the build name
     */
    public KernelDeviceBuildInfo(String buildId, String buildName) {
        super(buildId, buildName);
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

    /**
     * {@inheritDoc}
     */
    @Override
    public File getKernelFile() {
        return mKernelBuild.getKernelFile();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getKernelVersion() {
        return mKernelBuild.getKernelVersion();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setKernelFile(File kernelFile, String version) {
        mKernelBuild.setKernelFile(kernelFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getSha1() {
        return mKernelBuild.getSha1();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setSha1(String sha1) {
        mKernelBuild.setSha1(sha1);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getShortSha1() {
        return mKernelBuild.getShortSha1();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setShortSha1(String shortSha1) {
        mKernelBuild.setShortSha1(shortSha1);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public long getCommitTime() {
        return mKernelBuild.getCommitTime();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setCommitTime(long time) {
        mKernelBuild.getCommitTime();
    }

    /**
     * Sets the device build.
     */
    public void setDeviceBuild(IDeviceBuildInfo deviceBuild) {
        mDeviceBuild = deviceBuild;
    }

    /**
     * Sets the kernel build.
     */
    public void setKernelBuild(IKernelBuildInfo kernelBuild) {
        mKernelBuild = kernelBuild;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void cleanUp() {
        mDeviceBuild.cleanUp();
        mKernelBuild.cleanUp();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo clone() {
        KernelDeviceBuildInfo copy = new KernelDeviceBuildInfo(getBuildId(), getBuildTargetName());
        copy.addAllBuildAttributes(this);
        IDeviceBuildInfo deviceBuildClone = (IDeviceBuildInfo) mDeviceBuild.clone();
        copy.setDeviceBuild(deviceBuildClone);
        IKernelBuildInfo kernelBuildClone = (IKernelBuildInfo) mKernelBuild.clone();
        copy.setKernelBuild(kernelBuildClone);
        return copy;
    }
}
