/*
 * Copyright (C) 2014 The Android Open Source Project
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
import com.android.tradefed.build.ISdkBuildInfo;
import com.android.tradefed.build.SdkBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import java.io.File;

/**
 * A {@link ITargetPreparer} that will create an avd and launch an emulator
 */
@OptionClass(alias = "local-sdk-preparer")
public class LocalSdkAvdPreparer extends SdkAvdPreparer {

    @Option(name = "local-sdk-path", description =
            "the local filesystem path to sdk root directory to test.")
    private File mLocalSdkPath = null;

    @Option(name = "new-emulator", description = "launch a new emulator.")
    private boolean mNewEmulator = false;

    private ISdkBuildInfo mSdkBuildInfo = new SdkBuildInfo();

    /**
     * Creates a {@link LocalSdkAvdPreparer}.
     */
    public LocalSdkAvdPreparer() {
        super();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            DeviceNotAvailableException, BuildError {
        if (isDisabled() || !mNewEmulator) {
            // Note: If we want to launch the emulator, we need to pass the --new-emulator flag
            // defined in DeviceSelectionOptions, which will create a stub emulator.
            return;
        }
        mSdkBuildInfo.setSdkDir(mLocalSdkPath);
        mSdkBuildInfo.setTestsDir(null);
        if (mLocalSdkPath == null) {
            throw new TargetSetupError("Please set the path of the sdk using --local-sdk-path.",
                    device.getDeviceDescriptor());
        }
        launchEmulatorForAvd(mSdkBuildInfo, device, createAvd(mSdkBuildInfo));
    }
}
