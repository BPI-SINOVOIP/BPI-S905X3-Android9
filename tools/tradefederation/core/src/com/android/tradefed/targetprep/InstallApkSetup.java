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

package com.android.tradefed.targetprep;

import com.android.ddmlib.Log;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.AbiFormatter;
import com.google.common.annotations.VisibleForTesting;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * A {@link ITargetPreparer} that installs one or more apks located on the filesystem.
 *
 * <p>This class should only be used for installing apks from the filesystem when all versions of
 * the test rely on the apk being on the filesystem. For tests which use {@link TestAppInstallSetup}
 * to install apks from the tests zip file, use {@code --alt-dir} to specify an alternate directory
 * on the filesystem containing the apk for other test configurations (for example, local runs where
 * the tests zip file is not present).
 */
@OptionClass(alias = "install-apk")
public class InstallApkSetup extends BaseTargetPreparer {

    private static final String LOG_TAG = InstallApkSetup.class.getSimpleName();

    @Option(name = "apk-path", description =
            "the filesystem path of the apk to install. Can be repeated.",
            importance = Importance.IF_UNSET)
    private Collection<File> mApkPaths = new ArrayList<File>();

    @Option(name = AbiFormatter.FORCE_ABI_STRING,
            description = AbiFormatter.FORCE_ABI_DESCRIPTION,
            importance = Importance.IF_UNSET)
    private String mForceAbi = null;

    @Option(name = "install-arg",
            description = "Additional arguments to be passed to install command, "
                    + "including leading dash, e.g. \"-d\"")
    private Collection<String> mInstallArgs = new ArrayList<>();

    @Option(name = "post-install-cmd", description =
            "optional post-install adb shell commands; can be repeated.")
    private List<String> mPostInstallCmds = new ArrayList<>();

    @Option(name = "post-install-cmd-timeout", description =
            "max time allowed in ms for a post-install adb shell command." +
                    "DeviceUnresponsiveException will be thrown if it is timed out.")
    private long mPostInstallCmdTimeout = 2 * 60 * 1000;  // default to 2 minutes

    @Option(name = "throw-if-install-fail", description =
            "Throw exception if the APK installation failed due to any reason.")
    private boolean mThrowIfInstallFail = false;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        for (File apk : mApkPaths) {
            if (!apk.exists()) {
                throw new TargetSetupError(String.format("%s does not exist",
                        apk.getAbsolutePath()), device.getDeviceDescriptor());
            }
            Log.i(LOG_TAG, String.format("Installing %s on %s", apk.getName(),
                    device.getSerialNumber()));
            if (mForceAbi != null) {
                String abi = AbiFormatter.getDefaultAbi(device, mForceAbi);
                if (abi != null) {
                    mInstallArgs.add(String.format("--abi %s", abi));
                }
            }
            String result = device.installPackage(apk, true, mInstallArgs.toArray(new String[]{}));
            if (result != null) {
                if (mThrowIfInstallFail) {
                    throw new TargetSetupError(String.format(
                            "Stopping test: failed to install %s on device %s. Reason: %s",
                            apk.getAbsolutePath(), device.getSerialNumber(), result),
                            device.getDeviceDescriptor());
                }
                Log.e(LOG_TAG, String.format("Failed to install %s on device %s. Reason: %s",
                        apk.getAbsolutePath(), device.getSerialNumber(), result));
            }
        }

        if (mPostInstallCmds != null && !mPostInstallCmds.isEmpty()) {
            for (String cmd : mPostInstallCmds) {
                // If the command had any output, the executeShellCommand method will log it at the
                // VERBOSE level; so no need to do any logging from here.
                CLog.d("About to run setup command on device %s: %s",
                        device.getSerialNumber(), cmd);
                device.executeShellCommand(cmd, new CollectingOutputReceiver(),
                        mPostInstallCmdTimeout, TimeUnit.MILLISECONDS, 1);
            }
        }
    }

    protected Collection<File> getApkPaths() {
        return mApkPaths;
    }

    /**
     * Sets APK paths. Exposed for testing.
     */
    @VisibleForTesting
    public void setApkPaths(Collection<File> paths) {
        mApkPaths = paths;
    }

    /**
     * Set throw if install fail. Exposed for testing.
     */
    @VisibleForTesting
    public void setThrowIfInstallFail(boolean throwIfInstallFail) {
        mThrowIfInstallFail = throwIfInstallFail;
    }
}
