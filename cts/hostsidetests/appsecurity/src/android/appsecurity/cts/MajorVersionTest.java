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

package android.appsecurity.cts;

import android.platform.test.annotations.AppModeFull;
import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;

/**
 * Test that install of apps using major version codes is being handled properly.
 */
@AppModeFull // TODO: Needs porting to instant
public class MajorVersionTest extends DeviceTestCase implements IAbiReceiver, IBuildReceiver {
    private static final String PKG = "com.android.cts.majorversion";
    private static final String APK_000000000000ffff = "CtsMajorVersion000000000000ffff.apk";
    private static final String APK_00000000ffffffff = "CtsMajorVersion00000000ffffffff.apk";
    private static final String APK_000000ff00000000 = "CtsMajorVersion000000ff00000000.apk";
    private static final String APK_000000ffffffffff = "CtsMajorVersion000000ffffffffff.apk";

    private IAbi mAbi;
    private CompatibilityBuildHelper mBuildHelper;

    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildHelper = new CompatibilityBuildHelper(buildInfo);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        Utils.prepareSingleUser(getDevice());
        assertNotNull(mAbi);
        assertNotNull(mBuildHelper);

        getDevice().uninstallPackage(PKG);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();

        getDevice().uninstallPackage(PKG);
    }

    public void testInstallMinorVersion() throws Exception {
        assertNull(getDevice().installPackage(
                mBuildHelper.getTestFile(APK_000000000000ffff), false, false));
        assertTrue(getDevice().getInstalledPackageNames().contains(PKG));
        runVersionDeviceTests("testCheckVersion");
        getDevice().uninstallPackage(PKG);
    }

    public void testInstallMajorVersion() throws Exception {
        assertNull(getDevice().installPackage(
                mBuildHelper.getTestFile(APK_000000ff00000000), false, false));
        assertTrue(getDevice().getInstalledPackageNames().contains(PKG));
        runVersionDeviceTests("testCheckVersion");
        getDevice().uninstallPackage(PKG);
    }

    public void testInstallUpdateAcrossMinorMajorVersion() throws Exception {
        assertNull(getDevice().installPackage(
                mBuildHelper.getTestFile(APK_000000000000ffff), false, false));
        assertTrue(getDevice().getInstalledPackageNames().contains(PKG));
        runVersionDeviceTests("testCheckVersion");
        assertNull(getDevice().installPackage(
                mBuildHelper.getTestFile(APK_00000000ffffffff), true, false));
        assertTrue(getDevice().getInstalledPackageNames().contains(PKG));
        runVersionDeviceTests("testCheckVersion");
        assertNull(getDevice().installPackage(
                mBuildHelper.getTestFile(APK_000000ff00000000), true, false));
        assertTrue(getDevice().getInstalledPackageNames().contains(PKG));
        runVersionDeviceTests("testCheckVersion");
        assertNull(getDevice().installPackage(
                mBuildHelper.getTestFile(APK_000000ffffffffff), true, false));
        assertTrue(getDevice().getInstalledPackageNames().contains(PKG));
        runVersionDeviceTests("testCheckVersion");
        getDevice().uninstallPackage(PKG);
    }

    public void testInstallDowngradeAcrossMajorMinorVersion() throws Exception {
        assertNull(getDevice().installPackage(
                mBuildHelper.getTestFile(APK_000000ffffffffff), false, false));
        assertTrue(getDevice().getInstalledPackageNames().contains(PKG));
        runVersionDeviceTests("testCheckVersion");
        assertEquals("INSTALL_FAILED_VERSION_DOWNGRADE", getDevice().installPackage(
                mBuildHelper.getTestFile(APK_00000000ffffffff), true, false));
        assertTrue(getDevice().getInstalledPackageNames().contains(PKG));
        runVersionDeviceTests("testCheckVersion");
        assertEquals("INSTALL_FAILED_VERSION_DOWNGRADE", getDevice().installPackage(
                mBuildHelper.getTestFile(APK_000000ff00000000), true, false));
        assertTrue(getDevice().getInstalledPackageNames().contains(PKG));
        runVersionDeviceTests("testCheckVersion");
        assertEquals("INSTALL_FAILED_VERSION_DOWNGRADE", getDevice().installPackage(
                mBuildHelper.getTestFile(APK_000000000000ffff), true, false));
        assertTrue(getDevice().getInstalledPackageNames().contains(PKG));
        runVersionDeviceTests("testCheckVersion");
        getDevice().uninstallPackage(PKG);
    }

    private void runVersionDeviceTests(String testMethodName)
            throws DeviceNotAvailableException {
        runDeviceTests(PKG, PKG + ".VersionTest", testMethodName);
    }

    private void runDeviceTests(String packageName, String testClassName, String testMethodName)
            throws DeviceNotAvailableException {
        Utils.runDeviceTests(getDevice(), packageName, testClassName, testMethodName);
    }
}
