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

package com.android.server.wifi.util;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.when;

import android.Manifest;
import android.app.AppOpsManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.os.Build;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.support.test.filters.SmallTest;

import com.android.server.wifi.BinderUtil;
import com.android.server.wifi.FakeWifiLog;
import com.android.server.wifi.WifiInjector;
import com.android.server.wifi.WifiSettingsStore;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.util.Arrays;
import java.util.HashMap;

/** Unit tests for {@link WifiPermissionsUtil}. */
@RunWith(JUnit4.class)
@SmallTest
public class WifiPermissionsUtilTest {
    public static final String TAG = "WifiPermissionsUtilTest";

    // Mock objects for testing
    @Mock private WifiPermissionsWrapper mMockPermissionsWrapper;
    @Mock private Context mMockContext;
    @Mock private PackageManager mMockPkgMgr;
    @Mock private ApplicationInfo mMockApplInfo;
    @Mock private AppOpsManager mMockAppOps;
    @Mock private UserInfo mMockUserInfo;
    @Mock private UserManager mMockUserManager;
    @Mock private WifiSettingsStore mMockWifiSettingsStore;
    @Mock private ContentResolver mMockContentResolver;
    @Mock private WifiInjector mWifiInjector;
    @Spy private FakeWifiLog mWifiLog;

    private static final String TEST_PACKAGE_NAME = "com.google.somePackage";
    private static final String INVALID_PACKAGE  = "BAD_PACKAGE";
    private static final int MANAGED_PROFILE_UID = 1100000;
    private static final int OTHER_USER_UID = 1200000;

    private final int mCallingUser = UserHandle.USER_CURRENT_OR_SELF;
    private final String mMacAddressPermission = "android.permission.PEERS_MAC_ADDRESS";
    private final String mInteractAcrossUsersFullPermission =
            "android.permission.INTERACT_ACROSS_USERS_FULL";
    private final String mManifestStringCoarse =
            Manifest.permission.ACCESS_COARSE_LOCATION;

    // Test variables
    private int mWifiScanAllowApps;
    private int mUid;
    private int mCoarseLocationPermission;
    private int mAllowCoarseLocationApps;
    private String mPkgNameOfTopActivity;
    private int mCurrentUser;
    private int mLocationModeSetting;
    private boolean mThrowSecurityException;
    private Answer<Integer> mReturnPermission;
    private HashMap<String, Integer> mPermissionsList = new HashMap<String, Integer>();

    /**
    * Set up Mockito tests
    */
    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        initTestVars();
    }

    private void setupTestCase() throws Exception {
        setupMocks();
        setupMockInterface();
    }

    /**
     * Verify we return true when the UID does have the override config permission
     */
    @Test
    public void testCheckConfigOverridePermissionApproved() throws Exception {
        mUid = MANAGED_PROFILE_UID;  // do not really care about this value
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        when(mMockPermissionsWrapper.getOverrideWifiConfigPermission(anyInt()))
                .thenReturn(PackageManager.PERMISSION_GRANTED);
        assertTrue(codeUnderTest.checkConfigOverridePermission(mUid));
    }

    /**
     * Verify we return false when the UID does not have the override config permission.
     */
    @Test
    public void testCheckConfigOverridePermissionDenied() throws Exception {
        mUid = OTHER_USER_UID;  // do not really care about this value
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        when(mMockPermissionsWrapper.getOverrideWifiConfigPermission(anyInt()))
                .thenReturn(PackageManager.PERMISSION_DENIED);
        assertFalse(codeUnderTest.checkConfigOverridePermission(mUid));
    }

    /**
     * Verify we return false when the override config permission check throws a RemoteException.
     */
    @Test
    public void testCheckConfigOverridePermissionWithException() throws Exception {
        mUid = OTHER_USER_UID;  // do not really care about this value
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        doThrow(new RemoteException("Failed to check permissions for " + mUid))
                .when(mMockPermissionsWrapper).getOverrideWifiConfigPermission(mUid);
        assertFalse(codeUnderTest.checkConfigOverridePermission(mUid));
    }

    /**
     * Test case setting: Package is valid
     *                    Location mode is enabled
     *                    Caller can read peers mac address
     *                    This App has permission to request WIFI_SCAN
     *                    User is current
     * Validate no Exceptions are thrown
     * - User has all the permissions
     */
    @Test
    public void testCanReadPeersMacAddressCurrentUserAndAllPermissions() throws Exception {
        mThrowSecurityException = false;
        mUid = MANAGED_PROFILE_UID;
        mPermissionsList.put(mMacAddressPermission, mUid);
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mCurrentUser = UserHandle.USER_CURRENT_OR_SELF;
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_HIGH_ACCURACY;
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
    }

    /**
     * Test case setting: Package is valid
     *                    Location mode is enabled
     *                    Caller can read peers mac address
     *                    This App has permission to request WIFI_SCAN
     *                    User profile is current
     * Validate no Exceptions are thrown
     * - User has all the permissions
     */
    @Test
    public void testCanReadPeersMacAddressCurrentProfileAndAllPermissions() throws Exception {
        mThrowSecurityException = false;
        mUid = MANAGED_PROFILE_UID;
        mPermissionsList.put(mMacAddressPermission, mUid);
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mMockUserInfo.id = mCallingUser;
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_HIGH_ACCURACY;
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
    }

    /**
     * Test case setting: Package is valid
     *                    Caller can read peers mac address
     * Validate that a SecurityException is thrown
     * - This App doesn't have permission to request Wifi Scan
     */
    @Test
    public void testCannotAccessScanResult_AppNotAllowed() throws Exception {
        mThrowSecurityException = false;
        mPermissionsList.put(mMacAddressPermission, mUid);
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        try {
            codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
            fail("Expected SecurityException is not thrown");
        } catch (SecurityException e) {
        }
    }

    /**
     * Test case setting: Package is valid
     *                    Location mode is enabled
     *                    Caller can read peers mac address
     *                    This App has permission to request WIFI_SCAN
     *                    User or profile is not current but the uid has
     *                    permission to INTERACT_ACROSS_USERS_FULL
     * Validate no Exceptions are thrown
     * - User has all the permissions
     */
    @Test
    public void testenforceCanAccessScanResults_UserOrProfileNotCurrent() throws Exception {
        mThrowSecurityException = false;
        mUid = MANAGED_PROFILE_UID;
        mPermissionsList.put(mMacAddressPermission, mUid);
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mPermissionsList.put(mInteractAcrossUsersFullPermission, mUid);
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_HIGH_ACCURACY;
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
    }

    /**
     * Test case setting: Package is valid
     *                    Caller can read peers mac address
     *                    This App has permission to request WIFI_SCAN
     *                    User or profile is not Current
     * Validate that a SecurityException is thrown
     * - Calling uid doesn't have INTERACT_ACROSS_USERS_FULL permission
     */
    @Test
    public void testCannotAccessScanResults_NoInteractAcrossUsersFullPermission() throws Exception {
        mThrowSecurityException = false;
        mUid = MANAGED_PROFILE_UID;
        mPermissionsList.put(mMacAddressPermission, mUid);
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        try {
            codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
            fail("Expected SecurityException is not thrown");
        } catch (SecurityException e) {
        }
    }

    /**
     * Test case Setting: Package is valid
     *                    Foreground
     *                    This App has permission to request WIFI_SCAN
     *                    User is current
     *  Validate that a SecurityException is thrown - app does not have location permission
     */
    @Test
    public void testLegacyForegroundAppWithOtherPermissionsDenied() throws Exception {
        mThrowSecurityException = false;
        mMockApplInfo.targetSdkVersion = Build.VERSION_CODES.GINGERBREAD;
        mPkgNameOfTopActivity = TEST_PACKAGE_NAME;
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mUid = MANAGED_PROFILE_UID;
        mCurrentUser = UserHandle.USER_CURRENT_OR_SELF;
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        try {
            codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
            fail("Expected SecurityException is not thrown");
        } catch (SecurityException e) {
        }
    }

    /**
     * Test case Setting: Package is valid
     *                    Location Mode Enabled
     *                    Coarse Location Access
     *                    This App has permission to request WIFI_SCAN
     *                    User profile is current
     *  Validate no Exceptions are thrown - has all permissions
     */
    @Test
    public void testLegacyAppHasLocationAndAllPermissions() throws Exception {
        mThrowSecurityException = false;
        mMockApplInfo.targetSdkVersion = Build.VERSION_CODES.GINGERBREAD;
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_HIGH_ACCURACY;
        mCoarseLocationPermission = PackageManager.PERMISSION_GRANTED;
        mAllowCoarseLocationApps = AppOpsManager.MODE_ALLOWED;
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mUid = MANAGED_PROFILE_UID;
        mMockUserInfo.id = mCallingUser;
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
    }

    /**
     * Test case setting: Package is valid
     *                    Location Mode Enabled
     * Validate that a SecurityException is thrown
     * - Doesn't have Peer Mac Address read permission
     * - Uid is not an active network scorer
     * - Location Mode is enabled but the uid
     * - doesn't have Coarse Location Access
     * - which implies No Location Permission
     */
    @Test
    public void testCannotAccessScanResults_NoCoarseLocationPermission() throws Exception {
        mThrowSecurityException = false;
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_HIGH_ACCURACY;
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        try {
            codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
            fail("Expected SecurityException is not thrown");
        } catch (SecurityException e) {
        }
    }

    /**
     * Test case setting: Package is valid
     *                    Location Mode Disabled
     *                    Caller has location permission
     * Validate an Exception is thrown
     * - Uid is not an active network scorer
     * - Uid doesn't have Coarse Location Access
     * - which implies No Location Permission
     */
    @Test
    public void testCannotAccessScanResults_LocationModeDisabled() throws Exception {
        mThrowSecurityException = false;
        mUid = MANAGED_PROFILE_UID;
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mPermissionsList.put(mInteractAcrossUsersFullPermission, mUid);
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_OFF;

        setupTestCase();
        when(mMockPermissionsWrapper.getChangeWifiConfigPermission(mUid))
                .thenReturn(PackageManager.PERMISSION_DENIED);

        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        try {
            codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
            fail("Expected SecurityException is not thrown");
        } catch (SecurityException e) {
        }
    }

    /**
     * Test case setting: Package is valid
     *                    Location Mode Disabled
     *                    Caller has location permisson
     *                    Caller has CHANGE_WIFI_STATE
     * Validate SecurityException is thrown
     * - Doesn't have Peer Mac Address read permission
     * - Uid is not an active network scorer
     * - Location Mode is disabled
     * - which implies no scan result access
     */
    @Test
    public void testEnforceCannotAccessScanResults_LocationModeDisabledHasChangeWifiState()
            throws Exception {
        mThrowSecurityException = false;
        mUid = MANAGED_PROFILE_UID;
        mPermissionsList.put(mMacAddressPermission, mUid);
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mPermissionsList.put(mInteractAcrossUsersFullPermission, mUid);
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_OFF;

        setupTestCase();
        when(mMockPermissionsWrapper.getChangeWifiConfigPermission(mUid))
                .thenReturn(PackageManager.PERMISSION_GRANTED);

        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        try {
            codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
            fail("Expected SecurityException is not thrown");
        } catch (SecurityException e) {
        }
    }

    /**
     * Test case setting: Package is valid
     *                    Location Mode Disabled
     *                    Caller has location permisson
     *                    Caller has ACCESS_WIFI_STATE
     * Validate Exception is thrown
     * - Doesn't have Peer Mac Address read permission
     * - Uid is not an active network scorer
     * - Location Mode is disabled
     * - doesn't have Coarse Location Access
     * - which implies no scan result access
     */
    @Test
    public void testEnforceCannotAccessScanResults_LocationModeDisabledHasAccessWifiState()
            throws Exception {
        mThrowSecurityException = false;
        mUid = MANAGED_PROFILE_UID;
        mPermissionsList.put(mMacAddressPermission, mUid);
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mPermissionsList.put(mInteractAcrossUsersFullPermission, mUid);
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_OFF;

        setupTestCase();
        when(mMockPermissionsWrapper.getAccessWifiStatePermission(mUid))
                .thenReturn(PackageManager.PERMISSION_GRANTED);

        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        try {
            codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
            fail("Expected SecurityException is not thrown");
        } catch (SecurityException e) {
        }
    }

    /**
     * Test case setting: Package is valid
     *                    Location Mode Disabled
     *                    Caller has location permisson
     *                    Caller does not have NETWORK_SETTINGS
     * Validate Exception is thrown
     * - Doesn't have Peer Mac Address read permission
     * - Uid is not an active network scorer
     * - Location Mode is disabled
     * - doesn't have Coarse Location Access
     * - which implies no scan result access
     */
    @Test
    public void testEnforceCannotAccessScanResults_LocationModeDisabledHasNoNetworkSettings()
            throws Exception {
        mThrowSecurityException = false;
        mUid = MANAGED_PROFILE_UID;
        mPermissionsList.put(mMacAddressPermission, mUid);
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mPermissionsList.put(mInteractAcrossUsersFullPermission, mUid);
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_OFF;

        setupTestCase();
        when(mMockPermissionsWrapper.getUidPermission(
                Manifest.permission.NETWORK_SETTINGS, MANAGED_PROFILE_UID))
                .thenReturn(PackageManager.PERMISSION_DENIED);

        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        try {
            codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
            fail("Expected SecurityException is not thrown");
        } catch (SecurityException e) {
        }
    }

    /**
     * Test case setting: Package is valid
     *                    Location Mode Disabled
     *                    Caller has location permisson
     *                    Caller has NETWORK_SETTINGS
     * Validate Exception is thrown
     * - Doesn't have Peer Mac Address read permission
     * - Uid is not an active network scorer
     * - Location Mode is disabled
     * - doesn't have Coarse Location Access
     * - which implies no scan result access
     */
    @Test
    public void testEnforceCanAccessScanResults_LocationModeDisabledHasNetworkSettings()
            throws Exception {
        mThrowSecurityException = false;
        mUid = MANAGED_PROFILE_UID;
        mPermissionsList.put(mMacAddressPermission, mUid);
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mPermissionsList.put(mInteractAcrossUsersFullPermission, mUid);
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_OFF;

        setupTestCase();
        when(mMockPermissionsWrapper.getUidPermission(
                Manifest.permission.NETWORK_SETTINGS, MANAGED_PROFILE_UID))
                .thenReturn(PackageManager.PERMISSION_GRANTED);

        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
    }

    /**
     * Test case setting: Package is valid
     *                    Location Mode Disabled
     *                    Caller has location permisson
     *                    Caller does not have NETWORK_SETUP_WIZARD
     * Validate Exception is thrown
     * - Doesn't have Peer Mac Address read permission
     * - Uid is not an active network scorer
     * - Location Mode is disabled
     * - doesn't have Coarse Location Access
     * - which implies no scan result access
     */
    @Test
    public void testEnforceCannotAccessScanResults_LocationModeDisabledHasNoNetworkSetupWizard()
            throws Exception {
        mThrowSecurityException = false;
        mUid = MANAGED_PROFILE_UID;
        mPermissionsList.put(mMacAddressPermission, mUid);
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mPermissionsList.put(mInteractAcrossUsersFullPermission, mUid);
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_OFF;

        setupTestCase();
        when(mMockPermissionsWrapper.getUidPermission(
                android.Manifest.permission.NETWORK_SETUP_WIZARD, MANAGED_PROFILE_UID))
                .thenReturn(PackageManager.PERMISSION_DENIED);

        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        try {
            codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
            fail("Expected SecurityException is not thrown");
        } catch (SecurityException e) {
        }
    }

    /**
     * Test case setting: Package is valid
     *                    Location Mode Disabled
     *                    Caller has location permisson
     *                    Caller has NETWORK_SETUP_WIZARD
     * Validate Exception is thrown
     * - Doesn't have Peer Mac Address read permission
     * - Uid is not an active network scorer
     * - Location Mode is disabled
     * - doesn't have Coarse Location Access
     * - which implies no scan result access
     */
    @Test
    public void testEnforceCanAccessScanResults_LocationModeDisabledHasNetworkSetupWizard()
            throws Exception {
        mThrowSecurityException = false;
        mUid = MANAGED_PROFILE_UID;
        mPermissionsList.put(mMacAddressPermission, mUid);
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mPermissionsList.put(mInteractAcrossUsersFullPermission, mUid);
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_OFF;

        setupTestCase();
        when(mMockPermissionsWrapper.getUidPermission(
                android.Manifest.permission.NETWORK_SETUP_WIZARD, MANAGED_PROFILE_UID))
                .thenReturn(PackageManager.PERMISSION_GRANTED);

        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
    }

    /**
     * Test case setting: Invalid Package
     * Expect a securityException
     */
    @Test
    public void testInvalidPackage() throws Exception {
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        try {
            codeUnderTest.enforceCanAccessScanResults(TEST_PACKAGE_NAME, mUid);
            fail("Expected SecurityException is not thrown");
        } catch (SecurityException e) {
        }
    }

    /**
     * Test case setting: caller does have Location permission.
     * A SecurityException should not be thrown.
     */
    @Test
    public void testEnforceLocationPermission() throws Exception {
        mThrowSecurityException = false;
        mMockApplInfo.targetSdkVersion = Build.VERSION_CODES.GINGERBREAD;
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_HIGH_ACCURACY;
        mCoarseLocationPermission = PackageManager.PERMISSION_GRANTED;
        mAllowCoarseLocationApps = AppOpsManager.MODE_ALLOWED;
        mWifiScanAllowApps = AppOpsManager.MODE_ALLOWED;
        mUid = MANAGED_PROFILE_UID;
        mMockUserInfo.id = mCallingUser;
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        codeUnderTest.enforceLocationPermission(TEST_PACKAGE_NAME, mUid);
    }

    /**
     * Verifies the helper method exposed for checking NETWORK_SETUP_WIZARD permission.
     */
    @Test
    public void testCheckNetworkSetupWizard() throws Exception {
        setupMocks();
        WifiPermissionsUtil wifiPermissionsUtil = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);

        when(mMockPermissionsWrapper.getUidPermission(
                android.Manifest.permission.NETWORK_SETUP_WIZARD, MANAGED_PROFILE_UID))
                .thenReturn(PackageManager.PERMISSION_DENIED);
        assertFalse(wifiPermissionsUtil.checkNetworkSetupWizardPermission(MANAGED_PROFILE_UID));

        when(mMockPermissionsWrapper.getUidPermission(
                android.Manifest.permission.NETWORK_SETUP_WIZARD, MANAGED_PROFILE_UID))
                .thenReturn(PackageManager.PERMISSION_GRANTED);
        assertTrue(wifiPermissionsUtil.checkNetworkSetupWizardPermission(MANAGED_PROFILE_UID));
    }

    /**
     * Test case setting: caller does not have Location permission.
     * Expect a SecurityException
     */
    @Test(expected = SecurityException.class)
    public void testEnforceLocationPermissionExpectSecurityException() throws Exception {
        setupTestCase();
        WifiPermissionsUtil codeUnderTest = new WifiPermissionsUtil(mMockPermissionsWrapper,
                mMockContext, mMockWifiSettingsStore, mMockUserManager, mWifiInjector);
        codeUnderTest.enforceLocationPermission(TEST_PACKAGE_NAME, mUid);
    }

    private Answer<Integer> createPermissionAnswer() {
        return new Answer<Integer>() {
            @Override
            public Integer answer(InvocationOnMock invocation) {
                int myUid = (int) invocation.getArguments()[1];
                String myPermission = (String) invocation.getArguments()[0];
                mPermissionsList.get(myPermission);
                if (mPermissionsList.containsKey(myPermission)) {
                    int uid = mPermissionsList.get(myPermission);
                    if (myUid == uid) {
                        return PackageManager.PERMISSION_GRANTED;
                    }
                }
                return PackageManager.PERMISSION_DENIED;
            }
        };
    }

    private void setupMocks() throws Exception {
        when(mMockPkgMgr.getApplicationInfo(TEST_PACKAGE_NAME, 0))
            .thenReturn(mMockApplInfo);
        when(mMockContext.getPackageManager()).thenReturn(mMockPkgMgr);
        when(mMockAppOps.noteOp(AppOpsManager.OP_WIFI_SCAN, mUid, TEST_PACKAGE_NAME))
            .thenReturn(mWifiScanAllowApps);
        when(mMockAppOps.noteOp(AppOpsManager.OP_COARSE_LOCATION, mUid, TEST_PACKAGE_NAME))
            .thenReturn(mAllowCoarseLocationApps);
        if (mThrowSecurityException) {
            doThrow(new SecurityException("Package " + TEST_PACKAGE_NAME + " doesn't belong"
                    + " to application bound to user " + mUid))
                    .when(mMockAppOps).checkPackage(mUid, TEST_PACKAGE_NAME);
        }
        when(mMockContext.getSystemService(Context.APP_OPS_SERVICE))
            .thenReturn(mMockAppOps);
        when(mMockUserManager.getProfiles(mCurrentUser))
            .thenReturn(Arrays.asList(mMockUserInfo));
        when(mMockContext.getContentResolver()).thenReturn(mMockContentResolver);
        when(mMockContext.getSystemService(Context.USER_SERVICE))
            .thenReturn(mMockUserManager);
        when(mWifiInjector.makeLog(anyString())).thenReturn(mWifiLog);
    }

    private void initTestVars() {
        mPermissionsList.clear();
        mReturnPermission = createPermissionAnswer();
        mWifiScanAllowApps = AppOpsManager.MODE_ERRORED;
        mUid = OTHER_USER_UID;
        mThrowSecurityException = true;
        mMockUserInfo.id = UserHandle.USER_NULL;
        mMockApplInfo.targetSdkVersion = Build.VERSION_CODES.M;
        mPkgNameOfTopActivity = INVALID_PACKAGE;
        mLocationModeSetting = Settings.Secure.LOCATION_MODE_OFF;
        mCurrentUser = UserHandle.USER_SYSTEM;
        mCoarseLocationPermission = PackageManager.PERMISSION_DENIED;
        mAllowCoarseLocationApps = AppOpsManager.MODE_ERRORED;
    }

    private void setupMockInterface() {
        BinderUtil.setUid(mUid);
        doAnswer(mReturnPermission).when(mMockPermissionsWrapper).getUidPermission(
                        anyString(), anyInt());
        doAnswer(mReturnPermission).when(mMockPermissionsWrapper).getUidPermission(
                        anyString(), anyInt());
        when(mMockPermissionsWrapper.getCallingUserId(mUid)).thenReturn(mCallingUser);
        when(mMockPermissionsWrapper.getCurrentUser()).thenReturn(mCurrentUser);
        when(mMockPermissionsWrapper.getUidPermission(mManifestStringCoarse, mUid))
            .thenReturn(mCoarseLocationPermission);
        when(mMockWifiSettingsStore.getLocationModeSetting(mMockContext))
            .thenReturn(mLocationModeSetting);
        when(mMockPermissionsWrapper.getTopPkgName()).thenReturn(mPkgNameOfTopActivity);
    }
}
