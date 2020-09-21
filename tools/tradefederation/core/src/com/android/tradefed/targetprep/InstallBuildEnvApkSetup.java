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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;

/**
 * A {@link ITargetPreparer} that installs one or more test apks from an Android platform build env.
 *
 * <p>Uses the <code>ANDROID_PRODUCT_OUT</code> env variable to determine location for build apks.
 */
@OptionClass(alias = "install-apk-from-env")
public class InstallBuildEnvApkSetup extends BaseTargetPreparer {

    private static final String APK_SUFFIX = ".apk";

    @Option(name = "apk-name", description =
        "the file name of the apk to install. Can be repeated.",
        importance = Importance.IF_UNSET)
    private Collection<String> mApkNames = new ArrayList<String>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        File testAppDir = getDataAppDirFromBuildEnv(device);
        for (String apkName : mApkNames) {
            String apkPrimaryFileName = apkName;
            if (apkName.endsWith(APK_SUFFIX)) {
                apkPrimaryFileName = apkName.substring(0, apkName.length() - APK_SUFFIX.length());
            }
            File apk = FileUtil.getFileForPath(testAppDir, apkPrimaryFileName, apkName);
            CLog.i("Installing %s on %s", apk.getName(), device.getSerialNumber());
            String result = device.installPackage(apk, true);
            if (result != null) {
                throw new TargetSetupError(String.format(
                        "Failed to install %s on device %s. Reason: %s", apk.getAbsolutePath(),
                        device.getSerialNumber(), result), device.getDeviceDescriptor());
            }
        }
    }

    private File getDataAppDirFromBuildEnv(ITestDevice device) throws TargetSetupError {
        String buildRoot = System.getenv("ANDROID_PRODUCT_OUT");
        if (buildRoot == null) {
            throw new TargetSetupError("ANDROID_PRODUCT_OUT is not set. Are you in Android "
                    + "build env?", device.getDeviceDescriptor());
        }
        File testAppDir = new File(FileUtil.getPath(buildRoot, "data", "app"));
        if (!testAppDir.exists()) {
            throw new TargetSetupError(String.format("Could not find test app dir %s",
                    testAppDir.getAbsolutePath()),device.getDeviceDescriptor());
        }
        return testAppDir;
    }
}
