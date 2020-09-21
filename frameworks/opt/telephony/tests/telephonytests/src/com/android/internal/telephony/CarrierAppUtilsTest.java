/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.internal.telephony;

import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.test.InstrumentationTestCase;
import android.test.mock.MockContentProvider;
import android.test.mock.MockContentResolver;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.ArrayMap;
import android.util.ArraySet;

import org.junit.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.List;

public class CarrierAppUtilsTest extends InstrumentationTestCase {
    private static final String CARRIER_APP = "com.example.carrier";
    private static final ArraySet<String> CARRIER_APPS = new ArraySet<>();
    static {
        CARRIER_APPS.add(CARRIER_APP);
    }

    private static final String ASSOCIATED_APP = "com.example.associated";
    private static final ArrayMap<String, List<String>> ASSOCIATED_APPS = new ArrayMap<>();
    static {
        List<String> associatedAppList = new ArrayList<>();
        associatedAppList.add(ASSOCIATED_APP);
        ASSOCIATED_APPS.put(CARRIER_APP, associatedAppList);
    }
    private static final int USER_ID = 12345;
    private static final String CALLING_PACKAGE = "phone";

    @Mock private IPackageManager mPackageManager;
    @Mock private TelephonyManager mTelephonyManager;
    private SettingsMockContentProvider mContentProvider;
    private MockContentResolver mContentResolver;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        System.setProperty("dexmaker.dexcache",
                getInstrumentation().getTargetContext().getCacheDir().getPath());
        Thread.currentThread().setContextClassLoader(getClass().getClassLoader());
        MockitoAnnotations.initMocks(this);

        mContentResolver = new MockContentResolver();
        mContentProvider = new SettingsMockContentProvider();
        mContentResolver.addProvider(Settings.AUTHORITY, mContentProvider);
        Settings.Secure.putIntForUser(
                mContentResolver, Settings.Secure.CARRIER_APPS_HANDLED, 0, USER_ID);
    }

    /** No apps configured - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_EmptyList() {
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, new ArraySet<>(),
                ASSOCIATED_APPS);
        Mockito.verifyNoMoreInteractions(mPackageManager, mTelephonyManager);
    }

    /** Configured app is missing - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_MissingApp() throws Exception {
        Mockito.when(mPackageManager.getApplicationInfo("com.example.missing.app",
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(null);
        ArraySet<String> systemCarrierAppsDisabledUntilUsed = new ArraySet<>();
        systemCarrierAppsDisabledUntilUsed.add("com.example.missing.app");
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID,
                systemCarrierAppsDisabledUntilUsed, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(Mockito.any(String[].class),
                        Mockito.anyInt());
        Mockito.verifyNoMoreInteractions(mTelephonyManager);
    }

    /** Configured app is not bundled with the system - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NonSystemApp() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
        Mockito.verifyNoMoreInteractions(mTelephonyManager);
    }

    /**
     * Configured app has privileges, but was disabled by the user - should only grant
     * permissions.
     */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_HasPrivileges_DisabledUser()
            throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DISABLED_USER;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager).grantDefaultPermissionsToEnabledCarrierApps(
                new String[] {appInfo.packageName}, USER_ID);
    }

    /** Configured app has privileges, but was disabled - should only grant permissions. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_HasPrivileges_Disabled() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager).grantDefaultPermissionsToEnabledCarrierApps(
                new String[] {appInfo.packageName}, USER_ID);
    }

    /** Configured app has privileges, and is already enabled - should only grant permissions. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_HasPrivileges_Enabled() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager).grantDefaultPermissionsToEnabledCarrierApps(
                new String[] {appInfo.packageName}, USER_ID);
    }

    /** Configured /data app has privileges - should only grant permissions. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_HasPrivileges_UpdatedApp() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM | ApplicationInfo.FLAG_UPDATED_SYSTEM_APP;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager).grantDefaultPermissionsToEnabledCarrierApps(
                new String[] {appInfo.packageName}, USER_ID);
    }

    /**
     * Configured app has privileges, and is in the default state - should enable. Associated app
     * is missing and should not be touched.
     */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_HasPrivileges_MissingAssociated_Default()
            throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID))
                .thenReturn(appInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager).setApplicationEnabledSetting(
                CARRIER_APP, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP, USER_ID, CALLING_PACKAGE);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                ASSOCIATED_APP, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP, USER_ID, CALLING_PACKAGE);
        Mockito.verify(mPackageManager).grantDefaultPermissionsToEnabledCarrierApps(
                new String[] {appInfo.packageName}, USER_ID);
    }

    /**
     * Configured app has privileges, and is in the default state along with associated app - should
     * enable both.
     */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_HasPrivileges_Associated_Default()
            throws Exception {
        // Enabling should be done even if this isn't the first run.
        Settings.Secure.putIntForUser(
                mContentResolver, Settings.Secure.CARRIER_APPS_HANDLED, 1, USER_ID);
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID))
                .thenReturn(appInfo);
        ApplicationInfo associatedAppInfo = new ApplicationInfo();
        associatedAppInfo.packageName = ASSOCIATED_APP;
        associatedAppInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        associatedAppInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
        Mockito.when(mPackageManager.getApplicationInfo(ASSOCIATED_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID))
                .thenReturn(associatedAppInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager).setApplicationEnabledSetting(
                CARRIER_APP, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP, USER_ID, CALLING_PACKAGE);
        Mockito.verify(mPackageManager).setApplicationEnabledSetting(
                ASSOCIATED_APP, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP, USER_ID, CALLING_PACKAGE);
        Mockito.verify(mPackageManager).grantDefaultPermissionsToEnabledCarrierApps(
                new String[] {appInfo.packageName}, USER_ID);
    }

    /**
     * Configured app has privileges, and is disabled until used - should enable. Associated app has
     * been updated and should not be touched.
     */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_HasPrivileges_UpdatedAssociated_DisabledUntilUsed()
            throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        ApplicationInfo associatedAppInfo = new ApplicationInfo();
        associatedAppInfo.packageName = ASSOCIATED_APP;
        associatedAppInfo.flags |=
                ApplicationInfo.FLAG_SYSTEM | ApplicationInfo.FLAG_UPDATED_SYSTEM_APP;
        associatedAppInfo.enabledSetting =
                PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED;
        Mockito.when(mPackageManager.getApplicationInfo(ASSOCIATED_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID))
                .thenReturn(associatedAppInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager).setApplicationEnabledSetting(
                CARRIER_APP, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP, USER_ID, CALLING_PACKAGE);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                ASSOCIATED_APP, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP, USER_ID, CALLING_PACKAGE);
        Mockito.verify(mPackageManager).grantDefaultPermissionsToEnabledCarrierApps(
                new String[] {appInfo.packageName}, USER_ID);
    }

    /**
     * Configured app has privileges, and is disabled until used along with associated app - should
     * enable both.
     */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_HasPrivileges_Associated_DisabledUntilUsed()
            throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        ApplicationInfo associatedAppInfo = new ApplicationInfo();
        associatedAppInfo.packageName = ASSOCIATED_APP;
        associatedAppInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        associatedAppInfo.enabledSetting =
                PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED;
        Mockito.when(mPackageManager.getApplicationInfo(ASSOCIATED_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID))
                .thenReturn(associatedAppInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager).setApplicationEnabledSetting(
                CARRIER_APP, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP, USER_ID, CALLING_PACKAGE);
        Mockito.verify(mPackageManager).setApplicationEnabledSetting(
                ASSOCIATED_APP, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP, USER_ID, CALLING_PACKAGE);
        Mockito.verify(mPackageManager).grantDefaultPermissionsToEnabledCarrierApps(
                new String[] {appInfo.packageName}, USER_ID);
    }

    /** Configured app has no privileges, and was disabled by the user - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NoPrivileges_DisabledUser() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DISABLED_USER;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /** Telephony is not initialized, and app was disabled by the user - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NullPrivileges_DisabledUser()
            throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DISABLED_USER;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                null /* telephonyManager */, mContentResolver, USER_ID, CARRIER_APPS,
                ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /** Configured app has no privileges, and was disabled - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NoPrivileges_Disabled() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /** Telephony is not initialized, and app was disabled - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NullPrivileges_Disabled() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                null /* telephonyManager */, mContentResolver, USER_ID, CARRIER_APPS,
                ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /** Configured app has no privileges, and is explicitly enabled - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NoPrivileges_Enabled() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /** Telephony is not initialized, and app is explicitly enabled - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NullPrivileges_Enabled() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                null /* telephonyManager */, mContentResolver, USER_ID, CARRIER_APPS,
                ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /** Configured /data app has no privileges - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NoPrivileges_UpdatedApp() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM | ApplicationInfo.FLAG_UPDATED_SYSTEM_APP;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /** Telephony is not initialized and app is in /data - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NullPrivileges_UpdatedApp() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM | ApplicationInfo.FLAG_UPDATED_SYSTEM_APP;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                null /* telephonyManager */, mContentResolver, USER_ID, CARRIER_APPS,
                ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /**
     * Configured app has no privileges, and is in the default state - should disable until use.
     * Associated app is enabled and should not be touched.
     */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NoPrivileges_EnabledAssociated_Default()
            throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        ApplicationInfo associatedAppInfo = new ApplicationInfo();
        associatedAppInfo.packageName = ASSOCIATED_APP;
        associatedAppInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        associatedAppInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
        Mockito.when(mPackageManager.getApplicationInfo(ASSOCIATED_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID))
                .thenReturn(associatedAppInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager).setApplicationEnabledSetting(
                CARRIER_APP, PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED, 0, USER_ID,
                CALLING_PACKAGE);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                ASSOCIATED_APP, PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED, 0,
                USER_ID, CALLING_PACKAGE);
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /**
     * Configured app has no privileges, and is in the default state along with associated app -
     * should disable both.
     */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NoPrivileges_Associated_Default()
            throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        ApplicationInfo associatedAppInfo = new ApplicationInfo();
        associatedAppInfo.packageName = ASSOCIATED_APP;
        associatedAppInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        associatedAppInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
        Mockito.when(mPackageManager.getApplicationInfo(ASSOCIATED_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID))
                .thenReturn(associatedAppInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager).setApplicationEnabledSetting(
                CARRIER_APP, PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED, 0, USER_ID,
                CALLING_PACKAGE);
        Mockito.verify(mPackageManager).setApplicationEnabledSetting(
                ASSOCIATED_APP, PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED, 0,
                USER_ID, CALLING_PACKAGE);
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /**
     * Configured app has no privileges, and is in the default state along with associated app, and
     * disabling has already occurred - should only disable configured app.
     */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NoPrivileges_Associated_Default_AlreadyRun()
            throws Exception {
        Settings.Secure.putIntForUser(
                mContentResolver, Settings.Secure.CARRIER_APPS_HANDLED, 1, USER_ID);
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        ApplicationInfo associatedAppInfo = new ApplicationInfo();
        associatedAppInfo.packageName = ASSOCIATED_APP;
        associatedAppInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        associatedAppInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
        Mockito.when(mPackageManager.getApplicationInfo(ASSOCIATED_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID))
                .thenReturn(associatedAppInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager).setApplicationEnabledSetting(
                CARRIER_APP, PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED, 0, USER_ID,
                CALLING_PACKAGE);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.eq(ASSOCIATED_APP), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /** Telephony is not initialized, and app is in the default state - should disable until use. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NullPrivileges_Default() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                null /* telephonyManager */, mContentResolver, USER_ID, CARRIER_APPS,
                ASSOCIATED_APPS);
        Mockito.verify(mPackageManager).setApplicationEnabledSetting(
                CARRIER_APP, PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED, 0, USER_ID,
                CALLING_PACKAGE);
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /** Configured app has no privileges, and is disabled until used - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NoPrivileges_DisabledUntilUsed()
            throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        Mockito.when(mTelephonyManager.checkCarrierPrivilegesForPackageAnyPhone(CARRIER_APP))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                mTelephonyManager, mContentResolver, USER_ID, CARRIER_APPS, ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    /** Telephony is not initialized, and app is disabled until used - should do nothing. */
    @Test @SmallTest
    public void testDisableCarrierAppsUntilPrivileged_NullPrivileges_DisabledUntilUsed()
            throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.packageName = CARRIER_APP;
        appInfo.flags |= ApplicationInfo.FLAG_SYSTEM;
        appInfo.enabledSetting = PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED;
        Mockito.when(mPackageManager.getApplicationInfo(CARRIER_APP,
                PackageManager.MATCH_DISABLED_UNTIL_USED_COMPONENTS, USER_ID)).thenReturn(appInfo);
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(CALLING_PACKAGE, mPackageManager,
                null /* telephonyManager */, mContentResolver, USER_ID, CARRIER_APPS,
                ASSOCIATED_APPS);
        Mockito.verify(mPackageManager, Mockito.never()).setApplicationEnabledSetting(
                Mockito.anyString(), Mockito.anyInt(), Mockito.anyInt(), Mockito.anyInt(),
                Mockito.anyString());
        Mockito.verify(mPackageManager, Mockito.never())
                .grantDefaultPermissionsToEnabledCarrierApps(
                        Mockito.any(String[].class), Mockito.anyInt());
    }

    class SettingsMockContentProvider extends MockContentProvider {
        private int mExpectedValue;

        @Override
        public Bundle call(String method, String request, Bundle args) {
            Bundle result = new Bundle();
            if (Settings.CALL_METHOD_GET_SECURE.equals(method)) {
                result.putString(Settings.NameValueTable.VALUE, Integer.toString(mExpectedValue));
            } else {
                mExpectedValue = Integer.parseInt(args.getString(Settings.NameValueTable.VALUE));
            }
            return result;
        }
    }

}

