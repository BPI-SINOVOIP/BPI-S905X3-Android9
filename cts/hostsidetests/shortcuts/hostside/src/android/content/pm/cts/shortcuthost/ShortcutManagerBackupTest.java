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
package android.content.pm.cts.shortcuthost;


import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;

public class ShortcutManagerBackupTest extends BaseShortcutManagerHostTest {
    private static final String LAUNCHER1_APK = "CtsShortcutBackupLauncher1.apk";
    private static final String LAUNCHER2_APK = "CtsShortcutBackupLauncher2.apk";
    private static final String LAUNCHER3_APK = "CtsShortcutBackupLauncher3.apk";
    private static final String LAUNCHER4_OLD_APK = "CtsShortcutBackupLauncher4old.apk";
    private static final String LAUNCHER4_NEW_APK = "CtsShortcutBackupLauncher4new.apk";

    private static final String PUBLISHER1_APK = "CtsShortcutBackupPublisher1.apk";
    private static final String PUBLISHER2_APK = "CtsShortcutBackupPublisher2.apk";
    private static final String PUBLISHER3_APK = "CtsShortcutBackupPublisher3.apk";
    private static final String PUBLISHER4_OLD_APK = "CtsShortcutBackupPublisher4old.apk";
    private static final String PUBLISHER4_NEW_APK = "CtsShortcutBackupPublisher4new.apk";
    private static final String PUBLISHER4_NEW_NOBACKUP_APK
            = "CtsShortcutBackupPublisher4new_nobackup.apk";
    private static final String PUBLISHER4_NEW_WRONGKEY_APK
            = "CtsShortcutBackupPublisher4new_wrongkey.apk";
    private static final String PUBLISHER4_OLD_NO_MANIFST_APK
            = "CtsShortcutBackupPublisher4old_nomanifest.apk";
    private static final String PUBLISHER4_NEW_NO_MANIFST_APK
            = "CtsShortcutBackupPublisher4new_nomanifest.apk";

    private static final String LAUNCHER1_PKG =
            "android.content.pm.cts.shortcut.backup.launcher1";
    private static final String LAUNCHER2_PKG =
            "android.content.pm.cts.shortcut.backup.launcher2";
    private static final String LAUNCHER3_PKG =
            "android.content.pm.cts.shortcut.backup.launcher3";
    private static final String LAUNCHER4_PKG =
            "android.content.pm.cts.shortcut.backup.launcher4";

    private static final String PUBLISHER1_PKG =
            "android.content.pm.cts.shortcut.backup.publisher1";
    private static final String PUBLISHER2_PKG =
            "android.content.pm.cts.shortcut.backup.publisher2";
    private static final String PUBLISHER3_PKG =
            "android.content.pm.cts.shortcut.backup.publisher3";
    private static final String PUBLISHER4_PKG =
            "android.content.pm.cts.shortcut.backup.publisher4";

    private static final int BROADCAST_TIMEOUT_SECONDS = 120;

    private static final String FEATURE_BACKUP = "android.software.backup";

    private boolean mSupportsBackup;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mSupportsBackup = getDevice().hasFeature(FEATURE_BACKUP);

        if (mSupportsBackup) {
            clearShortcuts(LAUNCHER1_PKG, getPrimaryUserId());
            clearShortcuts(LAUNCHER2_PKG, getPrimaryUserId());
            clearShortcuts(LAUNCHER3_PKG, getPrimaryUserId());
            clearShortcuts(LAUNCHER4_PKG, getPrimaryUserId());

            clearShortcuts(PUBLISHER1_PKG, getPrimaryUserId());
            clearShortcuts(PUBLISHER2_PKG, getPrimaryUserId());
            clearShortcuts(PUBLISHER3_PKG, getPrimaryUserId());
            clearShortcuts(PUBLISHER4_PKG, getPrimaryUserId());

            uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER1_PKG);
            uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER2_PKG);
            uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER3_PKG);
            uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER4_PKG);

            uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER1_PKG);
            uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER2_PKG);
            uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER3_PKG);
            uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER4_PKG);

            waitUntilPackagesGone();
        }
    }

    @Override
    protected void tearDown() throws Exception {
        if (DUMPSYS_IN_TEARDOWN) {
            dumpsys("tearDown");
        }

        if (mSupportsBackup && !NO_UNINSTALL_IN_TEARDOWN) {
            getDevice().uninstallPackage(LAUNCHER1_PKG);
            getDevice().uninstallPackage(LAUNCHER2_PKG);
            getDevice().uninstallPackage(LAUNCHER3_PKG);
            getDevice().uninstallPackage(LAUNCHER4_PKG);

            getDevice().uninstallPackage(PUBLISHER1_PKG);
            getDevice().uninstallPackage(PUBLISHER2_PKG);
            getDevice().uninstallPackage(PUBLISHER3_PKG);
            getDevice().uninstallPackage(PUBLISHER4_PKG);
        }

        super.tearDown();
    }

    private void doBackup() throws Exception {
        CLog.i("Backing up package android...");

        waitUntilBroadcastsDrain(); // b/64203677

        CLog.i("Making sure the local transport is selected...");
        assertContainsRegex(
                "^Selected transport android/com.android.internal.backup.LocalTransport",
                executeShellCommandWithLog(
                        "bmgr transport android/com.android.internal.backup.LocalTransport"));

        executeShellCommandWithLog("dumpsys backup");

        assertContainsRegex(
                "Wiped",
                executeShellCommandWithLog(
                        "bmgr wipe android/com.android.internal.backup.LocalTransport android"));

        assertContainsRegex(
                "Backup finished with result: Success",
                executeShellCommandWithLog("bmgr backupnow android"));

    }

    private void doRestore() throws DeviceNotAvailableException {
        CLog.i("Restoring package android...");

        assertContainsRegex(
                "\\bdone\\b",
                executeShellCommandWithLog("bmgr restore 1 android"));

    }

    private void uninstallPackageAndWaitUntilBroadcastsDrain(String pkg) throws Exception {
        getDevice().uninstallPackage(pkg);
        waitUntilBroadcastsDrain();
    }

    /**
     * Wait until the broadcasts queues all drain.
     */
    private void waitUntilBroadcastsDrain() throws Exception {
        final long TIMEOUT = System.nanoTime() +
                TimeUnit.SECONDS.toNanos(BROADCAST_TIMEOUT_SECONDS);

        final Pattern re = Pattern.compile("^\\s+Active (ordered)? broadcasts \\[",
                Pattern.MULTILINE);

        String dumpsys = "";
        while (System.nanoTime() < TIMEOUT) {
            Thread.sleep(1000);

            dumpsys = getDevice().executeShellCommand("dumpsys activity broadcasts");

            if (re.matcher(dumpsys).find()) {
                continue;
            }

            CLog.d("Broadcast queues drained:\n" + dumpsys);

            dumpsys("Broadcast queues drained");

            // All packages gone.
            return;
        }
        fail("Broadcast queues didn't drain before time out."
                + " Last dumpsys=\n" + dumpsys);
    }

    /**
     * Wait until all the test packages are forgotten by the shortcut manager.
     */
    private void waitUntilPackagesGone() throws Exception {
        CLog.i("Waiting until all packages are removed from shortcut manager...");

        final String packages[] = {
                LAUNCHER1_PKG,  LAUNCHER2_PKG, LAUNCHER3_PKG, LAUNCHER4_PKG,
                PUBLISHER1_PKG, PUBLISHER2_PKG, PUBLISHER3_PKG, PUBLISHER4_PKG,
        };

        String dumpsys = "";
        final long TIMEOUT = System.nanoTime() +
                TimeUnit.SECONDS.toNanos(BROADCAST_TIMEOUT_SECONDS);

        while (System.nanoTime() < TIMEOUT) {
            Thread.sleep(2000);
            dumpsys = getDevice().executeShellCommand("dumpsys shortcut");

            if (dumpsys.contains("Launcher: " + LAUNCHER1_PKG)) continue;
            if (dumpsys.contains("Launcher: " + LAUNCHER2_PKG)) continue;
            if (dumpsys.contains("Launcher: " + LAUNCHER3_PKG)) continue;
            if (dumpsys.contains("Launcher: " + LAUNCHER4_PKG)) continue;
            if (dumpsys.contains("Package: " + PUBLISHER1_PKG)) continue;
            if (dumpsys.contains("Package: " + PUBLISHER2_PKG)) continue;
            if (dumpsys.contains("Package: " + PUBLISHER3_PKG)) continue;
            if (dumpsys.contains("Package: " + PUBLISHER4_PKG)) continue;

            dumpsys("Shortcut manager handled broadcasts");

            // All packages gone.
            return;
        }
        fail("ShortcutManager didn't handle all expected broadcasts before time out."
                + " Last dumpsys=\n" + dumpsys);
    }

    public void testBackupAndRestore() throws Exception {
        if (!mSupportsBackup) {
            return;
        }
        dumpsys("Test start");

        installAppAsUser(LAUNCHER1_APK, getPrimaryUserId());
        installAppAsUser(LAUNCHER2_APK, getPrimaryUserId());
        installAppAsUser(LAUNCHER3_APK, getPrimaryUserId());

        installAppAsUser(PUBLISHER1_APK, getPrimaryUserId());
        installAppAsUser(PUBLISHER2_APK, getPrimaryUserId());
        installAppAsUser(PUBLISHER3_APK, getPrimaryUserId());

        // Prepare shortcuts
        runDeviceTestsAsUser(PUBLISHER1_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());
        runDeviceTestsAsUser(PUBLISHER2_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());
        runDeviceTestsAsUser(PUBLISHER3_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());

        runDeviceTestsAsUser(LAUNCHER1_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());
        runDeviceTestsAsUser(LAUNCHER2_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());
        runDeviceTestsAsUser(LAUNCHER3_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());

        // Tweak shortcuts a little bit to make disabled shortcuts.
        runDeviceTestsAsUser(PUBLISHER2_PKG, ".ShortcutManagerPreBackup2Test", getPrimaryUserId());

        dumpsys("Before backup");

        // Backup
        doBackup();

        // Uninstall all apps
        uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER1_PKG);
        uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER2_PKG);
        uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER3_PKG);
        uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER1_PKG);
        uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER2_PKG);
        uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER3_PKG);


        // Make sure the shortcut service handled all the uninstall broadcasts.
        waitUntilPackagesGone();

        // Do it one more time just in case...
        waitUntilBroadcastsDrain();

        // Then restore
        doRestore();

        dumpsys("After restore");

        // First, restore launcher 1, which shouldn't see any shortcuts from the packages yet.
        installAppAsUser(LAUNCHER1_APK, getPrimaryUserId());
        runDeviceTestsAsUser(LAUNCHER1_PKG, ".ShortcutManagerPostBackupTest",
                "testWithUninstall_beforeAppRestore",
                getPrimaryUserId());

        // Restore the apps.  Even though launcher 2 hasn't been re-installed yet, they should
        // still have pinned shortcuts by launcher 2.
        installAppAsUser(PUBLISHER1_APK, getPrimaryUserId());
        installAppAsUser(PUBLISHER2_APK, getPrimaryUserId());
        installAppAsUser(PUBLISHER3_APK, getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER1_PKG, ".ShortcutManagerPostBackupTest",
                "testWithUninstall",
                getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER2_PKG, ".ShortcutManagerPostBackupTest",
                "testWithUninstall",
                getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER3_PKG, ".ShortcutManagerPostBackupTest",
                "testWithUninstall",
                getPrimaryUserId());

        // Now launcher 1 should see shortcuts from these packages.
        runDeviceTestsAsUser(LAUNCHER1_PKG, ".ShortcutManagerPostBackupTest",
                "testWithUninstall_afterAppRestore",
                getPrimaryUserId());

        // Then restore launcher 2 and check.
        installAppAsUser(LAUNCHER2_APK, getPrimaryUserId());
        runDeviceTestsAsUser(LAUNCHER2_PKG, ".ShortcutManagerPostBackupTest",
                "testWithUninstall_afterAppRestore",
                getPrimaryUserId());


        // Run the same package side check.  The result should be the same.
        runDeviceTestsAsUser(PUBLISHER1_PKG, ".ShortcutManagerPostBackupTest",
                "testWithUninstall",
                getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER2_PKG, ".ShortcutManagerPostBackupTest",
                "testWithUninstall",
                getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER3_PKG, ".ShortcutManagerPostBackupTest",
                "testWithUninstall",
                getPrimaryUserId());
    }

    public void testBackupAndRestore_downgrade() throws Exception {
        if (!mSupportsBackup) {
            return;
        }
        dumpsys("Test start");

        // First, publish shortcuts from the new version and pin them.

        installAppAsUser(PUBLISHER4_NEW_APK, getPrimaryUserId());
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());
        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());

        dumpsys("Before backup");

        // Backup
        doBackup();

        // Uninstall all apps
        uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER4_PKG);
        uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER4_PKG);

        // Make sure the shortcut service handled all the uninstall broadcasts.
        waitUntilPackagesGone();

        // Do it one more time just in case...
        waitUntilBroadcastsDrain();

        // Then restore
        doRestore();

        dumpsys("After restore");

        // Restore the old version of the app, and the launcher.
        // (But we don't check the launcher's version, so using old is fine.)
        installAppAsUser(LAUNCHER4_OLD_APK, getPrimaryUserId());
        installAppAsUser(PUBLISHER4_OLD_APK, getPrimaryUserId());
        waitUntilBroadcastsDrain();

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testRestoredOnOldVersion",
                getPrimaryUserId());

        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testRestoredOnOldVersion",
                getPrimaryUserId());

        // New install the original version. All blocked shortcuts should re-appear.
        installAppAsUser(PUBLISHER4_NEW_APK, getPrimaryUserId());
        waitUntilBroadcastsDrain();

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testRestoredOnNewVersion",
                getPrimaryUserId());

        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testRestoredOnNewVersion",
                getPrimaryUserId());

    }

    public void testBackupAndRestore_backupWasDisabled() throws Exception {
        if (!mSupportsBackup) {
            return;
        }
        dumpsys("Test start");

        // First, publish shortcuts from "nobackup" version.

        installAppAsUser(PUBLISHER4_NEW_NOBACKUP_APK, getPrimaryUserId());
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());
        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());

        dumpsys("Before backup");

        // Backup
        doBackup();

        // Uninstall all apps
        uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER4_PKG);
        uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER4_PKG);

        // Make sure the shortcut service handled all the uninstall broadcasts.
        waitUntilPackagesGone();

        // Do it one more time just in case...
        waitUntilBroadcastsDrain();

        // Then restore
        doRestore();

        dumpsys("After restore");

        // Install the "backup-ok" version. But restoration is limited.
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());
        installAppAsUser(PUBLISHER4_NEW_APK, getPrimaryUserId());
        waitUntilBroadcastsDrain();

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testBackupDisabled",
                getPrimaryUserId());
    }

    public void testBackupAndRestore_backupIsDisabled() throws Exception {
        if (!mSupportsBackup) {
            return;
        }
        dumpsys("Test start");

        // First, publish shortcuts from backup-ok version.

        installAppAsUser(PUBLISHER4_NEW_APK, getPrimaryUserId());
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());
        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());

        dumpsys("Before backup");

        // Backup
        doBackup();

        // Uninstall all apps
        uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER4_PKG);
        uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER4_PKG);

        // Make sure the shortcut service handled all the uninstall broadcasts.
        waitUntilPackagesGone();

        // Do it one more time just in case...
        waitUntilBroadcastsDrain();

        // Then restore
        doRestore();

        dumpsys("After restore");

        // Install the nobackup version. Restoration is limited.
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());
        installAppAsUser(PUBLISHER4_NEW_NOBACKUP_APK, getPrimaryUserId());
        waitUntilBroadcastsDrain();

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testBackupDisabled",
                getPrimaryUserId());
    }

    public void testBackupAndRestore_wrongKey() throws Exception {
        if (!mSupportsBackup) {
            return;
        }
        dumpsys("Test start");

        // First, publish shortcuts from backup-ok version.

        installAppAsUser(PUBLISHER4_NEW_APK, getPrimaryUserId());
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());
        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());

        dumpsys("Before backup");

        // Backup
        doBackup();

        // Uninstall all apps
        uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER4_PKG);
        uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER4_PKG);

        // Make sure the shortcut service handled all the uninstall broadcasts.
        waitUntilPackagesGone();

        // Do it one more time just in case...
        waitUntilBroadcastsDrain();

        // Then restore
        doRestore();

        dumpsys("After restore");

        // Install the nobackup version. Restoration is limited.
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());
        installAppAsUser(PUBLISHER4_NEW_WRONGKEY_APK, getPrimaryUserId());
        waitUntilBroadcastsDrain();

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testRestoreWrongKey",
                getPrimaryUserId());

        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testRestoreWrongKey",
                getPrimaryUserId());
    }

    public void testBackupAndRestore_noManifestOnOldVersion() throws Exception {
        if (!mSupportsBackup) {
            return;
        }
        dumpsys("Test start");

        // First, publish shortcuts from backup-ok version.

        installAppAsUser(PUBLISHER4_NEW_APK, getPrimaryUserId());
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());
        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());

        dumpsys("Before backup");

        // Backup
        doBackup();

        // Uninstall all apps
        uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER4_PKG);
        uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER4_PKG);

        // Make sure the shortcut service handled all the uninstall broadcasts.
        waitUntilPackagesGone();

        // Do it one more time just in case...
        waitUntilBroadcastsDrain();

        // Then restore
        doRestore();

        dumpsys("After restore");

        // Install the nobackup version. Restoration is limited.
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());
        installAppAsUser(PUBLISHER4_OLD_NO_MANIFST_APK, getPrimaryUserId());
        waitUntilBroadcastsDrain();

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testRestoreNoManifestOnOldVersion",
                getPrimaryUserId());

        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testRestoreNoManifestOnOldVersion",
                getPrimaryUserId());
    }

    public void testBackupAndRestore_noManifestOnNewVersion() throws Exception {
        if (!mSupportsBackup) {
            return;
        }
        dumpsys("Test start");

        // First, publish shortcuts from backup-ok version.

        installAppAsUser(PUBLISHER4_NEW_APK, getPrimaryUserId());
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());
        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());

        dumpsys("Before backup");

        // Backup
        doBackup();

        // Uninstall all apps
        uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER4_PKG);
        uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER4_PKG);

        // Make sure the shortcut service handled all the uninstall broadcasts.
        waitUntilPackagesGone();

        // Do it one more time just in case...
        waitUntilBroadcastsDrain();

        // Then restore
        doRestore();

        dumpsys("After restore");

        // Install the nobackup version. Restoration is limited.
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());
        installAppAsUser(PUBLISHER4_NEW_NO_MANIFST_APK, getPrimaryUserId());
        waitUntilBroadcastsDrain();

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testRestoreNoManifestOnNewVersion",
                getPrimaryUserId());

        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testRestoreNoManifestOnNewVersion",
                getPrimaryUserId());
    }

    /**
     * Make sure invisible shortcuts are ignored by all API calls.
     *
     * (Restore from new to old-nomanifest)
     */
    public void testBackupAndRestore_invisibleIgnored() throws Exception {
        if (!mSupportsBackup) {
            return;
        }
        dumpsys("Test start");

        // First, publish shortcuts from backup-ok version.

        installAppAsUser(PUBLISHER4_NEW_APK, getPrimaryUserId());
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());
        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());

        dumpsys("Before backup");

        // Backup
        doBackup();

        // Uninstall all apps
        uninstallPackageAndWaitUntilBroadcastsDrain(LAUNCHER4_PKG);
        uninstallPackageAndWaitUntilBroadcastsDrain(PUBLISHER4_PKG);

        // Make sure the shortcut service handled all the uninstall broadcasts.
        waitUntilPackagesGone();

        // Do it one more time just in case...
        waitUntilBroadcastsDrain();

        // Then restore
        doRestore();

        dumpsys("After restore");

        // Install the nobackup version. Restoration is limited.
        installAppAsUser(LAUNCHER4_NEW_APK, getPrimaryUserId());
        installAppAsUser(PUBLISHER4_OLD_NO_MANIFST_APK, getPrimaryUserId());
        waitUntilBroadcastsDrain();

        runDeviceTestsAsUser(PUBLISHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testInvisibleIgnored",
                getPrimaryUserId());

        runDeviceTestsAsUser(LAUNCHER4_PKG, ".ShortcutManagerPostBackupTest",
                "testInvisibleIgnored",
                getPrimaryUserId());
    }

    public void testBackupAndRestore_withNoUninstall() throws Exception {
        if (!mSupportsBackup) {
            return;
        }

        installAppAsUser(PUBLISHER1_APK, getPrimaryUserId());
        installAppAsUser(PUBLISHER3_APK, getPrimaryUserId());

        // Prepare shortcuts
        runDeviceTestsAsUser(PUBLISHER1_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());
        runDeviceTestsAsUser(PUBLISHER3_PKG, ".ShortcutManagerPreBackupTest", getPrimaryUserId());

        // Backup & restore.
        doBackup();
        doRestore();

        // Make sure the manifest shortcuts are re-published.
        runDeviceTestsAsUser(PUBLISHER1_PKG, ".ShortcutManagerPostBackupTest",
                "testWithNoUninstall",
                getPrimaryUserId());

        runDeviceTestsAsUser(PUBLISHER3_PKG, ".ShortcutManagerPostBackupTest",
                "testWithNoUninstall",
                getPrimaryUserId());
    }
}