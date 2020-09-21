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

package com.android.framework.tests;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.BaseTargetPreparer;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.FileUtil;

import java.io.File;

/**
 * A ITargetPreparer that forwards the location of the tests/data/app dir from the {@link
 * IDeviceBuildInfo} as an {@link Option}.
 */
public class TestDirForwarder extends BaseTargetPreparer implements IConfigurationReceiver {

    private IConfiguration mConfig = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        if (!(buildInfo instanceof IDeviceBuildInfo)) {
            throw new IllegalArgumentException("buildInfo is not a IDeviceBuildInfo");
        }
        if (mConfig == null) {
            // should never happen
            throw new IllegalArgumentException("missing config");
        }
        IDeviceBuildInfo deviceBuild = (IDeviceBuildInfo)buildInfo;
        File testAppDir = FileUtil.getFileForPath(deviceBuild.getTestsDir(), "DATA", "app");
        // option with name test-app-path must exist!
        try {
            mConfig.injectOptionValue("test-app-path", testAppDir.getAbsolutePath());
        } catch (ConfigurationException e) {
            throw new TargetSetupError("Mis-configuration: no object which consumes test-app-path "
                    + "option", e, device.getDeviceDescriptor());
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setConfiguration(IConfiguration config) {
        mConfig = config;
    }
}
