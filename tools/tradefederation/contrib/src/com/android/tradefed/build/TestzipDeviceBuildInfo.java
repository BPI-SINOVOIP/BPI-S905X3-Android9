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

package com.android.tradefed.build;

import java.io.File;

/**
 * A {@link IDeviceBuildInfo} that also contains a {@link ITestzipBuildInfo}.
 */
public class TestzipDeviceBuildInfo extends BuildInfo implements IDeviceBuildInfo, ITestzipBuildInfo {

    private static final long serialVersionUID = BuildSerializedVersion.VERSION;
    private IDeviceBuildInfo mDeviceBuild;
    private ITestzipBuildInfo mTestzipBuildInfo;


    @Override
    public File getTestzipDir() {
        return mTestzipBuildInfo.getTestzipDir();
    }

    @Override
    public void setTestzipDir(File testsZipFile, String version) {
        mTestzipBuildInfo.setTestzipDir(testsZipFile, version);
    }

    @Override
    public String getTestzipDirVersion() {
        return mTestzipBuildInfo.getTestzipDirVersion();
    }

    /**
     * @see DeviceBuildInfo#DeviceBuildInfo(String, String)
     */
    public TestzipDeviceBuildInfo(String buildId, String buildName) {
        super(buildId, buildName);
    }

    /**
     * @return the {@link IDeviceBuildInfo} for the device.
     */
    public IDeviceBuildInfo getDeviceBuildInfo() {
        return mDeviceBuild;
    }

    /**
     * @return the {@link IDeviceBuildInfo} for the device.
     */
    public void setTestzipBuild(ITestzipBuildInfo testzipBuild) {
        mTestzipBuildInfo = testzipBuild;
    }

    /**
     * @return the {@link ITestzipBuildInfo} for the application.
     */
    public ITestzipBuildInfo getTestzipBuildInfo() {
        return mTestzipBuildInfo;
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
        return mTestzipBuildInfo.getTestzipDir();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getTestsDirVersion() {
        return mTestzipBuildInfo.getTestzipDirVersion();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTestsDir(File testsDir, String version) {
        mTestzipBuildInfo.setTestzipDir(testsDir, version);
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
        if (mDeviceBuild != null) {
            mDeviceBuild.cleanUp();
        }

        if (mTestzipBuildInfo!= null) {
            mTestzipBuildInfo.cleanUp();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo clone() {
        TestzipDeviceBuildInfo copy = new TestzipDeviceBuildInfo(getBuildId(), getBuildTargetName());
        copy.addAllBuildAttributes(this);
        IDeviceBuildInfo deviceBuildClone = (IDeviceBuildInfo) mDeviceBuild.clone();
        copy.setDeviceBuild(deviceBuildClone);
        ITestzipBuildInfo testzipBuildClone = (ITestzipBuildInfo) mTestzipBuildInfo.clone();
        copy.setTestsDir(testzipBuildClone.getTestzipDir(), testzipBuildClone.getTestzipDirVersion());
        return copy;
    }

}
