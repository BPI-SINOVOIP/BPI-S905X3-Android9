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
package com.android.tradefed.targetprep;

import com.android.tradefed.build.IAppBuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.VersionedFile;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.AaptParser;

import com.google.common.annotations.VisibleForTesting;

import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * A {@link ITargetPreparer} that installs an apk and its tests.
 *
 * <p>Requires 'aapt' on PATH when --uninstall is set
 */
@OptionClass(alias = "app-setup")
public class AppSetup extends BaseTargetPreparer implements ITargetCleaner {

    @Option(name="reboot", description="reboot device after running tests.")
    private boolean mReboot = true;

    @Option(name = "install", description = "install all apks in build.")
    private boolean mInstall = true;

    @Option(name = "uninstall", description =
            "uninstall only apks in build after test completes.")
    private boolean mUninstall = true;

    @Option(name = "uninstall-all", description =
            "uninstall all unnstallable apks found on device after test completes.")
    private boolean mUninstallAll = false;

    @Option(name = "skip-uninstall-pkg", description =
            "force retention of this package when --uninstall-all is set.")
    private Set<String> mSkipUninstallPkgs = new HashSet<String>();

    @Option(name = "install-flag", description =
            "optional flag(s) to provide when installing apks.")
    private ArrayList<String> mInstallFlags = new ArrayList<>();

    @Option(name = "post-install-cmd", description =
            "optional post-install adb shell commands; can be repeated.")
    private List<String> mPostInstallCmds = new ArrayList<>();

    @Option(name = "post-install-cmd-timeout", description =
            "max time allowed in ms for a post-install adb shell command." +
            "DeviceUnresponsiveException will be thrown if it is timed out.")
    private long mPostInstallCmdTimeout = 2 * 60 * 1000;  // default to 2 minutes

    @Option(name = "check-min-sdk", description =
            "check app's min sdk prior to install and skip if device api level is too low.")
    private boolean mCheckMinSdk = false;

    /** contains package names of installed apps. Used for uninstall */
    private Set<String> mInstalledPkgs = new HashSet<String>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            DeviceNotAvailableException, BuildError {
        if (!(buildInfo instanceof IAppBuildInfo)) {
            throw new IllegalArgumentException("Provided buildInfo is not a AppBuildInfo");
        }
        IAppBuildInfo appBuild = (IAppBuildInfo)buildInfo;
        CLog.i("Performing setup on %s", device.getSerialNumber());

        // double check that device is clean, in case it has unexpected cruft on it
        if (mUninstallAll && !uninstallAllApps(device)) {
            // cannot cleanup device! Bad things may happen in future tests. Take device out
            // of service
            // TODO: in future, consider doing more sophisticated recovery operations
            throw new DeviceNotAvailableException(String.format(
                    "Failed to uninstall apps on %s", device.getSerialNumber()),
                    device.getSerialNumber());
        }

        if (mInstall) {
            for (VersionedFile apkFile : appBuild.getAppPackageFiles()) {
                if (mCheckMinSdk) {
                    AaptParser aaptParser = doAaptParse(apkFile.getFile());
                    if (aaptParser == null) {
                        throw new TargetSetupError(
                                String.format("Failed to extract info from '%s' using aapt",
                                        apkFile.getFile().getName()), device.getDeviceDescriptor());
                    }
                    if (device.getApiLevel() < aaptParser.getSdkVersion()) {
                        CLog.w("Skipping installing apk %s on device %s because " +
                                "SDK level require is %d, but device SDK level is %d",
                                apkFile.toString(), device.getSerialNumber(),
                                aaptParser.getSdkVersion(), device.getApiLevel());
                        continue;
                    }
                }
                String result = device.installPackage(apkFile.getFile(), true,
                        mInstallFlags.toArray(new String[mInstallFlags.size()]));
                if (result != null) {
                    // typically install failures means something is wrong with apk.
                    // TODO: in future add more logic to throw targetsetup vs build vs
                    // devicenotavail depending on error code
                    throw new BuildError(String.format(
                            "Failed to install %s on %s. Reason: %s",
                            apkFile.getFile().getName(), device.getSerialNumber(), result),
                            device.getDeviceDescriptor());
                }
                if (mUninstall && !mUninstallAll) {
                    addPackageNameToUninstall(apkFile.getFile(), device);
                }
            }
        }

       if (!mPostInstallCmds.isEmpty()){
           for (String cmd : mPostInstallCmds) {
               // If the command had any output, the executeShellCommand method will log it at the
               // VERBOSE level; so no need to do any logging from here.
               CLog.d("About to run setup command on device %s: %s", device.getSerialNumber(), cmd);
               device.executeShellCommand(cmd, new CollectingOutputReceiver(),
                       mPostInstallCmdTimeout, TimeUnit.MILLISECONDS, 1);
           }
       }
    }

    /**
     * Helper to parse an apk file with aapt.
     */
    @VisibleForTesting
    AaptParser doAaptParse(File apkFile) {
        return AaptParser.parse(apkFile);
    }

    private void addPackageNameToUninstall(File apkFile, ITestDevice device)
            throws TargetSetupError {
        AaptParser aaptParser = doAaptParse(apkFile);
        if (aaptParser == null) {
            throw new TargetSetupError(String.format("Failed to extract info from '%s' using aapt",
                    apkFile.getAbsolutePath()), device.getDeviceDescriptor());
        }
        if (aaptParser.getPackageName() == null) {
            throw new TargetSetupError(String.format(
                    "Failed to find package name for '%s' using aapt", apkFile.getAbsolutePath()),
                    device.getDeviceDescriptor());
        }
        mInstalledPkgs.add(aaptParser.getPackageName());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (e instanceof DeviceNotAvailableException) {
            return;
        }
        // reboot device before uninstalling apps, in case device is wedged
        if (mReboot) {
            device.reboot();
        }
        if (mUninstall && !mUninstallAll) {
            for (String pkgName : mInstalledPkgs) {
                String result = device.uninstallPackage(pkgName);
                if (result != null) {
                    CLog.e("Failed to uninstall %s: %s", pkgName, result);
                    // TODO: consider throwing here
                }
            }
        }
        if (mUninstallAll && !uninstallAllApps(device)) {
            // cannot cleanup device! Bad things may happen in future tests. Take device out
            // of service
            // TODO: in future, consider doing more sophisticated recovery operations
            throw new DeviceNotAvailableException(String.format(
                    "Failed to uninstall apps on %s", device.getSerialNumber()),
                    device.getSerialNumber());
        }
    }

    /**
     * Make multiple attempts to uninstall apps, aborting if failed
     *
     * @return {@code true} if all apps were uninstalled, {@code false} otherwise.
     */
    private boolean uninstallAllApps(ITestDevice device) throws DeviceNotAvailableException {
        // TODO: consider moving this to ITestDevice, so more sophisticated recovery attempts
        // can be performed
        for (int i = 0; i < 3; i++) {
            Set<String> pkgs = getAllAppsToUninstall(device);
            if (pkgs.isEmpty()) {
                return true;
            }
            for (String pkg : pkgs) {
                String result = device.uninstallPackage(pkg);
                if (result != null) {
                    CLog.w("Uninstall of %s on %s failed: %s", pkg, device.getSerialNumber(),
                            result);
                }
            }
        }
        // check getAppsToUninstall one more time, since last attempt through loop might have been
        // successful
        return getAllAppsToUninstall(device).isEmpty();
    }

    private Set<String> getAllAppsToUninstall(ITestDevice device) throws DeviceNotAvailableException {
        Set<String> pkgs = device.getUninstallablePackageNames();
        pkgs.removeAll(mSkipUninstallPkgs);
        return pkgs;
    }
}
