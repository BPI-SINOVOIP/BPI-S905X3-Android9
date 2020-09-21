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

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.ITestDevice.RecoveryMode;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.FileUtil;

/**
 * A {@link ITargetPreparer} that installs one or more apps from a {@link
 * IDeviceBuildInfo#getTestsDir()} folder onto the /system partition on device.
 *
 * <p>Requires adb root
 */
@OptionClass(alias = "tests-system-app")
public class TestSystemAppInstallSetup extends BaseTargetPreparer {

    @Option(name = "system-file-name", description =
        "the name of a test zip file to install on device system partition. Can be repeated.",
        importance = Importance.IF_UNSET)
    private Collection<String> mTestFileNames = new ArrayList<String>();

    /**
     * Adds a file to the list of apks to install
     *
     * @param fileName
     */
    public void addTestFileName(String fileName) {
        mTestFileNames.add(fileName);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            DeviceNotAvailableException {
        if (!(buildInfo instanceof IDeviceBuildInfo)) {
            throw new IllegalArgumentException(String.format("Provided buildInfo is not a %s",
                    IDeviceBuildInfo.class.getCanonicalName()));
        }
        if (mTestFileNames.size() == 0) {
            CLog.i("No test apps to install, skipping");
            return;
        }
        File testsDir = ((IDeviceBuildInfo)buildInfo).getTestsDir();
        if (testsDir == null || !testsDir.exists()) {
            throw new TargetSetupError("Provided buildInfo does not contain a valid tests "
                    + "directory", device.getDeviceDescriptor());
        }
        device.remountSystemWritable();
        device.setRecoveryMode(RecoveryMode.ONLINE);
        device.executeShellCommand("stop");

        for (String testAppName : mTestFileNames) {
            File testAppFile = FileUtil.getFileForPath(testsDir, "DATA", "app", testAppName);
            if (!testAppFile.exists()) {
                throw new TargetSetupError(String.format("Could not find test app %s directory in "
                        + "extracted tests.zip", testAppFile), device.getDeviceDescriptor());
            }
            device.pushFile(testAppFile,  String.format("/system/app/%s", testAppName));
        }
        device.setRecoveryMode(RecoveryMode.AVAILABLE);
        device.executeShellCommand("start");
        device.waitForDeviceAvailable();
    }
}
