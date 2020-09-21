/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.android.ddmlib.Log;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.util.FileUtil;

import java.io.File;

/**
 * Set of tests that verify host side install cases
 */
public class PackageManagerHostTests extends DeviceTestCase {

    private static final String ABI_PROPERTY = "ro.product.cpu.abi";
    private static final String ARM64_V8A = "arm64-v8a";
    private static final String ARMEABI_V7A = "armeabi-v7a";
    private static final String LOG_TAG = "PackageManagerHostTests";
    private PackageManagerHostTestUtils mPMHostUtils = null;

    // Various test files and their corresponding package names...

    // testPushAppPrivate constants
    // these constants must match values defined in test-apps/SimpleTestApp
    private static final String SIMPLE_APK = "SimpleTestApp.apk";
    private static final String SIMPLE_PKG = "com.android.framework.simpletestapp";

    // Apk with install location set to auto
    private static final String AUTO_LOC_APK = "AutoLocTestApp.apk";
    private static final String AUTO_LOC_PKG = "com.android.framework.autoloctestapp";
    // Apk with install location set to internalOnly
    private static final String INTERNAL_LOC_APK = "InternalLocTestApp.apk";
    private static final String INTERNAL_LOC_PKG = "com.android.framework.internalloctestapp";
    // Apk with install location set to preferExternal
    private static final String EXTERNAL_LOC_APK = "ExternalLocTestApp.apk";
    private static final String EXTERNAL_LOC_PKG = "com.android.framework.externalloctestapp";
    // Apk with install location set to auto (2 versions, for update testing)
    @SuppressWarnings("unused")
    private static final String AUTO_LOC_VERSION_V1_APK = "AutoLocVersionedTestApp_v1.apk";
    @SuppressWarnings("unused")
    private static final String AUTO_LOC_VERSION_V2_APK = "AutoLocVersionedTestApp_v2.apk";
    @SuppressWarnings("unused")
    private static final String AUTO_LOC_VERSION_PKG =
            "com.android.framework.autolocversionedtestapp";
    // Apk with install location set to preferExternal (2 versions, for update
    // testing)
    private static final String EXTERNAL_LOC_VERSION_V1_APK = "ExternalLocVersionedTestApp_v1.apk";
    private static final String EXTERNAL_LOC_VERSION_V2_APK = "ExternalLocVersionedTestApp_v2.apk";
    private static final String EXTERNAL_LOC_VERSION_PKG =
            "com.android.framework.externallocversionedtestapp";
    // Apk with install location set to auto (2 versions, for update testing)
    private static final String NO_LOC_VERSION_V1_APK = "NoLocVersionedTestApp_v1.apk";
    private static final String NO_LOC_VERSION_V2_APK = "NoLocVersionedTestApp_v2.apk";
    private static final String NO_LOC_VERSION_PKG =
            "com.android.framework.nolocversionedtestapp";
    // Apk with no install location set
    private static final String NO_LOC_APK = "NoLocTestApp.apk";
    private static final String NO_LOC_PKG = "com.android.framework.noloctestapp";
    // Apk with 2 different versions - v1 is set to external, v2 has no location
    // setting
    private static final String UPDATE_EXTERNAL_LOC_V1_EXT_APK = "UpdateExternalLocTestApp_v1_ext.apk";
    private static final String UPDATE_EXTERNAL_LOC_V2_NONE_APK = "UpdateExternalLocTestApp_v2_none.apk";
    private static final String UPDATE_EXTERNAL_LOC_PKG = "com.android.framework.updateexternalloctestapp";
    // Apk with 2 different versions - v1 is set to external, v2 is set to
    // internalOnly
    private static final String UPDATE_EXT_TO_INT_LOC_V1_EXT_APK = "UpdateExtToIntLocTestApp_v1_ext.apk";
    private static final String UPDATE_EXT_TO_INT_LOC_V2_INT_APK = "UpdateExtToIntLocTestApp_v2_int.apk";
    private static final String UPDATE_EXT_TO_INT_LOC_PKG = "com.android.framework.updateexttointloctestapp";
    // Apk set to preferExternal, with Access Fine Location permissions set in
    // its manifest
    @SuppressWarnings("unused")
    private static final String FL_PERMS_APK = "ExternalLocPermsFLTestApp.apk";
    @SuppressWarnings("unused")
    private static final String FL_PERMS_PKG = "com.android.framework.externallocpermsfltestapp";
    // Apk set to preferExternal, with all permissions set in manifest
    private static final String ALL_PERMS_APK = "ExternalLocAllPermsTestApp.apk";
    private static final String ALL_PERMS_PKG = "com.android.framework.externallocallpermstestapp";
    // Apks with the same package name, but install location set to
    // one of: Internal, External, Auto, or None
    private static final String VERSATILE_LOC_PKG = "com.android.framework.versatiletestapp";
    private static final String VERSATILE_LOC_INTERNAL_APK = "VersatileTestApp_Internal.apk";
    private static final String VERSATILE_LOC_EXTERNAL_APK = "VersatileTestApp_External.apk";
    @SuppressWarnings("unused")
    private static final String VERSATILE_LOC_AUTO_APK = "VersatileTestApp_Auto.apk";
    @SuppressWarnings("unused")
    private static final String VERSATILE_LOC_NONE_APK = "VersatileTestApp_None.apk";
    // Apks with shared UserID
    private static final String SHARED_PERMS_APK = "ExternalSharedPermsTestApp.apk";
    private static final String SHARED_PERMS_PKG = "com.android.framework.externalsharedpermstestapp";
    private static final String SHARED_PERMS_FL_APK = "ExternalSharedPermsFLTestApp.apk";
    private static final String SHARED_PERMS_FL_PKG = "com.android.framework.externalsharedpermsfltestapp";
    private static final String SHARED_PERMS_BT_APK = "ExternalSharedPermsBTTestApp.apk";
    private static final String SHARED_PERMS_BT_PKG = "com.android.framework.externalsharedpermsbttestapp";
    // Apk with shared UserID, but signed with a different cert (the media cert)
    @SuppressWarnings("unused")
    private static final String SHARED_PERMS_DIFF_KEY_APK = "ExternalSharedPermsDiffKeyTestApp.apk";
    @SuppressWarnings("unused")
    private static final String SHARED_PERMS_DIFF_KEY_PKG = "com.android.framework.externalsharedpermsdiffkeytestapp";

    // Shared uid apks
    private static final String SHARED_UID_APK = "PMTest_Java.apk";
    private static final String SHARED_UID_PKG = "com.framework.shareduid.java";
    private static final String SHARED_UID_APK_32 = "PMTest_Java32.apk";
    private static final String SHARED_UID_PKG_32 = "com.framework.shareduid.bit32";
    private static final String SHARED_UID_APK_64 = "PMTest_Java64.apk";
    private static final String SHARED_UID_PKG_64 = "com.framework.shareduid.bit64";
    private static final String SHARED_UID_APK_DUAL = "PMTest_Java_dual.apk";
    private static final String SHARED_UID_PKG_DUAL = "com.framework.shareduid.dual";

    // TODO: consider fetching these files from build server instead.
    @Option(name = "app-repository-path", description =
            "path to the app repository containing large apks", importance = Importance.IF_UNSET)
    private File mAppRepositoryPath = null;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // ensure apk path has been set before test is run
        assertNotNull("Missing --app-repository-path option", mAppRepositoryPath);

        // setup the PackageManager host tests utilities class, and get various
        // paths we'll need...
        mPMHostUtils = new PackageManagerHostTestUtils(getDevice());
    }

    /**
     * Get the absolute file system location of test app with given filename
     *
     * @param fileName the file name of the test app apk
     * @return {@link String} of absolute file path
     */
    public File getTestAppFilePath(String fileName) {
        return FileUtil.getFileForPath(mAppRepositoryPath, fileName);
    }

    /**
     * Regression test to verify that pushing an apk to the private app
     * directory doesn't install the app, and otherwise cause the system to blow
     * up.
     * <p/>
     * Assumes adb is running as root in device under test.
     *
     * @throws DeviceNotAvailableException
     */
    public void testPushAppPrivate() throws DeviceNotAvailableException {
        Log.i(LOG_TAG, "testing pushing an apk to /data/app-private");
        final String apkAppPrivatePath = PackageManagerHostTestUtils.getAppPrivatePath()
                + SIMPLE_APK;

        // cleanup test app just in case it was accidently installed
        getDevice().uninstallPackage(SIMPLE_PKG);
        getDevice().executeShellCommand("stop");
        getDevice().pushFile(getTestAppFilePath(SIMPLE_APK), apkAppPrivatePath);

        // sanity check to make sure file is there
        assertTrue(getDevice().doesFileExist(apkAppPrivatePath));
        getDevice().executeShellCommand("start");

        mPMHostUtils.waitForPackageManager();

        // grep for package to make sure its not installed
        assertFalse(mPMHostUtils.doesPackageExist(SIMPLE_PKG));
        // TODO: Is the apk supposed to uninstall itself?
        // ensure it has been deleted from app-private
        // assertFalse(getDevice().doesFileExist(apkAppPrivatePath));
    }

    /**
     * Helper to do a standard install of an apk and verify it installed to the
     * correct location.
     * <p/>
     * Assumes adb is running as root in device under test.
     *
     * @param apkName the file name of the test app apk
     * @param pkgName the package name of the test app apk
     * @param expectedLocation the file name of the test app apk
     * @throws DeviceNotAvailableException
     */
    private void doStandardInstall(String apkName, String pkgName,
            PackageManagerHostTestUtils.InstallLocation expectedLocation)
            throws DeviceNotAvailableException {

        if (expectedLocation == PackageManagerHostTestUtils.InstallLocation.DEVICE) {
            mPMHostUtils.installAppAndVerifyExistsOnDevice(getTestAppFilePath(apkName), pkgName,
                    false);
        } else {
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(apkName), pkgName,
                    false);
        }
    }

    /**
     * Installs the Auto app and verifies it was installed  at expected loc.
     * <p/>
     * Assumes adb is running as root in device under test.
     *
     * @param expectedLocation the expected location of where the apk was
     *            installed
     * @throws DeviceNotAvailableException
     */
    public void installAppAutoLoc(PackageManagerHostTestUtils.InstallLocation expectedLocation)
            throws DeviceNotAvailableException {
        try {
            // Auto app should go to storage with more free space when device has adopted storage
            doStandardInstall(AUTO_LOC_APK, AUTO_LOC_PKG, expectedLocation);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(AUTO_LOC_PKG);
        }
    }

    /**
     * Regression test to verify that an app with its manifest set to
     * installLocation=auto will install the app to the device
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAppLocPrefIsAuto() throws Exception {
        Log.i(LOG_TAG, "Test app manifest installLocation=auto gets installed on device");
        installAppAutoLoc(PackageManagerHostTestUtils.InstallLocation.DEVICE);
    }

    /**
     * Installs the Internal app and verifies it was installed at expected loc.
     * <p/>
     * Assumes adb is running as root in device under test.
     *
     * @param expectedLocation the expected location of where the apk was
     *            installed
     */
    public void installAppInternalLoc(PackageManagerHostTestUtils.InstallLocation expectedLocation)
            throws Exception {
        try {
            doStandardInstall(INTERNAL_LOC_APK, INTERNAL_LOC_PKG, expectedLocation);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(INTERNAL_LOC_PKG);
        }
    }

    /**
     * Regression test to verify that an app with its manifest set to
     * installLocation=internalOnly will install the app to the device when
     * device's preference is auto.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAppLocPrefIsInternal() throws Exception {
      Log.i(LOG_TAG, "Test app manifest installLocation=internal gets installed on device");
        installAppInternalLoc(PackageManagerHostTestUtils.InstallLocation.DEVICE);
    }

    /**
     * Regression test to verify that an app with its manifest set to
     * installLocation=preferExternal will install the app to expected loc.
     * <p/>
     * Assumes adb is running as root in device under test.
     *
     * @param expectedLocation the expected location of where the apk was
     *            installed
     */
    public void installAppExternalLoc(
            PackageManagerHostTestUtils.InstallLocation expectedLocation)
            throws Exception {
        try {
            doStandardInstall(EXTERNAL_LOC_APK, EXTERNAL_LOC_PKG, expectedLocation);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(EXTERNAL_LOC_PKG);
        }
    }

    /**
     * Regression test to verify that an app with its manifest set to
     * installLocation=preferExternal will install the app to the appropriate external storage.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAppLocPrefIsExternal() throws Exception {
        Log.i(LOG_TAG, "Test installLocation=external gets installed on SD Card");
        installAppExternalLoc(PackageManagerHostTestUtils.InstallLocation.SDCARD);
    }

    /**
     * Regression test to verify that an app without installLocation in its
     * manifest will install the app to the device by default
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAppNoLocPrefIsAuto() throws Exception {
        Log.i(LOG_TAG, "Test an app with no installLocation gets installed on device");
        try {
            mPMHostUtils.installAppAndVerifyExistsOnDevice(
                    getTestAppFilePath(NO_LOC_APK), NO_LOC_PKG, false);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(NO_LOC_PKG);
        }
    }

    /**
     * Regression test to verify that an app with its installLocation set to
     * internal that is forward-locked will get installed to the correct
     * location.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallFwdLockedAppInternal() throws Exception {
        Log.i(LOG_TAG, "Test an app with installLoc set to Internal gets installed to app-private");

        try {
            mPMHostUtils.installFwdLockedAppAndVerifyExists(
                    getTestAppFilePath(INTERNAL_LOC_APK), INTERNAL_LOC_PKG, false);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(INTERNAL_LOC_PKG);
        }
    }

    /**
     * Regression test to verify that an app with its installLocation set to
     * external that is forward-locked will get installed to the correct
     * location.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallFwdLockedAppExternal() throws Exception {
        Log.i(LOG_TAG, "Test an app with installLoc set to Internal gets installed to app-private");

        try {
            mPMHostUtils.installFwdLockedAppAndVerifyExists(
                    getTestAppFilePath(EXTERNAL_LOC_APK), EXTERNAL_LOC_PKG, false);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(EXTERNAL_LOC_PKG);
        }
    }

    /**
     * Regression test to verify that an app with its installLocation set to
     * external that is forward-locked will get installed to the correct
     * location.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallFwdLockedAppAuto() throws Exception {
        Log.i(LOG_TAG, "Test an app with installLoc set to Auto gets installed to app-private");

        try {
            mPMHostUtils.installFwdLockedAppAndVerifyExists(
                    getTestAppFilePath(AUTO_LOC_APK), AUTO_LOC_PKG, false);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(AUTO_LOC_PKG);
        }
    }

    /**
     * Regression test to verify that an app with no installLocation set and is
     * forward-locked installed will get installed to the correct location.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallFwdLockedAppNone() throws Exception {
        Log.i(LOG_TAG, "Test an app with no installLoc set gets installed to app-private");

        try {
            mPMHostUtils.installFwdLockedAppAndVerifyExists(
                    getTestAppFilePath(NO_LOC_APK), NO_LOC_PKG, false);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(NO_LOC_PKG);
        }
    }

    /**
     * Regression test to verify that we can install an app onto the device,
     * uninstall it, and reinstall it onto the SD card.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    // TODO: This currently relies on the app's manifest to switch from device
    // to
    // SD card install locations. We might want to make Device's
    // installPackage()
    // accept a installLocation flag so we can install a package to the
    // destination of our choosing.
    public void testReinstallInternalToExternal() throws Exception {
        Log.i(LOG_TAG, "Test installing an app first to the device, then to the SD Card");

        try {
            mPMHostUtils.installAppAndVerifyExistsOnDevice(
                    getTestAppFilePath(VERSATILE_LOC_INTERNAL_APK), VERSATILE_LOC_PKG, false);
            mPMHostUtils.uninstallApp(VERSATILE_LOC_PKG);
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(
                    getTestAppFilePath(VERSATILE_LOC_EXTERNAL_APK), VERSATILE_LOC_PKG, false);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(VERSATILE_LOC_PKG);
        }
    }

    /**
     * Regression test to verify that we can install an app onto the SD Card,
     * uninstall it, and reinstall it onto the device.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    // TODO: This currently relies on the app's manifest to switch from device
    // to
    // SD card install locations. We might want to make Device's
    // installPackage()
    // accept a installLocation flag so we can install a package to the
    // destination of our choosing.
    public void testReinstallExternalToInternal() throws Exception {
        Log.i(LOG_TAG, "Test installing an app first to the SD Care, then to the device");

        try {
            // install the app externally
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(
                    getTestAppFilePath(VERSATILE_LOC_EXTERNAL_APK), VERSATILE_LOC_PKG, false);
            mPMHostUtils.uninstallApp(VERSATILE_LOC_PKG);
            // then replace the app with one marked for internalOnly
            mPMHostUtils.installAppAndVerifyExistsOnDevice(
                    getTestAppFilePath(VERSATILE_LOC_INTERNAL_APK), VERSATILE_LOC_PKG, false);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(VERSATILE_LOC_PKG);
        }
    }

    /**
     * Regression test to verify that updating an app on the SD card will
     * install the update onto the SD card as well when location is set to
     * external for both versions
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testUpdateBothExternal() throws Exception {
        Log.i(LOG_TAG, "Test updating an app on the SD card stays on the SD card");

        try {
            // install the app externally
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    EXTERNAL_LOC_VERSION_V1_APK), EXTERNAL_LOC_VERSION_PKG, false);
            // now replace the app with one where the location is still set to
            // preferExternal
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    EXTERNAL_LOC_VERSION_V2_APK), EXTERNAL_LOC_VERSION_PKG, true);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(EXTERNAL_LOC_VERSION_PKG);
        }
    }

    /**
     * Regression test to verify that updating an app on the SD card will
     * install the update onto the SD card as well when location is not
     * explicitly set in the updated apps' manifest file.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testUpdateToSDCard() throws Exception {
        Log.i(LOG_TAG, "Test updating an app on the SD card stays on the SD card");

        try {
            // install the app externally
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    UPDATE_EXTERNAL_LOC_V1_EXT_APK), UPDATE_EXTERNAL_LOC_PKG, false);
            // now replace the app with one where the location is blank (app
            // should stay external)
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    UPDATE_EXTERNAL_LOC_V2_NONE_APK), UPDATE_EXTERNAL_LOC_PKG, true);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(UPDATE_EXTERNAL_LOC_PKG);
        }
    }

    /**
     * Regression test to verify that updating an app on the SD card will
     * install the update onto the device if the manifest has changed to
     * installLocation=internalOnly
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testUpdateSDCardToDevice() throws Exception {
        Log.i(LOG_TAG, "Test updating an app on the SD card to the Device through manifest change");

        try {
            // install the app externally
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    UPDATE_EXT_TO_INT_LOC_V1_EXT_APK), UPDATE_EXT_TO_INT_LOC_PKG, false);
            // now replace the app with an update marked for
            // internalOnly...(should move internal)
            mPMHostUtils.installAppAndVerifyExistsOnDevice(getTestAppFilePath(
                    UPDATE_EXT_TO_INT_LOC_V2_INT_APK), UPDATE_EXT_TO_INT_LOC_PKG, true);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(UPDATE_EXT_TO_INT_LOC_PKG);
        }
    }

    /**
     * Regression test to verify that installing and updating a forward-locked
     * app will install the update onto the device's forward-locked location
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAndUpdateExternalLocForwardLockedApp() throws Exception {
        Log.i(LOG_TAG, "Test updating a forward-locked app marked preferExternal");

        try {
            // first try to install the forward-locked app externally
            mPMHostUtils.installFwdLockedAppAndVerifyExists(getTestAppFilePath(
                    EXTERNAL_LOC_VERSION_V1_APK), EXTERNAL_LOC_VERSION_PKG, false);
            // now replace the app with an update marked for internalOnly and as
            // forward locked
            mPMHostUtils.installFwdLockedAppAndVerifyExists(getTestAppFilePath(
                    EXTERNAL_LOC_VERSION_V2_APK), EXTERNAL_LOC_VERSION_PKG, true);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(EXTERNAL_LOC_VERSION_PKG);
        }
    }

    /**
     * Regression test to verify that updating a forward-locked app will install
     * the update onto the device's forward-locked location
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAndUpdateNoLocForwardLockedApp() throws Exception {
        Log.i(LOG_TAG, "Test updating a forward-locked app with no installLocation pref set");

        try {
            // install the app
            mPMHostUtils.installFwdLockedAppAndVerifyExists(getTestAppFilePath(
                    NO_LOC_VERSION_V1_APK), NO_LOC_VERSION_PKG, false);
            // now replace the app with an update marked for internalOnly...
            mPMHostUtils.installFwdLockedAppAndVerifyExists(getTestAppFilePath(
                    NO_LOC_VERSION_V2_APK), NO_LOC_VERSION_PKG, true);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(NO_LOC_VERSION_PKG);
        }
    }

    /**
     * Regression test to verify that an app with all permissions set can be
     * installed on SD card and then launched without crashing.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAndLaunchAllPermsAppOnSD() throws Exception {
        Log.i(LOG_TAG, "Test launching an app with all perms set, installed on SD card");

        try {
            // install the app
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    ALL_PERMS_APK), ALL_PERMS_PKG, false);
            boolean testsPassed = mPMHostUtils.runDeviceTestsDidAllTestsPass(ALL_PERMS_PKG);
            assert (testsPassed);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(ALL_PERMS_PKG);
        }
    }

    /**
     * Regression test to verify that an app with ACCESS_FINE_LOCATION (GPS)
     * permissions can run without permissions errors.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAndLaunchFLPermsAppOnSD() throws Exception {
        Log.i(LOG_TAG, "Test launching an app with location perms set, installed on SD card");

        try {
            // install the app and verify we can launch it without permissions
            // errors
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    SHARED_PERMS_FL_APK), SHARED_PERMS_FL_PKG, false);
            boolean testsPassed = mPMHostUtils.runDeviceTestsDidAllTestsPass(SHARED_PERMS_FL_PKG);
            assert (testsPassed);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(SHARED_PERMS_FL_PKG);
        }
    }

    /**
     * Regression test to verify that an app with BLUE_TOOTH permissions can run
     * without permissions errors.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAndLaunchBTPermsAppOnSD() throws Exception {
        Log.i(LOG_TAG, "Test launching an app with bluetooth perms set, installed on SD card");

        try {
            // install the app and verify we can launch it without permissions
            // errors
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    SHARED_PERMS_BT_APK), SHARED_PERMS_BT_PKG, false);
            boolean testsPassed = mPMHostUtils.runDeviceTestsDidAllTestsPass(SHARED_PERMS_BT_PKG);
            assert (testsPassed);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(SHARED_PERMS_BT_PKG);
        }
    }

    /**
     * Regression test to verify that a shared app with no explicit permissions
     * throws a SecurityException when launched if its other shared apps are not
     * installed.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAndLaunchSharedPermsAppOnSD_NoPerms() throws Exception {
        Log.i(LOG_TAG, "Test launching an app with no explicit perms set, installed on SD card");

        try {
            // Make sure the 2 shared apps with needed permissions are not
            // installed...
            mPMHostUtils.uninstallApp(SHARED_PERMS_FL_PKG);
            mPMHostUtils.uninstallApp(SHARED_PERMS_BT_PKG);

            // now install the app and see if when we launch it we get a
            // permissions error
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    SHARED_PERMS_APK), SHARED_PERMS_PKG, false);

            boolean testsPassed = mPMHostUtils.runDeviceTestsDidAllTestsPass(SHARED_PERMS_PKG);
            assertEquals("Shared perms app should fail to run", false, testsPassed);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(SHARED_PERMS_PKG);
        }
    }

    /**
     * Regression test to verify that a shared app with no explicit permissions
     * can run if its other shared apps are installed.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAndLaunchSharedPermsAppOnSD_GrantedPerms() throws Exception {
        Log.i(LOG_TAG, "Test launching an app with no explicit perms set, installed on SD card");

        try {
            // install the 2 shared apps with needed permissions first
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    SHARED_PERMS_FL_APK), SHARED_PERMS_FL_PKG, false);
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    SHARED_PERMS_BT_APK), SHARED_PERMS_BT_PKG, false);

            // now install the test app and see if we can launch it without
            // errors
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    SHARED_PERMS_APK), SHARED_PERMS_PKG, false);
            boolean testsPassed = mPMHostUtils.runDeviceTestsDidAllTestsPass(SHARED_PERMS_PKG);
            assert (testsPassed);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(SHARED_PERMS_PKG);
            mPMHostUtils.uninstallApp(SHARED_PERMS_BT_PKG);
            mPMHostUtils.uninstallApp(SHARED_PERMS_FL_PKG);
        }
    }

    /**
     * Regression test to verify that an app with ACCESS_FINE_LOCATION (GPS)
     * permissions can run without permissions errors even after a reboot
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAndLaunchFLPermsAppOnSD_Reboot() throws Exception {
        Log.i(LOG_TAG, "Test launching an app with location perms set, installed on SD card");

        try {
            // install the app and verify we can launch it without permissions
            // errors
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    SHARED_PERMS_FL_APK), SHARED_PERMS_FL_PKG, false);
            boolean testsPassed = mPMHostUtils.runDeviceTestsDidAllTestsPass(SHARED_PERMS_FL_PKG);
            assert (testsPassed);

            getDevice().reboot();

            testsPassed = mPMHostUtils.runDeviceTestsDidAllTestsPass(SHARED_PERMS_FL_PKG);
            assert (testsPassed);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(SHARED_PERMS_FL_PKG);
        }
    }

    /**
     * Regression test to verify that a shared app with no explicit permissions
     * can run if its other shared apps are installed, even after a reboot.
     * <p/>
     * Assumes adb is running as root in device under test.
     */
    public void testInstallAndLaunchSharedPermsAppOnSD_Reboot() throws Exception {
        Log.i(LOG_TAG, "Test launching an app on SD, with no explicit perms set after reboot");

        try {
            // install the 2 shared apps with needed permissions first
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    SHARED_PERMS_FL_APK), SHARED_PERMS_FL_PKG, false);
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    SHARED_PERMS_BT_APK), SHARED_PERMS_BT_PKG, false);

            // now install the test app and see if we can launch it without
            // errors
            mPMHostUtils.installAppAndVerifyExistsOnSDCard(getTestAppFilePath(
                    SHARED_PERMS_APK), SHARED_PERMS_PKG, false);
            boolean testsPassed = mPMHostUtils.runDeviceTestsDidAllTestsPass(SHARED_PERMS_PKG);
            assert (testsPassed);

            // reboot
            getDevice().reboot();

            // Verify we can still launch the app
            testsPassed = mPMHostUtils.runDeviceTestsDidAllTestsPass(SHARED_PERMS_PKG);
            assert (testsPassed);
        }
        // cleanup test app
        finally {
            mPMHostUtils.uninstallApp(SHARED_PERMS_PKG);
            mPMHostUtils.uninstallApp(SHARED_PERMS_BT_PKG);
            mPMHostUtils.uninstallApp(SHARED_PERMS_FL_PKG);
        }
    }

    public void testInstallApk32bit() throws DeviceNotAvailableException {
        try {
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK_32), true);
            assertEquals(ARMEABI_V7A, mPMHostUtils.getAbi(SHARED_UID_PKG_32));
        } finally {
            mPMHostUtils.uninstallApp(SHARED_UID_PKG_32);
        }

    }

    public void testInstallApk64bit() throws DeviceNotAvailableException {
        try {
            if (!ARM64_V8A.equals(getDevice().getProperty(ABI_PROPERTY))) {
                return;
            }
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK_64), true);
            assertEquals(ARM64_V8A, mPMHostUtils.getAbi(SHARED_UID_PKG_64));
        } finally {
            mPMHostUtils.uninstallApp(SHARED_UID_PKG_64);
        }

    }

    public void testInstallApkDualAbi() throws DeviceNotAvailableException {
        try {
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK_DUAL), true);
            assertEquals(getDevice().getProperty(ABI_PROPERTY),
                    mPMHostUtils.getAbi(SHARED_UID_PKG_DUAL));
        } finally {
            mPMHostUtils.uninstallApp(SHARED_UID_PKG_DUAL);
        }
    }

    public void testInstallSharedUid32() throws DeviceNotAvailableException {
        try{
            if (!ARMEABI_V7A.equals(getDevice().getProperty(ABI_PROPERTY))) {
                return;
            }
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK), true);
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK_32), true);
            assertEquals(mPMHostUtils.getUid(SHARED_UID_PKG),
                    mPMHostUtils.getUid(SHARED_UID_PKG_32));
        } finally {
            mPMHostUtils.uninstallApp(SHARED_UID_PKG);
            mPMHostUtils.uninstallApp(SHARED_UID_PKG_32);
        }
    }

    public void testInstallSharedUid64() throws DeviceNotAvailableException {
        try{
            if (!ARM64_V8A.equals(getDevice().getProperty(ABI_PROPERTY))) {
                return;
            }
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK), true);
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK_64), true);
            assertEquals(mPMHostUtils.getUid(SHARED_UID_PKG),
                    mPMHostUtils.getUid(SHARED_UID_PKG_64));
        } finally {
            mPMHostUtils.uninstallApp(SHARED_UID_PKG);
            mPMHostUtils.uninstallApp(SHARED_UID_PKG_64);
        }
    }

    public void testInstallSharedUidDual64() throws DeviceNotAvailableException {
        try{
            if (!ARM64_V8A.equals(getDevice().getProperty(ABI_PROPERTY))) {
                return;
            }
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK_DUAL), true);
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK_64), true);
            assertEquals(mPMHostUtils.getUid(SHARED_UID_PKG_DUAL),
                    mPMHostUtils.getUid(SHARED_UID_PKG_64));
        } finally {
            mPMHostUtils.uninstallApp(SHARED_UID_PKG_DUAL);
            mPMHostUtils.uninstallApp(SHARED_UID_PKG_64);
        }
    }

    public void testInstallSharedUidDual32() throws DeviceNotAvailableException {
        try{
            if (!ARMEABI_V7A.equals(getDevice().getProperty(ABI_PROPERTY))) {
                return;
            }
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK_DUAL), true);
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK_32), true);
            assertEquals(mPMHostUtils.getUid(SHARED_UID_PKG_DUAL),
                    mPMHostUtils.getUid(SHARED_UID_PKG_32));
        } finally {
            mPMHostUtils.uninstallApp(SHARED_UID_PKG_DUAL);
            mPMHostUtils.uninstallApp(SHARED_UID_PKG_32);
        }
    }

    public void testInstallSharedUidJavaDual() throws DeviceNotAvailableException {
        try{
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK_DUAL), true);
            mPMHostUtils.installFile(getTestAppFilePath(SHARED_UID_APK), true);
            assertEquals(mPMHostUtils.getUid(SHARED_UID_PKG_DUAL),
                    mPMHostUtils.getUid(SHARED_UID_PKG));
        } finally {
            mPMHostUtils.uninstallApp(SHARED_UID_PKG_DUAL);
            mPMHostUtils.uninstallApp(SHARED_UID_PKG);
        }
    }
}
