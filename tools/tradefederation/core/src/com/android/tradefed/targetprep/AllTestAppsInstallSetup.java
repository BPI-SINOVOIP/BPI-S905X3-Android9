/*
 * Copyright (C) 2015 The Android Open Source Project
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
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.util.AaptParser;
import com.android.tradefed.util.AbiFormatter;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/**
 * A {@link ITargetPreparer} that installs all apps from a {@link IDeviceBuildInfo#getTestsDir()}
 * folder onto device. For individual test app install please look at {@link TestAppInstallSetup}
 */
@OptionClass(alias = "all-tests-installer")
public class AllTestAppsInstallSetup extends BaseTargetPreparer
        implements ITargetCleaner, IAbiReceiver {
    @Option(name = AbiFormatter.FORCE_ABI_STRING,
            description = AbiFormatter.FORCE_ABI_DESCRIPTION,
            importance = Importance.IF_UNSET)
    private String mForceAbi = null;

    @Option(name = "install-arg",
            description = "Additional arguments to be passed to install command, "
                    + "including leading dash, e.g. \"-d\"")
    private Collection<String> mInstallArgs = new ArrayList<>();

    @Option(name = "cleanup-apks",
            description = "Whether apks installed should be uninstalled after test. Note that the "
                    + "preparer does not verify if the apks are successfully removed.")
    private boolean mCleanup = false;

    @Option(name = "stop-install-on-failure",
            description = "Whether to stop the preparer by throwing an exception or only log the "
                    + "error on continue.")
    private boolean mStopInstallOnFailure = true;

    private IAbi mAbi = null;

    private List<String> mPackagesInstalled = new ArrayList<>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            DeviceNotAvailableException {
        if (!(buildInfo instanceof IDeviceBuildInfo)) {
            throw new TargetSetupError("Invalid buildInfo, expecting an IDeviceBuildInfo",
                    device.getDeviceDescriptor());
        }
        // Locate test dir where the test zip file was unzip to.
        File testsDir = ((IDeviceBuildInfo) buildInfo).getTestsDir();
        if (testsDir == null || !testsDir.exists()) {
            throw new TargetSetupError("Failed to find a valid test zip directory.",
                    device.getDeviceDescriptor());
        }
        resolveAbi(device);
        installApksRecursively(testsDir, device);
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
            throw new TargetSetupError("Invalid test zip directory!", device.getDeviceDescriptor());
        }
        CLog.d("Installing all apks found in dir %s ...", directory.getAbsolutePath());
        File[] files = directory.listFiles();
        for (File f : files) {
            if (f.isDirectory()) {
                installApksRecursively(f, device);
            }
            if (FileUtil.getExtension(f.getAbsolutePath()).toLowerCase().equals(".apk")) {
                installApk(f, device);
            } else {
                CLog.d("Skipping %s because it is not an apk", f.getAbsolutePath());
            }
        }
    }

    /**
     * Installs a single app to the device.
     *
     * @param appFile {@link File} of the apk to install.
     * @param device {@link ITestDevice} to install the apk to.
     * @throws TargetSetupError
     * @throws DeviceNotAvailableException
     */
    void installApk(File appFile, ITestDevice device) throws TargetSetupError,
            DeviceNotAvailableException {
        CLog.d("Installing apk from %s ...", appFile.getAbsolutePath());
        String result = device.installPackage(appFile, true,
                mInstallArgs.toArray(new String[] {}));
        if (result == null) {
            // only consider cleanup if install was successful
            if (mCleanup) {
                AaptParser parser = AaptParser.parse(appFile);
                if (parser == null) {
                    throw new TargetSetupError("apk installed but AaptParser failed",
                            device.getDeviceDescriptor());
                }
                mPackagesInstalled.add(parser.getPackageName());
            }
        } else if (mStopInstallOnFailure) {
            // if flag is true, we stop the sequence for an exception.
            throw new TargetSetupError(String.format("Failed to install %s on %s. Reason: '%s'",
                    appFile, device.getSerialNumber(), result), device.getDeviceDescriptor());
        } else {
            CLog.e("Failed to install %s on %s. Reason: '%s'", appFile,
                    device.getSerialNumber(), result);
        }
    }

    /**
     * Determines the abi arguments when installing the apk, if needed.
     *
     * @param device {@link ITestDevice}
     * @throws DeviceNotAvailableException
     */
    void resolveAbi(ITestDevice device) throws DeviceNotAvailableException {
        if (mAbi != null && mForceAbi != null) {
            throw new IllegalStateException("cannot specify both abi flags");
        }
        String abiName = null;
        if (mAbi != null) {
            abiName = mAbi.getName();
        } else if (mForceAbi != null) {
            abiName = AbiFormatter.getDefaultAbi(device, mForceAbi);
        }
        if (abiName != null) {
            mInstallArgs.add(String.format("--abi %s", abiName));
        }
    }

    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public IAbi getAbi() {
        return mAbi;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (mCleanup && !(e instanceof DeviceNotAvailableException)) {
            for (String packageName : mPackagesInstalled) {
                String msg = device.uninstallPackage(packageName);
                if (msg != null) {
                    CLog.w(String.format("error uninstalling package '%s': %s",
                            packageName, msg));
                }
            }
        }
    }
}
