package com.android.cts.devicepolicy;

import com.android.tradefed.device.DeviceNotAvailableException;

import java.io.FileNotFoundException;
import java.util.Collections;
import java.util.Map;

/**
 * In the test, managed profile and secondary user are created. We then verify
 * {@link android.content.pm.crossprofile.CrossProfileApps} APIs in different directions, like
 * primary user to managed profile.
 */
public class CrossProfileAppsHostSideTest extends BaseDevicePolicyTest {
    private static final String TEST_PACKAGE = "com.android.cts.crossprofileappstest";
    private static final String NON_TARGET_USER_TEST_CLASS = ".CrossProfileAppsNonTargetUserTest";
    private static final String TARGET_USER_TEST_CLASS = ".CrossProfileAppsTargetUserTest";
    private static final String PARAM_TARGET_USER = "TARGET_USER";
    private static final String EXTRA_TEST_APK = "CtsCrossProfileAppsTests.apk";
    private static final String SIMPLE_APP_APK ="CtsSimpleApp.apk";

    private int mProfileId;
    private int mSecondaryUserId;
    private boolean mHasManagedUserFeature;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // We need managed users to be supported in order to create a profile of the user owner.
        mHasManagedUserFeature = hasDeviceFeature("android.software.managed_users");
        installRequiredApps(mPrimaryUserId);

        if (mHasManagedUserFeature) {
            createAndStartManagedProfile();
            installRequiredApps(mProfileId);
        }
        if (mSupportsMultiUser) {
            mSecondaryUserId = createUser();
            installRequiredApps(mSecondaryUserId);
        }
    }

    private void installRequiredApps(int userId)
            throws FileNotFoundException, DeviceNotAvailableException {
        installAppAsUser(EXTRA_TEST_APK, userId);
        installAppAsUser(SIMPLE_APP_APK, userId);
    }

    public void testPrimaryUserToPrimaryUser() throws Exception {
        verifyCrossProfileAppsApi(mPrimaryUserId, mPrimaryUserId, NON_TARGET_USER_TEST_CLASS);
    }

    public void testPrimaryUserToManagedProfile() throws Exception {
        if (!mHasManagedUserFeature) {
            return;
        }
        verifyCrossProfileAppsApi(mPrimaryUserId, mProfileId, TARGET_USER_TEST_CLASS);
    }

    public void testManagedProfileToPrimaryUser() throws Exception {
        if (!mHasManagedUserFeature) {
            return;
        }
        verifyCrossProfileAppsApi(mProfileId, mPrimaryUserId, TARGET_USER_TEST_CLASS);
    }

    public void testPrimaryUserToSecondaryUser() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }
        verifyCrossProfileAppsApi(mPrimaryUserId, mSecondaryUserId, NON_TARGET_USER_TEST_CLASS);
    }

    public void testSecondaryUserToManagedProfile() throws Exception {
        if (!mSupportsMultiUser || !mHasManagedUserFeature) {
            return;
        }
        verifyCrossProfileAppsApi(mSecondaryUserId, mProfileId, NON_TARGET_USER_TEST_CLASS);

    }

    public void testManagedProfileToSecondaryUser() throws Exception {
        if (!mSupportsMultiUser || !mHasManagedUserFeature) {
            return;
        }
        verifyCrossProfileAppsApi(mProfileId, mSecondaryUserId, NON_TARGET_USER_TEST_CLASS);
    }

    private void verifyCrossProfileAppsApi(int fromUserId, int targetUserId, String testClass)
            throws Exception {
        runDeviceTestsAsUser(
                TEST_PACKAGE,
                testClass,
                null,
                fromUserId,
                createTargetUserParam(targetUserId));
    }

    private void createAndStartManagedProfile() throws Exception {
        mProfileId = createManagedProfile(mPrimaryUserId);
        switchUser(mPrimaryUserId);
        startUser(mProfileId);
    }

    private Map<String, String> createTargetUserParam(int targetUserId) throws Exception {
        return Collections.singletonMap(PARAM_TARGET_USER,
                Integer.toString(getUserSerialNumber(targetUserId)));
    }
}
