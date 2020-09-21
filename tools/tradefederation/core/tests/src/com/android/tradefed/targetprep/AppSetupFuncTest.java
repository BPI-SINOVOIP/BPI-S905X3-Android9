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

import static org.junit.Assert.*;

import com.android.tradefed.build.AppBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.WifiHelper;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.util.FileUtil;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;

/**
 * A functional test for {@link AppSetup}.
 *
 * <p>Relies on WifiUtil.apk in tradefed.jar.
 *
 * <p>'aapt' must be in PATH.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class AppSetupFuncTest implements IDeviceTest {

    private ITestDevice mDevice;

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /** Test end to end normal case for {@link AppSetup}. */
    @Test
    public void testSetupTeardown() throws Exception {
        // use wifiutil as a test apk since it already exists
        getDevice().uninstallPackage(WifiHelper.INSTRUMENTATION_PKG);
        File wifiapk = WifiHelper.extractWifiUtilApk();
        try {
            AppBuildInfo appBuild = new AppBuildInfo("0", "stub");
            appBuild.addAppPackageFile(wifiapk, "0");
            AppSetup appSetup = new AppSetup();
            // turn off reboot to reduce test execution time
            OptionSetter optionSetter = new OptionSetter(appSetup);
            optionSetter.setOptionValue("reboot", "false");
            appSetup.setUp(getDevice(), appBuild);
            assertTrue(getDevice().getUninstallablePackageNames().contains(
                    WifiHelper.INSTRUMENTATION_PKG));
            appSetup.tearDown(getDevice(), appBuild, null);
            assertFalse(getDevice().getUninstallablePackageNames().contains(
                    WifiHelper.INSTRUMENTATION_PKG));
        } finally {
            FileUtil.deleteFile(wifiapk);
        }
    }
}
