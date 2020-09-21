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

import java.io.File;

/**
 * Target preparer to write a string to a file.
 *
 * <p>If the file already exists, this will restore the old version on tear down.
 */
@OptionClass(alias = "write-file")
public class DeviceStringPusher extends BaseTargetPreparer implements ITargetCleaner {
    @Option(name = "file-path", description = "Path of file to write")
    private String mFileName;

    @Option(name = "file-content", description = "Contents to write to file")
    private String mFileContent;

    private File mOldContents;

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        if (device.doesFileExist(mFileName)) {
            mOldContents = device.pullFile(mFileName);
        }
        if (!device.pushString(mFileContent, mFileName)) {
            throw new TargetSetupError(
                    "Failed to push string to file", device.getDeviceDescriptor());
        }
    }

    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (mOldContents == null) {
            device.executeShellCommand(String.format("rm -f %s", mFileName));
        } else {
            device.pushFile(mOldContents, mFileName);
        }
    }
}
