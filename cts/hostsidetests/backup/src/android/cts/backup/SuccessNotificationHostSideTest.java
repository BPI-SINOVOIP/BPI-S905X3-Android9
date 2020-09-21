/*
 * Copyright (C) 2018 The Android Open Source Project
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
 * limitations under the License
 */

package android.cts.backup;

import static org.junit.Assert.assertNull;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Tests for checking that an observer app is notified by a broadcast Intent whenever a backup
 * succeeds.
 *
 * NB: The tests use "bmgr backupnow" for backup, which works on N+ devices.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class SuccessNotificationHostSideTest extends BaseBackupHostSideTest {

    /** The name of the package that a key/value backup will be run for */
    private static final String KEY_VALUE_BACKUP_APP_PACKAGE =
            "android.cts.backup.keyvaluerestoreapp";

    /** The name of the package that a full backup will be run for */
    private static final String FULL_BACKUP_APP_PACKAGE = "android.cts.backup.fullbackuponlyapp";

    /** The name of the package that observes backup results. */
    private static final String SUCCESS_NOTIFICATION_APP_PACKAGE =
            "android.cts.backup.successnotificationapp";

    /** The name of the device side test class in the APK that a key/value backup will be run for */
    private static final String KEY_VALUE_BACKUP_DEVICE_TEST_NAME =
            KEY_VALUE_BACKUP_APP_PACKAGE + ".KeyValueBackupRestoreTest";

    /** The name of the device side test class in the APK that a full backup will be run for */
    private static final String FULL_BACKUP_DEVICE_TEST_CLASS_NAME =
            FULL_BACKUP_APP_PACKAGE + ".FullBackupOnlyTest";

    /** The name of the device side test class in the APK that observes backup results */
    private static final String SUCCESS_NOTIFICATION_DEVICE_TEST_NAME =
            SUCCESS_NOTIFICATION_APP_PACKAGE + ".SuccessNotificationTest";

    /** The name of the APK that a key/value backup will be run for */
    private static final String KEY_VALUE_BACKUP_APP_APK = "CtsKeyValueBackupRestoreApp.apk";

    /** The name of the APK that a full backup will be run for */
    private static final String FULL_BACKUP_APP_APK = "FullBackupOnlyFalseWithAgentApp.apk";

    /** The name of the APK that observes backup results */
    private static final String SUCCESS_NOTIFICATION_APP_APK =
            "CtsBackupSuccessNotificationApp.apk";

    /** Secure setting that holds the backup manager configuration as key-value pairs */
    private static final String BACKUP_MANAGER_CONSTANTS_PREF = "backup_manager_constants";

    /** Key for specifying the apps that the backup manager should notify of successful backups */
    private static final String BACKUP_FINISHED_NOTIFICATION_RECEIVERS =
            "backup_finished_notification_receivers";

    /** The original backup manager configuration */
    private String mOriginalBackupManagerConstants = null;

    @Before
    @Override
    public void setUp() throws Exception {
        super.setUp();

        if (!mIsBackupSupported) {
            CLog.i("android.software.backup feature is not supported on this device");
            return;
        }

        installPackage(KEY_VALUE_BACKUP_APP_APK);
        installPackage(FULL_BACKUP_APP_APK);

        installPackage(SUCCESS_NOTIFICATION_APP_APK);
        checkDeviceTest("clearBackupSuccessNotificationsReceived");
        addBackupFinishedNotificationReceiver();
    }

    @After
    @Override
    public void tearDown() throws Exception {
        super.tearDown();

        if (!mIsBackupSupported) {
            return;
        }

        restoreBackupFinishedNotificationReceivers();
        assertNull(uninstallPackage(SUCCESS_NOTIFICATION_APP_PACKAGE));

        // Clear backup data and uninstall the package (in that order!)
        clearBackupDataInLocalTransport(KEY_VALUE_BACKUP_APP_PACKAGE);
        assertNull(uninstallPackage(KEY_VALUE_BACKUP_APP_PACKAGE));

        clearBackupDataInLocalTransport(FULL_BACKUP_APP_PACKAGE);
        assertNull(uninstallPackage(FULL_BACKUP_APP_PACKAGE));
    }

    /**
     * Test that the observer app is notified when a key/value backup succeeds.
     *
     * Test logic:
     *   1. Change a test app's data, trigger a key/value backup and wait for it to complete.
     *   2. Verify that the observer app was informed about the backup.
     */
    @Test
    public void testSuccessNotificationForKeyValueBackup() throws Exception {
        if (!mIsBackupSupported) {
            return;
        }

        checkDeviceTest(KEY_VALUE_BACKUP_APP_PACKAGE, KEY_VALUE_BACKUP_DEVICE_TEST_NAME,
                "saveSharedPreferencesAndNotifyBackupManager");
        backupNowAndAssertSuccess(KEY_VALUE_BACKUP_APP_PACKAGE);

        checkDeviceTest("verifyBackupSuccessNotificationReceivedForKeyValueApp");
    }

    /**
     * Test that the observer app is notified when a full backup succeeds.
     *
     * Test logic:
     *   1. Change a test app's data, trigger a full backup and wait for it to complete.
     *   2. Verify that the observer app was informed about the backup.
     */
    @Test
    public void testSuccessNotificationForFullBackup() throws Exception {
        if (!mIsBackupSupported) {
            return;
        }

        checkDeviceTest(FULL_BACKUP_APP_PACKAGE, FULL_BACKUP_DEVICE_TEST_CLASS_NAME, "createFiles");
        backupNowAndAssertSuccess(FULL_BACKUP_APP_PACKAGE);

        checkDeviceTest("verifyBackupSuccessNotificationReceivedForFullBackupApp");
    }

    /**
     * Instructs the backup manager to notify the observer app whenever a backup succeeds. The old
     * backup manager configuration is stored in a member variable and can be restored by calling
     * {@link restoreBackupFinishedNotificationReceivers}.
     */
    private void addBackupFinishedNotificationReceiver()
            throws DeviceNotAvailableException {
        mOriginalBackupManagerConstants = getDevice().executeShellCommand(String.format(
                "settings get secure %s", BACKUP_MANAGER_CONSTANTS_PREF)).trim();
        if ("null".equals(mOriginalBackupManagerConstants)) {
            mOriginalBackupManagerConstants = null;
        }
        String backupManagerConstants = null;

        if (mOriginalBackupManagerConstants == null || mOriginalBackupManagerConstants.isEmpty()) {
            backupManagerConstants =
                    BACKUP_FINISHED_NOTIFICATION_RECEIVERS + "=" + SUCCESS_NOTIFICATION_APP_PACKAGE;
        } else {
            final List<String> keyValuePairs =
                    new ArrayList<>(Arrays.asList(mOriginalBackupManagerConstants.split(",")));
            boolean present = false;
            for (int i = 0; i < keyValuePairs.size(); ++i) {
                final String keyValuePair = keyValuePairs.get(i);
                final String[] fields = keyValuePair.split("=");
                final String key = fields[0].trim();
                if (BACKUP_FINISHED_NOTIFICATION_RECEIVERS.equals(key)) {
                    if (fields.length == 1 || fields[1].trim().isEmpty()) {
                        keyValuePairs.set(i, key + "=" + SUCCESS_NOTIFICATION_APP_PACKAGE);
                    } else {
                        final String[] values = fields[1].split(":");
                        for (int j = 0; j < values.length; ++j) {
                            if (SUCCESS_NOTIFICATION_APP_PACKAGE.equals(values[j].trim())) {
                                present = true;
                                break;
                            }
                        }
                        if (!present) {
                            keyValuePairs.set(i,
                                    keyValuePair + ":" + SUCCESS_NOTIFICATION_APP_PACKAGE);
                        }
                    }
                    present = true;
                    break;
                }
            }
            if (!present) {
                keyValuePairs.add(BACKUP_FINISHED_NOTIFICATION_RECEIVERS + "=" +
                        SUCCESS_NOTIFICATION_APP_PACKAGE);
            }
            backupManagerConstants = String.join(",", keyValuePairs);
        }
        setBackupManagerConstants(backupManagerConstants);
    }

    /**
     * Restores the backup manager configuration stored by a previous call to
     * {@link addBackupFinishedNotificationReceiver}.
     */
    private void restoreBackupFinishedNotificationReceivers() throws DeviceNotAvailableException {
        setBackupManagerConstants(mOriginalBackupManagerConstants);
    }

    private void setBackupManagerConstants(String backupManagerConstants)
            throws DeviceNotAvailableException {
        getDevice().executeShellCommand(String.format("settings put secure %s %s",
                BACKUP_MANAGER_CONSTANTS_PREF, backupManagerConstants));
    }

    private void checkDeviceTest(String methodName) throws DeviceNotAvailableException {
        checkDeviceTest(SUCCESS_NOTIFICATION_APP_PACKAGE, SUCCESS_NOTIFICATION_DEVICE_TEST_NAME,
                methodName);
    }
}
