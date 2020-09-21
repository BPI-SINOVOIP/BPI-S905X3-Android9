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

import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.NullUtil;

import java.io.File;
import java.util.List;

/**
 * A {@link IDeviceBuildInfo} that also contains a {@link IAppBuildInfo}.
 */
public class AppDeviceBuildInfo extends BuildInfo implements IDeviceBuildInfo, IAppBuildInfo {

    private static final long serialVersionUID = BuildSerializedVersion.VERSION;
    private IDeviceBuildInfo mDeviceBuild;
    private IAppBuildInfo mAppBuildInfo;

    /**
     * @see DeviceBuildInfo#DeviceBuildInfo(String, String, String)
     */
    /*public AppDeviceBuildInfo(String buildId, String testTarget, String buildName) {
        super(buildId, testTarget, buildName);
    }*/

    /**
     * @see DeviceBuildInfo#DeviceBuildInfo(String, String)
     */
    public AppDeviceBuildInfo(String buildId, String buildName) {
        super(buildId, buildName);
    }

    /**
     * @return the {@link IDeviceBuildInfo} for the device.
     */
    public IDeviceBuildInfo getDeviceBuildInfo() {
        return mDeviceBuild;
    }

    /**
     * @return the {@link IAppBuildInfo} for the application.
     */
    public IAppBuildInfo getAppBuildInfo() {
        return mAppBuildInfo;
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
     * @param deviceBuild
     */
    public void setDeviceBuild(IDeviceBuildInfo deviceBuild) {
        mDeviceBuild = deviceBuild;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAppPackageFile(File appPackageFile, String version) {
        mAppBuildInfo.addAppPackageFile(appPackageFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<VersionedFile> getAppPackageFiles() {
        return mAppBuildInfo.getAppPackageFiles();
    }

    /**
     * @param appBuild
     */
    public void setAppBuild(IAppBuildInfo appBuild) {
        mAppBuildInfo = appBuild;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void cleanUp() {
        if (mDeviceBuild != null) {
            mDeviceBuild.cleanUp();
        }
        if (mAppBuildInfo != null) {
            mAppBuildInfo.cleanUp();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getFile(String name) {
        File localRecord = super.getFile(name);
        File deviceFileRecord = mDeviceBuild.getFile(name);
        File appFileRecord = mAppBuildInfo.getFile(name);

        if (NullUtil.countNonNulls(localRecord, deviceFileRecord, appFileRecord) > 1) {
            CLog.e("Found duplicate records while fetching file with name \"%s\"", name);
            return null;
        } else if (localRecord != null) {
            return localRecord;
        } else if (deviceFileRecord != null) {
            return deviceFileRecord;
        } else {
            return appFileRecord;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getVersion(String name) {
        String localFileVersion = super.getVersion(name);
        String deviceFileVersion = mDeviceBuild.getVersion(name);
        String appFileVersion = mAppBuildInfo.getVersion(name);

        if (NullUtil.countNonNulls(localFileVersion, deviceFileVersion, appFileVersion) > 1) {
            CLog.e("Found duplicate records while fetching file version for \"%s\"", name);
            return null;
        } else if (localFileVersion != null) {
            return localFileVersion;
        } else if (deviceFileVersion != null) {
            return deviceFileVersion;
        } else {
            return appFileVersion;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo clone() {
        AppDeviceBuildInfo copy = new AppDeviceBuildInfo(getBuildId(), getBuildTargetName());
        copy.addAllBuildAttributes(this);
        IDeviceBuildInfo deviceBuildClone = (IDeviceBuildInfo) mDeviceBuild.clone();
        copy.setDeviceBuild(deviceBuildClone);
        IAppBuildInfo appBuildClone = (IAppBuildInfo) mAppBuildInfo.clone();
        copy.setAppBuild(appBuildClone);
        return copy;
    }
}
