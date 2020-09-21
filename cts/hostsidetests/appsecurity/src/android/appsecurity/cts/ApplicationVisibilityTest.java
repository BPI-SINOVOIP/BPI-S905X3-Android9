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

package android.appsecurity.cts;

import android.platform.test.annotations.AppModeFull;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.HashMap;
import java.util.Map;

/**
 * Tests the visibility of installed applications.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class ApplicationVisibilityTest extends BaseAppSecurityTest {

    private static final String TINY_APK = "CtsPkgInstallTinyApp.apk";
    private static final String TINY_PKG = "android.appsecurity.cts.tinyapp";

    private static final String TEST_WITH_PERMISSION_APK =
            "CtsApplicationVisibilityCrossUserApp.apk";
    private static final String TEST_WITH_PERMISSION_PKG =
            "com.android.cts.applicationvisibility";

    private int[] mUsers;
    private String mOldVerifierValue;

    @Before
    public void setUpPackage() throws Exception {
        mUsers = Utils.prepareMultipleUsers(getDevice(), 3);
        mOldVerifierValue =
                getDevice().executeShellCommand("settings get global package_verifier_enable");
        getDevice().executeShellCommand("settings put global package_verifier_enable 0");
    }

    @After
    public void tearDown() throws Exception {
        getDevice().uninstallPackage(TEST_WITH_PERMISSION_PKG);
        getDevice().uninstallPackage(TINY_PKG);
        getDevice().executeShellCommand("settings put global package_verifier_enable "
                + mOldVerifierValue);
    }

    @Test
    @AppModeFull(reason = "instant applications cannot be granted INTERACT_ACROSS_USERS")
    public void testPackageListCrossUserGrant() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }

        final int installUserId = mUsers[1];
        final int testUserId = mUsers[2];

        installTestAppForUser(TINY_APK, installUserId);
        installTestAppForUser(TEST_WITH_PERMISSION_APK, testUserId);

        final String grantCmd = "pm grant"
                + " com.android.cts.applicationvisibility"
                + " android.permission.INTERACT_ACROSS_USERS";
        getDevice().executeShellCommand(grantCmd);
        Utils.runDeviceTests(
                getDevice(),
                TEST_WITH_PERMISSION_PKG,
                ".ApplicationVisibilityCrossUserTest",
                "testPackageVisibility_currentUser",
                testUserId);
        Utils.runDeviceTests(
                getDevice(),
                TEST_WITH_PERMISSION_PKG,
                ".ApplicationVisibilityCrossUserTest",
                "testPackageVisibility_anyUserCrossUserGrant",
                testUserId);
    }

    @Test
    @AppModeFull(reason = "instant applications cannot see any other application")
    public void testPackageListCrossUserNoGrant() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }

        final int installUserId = mUsers[1];
        final int testUserId = mUsers[2];

        installTestAppForUser(TINY_APK, installUserId);
        installTestAppForUser(TEST_WITH_PERMISSION_APK, testUserId);

        Utils.runDeviceTests(
                getDevice(),
                TEST_WITH_PERMISSION_PKG,
                ".ApplicationVisibilityCrossUserTest",
                "testPackageVisibility_currentUser",
                testUserId);
        Utils.runDeviceTests(
                getDevice(),
                TEST_WITH_PERMISSION_PKG,
                ".ApplicationVisibilityCrossUserTest",
                "testPackageVisibility_anyUserCrossUserNoGrant",
                testUserId);
    }

    @Test
    @AppModeFull(reason = "instant applications cannot be granted INTERACT_ACROSS_USERS")
    public void testPackageListOtherUserCrossUserGrant() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }

        final int installUserId = mUsers[1];
        final int testUserId = mUsers[2];
        final Map<String, String> testArgs = new HashMap<>();
        testArgs.put("testUser", Integer.toString(installUserId));

        installTestAppForUser(TINY_APK, installUserId);
        installTestAppForUser(TEST_WITH_PERMISSION_APK, testUserId);

        final String grantCmd = "pm grant"
                + " com.android.cts.applicationvisibility"
                + " android.permission.INTERACT_ACROSS_USERS";
        getDevice().executeShellCommand(grantCmd);
        Utils.runDeviceTests(
                getDevice(),
                TEST_WITH_PERMISSION_PKG,
                ".ApplicationVisibilityCrossUserTest",
                "testPackageVisibility_otherUserGrant",
                testUserId,
                testArgs);
    }

    @Test
    @AppModeFull(reason = "instant applications cannot see any other application")
    public void testPackageListOtherUserCrossUserNoGrant() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }

        final int installUserId = mUsers[1];
        final int testUserId = mUsers[2];
        final Map<String, String> testArgs = new HashMap<>();
        testArgs.put("testUser", Integer.toString(installUserId));

        installTestAppForUser(TINY_APK, installUserId);
        installTestAppForUser(TEST_WITH_PERMISSION_APK, testUserId);

        Utils.runDeviceTests(
                getDevice(),
                TEST_WITH_PERMISSION_PKG,
                ".ApplicationVisibilityCrossUserTest",
                "testPackageVisibility_otherUserNoGrant",
                testUserId,
                testArgs);
    }

    @Test
    @AppModeFull(reason = "instant applications cannot be granted INTERACT_ACROSS_USERS")
    public void testApplicationListCrossUserGrant() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }

        final int installUserId = mUsers[1];
        final int testUserId = mUsers[2];

        installTestAppForUser(TINY_APK, installUserId);
        installTestAppForUser(TEST_WITH_PERMISSION_APK, testUserId);

        final String grantCmd = "pm grant"
                + " com.android.cts.applicationvisibility"
                + " android.permission.INTERACT_ACROSS_USERS";
        getDevice().executeShellCommand(grantCmd);
        Utils.runDeviceTests(
                getDevice(),
                TEST_WITH_PERMISSION_PKG,
                ".ApplicationVisibilityCrossUserTest",
                "testApplicationVisibility_currentUser",
                testUserId);
        Utils.runDeviceTests(
                getDevice(),
                TEST_WITH_PERMISSION_PKG,
                ".ApplicationVisibilityCrossUserTest",
                "testApplicationVisibility_anyUserCrossUserGrant",
                testUserId);
    }

    @Test
    @AppModeFull(reason = "instant applications cannot see any other application")
    public void testApplicationListCrossUserNoGrant() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }

        final int installUserId = mUsers[1];
        final int testUserId = mUsers[2];

        installTestAppForUser(TINY_APK, installUserId);
        installTestAppForUser(TEST_WITH_PERMISSION_APK, testUserId);

        Utils.runDeviceTests(
                getDevice(),
                TEST_WITH_PERMISSION_PKG,
                ".ApplicationVisibilityCrossUserTest",
                "testApplicationVisibility_currentUser",
                testUserId);
        Utils.runDeviceTests(
                getDevice(),
                TEST_WITH_PERMISSION_PKG,
                ".ApplicationVisibilityCrossUserTest",
                "testApplicationVisibility_anyUserCrossUserNoGrant",
                testUserId);
    }

    @Test
    @AppModeFull(reason = "instant applications cannot be granted INTERACT_ACROSS_USERS")
    public void testApplicationListOtherUserCrossUserGrant() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }

        final int installUserId = mUsers[1];
        final int testUserId = mUsers[2];
        final Map<String, String> testArgs = new HashMap<>();
        testArgs.put("testUser", Integer.toString(installUserId));

        installTestAppForUser(TINY_APK, installUserId);
        installTestAppForUser(TEST_WITH_PERMISSION_APK, testUserId);

        final String grantCmd = "pm grant"
                + " com.android.cts.applicationvisibility"
                + " android.permission.INTERACT_ACROSS_USERS";
        getDevice().executeShellCommand(grantCmd);
        Utils.runDeviceTests(
                getDevice(),
                TEST_WITH_PERMISSION_PKG,
                ".ApplicationVisibilityCrossUserTest",
                "testApplicationVisibility_otherUserGrant",
                testUserId,
                testArgs);
    }

    @Test
    @AppModeFull(reason = "instant applications cannot see any other application")
    public void testApplicationListOtherUserCrossUserNoGrant() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }

        final int installUserId = mUsers[1];
        final int testUserId = mUsers[2];
        final Map<String, String> testArgs = new HashMap<>();
        testArgs.put("testUser", Integer.toString(installUserId));

        installTestAppForUser(TINY_APK, installUserId);
        installTestAppForUser(TEST_WITH_PERMISSION_APK, testUserId);

        Utils.runDeviceTests(
                getDevice(),
                TEST_WITH_PERMISSION_PKG,
                ".ApplicationVisibilityCrossUserTest",
                "testApplicationVisibility_otherUserNoGrant",
                testUserId,
                testArgs);
    }
}
