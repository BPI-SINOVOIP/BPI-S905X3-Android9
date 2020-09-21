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

import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.FlashingResourcesParser;
import com.android.tradefed.targetprep.IFlashingResourcesParser;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.SystemUtil;
import com.android.tradefed.util.ZipUtil;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.io.PatternFilenameFilter;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * A {@link IBuildProvider} that constructs a {@link IDeviceBuildInfo} based on a provided
 * filesystem directory path.
 * <p/>
 * Specific device build files are recognized based on a configurable set of patterns.
 */
@OptionClass(alias = "local-device-build")
public class LocalDeviceBuildProvider extends StubBuildProvider {

    private static final String BUILD_INFO_FILE = "android-info.txt";
    private static final String BUILD_DIR_OPTION_NAME = "build-dir";

    private String mBootloaderVersion = null;
    private String mRadioVersion = null;

    @Option(name = BUILD_DIR_OPTION_NAME, description = "the directory containing device files.")
    private File mBuildDir = SystemUtil.getProductOutputDir();

    @Option(name = "device-img-pattern", description =
            "the regex use to find device system image zip file within --build-dir.")
    private String mImgPattern = ".*-img-.*\\.zip";

    @Option(name = "test-dir", description = "the directory containing test artifacts.")
    private File mTestDir = null;

    @Option(name = "test-dir-pattern", description =
            "the regex use to find optional test artifact directory within --build-dir. " +
            "Will be ignored if a test-dir is set.")
    private String mTestDirPattern = ".*-tests-.*";

    @Option(name = "bootloader-pattern", description =
            "the regex use to find device bootloader image file within --build-dir.")
    private String mBootloaderPattern = "bootloader.*\\.img";

    @Option(name = "radio-pattern", description =
            "the regex use to find device radio image file within --build-dir.")
    private String mRadioPattern = "radio.*\\.img";

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo getBuild() throws BuildRetrievalError {
        if (mBuildDir == null) {
            throw new BuildRetrievalError(
                    "Product output directory is not specified. If running "
                            + "from a Android source tree, make sure `lunch` has been run; "
                            + "if outside, provide a valid path via --"
                            + BUILD_DIR_OPTION_NAME);
        }
        if (!mBuildDir.exists()) {
            throw new BuildRetrievalError(String.format("Directory '%s' does not exist. " +
                    "Please provide a valid path via --%s", mBuildDir.getAbsolutePath(),
                    BUILD_DIR_OPTION_NAME));
        }
        if (!mBuildDir.isDirectory()) {
            throw new BuildRetrievalError(String.format("Path '%s' is not a directory. " +
                    "Please provide a valid path via --%s", mBuildDir.getAbsolutePath(),
                    BUILD_DIR_OPTION_NAME));
        }
        CLog.d("Using device build files from %s", mBuildDir.getAbsolutePath());

        BuildInfo stubBuild = (BuildInfo)super.getBuild();
        DeviceBuildInfo buildInfo = new DeviceBuildInfo(stubBuild.getBuildId(),
                stubBuild.getBuildTargetName());
        buildInfo.addAllBuildAttributes(stubBuild);

        setDeviceImageFile(buildInfo);
        parseBootloaderAndRadioVersions(buildInfo);
        setTestsDir(buildInfo);
        setBootloaderImage(buildInfo);
        setRadioImage(buildInfo);

        return buildInfo;
    }

    /**
     * Parse bootloader and radio versions from the android build info file.
     *
     * @param buildInfo a {@link DeviceBuildInfo}
     * @throws BuildRetrievalError
     */
    void parseBootloaderAndRadioVersions(DeviceBuildInfo buildInfo) throws BuildRetrievalError {
        try {
            IFlashingResourcesParser flashingResourcesParser;
            flashingResourcesParser = new FlashingResourcesParser(
                    buildInfo.getDeviceImageFile());
            mBootloaderVersion = flashingResourcesParser.getRequiredBootloaderVersion();
            mRadioVersion = flashingResourcesParser.getRequiredBasebandVersion();
        } catch (TargetSetupError e) {
            throw new BuildRetrievalError("Unable parse bootloader and radio versions", e);
        }
    }

    /**
     * Find and and set matching device image file to the build.
     *
     * @param buildInfo a {@link DeviceBuildInfo} to set the device image file
     * @throws BuildRetrievalError
     */
    @VisibleForTesting
    void setDeviceImageFile(DeviceBuildInfo buildInfo) throws BuildRetrievalError {
        File deviceImgFile = findFileInDir(mImgPattern);
        if (deviceImgFile == null) {
            CLog.i("Unable to find build image zip on %s", mBuildDir.getAbsolutePath());
            deviceImgFile = createBuildImageZip();
            if (deviceImgFile == null) {
                throw new BuildRetrievalError(String.format(
                        "Could not find device image file matching matching '%s' in '%s'.",
                        mImgPattern, mBuildDir.getAbsolutePath()));
            }
        }
        CLog.i("Set build image zip to %s", deviceImgFile.getAbsolutePath());
        buildInfo.setDeviceImageFile(deviceImgFile, buildInfo.getBuildId());
    }

    /**
     * Creates a build image zip file from the given build-dir
     *
     * @return the {@link File} referencing the zip output.
     * @throws BuildRetrievalError
     */
    @VisibleForTesting
    File createBuildImageZip() throws BuildRetrievalError {
        File zipFile = null;
        File[] imageFiles = mBuildDir.listFiles(new PatternFilenameFilter(".*\\.img"));
        File buildInfo = findFileInDir(BUILD_INFO_FILE);
        List<File> buildFiles = new ArrayList<>(Arrays.asList(imageFiles));
        buildFiles.add(buildInfo);
        try {
            zipFile = ZipUtil.createZip(buildFiles);
        } catch (IOException e) {
            throw new BuildRetrievalError("Unable to create build image zip file", e);
        }
        CLog.i("Created build image zip on: %s", zipFile.getAbsolutePath());
        return zipFile;
    }

    void setRadioImage(DeviceBuildInfo buildInfo) throws BuildRetrievalError {
        File radioImgFile = findFileInDir(mRadioPattern);
        if (radioImgFile != null) {
            buildInfo.setBasebandImage(radioImgFile, mRadioVersion);
        }
    }

    void setBootloaderImage(DeviceBuildInfo buildInfo) throws BuildRetrievalError {
        File bootloaderImgFile = findFileInDir(mBootloaderPattern);
        if (bootloaderImgFile != null) {
            buildInfo.setBootloaderImageFile(bootloaderImgFile, mBootloaderVersion);
        }
    }

    /**
     * Find and set a test directory to the build.
     *
     * @param buildInfo a {@link DeviceBuildInfo} to set the test directory
     * @throws BuildRetrievalError
     */
    @VisibleForTesting
    void setTestsDir(DeviceBuildInfo buildInfo) throws BuildRetrievalError {
        File testsDir = null;
        // If test-dir is specified, use it
        if (mTestDir != null) {
            CLog.i("Looking for tests on %s", mTestDir.getAbsolutePath());
            testsDir = mTestDir;
        } else {
            CLog.i("Looking for tests on %s matching %s", mBuildDir.getAbsolutePath(),
                    mTestDirPattern);
            testsDir = findFileInDir(mTestDirPattern);
        }
        if (testsDir != null) {
            buildInfo.setTestsDir(testsDir, buildInfo.getBuildId());
            CLog.d("Using test files from %s", testsDir.getAbsolutePath());
        }
    }

    /**
     * Find a matching file in the build directory.
     *
     * @param regex Regular expression to match a file
     * @return A matching {@link File} or null if none is found
     * @throws BuildRetrievalError
     */
    @VisibleForTesting
    File findFileInDir(String regex) throws BuildRetrievalError {
        return findFileInDir(regex, mBuildDir);
    }

    /**
     * Find a matching file in a given directory.
     *
     * @param regex Regular expression to match a file
     * @param dir a {@link File} referencing the directory to search
     * @return A matching {@link File} or null if none is found
     * @throws BuildRetrievalError
     */
    @VisibleForTesting
    File findFileInDir(String regex, File dir) throws BuildRetrievalError {
        File[] files = dir.listFiles(new PatternFilenameFilter(regex));
        if (files.length == 0) {
            return null;
        } else if (files.length > 1) {
            throw new BuildRetrievalError(String.format(
                    "Found more than one file matching '%s' in '%s'.", regex,
                    mBuildDir.getAbsolutePath()));
        }
        return files[0];
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void buildNotTested(IBuildInfo info) {
        // ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void cleanUp(IBuildInfo info) {
        // ignore
    }

    String getBootloaderVersion() {
        return mBootloaderVersion;
    }

    void setBootloaderVersion(String bootloaderVersion) {
        mBootloaderVersion = bootloaderVersion;
    }

    String getRadioVersion() {
        return mRadioVersion;
    }

    void setRadioVersion(String radioVersion) {
        mRadioVersion = radioVersion;
    }

    File getBuildDir() {
        return mBuildDir;
    }

    void setBuildDir(File buildDir) {
        this.mBuildDir = buildDir;
    }

    /** Returns the directory where the tests are located. */
    public File getTestDir() {
        return mTestDir;
    }

    void setTestDir(File testDir) {
        this.mTestDir = testDir;
    }

    String getTestDirPattern() {
        return mTestDirPattern;
    }

    void setTestDirPattern(String testDirPattern) {
        this.mTestDirPattern = testDirPattern;
    }
}
