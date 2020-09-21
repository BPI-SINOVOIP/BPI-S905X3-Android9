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
package com.android.timezone.xts;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.util.FileUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.util.function.BooleanSupplier;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

/**
 * Class for host-side tests that the time zone rules update feature works as intended. This is
 * intended to give confidence to OEMs that they have implemented / configured the OEM parts of the
 * feature correctly.
 *
 * <p>There are two main operations involved in time zone updates:
 * <ol>
 *     <li>Package installs/uninstalls - asynchronously stage operations for install</li>
 *     <li>Reboots - perform the staged operations / delete bad installed data</li>
 * </ol>
 * Both these operations are time consuming and there's a degree of non-determinism involved.
 *
 * <p>A "clean" device can also be in one of two main states depending on whether it has been wiped
 * and/or rebooted before this test runs:
 * <ul>
 *     <li>A device may have nothing staged / installed in /data/misc/zoneinfo at all.</li>
 *     <li>A device may have the time zone data from the default system image version of the time
 *     zone data app staged or installed.</li>
 * </ul>
 * This test attempts to handle both of these cases.
 *
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class TimeZoneUpdateHostTest implements IDeviceTest, IBuildReceiver {

    // These must match equivalent values in RulesManagerService dumpsys code.
    private static final String STAGED_OPERATION_NONE = "None";
    private static final String STAGED_OPERATION_INSTALL = "Install";
    private static final String STAGED_OPERATION_UNINSTALL = "Uninstall";
    private static final String INSTALL_STATE_INSTALLED = "Installed";

    private IBuildInfo mBuildInfo;
    private ITestDevice mDevice;
    private File mTempDir;

    @Option(name = "oem-data-app-package-name",
            description="The OEM-specific package name for the data app",
            mandatory = true)
    private String mOemDataAppPackageName;

    private String getTimeZoneDataPackageName() {
        assertNotNull(mOemDataAppPackageName);
        return mOemDataAppPackageName;
    }

    @Option(name = "oem-data-app-apk-prefix",
            description="The OEM-specific APK name for the data app test files, e.g."
                    + "for TimeZoneDataOemCorp_test1.apk the prefix would be"
                    + "\"TimeZoneDataOemCorp\"",
            mandatory = true)
    private String mOemDataAppApkPrefix;

    private String getTimeZoneDataApkName(String testId) {
        assertNotNull(mOemDataAppApkPrefix);
        return mOemDataAppApkPrefix + "_" + testId + ".apk";
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    @Before
    public void setUp() throws Exception {
        createTempDir();
        resetDeviceToClean();
    }

    @After
    public void tearDown() throws Exception {
        resetDeviceToClean();
        deleteTempDir();
    }

    private void createTempDir() throws Exception {
        mTempDir = File.createTempFile("timeZoneUpdateTest", null);
        assertTrue(mTempDir.delete());
        assertTrue(mTempDir.mkdir());
    }

    private void deleteTempDir() throws Exception {
        FileUtil.recursiveDelete(mTempDir);
    }

    /**
     * Reset the device to having no installed time zone data outside of the /system/priv-app
     * version that came with the system image.
     */
    private void resetDeviceToClean() throws Exception {
        // If this fails the data app isn't present on device. No point in starting.
        assertTrue(getTimeZoneDataPackageName() + " not installed",
                isPackageInstalled(getTimeZoneDataPackageName()));

        // Reboot as needed to apply any staged operation.
        if (!STAGED_OPERATION_NONE.equals(getStagedOperationType())) {
            rebootDeviceAndWaitForRestart();
        }

        // A "clean" device means no time zone data .apk installed in /data at all, try to get to
        // that state.
        for (int i = 0; i < 2; i++) {
            logDeviceTimeZoneState();

            // Even if there's no distro installed, there may be an updated APK installed, so try to
            // remove it unconditionally.
            String errorCode = uninstallPackage(getTimeZoneDataPackageName());
            if (errorCode != null) {
                // Failed to uninstall, which we take to mean the device is "clean".
                break;
            }
            // Success, meaning there was an APK that could be uninstalled.
            // If there is a distro installed we need wait for the distro uninstall that should now
            // become staged.
            boolean distroIsInstalled = INSTALL_STATE_INSTALLED.equals(getCurrentInstallState());
            if (distroIsInstalled) {
                // It may take a short while before we can detect anything: the package manager
                // should have triggered an intent, and the PackageTracker has to receive that and
                // send its own intent, which then has to be acted on before we could detect an
                // operation in progress. We expect the device eventually to get to the staged state
                // "UNINSTALL", meaning it will try to revert to no distro installed on next boot.
                waitForStagedUninstall();

                rebootDeviceAndWaitForRestart();
            } else {
                // There was an apk installed, but no time zone distro was installed. It was
                // probably a "bad" .apk that was rejected. The update app will request an uninstall
                // anyway just to be sure, so we'll give it a chance to do that before continuing
                // otherwise we could get an "operation in progress" later on when we're not
                // expecting it.
                Thread.sleep(10000);
            }
        }
        assertActiveRulesVersion(getSystemRulesVersion());
        assertEquals(STAGED_OPERATION_NONE, getStagedOperationType());
    }

    @Test
    public void testInstallNewerRulesVersion() throws Exception {
        // This information must match the rules version in test1: IANA version=2030a, revision=1
        String test1VersionInfo = "2030a,1";

        // Confirm the staged / install state before we start.
        assertFalse(test1VersionInfo.equals(getCurrentInstalledVersion()));
        assertEquals(STAGED_OPERATION_NONE, getStagedOperationType());

        File appFile = getTimeZoneDataApkFile("test1");
        getDevice().installPackage(appFile, true /* reinstall */);

        waitForStagedInstall(test1VersionInfo);

        // Confirm the install state hasn't changed.
        assertFalse(test1VersionInfo.equals(getCurrentInstalledVersion()));

        // Now reboot, and the staged version should become the installed version.
        rebootDeviceAndWaitForRestart();

        // After reboot, check the state.
        assertEquals(STAGED_OPERATION_NONE, getStagedOperationType());
        assertEquals(INSTALL_STATE_INSTALLED, getCurrentInstallState());
        assertEquals(test1VersionInfo, getCurrentInstalledVersion());
    }

    @Test
    public void testInstallNewerRulesVersion_secondaryUser() throws Exception {
        ITestDevice device = getDevice();
        if (!device.isMultiUserSupported()) {
            // Just pass on non-multi-user devices.
            return;
        }

        int userId = device.createUser("TimeZoneTest", false /* guest */, false /* ephemeral */);
        try {

            // This information must match the rules version in test1: IANA version=2030a, revision=1
            String test1VersionInfo = "2030a,1";

            // Confirm the staged / install state before we start.
            assertFalse(test1VersionInfo.equals(getCurrentInstalledVersion()));
            assertEquals(STAGED_OPERATION_NONE, getStagedOperationType());

            File appFile = getTimeZoneDataApkFile("test1");

            // Install the app for the test user. It should still all work.
            device.installPackageForUser(appFile, true /* reinstall */, userId);

            waitForStagedInstall(test1VersionInfo);

            // Confirm the install state hasn't changed.
            assertFalse(test1VersionInfo.equals(getCurrentInstalledVersion()));

            // Now reboot, and the staged version should become the installed version.
            rebootDeviceAndWaitForRestart();

            // After reboot, check the state.
            assertEquals(STAGED_OPERATION_NONE, getStagedOperationType());
            assertEquals(INSTALL_STATE_INSTALLED, getCurrentInstallState());
            assertEquals(test1VersionInfo, getCurrentInstalledVersion());
        }
        finally {
            // If this fails, the device may be left in a bad state.
            device.removeUser(userId);
        }
    }

    @Test
    public void testInstallOlderRulesVersion() throws Exception {
        File appFile = getTimeZoneDataApkFile("test2");
        getDevice().installPackage(appFile, true /* reinstall */);

        // The attempt to install a version of the data that is older than the version in the system
        // image should be rejected and nothing should be staged. There's currently no way (short of
        // looking at logs) to tell this has happened, but combined with other tests and given a
        // suitable delay it gives us some confidence that the attempt has been made and it was
        // rejected.

        Thread.sleep(30000);

        assertEquals(STAGED_OPERATION_NONE, getStagedOperationType());
    }

    private void rebootDeviceAndWaitForRestart() throws Exception {
        log("Rebooting device");
        getDevice().reboot();
    }

    private void logDeviceTimeZoneState() throws Exception {
        log("Initial device state: " + dumpEntireTimeZoneStatusToString());
    }

    private static void log(String msg) {
        LogUtil.CLog.i(msg);
    }

    private void assertActiveRulesVersion(String expectedRulesVersion) throws Exception {
        // Dumpsys reports the version reported by ICU, ZoneInfoDB and TimeZoneFinder and they
        // should always match.
        String expectedActiveRulesVersion =
                expectedRulesVersion + "," + expectedRulesVersion + "," + expectedRulesVersion;

        String actualActiveRulesVersion =
                waitForNoOperationInProgressAndReturn(StateType.ACTIVE_RULES_VERSION);
        assertEquals(expectedActiveRulesVersion, actualActiveRulesVersion);
    }

    private String getCurrentInstalledVersion() throws Exception {
        return waitForNoOperationInProgressAndReturn(StateType.CURRENTLY_INSTALLED_VERSION);
    }

    private String getCurrentInstallState() throws Exception {
        return waitForNoOperationInProgressAndReturn(StateType.CURRENT_INSTALL_STATE);
    }

    private String getStagedInstallVersion() throws Exception {
        return waitForNoOperationInProgressAndReturn(StateType.STAGED_INSTALL_VERSION);
    }

    private String getStagedOperationType() throws Exception {
        return waitForNoOperationInProgressAndReturn(StateType.STAGED_OPERATION_TYPE);
    }

    private String getSystemRulesVersion() throws Exception {
        return waitForNoOperationInProgressAndReturn(StateType.SYSTEM_RULES_VERSION);
    }

    private boolean isOperationInProgress() {
        try {
            String operationInProgressString =
                    getDeviceTimeZoneState(StateType.OPERATION_IN_PROGRESS);
            return Boolean.parseBoolean(operationInProgressString);
        } catch (Exception e) {
            throw new AssertionError("Failed to read staged status", e);
        }
    }

    private String waitForNoOperationInProgressAndReturn(StateType stateType) throws Exception {
        waitForCondition(() -> !isOperationInProgress());
        return getDeviceTimeZoneState(stateType);
    }

    private void waitForStagedUninstall() throws Exception {
        waitForCondition(() -> isStagedUninstall());
    }

    private void waitForStagedInstall(String versionString) throws Exception {
        waitForCondition(() -> isStagedInstall(versionString));
    }

    private boolean isStagedUninstall() {
        try {
            return getStagedOperationType().equals(STAGED_OPERATION_UNINSTALL);
        } catch (Exception e) {
            throw new AssertionError("Failed to read staged status", e);
        }
    }

    private boolean isStagedInstall(String versionString) {
        try {
            return getStagedOperationType().equals(STAGED_OPERATION_INSTALL)
                    && getStagedInstallVersion().equals(versionString);
        } catch (Exception e) {
            throw new AssertionError("Failed to read staged status", e);
        }
    }

    private static void waitForCondition(BooleanSupplier condition) throws Exception {
        int count = 0;
        boolean lastResult;
        while (!(lastResult = condition.getAsBoolean()) && count++ < 120) {
            Thread.sleep(1000);
        }
        // Some conditions may not be stable so using the lastResult instead of
        // condition.getAsBoolean() ensures we understand why we exited the loop.
        assertTrue("Failed condition: " + condition, lastResult);
    }

    private enum StateType {
        OPERATION_IN_PROGRESS,
        SYSTEM_RULES_VERSION,
        CURRENT_INSTALL_STATE,
        CURRENTLY_INSTALLED_VERSION,
        STAGED_OPERATION_TYPE,
        STAGED_INSTALL_VERSION,
        ACTIVE_RULES_VERSION;

        public String getFormatStateChar() {
            // This switch must match values in com.android.server.timezone.RulesManagerService.
            switch (this) {
                case OPERATION_IN_PROGRESS:
                    return "p";
                case SYSTEM_RULES_VERSION:
                    return "s";
                case CURRENT_INSTALL_STATE:
                    return "c";
                case CURRENTLY_INSTALLED_VERSION:
                    return "i";
                case STAGED_OPERATION_TYPE:
                    return "o";
                case STAGED_INSTALL_VERSION:
                    return "t";
                case ACTIVE_RULES_VERSION:
                    return "a";
                default:
                    throw new AssertionError("Unknown state type: " + this);
            }
        }
    }

    private String getDeviceTimeZoneState(StateType stateType) throws Exception {
        String output = getDevice().executeShellCommand(
                "dumpsys timezone -format_state " + stateType.getFormatStateChar());
        assertNotNull(output);
        // Output will be "Foo: bar\n". We want the "bar".
        String value = output.split(":")[1];
        return value.substring(1, value.length() - 1);
    }

    private String dumpEntireTimeZoneStatusToString() throws Exception {
        String output = getDevice().executeShellCommand("dumpsys timezone");
        assertNotNull(output);
        return output;
    }

    private File getTimeZoneDataApkFile(String testId) throws Exception {
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mBuildInfo);
        String fileName = getTimeZoneDataApkName(testId);
        return buildHelper.getTestFile(fileName);
    }

    private boolean isPackageInstalled(String pkg) throws Exception {
        for (String installedPackage : getDevice().getInstalledPackageNames()) {
            if (pkg.equals(installedPackage)) {
                return true;
            }
        }
        return false;
    }

    private String uninstallPackage(String packageName) throws Exception {
        return getDevice().uninstallPackage(packageName);
    }
}
