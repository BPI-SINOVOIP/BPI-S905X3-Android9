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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.AaptParser;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.ZipUtil2;

import org.apache.commons.compress.archivers.zip.ZipFile;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/**
 * A {@link ITargetPreparer} that installs all apps in a test zip. For individual test app install
 * please look at {@link TestAppInstallSetup}.
 */
@OptionClass(alias = "all-tests-zip-installer")
public class InstallAllTestZipAppsSetup extends BaseTargetPreparer implements ITargetCleaner {
    @Option(
        name = "install-arg",
        description =
                "Additional arguments to be passed to install command, "
                        + "including leading dash, e.g. \"-d\""
    )
    private Collection<String> mInstallArgs = new ArrayList<>();

    @Option(
        name = "cleanup-apks",
        description =
                "Whether apks installed should be uninstalled after test. Note that the "
                        + "preparer does not verify if the apks are successfully removed."
    )
    private boolean mCleanup = true;

    @Option(
        name = "stop-install-on-failure",
        description =
                "Whether to stop the preparer by throwing an exception or only log the "
                        + "error on continue."
    )
    private boolean mStopInstallOnFailure = true;

    @Option(name = "test-zip-name", description = "File name for test zip containing APKs.")
    private String mTestZipName;

    List<String> mPackagesInstalled = new ArrayList<>();

    public void setTestZipName(String testZipName) {
        mTestZipName = testZipName;
    }

    public void setStopInstallOnFailure(boolean stopInstallOnFailure) {
        mStopInstallOnFailure = stopInstallOnFailure;
    }

    public void setCleanup(boolean cleanup) {
        mCleanup = cleanup;
    }

    /** {@inheritDoc} */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, DeviceNotAvailableException {
        if (isDisabled()) {
            CLog.d("InstallAllTestZipAppsSetup disabled, skipping setUp");
            return;
        }
        File testsZip = getZipFile(device, buildInfo);
        // Locate test dir where the test zip file was unzip to.
        if (testsZip == null) {
            throw new TargetSetupError(
                    "Failed to find a valid test zip directory.", device.getDeviceDescriptor());
        }
        File testsDir;
        try {
            testsDir = extractZip(testsZip);
        } catch (IOException e) {
            throw new TargetSetupError(
                    "Failed to extract test zip.", e, device.getDeviceDescriptor());
        }

        try {
            installApksRecursively(testsDir, device);
        } finally {
            FileUtil.recursiveDelete(testsDir);
        }
    }

    /**
     * Returns the zip file.
     *
     * @param buildInfo {@link IBuildInfo} containing files.
     * @return the {@link File} for the zip file.
     */
    File getZipFile(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError {
        if (mTestZipName == null) {
            throw new TargetSetupError("test-zip-name is null.", device.getDeviceDescriptor());
        }
        return buildInfo.getFile(mTestZipName);
    }

    /**
     * Install all apks found in a given directory.
     *
     * @param directory {@link File} directory to install from.
     * @param device {@link ITestDevice} to install all apks to.
     * @throws TargetSetupError
     * @throws DeviceNotAvailableException
     */
    void installApksRecursively(File directory, ITestDevice device)
            throws TargetSetupError, DeviceNotAvailableException {
        if (directory == null || !directory.isDirectory()) {
            throw new TargetSetupError("Invalid test directory!", device.getDeviceDescriptor());
        }
        CLog.d("Installing all apks found in dir %s ...", directory.getAbsolutePath());
        File[] files = directory.listFiles();
        for (File f : files) {
            if (f.isDirectory()) {
                installApksRecursively(f, device);
            } else if (FileUtil.getExtension(f.getAbsolutePath()).toLowerCase().equals(".apk")) {
                installApk(f, device);
            } else {
                CLog.d("Skipping %s because it is not an apk", f.getAbsolutePath());
            }
        }
    }

    /**
     * Extract the given zip file to a local dir.
     *
     * <p>Exposed so unit tests can mock
     *
     * @param testsZip
     * @return the {@link File} referencing the zip output.
     * @throws IOException
     */
    File extractZip(File testsZip) throws IOException {
        File testsDir = null;
        try (ZipFile zip = new ZipFile(testsZip)) {
            testsDir = FileUtil.createTempDir("tests-zip_");
            ZipUtil2.extractZip(zip, testsDir);
        } catch (IOException e) {
            FileUtil.recursiveDelete(testsDir);
            throw e;
        }
        return testsDir;
    }

    /**
     * Installs a single app to the device.
     *
     * @param appFile {@link File} of the apk to install.
     * @param device {@link ITestDevice} to install the apk to.
     * @throws TargetSetupError
     * @throws DeviceNotAvailableException
     */
    void installApk(File appFile, ITestDevice device)
            throws TargetSetupError, DeviceNotAvailableException {
        CLog.d("Installing apk from %s ...", appFile.getAbsolutePath());
        String result = device.installPackage(appFile, true, mInstallArgs.toArray(new String[] {}));
        if (result == null) {
            // only consider cleanup if install was successful
            if (mCleanup) {
                addApkToInstalledList(appFile, device);
            }
        } else if (mStopInstallOnFailure) {
            // if flag is true, we stop the sequence for an exception.
            throw new TargetSetupError(
                    String.format(
                            "Failed to install %s on %s. Reason: '%s'",
                            appFile, device.getSerialNumber(), result),
                    device.getDeviceDescriptor());
        } else {
            CLog.e(
                    "Failed to install %s on %s. Reason: '%s'",
                    appFile, device.getSerialNumber(), result);
        }
    }

    /**
     * Adds an app to the list of apps to uninstall in tearDown() if necessary.
     *
     * @param appFile {@link File} of apk.
     * @param device {@link ITestDevice} apk was installed on.
     * @throws TargetSetupError
     */
    void addApkToInstalledList(File appFile, ITestDevice device) throws TargetSetupError {
        String packageName = getAppPackageName(appFile);
        if (packageName == null) {
            throw new TargetSetupError(
                    "apk installed but AaptParser failed", device.getDeviceDescriptor());
        }
        mPackagesInstalled.add(packageName);
    }

    /**
     * Returns the package name for a an apk. Returns null if any errors.
     *
     * @param appFile {@link File} of apk.
     * @return Package name of appFile, null if errors.
     */
    String getAppPackageName(File appFile) {
        AaptParser parser = AaptParser.parse(appFile);
        if (parser == null) {
            return null;
        }
        return parser.getPackageName();
    }

    /** {@inheritDoc} */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (isDisabled()) {
            CLog.d("InstallAllTestZipAppsSetup disabled, skipping tearDown");
            return;
        }
        if (mCleanup && !(e instanceof DeviceNotAvailableException)) {
            for (String packageName : mPackagesInstalled) {
                String msg = device.uninstallPackage(packageName);
                if (msg != null) {
                    CLog.w(String.format("error uninstalling package '%s': %s", packageName, msg));
                }
            }
        }
    }
}
