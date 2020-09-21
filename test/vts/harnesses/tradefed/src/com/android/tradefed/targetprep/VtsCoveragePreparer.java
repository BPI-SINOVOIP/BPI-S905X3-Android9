/*
 * Copyright (C) 2017 The Android Open Source Project
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

import com.android.annotations.VisibleForTesting;
import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.VtsVendorConfigFileUtil;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
/**
 * Preparer class for sanitizer and gcov coverage.
 *
 * <p>For devices instrumented with sanitizer coverage, this preparer fetches the unstripped
 * binaries and copies them to a temporary directory whose path is passed down with the test
 * IBuildInfo object. For gcov-instrumented devices, the zip containing gcno files is retrieved
 * along with the build info artifact.
 */
@OptionClass(alias = "vts-coverage-preparer")
public class VtsCoveragePreparer implements ITargetPreparer, ITargetCleaner {
    static final long BASE_TIMEOUT = 1000 * 60 * 20; // timeout for fetching artifacts
    static final String BUILD_INFO_ARTIFACT = "BUILD_INFO"; // name of build info artifact
    static final String GCOV_PROPERTY = "ro.vts.coverage"; // indicates gcov when val is 1
    static final String GCOV_ARTIFACT = "%s-coverage-%s.zip"; // gcov coverage artifact
    static final String GCOV_FILE_NAME = "gcov.zip"; // gcov zip file to pass to VTS

    static final String SELINUX_DISABLED = "Disabled"; // selinux disabled
    static final String SELINUX_ENFORCING = "Enforcing"; // selinux enforcing mode
    static final String SELINUX_PERMISSIVE = "Permissive"; // selinux permissive mode

    static final String SYMBOLS_ARTIFACT = "%s-symbols-%s.zip"; // symbolized binary zip
    static final String SYMBOLS_FILE_NAME = "symbols.zip"; // sancov zip to pass to VTS
    static final String SANCOV_FLAVOR = "_asan_coverage"; // sancov device build flavor

    // Path to store coverage data on the target.
    static final String COVERAGE_DATA_PATH = "/data/misc/trace/";
    // Path to store coverage report.
    static final String COVERAGE_REPORT_PATH = "coverage_report_path";

    // Build key for gcov resources
    static final String GCOV_RESOURCES_KEY = "gcov-resources-path-%s";

    // Buid key for sancov resources
    static final String SANCOV_RESOURCES_KEY = "sancov-resources-path-%s";

    // Relative path to coverage configure binary in VTS package
    static final String COVERAGE_CONFIGURE_SRC = "DATA/bin/vts_coverage_configure";

    // Target path for coverage configure binary, will be removed in teardown
    static final String COVERAGE_CONFIGURE_DST = "/data/local/tmp/vts_coverage_configure";

    // Default path to store coverage reports.
    static final String DEFAULT_COVRAGE_REPORT_PATH = "/tmp/vts-coverage-report/";

    // Default path to store coverage resource files locally.
    static final String DEFAULT_LOCAL_COVERAGE_RESOURCE_PATH = "/tmp/coverage/";

    private File mDeviceInfoPath = null; // host path where coverage device artifacts are stored
    private String mEnforcingState = null; // start state for selinux enforcement
    private IRunUtil mRunUtil = null;

    @Option(name = "use-local-artifects", description = "Whether to use local artifacts.")
    private boolean mUseLocalArtifects = false;

    @Option(name = "local-coverage-resource-path",
            description = "Path to look for local artifacts.")
    private String mLocalCoverageResourcePath = DEFAULT_LOCAL_COVERAGE_RESOURCE_PATH;

    @Option(name = "coverage-report-dir", description = "Local directory to store coverage report.")
    private String mCoverageReportDir = null;

    /** {@inheritDoc} */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws DeviceNotAvailableException, TargetSetupError {
        String flavor = device.getBuildFlavor();
        String buildId = device.getBuildId();

        if (buildId == null) {
            CLog.w("Missing build ID. Coverage disabled.");
            return;
        }

        boolean sancovEnabled = (flavor != null) && flavor.contains(SANCOV_FLAVOR);

        String coverageProperty = device.getProperty(GCOV_PROPERTY);
        boolean gcovEnabled = (coverageProperty != null) && coverageProperty.equals("1");

        if (!sancovEnabled && !gcovEnabled) {
            CLog.i("Coverage disabled.");
            return;
        }
        if (sancovEnabled) {
            CLog.i("Sanitizer coverage processing enabled.");
        }
        if (gcovEnabled) {
            CLog.i("Gcov coverage processing enabled.");
        }

        if (mRunUtil == null) {
            mRunUtil = new RunUtil();
        }

        CompatibilityBuildHelper buildHelper = createBuildHelper(buildInfo);
        if (!mUseLocalArtifects) {
            // Load the vendor configuration
            String artifactFetcher = getArtifactFetcher(buildInfo);
            if (artifactFetcher == null) {
                throw new TargetSetupError(
                        "Vendor configuration with build_artifact_fetcher required.");
            }

            try {
                // Create a temporary coverage directory
                mDeviceInfoPath = createTempDir(device);
            } catch (IOException e) {
                cleanupCoverageData(device);
                throw new TargetSetupError(
                        "Failed to create temp dir to store coverage resource files.");
            }

            if (sancovEnabled) {
                // Fetch the symbolized binaries
                String artifactName = String.format(
                        SYMBOLS_ARTIFACT, flavor.substring(0, flavor.lastIndexOf("-")), buildId);
                File artifactFile = new File(mDeviceInfoPath, SYMBOLS_FILE_NAME);

                String cmdString = String.format(artifactFetcher, buildId, flavor, artifactName,
                        artifactFile.getAbsolutePath().toString());
                String[] cmd = cmdString.split("\\s+");
                CommandResult commandResult = mRunUtil.runTimedCmd(BASE_TIMEOUT, cmd);
                if (commandResult == null || commandResult.getStatus() != CommandStatus.SUCCESS
                        || !artifactFile.exists()) {
                    cleanupCoverageData(device);
                    throw new TargetSetupError("Could not fetch unstripped binaries.");
                }
            }

            if (gcovEnabled) {
                // Fetch the symbolized binaries
                String artifactName = String.format(
                        GCOV_ARTIFACT, flavor.substring(0, flavor.lastIndexOf("-")), buildId);
                File artifactFile = new File(mDeviceInfoPath, GCOV_FILE_NAME);

                String cmdString = String.format(artifactFetcher, buildId, flavor, artifactName,
                        artifactFile.getAbsolutePath().toString());
                String[] cmd = cmdString.split("\\s+");
                CommandResult commandResult = mRunUtil.runTimedCmd(BASE_TIMEOUT, cmd);
                if (commandResult == null || commandResult.getStatus() != CommandStatus.SUCCESS
                        || !artifactFile.exists()) {
                    cleanupCoverageData(device);
                    throw new TargetSetupError("Could not fetch gcov build artifacts.");
                }
            }

            // Fetch the device build information file
            String cmdString = String.format(artifactFetcher, buildId, flavor, BUILD_INFO_ARTIFACT,
                    mDeviceInfoPath.getAbsolutePath().toString());
            String[] cmd = cmdString.split("\\s+");
            CommandResult commandResult = mRunUtil.runTimedCmd(BASE_TIMEOUT, cmd);
            File artifactFile = new File(mDeviceInfoPath, BUILD_INFO_ARTIFACT);
            if (commandResult == null || commandResult.getStatus() != CommandStatus.SUCCESS
                    || !artifactFile.exists()) {
                cleanupCoverageData(device);
                throw new TargetSetupError("Could not fetch build info.");
            }
        } else {
            mDeviceInfoPath = new File(mLocalCoverageResourcePath);
            String fileName = sancovEnabled ? SYMBOLS_FILE_NAME : GCOV_FILE_NAME;
            File artifactFile = new File(mDeviceInfoPath, fileName);
            if (!artifactFile.exists()) {
                cleanupCoverageData(device);
                throw new TargetSetupError(String.format("Could not find %s under %s.",
                        GCOV_FILE_NAME, mDeviceInfoPath.getAbsolutePath()));
            }
            File buildInfoArtifact = new File(mDeviceInfoPath, BUILD_INFO_ARTIFACT);
            if (!buildInfoArtifact.exists()) {
                cleanupCoverageData(device);
                throw new TargetSetupError(String.format("Could not find %s under %s.",
                        BUILD_INFO_ARTIFACT, mDeviceInfoPath.getAbsolutePath()));
            }
        }

        try {
            // Push the coverage configure tool
            File configureSrc = new File(buildHelper.getTestsDir(), COVERAGE_CONFIGURE_SRC);
            device.pushFile(configureSrc, COVERAGE_CONFIGURE_DST);
        } catch (FileNotFoundException e) {
            cleanupCoverageData(device);
            throw new TargetSetupError("Failed to push the vts coverage configure tool.");
        }

        if (mCoverageReportDir != null) {
            try {
                File resultDir = buildHelper.getResultDir();
                File coverageDir = new File(resultDir, mCoverageReportDir);
                buildInfo.addBuildAttribute(COVERAGE_REPORT_PATH, coverageDir.getAbsolutePath());
            } catch (FileNotFoundException e) {
                cleanupCoverageData(device);
                throw new TargetSetupError("Failed to get coverageDir.");
            }
        }

        device.executeShellCommand(String.format("rm -rf %s/*", COVERAGE_DATA_PATH));
        mEnforcingState = device.executeShellCommand("getenforce");
        if (!mEnforcingState.equals(SELINUX_DISABLED)
                && !mEnforcingState.equals(SELINUX_PERMISSIVE)) {
            device.executeShellCommand("setenforce " + SELINUX_PERMISSIVE);
        }

        if (sancovEnabled) {
            buildInfo.setFile(
                    getSancovResourceDirKey(device), mDeviceInfoPath, buildInfo.getBuildId());
        }

        if (gcovEnabled) {
            buildInfo.setFile(
                    getGcovResourceDirKey(device), mDeviceInfoPath, buildInfo.getBuildId());
        }
    }

    /** {@inheritDoc} */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (mEnforcingState != null && !mEnforcingState.equals(SELINUX_DISABLED)) {
            device.executeShellCommand("setenforce " + mEnforcingState);
        }
        cleanupCoverageData(device);
    }

    /**
     * Get the key of the symbolized binary directory for the specified device.
     *
     * @param device the target device whose sancov resources directory to get.
     * @return the (String) key name of the device's sancov resources directory.
     */
    public static String getSancovResourceDirKey(ITestDevice device) {
        return String.format(SANCOV_RESOURCES_KEY, device.getSerialNumber());
    }

    /**
     * Get the key of the gcov resources for the specified device.
     *
     * @param device the target device whose sancov resources directory to get.
     * @return the (String) key name of the device's gcov resources directory.
     */
    public static String getGcovResourceDirKey(ITestDevice device) {
        return String.format(GCOV_RESOURCES_KEY, device.getSerialNumber());
    }

    /**
     * Cleanup the coverage data on target and host.
     *
     * @param device the target device.
     */
    private void cleanupCoverageData(ITestDevice device) throws DeviceNotAvailableException {
        if (mDeviceInfoPath != null) {
            FileUtil.recursiveDelete(mDeviceInfoPath);
        }
        device.executeShellCommand("rm -rf " + COVERAGE_CONFIGURE_DST);
        device.executeShellCommand(String.format("rm -rf %s/*", COVERAGE_DATA_PATH));
    }

    @VisibleForTesting
    File createTempDir(ITestDevice device) throws IOException {
        return FileUtil.createTempDir(device.getSerialNumber());
    }

    @VisibleForTesting
    String getArtifactFetcher(IBuildInfo buildInfo) {
        VtsVendorConfigFileUtil configFileUtil = new VtsVendorConfigFileUtil();
        if (configFileUtil.LoadVendorConfig(buildInfo)) {
            return configFileUtil.GetVendorConfigVariable("build_artifact_fetcher");
        }
        return null;
    }

    /**
     * Create and return a {@link CompatibilityBuildHelper} to use during the preparer.
     */
    @VisibleForTesting
    CompatibilityBuildHelper createBuildHelper(IBuildInfo buildInfo) {
        return new CompatibilityBuildHelper(buildInfo);
    }
}
