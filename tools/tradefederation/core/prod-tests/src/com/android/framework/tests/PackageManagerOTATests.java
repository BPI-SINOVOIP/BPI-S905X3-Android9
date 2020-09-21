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

package com.android.framework.tests;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.util.FileUtil;

import java.io.File;

public class PackageManagerOTATests extends DeviceTestCase {

    @Option(name = "test-app-path", description =
            "path to the app repository containing test apks", importance = Importance.IF_UNSET)
    private File mTestAppRepositoryPath = null;
    @Option(name = "use-priv-path", description =
            "set to true if the special priviledged app directory should be used; default is false",
            importance = Importance.IF_UNSET)
    private boolean mUsePrivAppDirectory = false;

    private PackageManagerOTATestUtils mUtils = null;
    private String mSystemAppPath = "/system/app/version_test.apk";
    private String mDiffSystemAppPath = "/system/app/version_test_diff.apk";

    // String constants use for the tests.
    private static final String PACKAGE_XPATH = "/packages/package[@name=\"" +
            "com.android.frameworks.coretests.version_test\"]";
    private static final String UPDATE_PACKAGE_XPATH = "/packages/updated-package[@name=\"" +
            "com.android.frameworks.coretests.version_test\"]";
    private static final String VERSION_XPATH = "/packages/package[@name=\"" +
            "com.android.frameworks.coretests.version_test\"]/@version";
    private static final String CODE_PATH_XPATH = "/packages/package[@name=\"" +
            "com.android.frameworks.coretests.version_test\"]/@codePath";
    private static final String VERSION_1_APK = "FrameworkCoreTests_version_1.apk";
    private static final String VERSION_2_APK = "FrameworkCoreTests_version_2.apk";
    private static final String VERSION_3_APK = "FrameworkCoreTests_version_3.apk";
    private static final String VERSION_1_NO_SYS_PERMISSION_APK =
            "FrameworkCoreTests_version_1_nosys.apk";
    private static final String DATA_APP_DIRECTORY = "/data/app/";
    private static final String PACKAGE_NAME = "com.android.frameworks.coretests.version_test";
    private static final String VIBRATE_PERMISSION = "android.permission.VIBRATE";
    private static final String CACHE_PERMISSION = "android.permission.ACCESS_CACHE_FILESYSTEM";


    // Temporary file used when examine the packages xml file from the device.
    private File mPackageXml = null;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mUtils = new PackageManagerOTATestUtils(getDevice());

        if (mUsePrivAppDirectory) {
            mSystemAppPath = "/system/priv-app/version_test.apk";
            mDiffSystemAppPath = "/system/priv-app/version_test_diff.apk";
        }

        // Clean up any potential old files from previous tests.
        // delete from /system if exists
        getDevice().enableAdbRoot();
        mUtils.removeSystemApp(mSystemAppPath, false);
        mUtils.removeSystemApp(mDiffSystemAppPath, false);
        mUtils.restartSystem();
        // delete from /data if there is one
        getDevice().uninstallPackage(PACKAGE_NAME);

        String res = getDevice().executeShellCommand("pm path " + PACKAGE_NAME).trim();
        assertTrue("Package should not be installed before test", res.isEmpty());
    }

    @Override
    protected void tearDown() throws Exception {
        // Clean up.
        if (mPackageXml != null) {
            FileUtil.deleteFile(mPackageXml);
        }
    }

    /**
     * Get the absolute file system location of test app with given filename
     *
     * @param fileName the file name of the test app apk
     * @return {@link String} of absolute file path
     */
    public File getTestAppFilePath(String fileName) {
        // need to check both data/app/apkFileName and
        // data/app/apkFileName/apkFileName
        File file = FileUtil.getFileForPath(mTestAppRepositoryPath, fileName);
        if (file.exists()) {
            return file;
        }

        int index = fileName.lastIndexOf('.');
        String dir = fileName.substring(0, index);
        file = FileUtil.getFileForPath(mTestAppRepositoryPath, dir, fileName);
        CLog.d("Test path : %s", file.getAbsolutePath());
        return file;
    }

    /**
     * Test case when system app added is newer than update.
     * <p/>
     * Assumes adb is running as root in device under test.
     *
     * @throws DeviceNotAvailableException
     */
    public void testSystemAppAddedNewerThanUpdate() throws DeviceNotAvailableException {
        mUtils.installFile(getTestAppFilePath(VERSION_1_APK), true);
        mPackageXml = mUtils.pullPackagesXML();
        assertNotNull("Failed to pull packages xml file from device", mPackageXml);
        assertTrue("Initial package should be installed",
                mUtils.expectExists(mPackageXml, PACKAGE_XPATH));
        assertTrue("Package version should be 1",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "1"));
        assertFalse("Updated-package should not be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertFalse("Package should not have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertFalse("ACCESS_CACHE_FILESYSTEM permission should NOT be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.pushSystemApp(getTestAppFilePath(VERSION_2_APK), mSystemAppPath);
        mPackageXml = mUtils.pullPackagesXML();
        assertTrue("After system app push, package should still be installed",
                mUtils.expectExists(mPackageXml, PACKAGE_XPATH));
        assertTrue("After system app push, system app should be visible",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "2"));
        assertFalse("Updated-package should not be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));
    }

    /**
     * Test case when system app added is older than update.
     * <p/>
     * Assumes adb is running as root in device under test.
     *
     * @throws DeviceNotAvailableException
     */
    public void testSystemAppAddedOlderThanUpdate() throws DeviceNotAvailableException {
        mUtils.installFile(getTestAppFilePath(VERSION_2_APK), true);
        mPackageXml = mUtils.pullPackagesXML();
        assertNotNull("Failed to pull packages xml file from device", mPackageXml);
        assertTrue("Initial package should be installed",
                mUtils.expectExists(mPackageXml, PACKAGE_XPATH));
        assertTrue("Package version should be 2",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "2"));
        assertFalse("Updated-package should not be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertFalse("Package should not have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertFalse("ACCESS_CACHE_FILESYSTEM permission should NOT be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.pushSystemApp(getTestAppFilePath(VERSION_1_APK), mSystemAppPath);
        mPackageXml = mUtils.pullPackagesXML();
        assertTrue("After system app push, package should still be installed",
                mUtils.expectExists(mPackageXml, PACKAGE_XPATH));
        assertTrue("After system app push, system app should be visible",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "2"));
        assertTrue("Updated-package should be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));
    }

    /**
     * Test when system app gets removed.
     * <p/>
     * Assumes adb is running as root in device under test.
     *
     * @throws DeviceNotAvailableException
     */
    public void testSystemAppRemoved() throws DeviceNotAvailableException {
        mUtils.pushSystemApp(getTestAppFilePath(VERSION_1_APK), mSystemAppPath);
        mPackageXml = mUtils.pullPackagesXML();
        assertTrue("Initial package should be installed",
                mUtils.expectExists(mPackageXml, PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertFalse("Updated-package should not be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        mUtils.removeSystemApp(mSystemAppPath, true);
        mPackageXml = mUtils.pullPackagesXML();
        assertFalse("Package should not be installed",
                mUtils.expectExists(mPackageXml, PACKAGE_XPATH));
        assertFalse("Updated-package should not be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
    }

    /**
     * Test when update has a newer version.
     * <p/>
     * Assumes adb is running as root in device under test.
     *
     * @throws DeviceNotAvailableException
     */
    public void testSystemAppUpdatedNewerVersion() throws DeviceNotAvailableException {
        mUtils.pushSystemApp(getTestAppFilePath(VERSION_2_APK), mSystemAppPath);
        mPackageXml = mUtils.pullPackagesXML();
        assertTrue("The package should be installed",
                mUtils.expectExists(mPackageXml, PACKAGE_XPATH));
        assertTrue("Package version should be 2",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "2"));
        assertFalse("Updated-package should not be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.installFile(getTestAppFilePath(VERSION_3_APK), true);
        mPackageXml = mUtils.pullPackagesXML();
        assertFalse("After system app upgrade, the path should be the upgraded app on /data",
                mUtils.expectEquals(mPackageXml, CODE_PATH_XPATH, mSystemAppPath));
        assertTrue("Package version should be 3",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "3"));
        assertTrue("Updated-package should be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.restartSystem();
        mPackageXml = mUtils.pullPackagesXML();
        assertFalse("After system app upgrade, the path should be the upgraded app on /data",
                mUtils.expectEquals(mPackageXml, CODE_PATH_XPATH, mSystemAppPath));
        assertTrue("Package version should be 3",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "3"));
        assertTrue("Updated-package should be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.restartSystem();
        mPackageXml = mUtils.pullPackagesXML();
        assertFalse("After system app upgrade, the path should be the upgraded app on /data",
                mUtils.expectEquals(mPackageXml, CODE_PATH_XPATH, mSystemAppPath));
        assertTrue("Package version should be 3",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "3"));
        assertTrue("Updated-package should be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));
    }

    /**
     * Test when update has an older version.
     * <p/>
     * Assumes adb is running as root in device under test.
     *
     * @throws DeviceNotAvailableException
     */
    public void testSystemAppUpdatedOlderVersion() throws DeviceNotAvailableException {
        mUtils.pushSystemApp(getTestAppFilePath(VERSION_2_APK), mSystemAppPath);
        mPackageXml = mUtils.pullPackagesXML();
        assertTrue("After system app push, the package should be installed",
                mUtils.expectExists(mPackageXml, PACKAGE_XPATH));
        assertTrue("Package version should be 2",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "2"));
        assertFalse("Updated-package should not be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        // The "-d" command forces a downgrade.
        mUtils.installFile(getTestAppFilePath(VERSION_1_APK), true, "-d");
        mPackageXml = mUtils.pullPackagesXML();
        assertTrue("After system app upgrade, the path should be the upgraded app on /data",
                mUtils.expectStartsWith(mPackageXml, CODE_PATH_XPATH,
                DATA_APP_DIRECTORY + PACKAGE_NAME));
        assertTrue("Package version should be 1",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "1"));
        assertTrue("Updated-package should be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.restartSystem();
        mPackageXml = mUtils.pullPackagesXML();
        assertTrue("After reboot, the path should be the be installed",
                mUtils.expectEquals(mPackageXml, CODE_PATH_XPATH, mSystemAppPath));
        assertTrue("Package version should be 2",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "2"));
        assertFalse("Updated-package should NOT be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.restartSystem();
        mPackageXml = mUtils.pullPackagesXML();
        assertTrue("After reboot, the path should be the be installed",
                mUtils.expectEquals(mPackageXml, CODE_PATH_XPATH, mSystemAppPath));
        assertTrue("Package version should be 2",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "2"));
        assertFalse("Updated-package should NOT be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));
    }

    /**
     * Test when updated system app has the same version. Package manager is expected to use
     * the newly installed upgrade.
     * <p/>
     * Assumes adb is running as root in device under test.
     *
     * @throws DeviceNotAvailableException
     */
    public void testSystemAppUpdatedSameVersion_PreferUpdatedApk()
            throws DeviceNotAvailableException {
        mUtils.pushSystemApp(getTestAppFilePath(VERSION_2_APK), mSystemAppPath);
        mPackageXml = mUtils.pullPackagesXML();
        assertTrue("The package should be installed",
                mUtils.expectExists(mPackageXml, PACKAGE_XPATH));
        assertTrue("Package version should be 2",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "2"));
        assertFalse("Updated-package should not be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.installFile(getTestAppFilePath(VERSION_2_APK), true);
        mPackageXml = mUtils.pullPackagesXML();
        assertFalse("After system app upgrade, the path should be the upgraded app in /data",
                mUtils.expectEquals(mPackageXml, CODE_PATH_XPATH, mSystemAppPath));
        assertTrue("Package version should be 2",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "2"));
        assertTrue("Updated-package should be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.restartSystem();
        mPackageXml = mUtils.pullPackagesXML();
        assertFalse("After reboot, the path should be the upgraded app in /data",
                mUtils.expectEquals(mPackageXml, CODE_PATH_XPATH, mSystemAppPath));
        assertTrue("Package version should be 2",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "2"));
        assertTrue("Updated-package should be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

    }

    /**
     * Test when update has system app removed.
     * <p/>
     * Assumes adb is running as root in device under test.
     *
     * @throws DeviceNotAvailableException
     */
    public void testUpdatedSystemAppRemoved() throws DeviceNotAvailableException {
        mUtils.pushSystemApp(getTestAppFilePath(VERSION_1_APK), mSystemAppPath);
        mUtils.installFile(getTestAppFilePath(VERSION_2_APK), true);
        mPackageXml = mUtils.pullPackagesXML();
        assertTrue("Package should be installed",
                mUtils.expectExists(mPackageXml, PACKAGE_XPATH));
        assertTrue("Updated-package should be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.removeSystemApp(mSystemAppPath, true);
        mPackageXml = mUtils.pullPackagesXML();
        assertTrue("Package should still be installed",
                mUtils.expectExists(mPackageXml, PACKAGE_XPATH));
        assertFalse("Updated-package entry should not be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertFalse("Package should not have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertFalse("ACCESS_CACHE_FILESYSTEM permission should NOT be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));
    }

    /**
     * Test when system app is updated with a new permission. Specifically:
     * <ol>
     * <li>system app FOO is present, does not declare system permission</li>
     * <li>FOO is overlain by an installed update that declares new permission</li>
     * <li>FOO is replaced during an OTA, but installed update still has higher version number</li>
     * <li>Verify permission is granted</li>
     * </ol>
     *
     * @throws DeviceNotAvailableException
     */
    public void testSystemAppUpdatedNewPermission() throws DeviceNotAvailableException {
        mUtils.pushSystemApp(getTestAppFilePath(VERSION_1_NO_SYS_PERMISSION_APK), mSystemAppPath);
        mPackageXml = mUtils.pullPackagesXML();
        assertTrue("The package should be installed",
                mUtils.expectExists(mPackageXml, PACKAGE_XPATH));
        assertTrue("Package version should be 1",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "1"));
        assertFalse("Updated-package should not be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertFalse("ACCESS_CACHE_FILESYSTEM permission should NOT be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.installFile(getTestAppFilePath(VERSION_3_APK), true);
        mPackageXml = mUtils.pullPackagesXML();
        assertFalse("After system app upgrade, the path should be the upgraded app on /data",
                mUtils.expectEquals(mPackageXml, CODE_PATH_XPATH, mSystemAppPath));
        assertTrue("Package version should be 3",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "3"));
        assertTrue("Updated-package should be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertFalse("ACCESS_CACHE_FILESYSTEM permission should NOT be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.pushSystemApp(getTestAppFilePath(VERSION_2_APK), mSystemAppPath);
        mUtils.restartSystem();
        mPackageXml = mUtils.pullPackagesXML();
        assertFalse("After reboot, the path should be the data app",
                mUtils.expectEquals(mPackageXml, CODE_PATH_XPATH, mSystemAppPath));
        assertTrue("Package version should be 3",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "3"));
        assertTrue("Updated-package should be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));

        mUtils.restartSystem();
        mPackageXml = mUtils.pullPackagesXML();
        assertFalse("After reboot, the path should be the data app",
                mUtils.expectEquals(mPackageXml, CODE_PATH_XPATH, mSystemAppPath));
        assertTrue("Package version should be 3",
                mUtils.expectEquals(mPackageXml, VERSION_XPATH, "3"));
        assertTrue("Updated-package should be present",
                mUtils.expectExists(mPackageXml, UPDATE_PACKAGE_XPATH));
        assertTrue("Package should have FLAG_SYSTEM",
                mUtils.packageHasFlag(PACKAGE_NAME, " SYSTEM "));
        assertTrue("VIBRATE permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, VIBRATE_PERMISSION));
        assertTrue("ACCESS_CACHE_FILESYSTEM permission should be granted",
                mUtils.packageHasPermission(PACKAGE_NAME, CACHE_PERMISSION));
    }
}
