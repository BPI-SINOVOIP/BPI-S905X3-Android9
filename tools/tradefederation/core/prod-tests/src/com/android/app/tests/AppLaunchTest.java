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
package com.android.app.tests;

import com.android.tradefed.build.IAppBuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.VersionedFile;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.InstrumentationTest;
import com.android.tradefed.util.AaptParser;
import com.android.tradefed.util.FileUtil;

import org.junit.Assert;

import java.io.File;
import java.util.HashMap;

/**
 * A harness that installs and launches an app on device and verifies it doesn't crash.
 * <p/>
 * Requires a {@link IAppBuildInfo} and 'aapt' being present in path. Assume the AppLaunch
 * test app is already present on device.
 */
public class AppLaunchTest implements IDeviceTest, IRemoteTest, IBuildReceiver {

    private static final String RUN_NAME = "AppLaunch";
    private ITestDevice mDevice;
    private IBuildInfo mBuild;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuild = buildInfo;
    }

    /**
     * Installs all apks listed in {@link IAppBuildInfo}, then attempts to run the package in the
     * first apk.  Note that this does <emph>not</emph> attempt to uninstall the apks, and requires
     * external cleanup.
     * <p />
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        long startTime = System.currentTimeMillis();
        listener.testRunStarted(RUN_NAME, 2);
        try {
            Assert.assertTrue(mBuild instanceof IAppBuildInfo);
            IAppBuildInfo appBuild = (IAppBuildInfo)mBuild;
            Assert.assertFalse(appBuild.getAppPackageFiles().isEmpty());

            // We assume that the first apk is the one to be executed, and any others are to be
            // installed and uninstalled.
            File appApkFile = appBuild.getAppPackageFiles().get(0).getFile();
            AaptParser p = AaptParser.parse(appApkFile);
            Assert.assertNotNull(p);
            String packageName = p.getPackageName();
            Assert.assertNotNull(String.format("Failed to parse package name from %s",
                    appApkFile.getAbsolutePath()), packageName);

            for (final VersionedFile apkVersionedFile : appBuild.getAppPackageFiles()) {
                final File apkFile = apkVersionedFile.getFile();
                performInstallTest(apkFile, listener);
            }

            performLaunchTest(packageName, listener);
        } catch (AssertionError e) {
            listener.testRunFailed(e.toString());
        } finally {
            listener.testRunEnded(
                    System.currentTimeMillis() - startTime, new HashMap<String, Metric>());
        }

    }

    private void performInstallTest(File apkFile, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        TestDescription installTest =
                new TestDescription(
                        "com.android.app.tests.InstallTest",
                        FileUtil.getBaseName(apkFile.getName()));
        listener.testStarted(installTest);
        String result = getDevice().installPackage(apkFile, true);
        if (result != null) {
            listener.testFailed(installTest, result);
        }
        listener.testEnded(installTest, new HashMap<String, Metric>());
    }

    private void performLaunchTest(String packageName, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        InstrumentationTest i = new InstrumentationTest();
        i.setRunName(RUN_NAME);
        i.setPackageName("com.android.applaunchtest");
        i.setRunnerName("com.android.applaunchtest.AppLaunchRunner");
        i.setDevice(getDevice());
        i.addInstrumentationArg("packageName", packageName);
        i.run(listener);
        try (InputStreamSource s = getDevice().getScreenshot()) {
            listener.testLog("screenshot", LogDataType.PNG, s);
        }
    }
}
