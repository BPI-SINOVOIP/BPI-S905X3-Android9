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
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import com.google.common.annotations.VisibleForTesting;

import java.io.File;

/**
 * A {@link ITargetPreparer} that replaces the preloaded classes file on a device.
 *
 * <p>Note that this preparer requires a rooted, debug build to work.
 */
@OptionClass(alias = "preloaded-classes-preparer")
public class PreloadedClassesPreparer extends BaseTargetPreparer {
    // Preload tool commands
    private static final String TOOL_CMD = "java -cp %s com.android.preload.Main --seq %s %s";
    private static final String WRITE_CMD = "write %s";
    // Defaults
    public static final long DEFAULT_TIMEOUT_MS = 5 * 60 * 1000;

    @Option(
        name = "preload-file",
        description = "The new preloaded classes file to put on the device."
    )
    private File mNewClassesFile = null;

    @Option(name = "preload-tool", description = "Overridden location of the preload tool JAR.")
    private String mPreloadToolJarPath = "";

    @Option(name = "skip", description = "Skip this preparer entirely.")
    private boolean mSkip = false;

    @Option(
        name = "write-timeout",
        isTimeVal = true,
        description = "Maximum timeout for overwriting the file in milliseconds."
    )
    private long mWriteTimeout = DEFAULT_TIMEOUT_MS;

    /** {@inheritDoc} */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        if (mSkip) {
            return;
        } else if (getPreloadedClassesPath().isEmpty()) {
            CLog.w("No preloaded classes file specified. Skipping preparer.");
            return;
        }

        // Download preload tool, if not supplied
        if (getPreloadToolPath().isEmpty()) {
            File preload = buildInfo.getFile("preload2.jar");
            if (preload != null && preload.exists()) {
                setPreloadToolPath(preload.getAbsolutePath());
            } else {
                throw new TargetSetupError(
                        "Unable to find the preload tool.", device.getDeviceDescriptor());
            }
        }

        // Root, disable verity, and remount
        device.enableAdbRoot();
        device.remountSystemWritable();
        // Root again after rebooting
        device.enableAdbRoot();
        // Construct the command
        String exportCmd = String.format(WRITE_CMD, getPreloadedClassesPath());
        String[] fullCmd =
                String.format(TOOL_CMD, getPreloadToolPath(), device.getSerialNumber(), exportCmd)
                        .split(" ");
        CommandResult result = getRunUtil().runTimedCmd(mWriteTimeout, fullCmd);
        if (!CommandStatus.SUCCESS.equals(result.getStatus())) {
            throw new TargetSetupError(
                    String.format("Error writing: %s", result.getStderr()),
                    device.getDeviceDescriptor());
        }
        // Wait for the device to be reconnected
        device.waitForDeviceAvailable();
    }

    /**
     * Get the {@link IRunUtil} instance to use.
     *
     * <p>Exposed so unit tests can mock.
     */
    @VisibleForTesting
    protected IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /**
     * Get the preloaded classes file.
     *
     * <p>Exposed so unit tests can mock.
     */
    @VisibleForTesting
    protected String getPreloadedClassesPath() {
        return (mNewClassesFile != null) ? mNewClassesFile.getAbsolutePath() : "";
    }

    /** Get the preload tool path. */
    protected String getPreloadToolPath() {
        return mPreloadToolJarPath;
    }

    /** Set the preload tool path. */
    protected void setPreloadToolPath(String path) {
        mPreloadToolJarPath = path;
    }
}
