/*
 * Copyright (C) 2016 The Android Open Source Project
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
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Sets up a Python virtualenv on the host and installs packages. To activate it, the working
 * directory is changed to the root of the virtualenv.
 */
@OptionClass(alias = "python-venv")
public class PythonVirtualenvPreparer extends BaseTargetPreparer {

    private static final String PIP = "pip";
    private static final String PATH = "PATH";
    protected static final String PYTHONPATH = "PYTHONPATH";
    private static final int BASE_TIMEOUT = 1000 * 60;

    @Option(name = "venv-dir", description = "path of an existing virtualenv to use")
    private File mVenvDir = null;

    @Option(name = "requirements-file", description = "pip-formatted requirements file")
    private File mRequirementsFile = null;

    @Option(name = "dep-module", description = "modules which need to be installed by pip")
    private List<String> mDepModules = new ArrayList<>();

    IRunUtil mRunUtil = new RunUtil();
    String mPip = PIP;

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        if (isDisabled()) {
            CLog.i("Skipping PythonVirtualenvPreparer");
            return;
        }
        startVirtualenv(buildInfo, device);
        installDeps(buildInfo, device);
    }

    protected void installDeps(IBuildInfo buildInfo, ITestDevice device) throws TargetSetupError {
        boolean hasDependencies = false;
        if (mRequirementsFile != null) {
            CommandResult c = mRunUtil.runTimedCmd(BASE_TIMEOUT * 5, mPip,
                    "install", "-r", mRequirementsFile.getAbsolutePath());
            if (c.getStatus() != CommandStatus.SUCCESS) {
                CLog.e("Installing dependencies from %s failed",
                        mRequirementsFile.getAbsolutePath());
                throw new TargetSetupError("Failed to install dependencies with pip",
                        device.getDeviceDescriptor());
            }
            hasDependencies = true;
        }
        if (!mDepModules.isEmpty()) {
            for (String dep : mDepModules) {
                CLog.i("Attempting installation of %s", dep);
                CommandResult c = mRunUtil.runTimedCmd(BASE_TIMEOUT * 5, mPip,
                        "install", dep);
                if (c.getStatus() != CommandStatus.SUCCESS) {
                    CLog.e("Installing %s failed", dep);
                    throw new TargetSetupError("Failed to install dependencies with pip",
                            device.getDeviceDescriptor());
                }
                hasDependencies = true;
            }
        }
        if (!hasDependencies) {
            CLog.i("No dependencies to install");
        } else {
            // make the install directory of new packages available to other classes that
            // receive the build
            buildInfo.setFile(PYTHONPATH, new File(mVenvDir,
                    "local/lib/python2.7/site-packages"),
                    buildInfo.getBuildId());
        }
    }

    protected void startVirtualenv(IBuildInfo buildInfo, ITestDevice device)
            throws TargetSetupError {
        if (mVenvDir != null) {
            CLog.i("Using existing virtualenv based at %s", mVenvDir.getAbsolutePath());
            activate();
            return;
        }
        try {
            mVenvDir = FileUtil.createNamedTempDir(buildInfo.getTestTag() + "-virtualenv");
            mRunUtil.runTimedCmd(BASE_TIMEOUT, "virtualenv", mVenvDir.getAbsolutePath());
            activate();
        } catch (IOException e) {
            CLog.e("Failed to create temp directory for virtualenv");
            throw new TargetSetupError("Error creating virtualenv", e,
                    device.getDeviceDescriptor());
        }
    }

    protected void addDepModule(String module) {
        mDepModules.add(module);
    }

    protected void setRequirementsFile(File f) {
        mRequirementsFile = f;
    }

    private void activate() {
        File binDir = new File(mVenvDir, "bin");
        mRunUtil.setWorkingDir(binDir);
        String path = System.getenv(PATH);
        mRunUtil.setEnvVariable(PATH, binDir + ":" + path);
        File pipFile = new File(binDir, PIP);
        pipFile.setExecutable(true);
        mPip = pipFile.getAbsolutePath();
    }
}