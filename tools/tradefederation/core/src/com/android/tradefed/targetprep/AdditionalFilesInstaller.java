/*
 * Copyright (C) 2013 The Android Open Source Project
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
import com.android.tradefed.build.VersionedFile;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

import java.io.File;

/**
 * A {@link ITargetPreparer} that pushes all {@link IBuildInfo#getFiles()} to specific path on
 * device.
 */
public class AdditionalFilesInstaller extends BaseTargetPreparer implements ITargetCleaner {

    // TODO: make this an option
    private static final String DEST_PATH = "/data/local/tmp/";

    @Option(name = "uninstall", description = "remove all contents after test completes.")
    private boolean mUninstall = true;

    // TODO: add a filter options to allow selective pushing of files

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        removeFiles(device);
        for (VersionedFile buildFile : buildInfo.getFiles()) {
            File file = buildFile.getFile();
            CLog.d("Examining build file %s", file.getName());
            String remotePath = String.format("%s%s", DEST_PATH, file.getName());
            CLog.d("Pushing %s to %s", file.getName(), remotePath);
            if (!device.pushFile(file, remotePath)) {
                throw new TargetSetupError(String.format("Failed to push %s to %s",
                        file.getName(), remotePath), device.getDeviceDescriptor());
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable t)
            throws DeviceNotAvailableException {
        try {
            removeFiles(device);
        } catch (TargetSetupError e) {
            CLog.e(e);
        }

    }

    /**
     * Remove all files from dest path, if --uninstall option is set/
     *
     * @param device the {@link ITestDevice} to use
     * @throws DeviceNotAvailableException if communication was lost with device.
     * @throws TargetSetupError if failed to remove contents after 3 attempts
     */
    private void removeFiles(ITestDevice device) throws DeviceNotAvailableException,
            TargetSetupError {
        if (mUninstall) {
            for (int i=0; i < 3; i++) {
                device.executeShellCommand(String.format("rm -r %s*", DEST_PATH));
                if (!hasContents(device, DEST_PATH)) {
                    return;
                }
            }
            throw new TargetSetupError(String.format("failed to remove files from %s", DEST_PATH),
                    device.getDeviceDescriptor());
        }
    }

    /**
     * Check for presence of any files in <var>path</var> on <var>device</var>.
     *
     * @param device the {@link ITestDevice} to use
     * @param path the absolute file system path to check
     * @return true if path has contents, false otherwise
     * @throws DeviceNotAvailableException if communication was lost with device.
     */
    private boolean hasContents(ITestDevice device, String path)
            throws DeviceNotAvailableException {
        return device.executeShellCommand(String.format("ls %s", path)).trim().length() > 0;
    }
}
