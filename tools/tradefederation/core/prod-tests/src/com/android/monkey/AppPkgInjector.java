/*
 * Copyright (C) 2012 The Android Open Source Project
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
package com.android.monkey;

import com.android.tradefed.build.IAppBuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.VersionedFile;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.BaseTargetPreparer;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.AaptParser;

import org.junit.Assert;

/**
 * A {@link ITargetPreparer} for {@link IAppBuildInfo}, that dynamically determines the app package
 * name given its apk file. This saves the user from having to manually specify the --package arg
 * when running monkey.
 *
 * <p>Requires that aapt is on current path.
 */
public class AppPkgInjector extends BaseTargetPreparer implements IConfigurationReceiver {

    @Option(name = "skip-injector", description = "Set to true if we should skip automatically " +
            "adding all package names of installed packages. Default is false.")
    private boolean mSkipInjecting = false;

    private IConfiguration mConfig;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setConfiguration(IConfiguration configuration) {
        mConfig = configuration;

    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        if (mSkipInjecting) {
            return;
        }
        Assert.assertNotNull(mConfig);
        Assert.assertTrue("provided build is not a IAppBuildInfo",
                buildInfo instanceof IAppBuildInfo);
        IAppBuildInfo appBuild = (IAppBuildInfo)buildInfo;
        for (VersionedFile apkFile : appBuild.getAppPackageFiles()) {
            AaptParser aapt = AaptParser.parse(apkFile.getFile());
            if (aapt == null) {
                // throw a build error because typically aapt parse errors are issues with the apk
                throw new BuildError(String.format("aapt parse of %s failed",
                        apkFile.getFile().getAbsolutePath()), device.getDeviceDescriptor());
            }
            String pkgName = aapt.getPackageName();
            if (pkgName == null) {
                // this should never happen
                throw new TargetSetupError(String.format("Failed to parse package name from %s",
                        apkFile.getFile().getAbsolutePath()), device.getDeviceDescriptor());
            }
            try {
                mConfig.injectOptionValue("package", pkgName);
            } catch (ConfigurationException e) {
                throw new TargetSetupError("Failed to inject --package option.", e,
                        device.getDeviceDescriptor());
            }
        }
    }
}
