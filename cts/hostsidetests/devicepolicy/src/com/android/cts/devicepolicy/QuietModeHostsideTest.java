package com.android.cts.devicepolicy;

import java.util.HashMap;
import java.util.Map;

/**
 * CTS to verify toggling quiet mode in work profile by using
 * {@link android.os.UserManager#requestQuietModeEnabled(boolean, android.os.UserHandle)}.
 */
public class QuietModeHostsideTest extends BaseDevicePolicyTest {
    private static final String TEST_PACKAGE = "com.android.cts.launchertests";
    private static final String TEST_CLASS = ".QuietModeTest";
    private static final String PARAM_TARGET_USER = "TARGET_USER";
    private static final String PARAM_ORIGINAL_DEFAULT_LAUNCHER = "ORIGINAL_DEFAULT_LAUNCHER";
    private static final String TEST_APK = "CtsLauncherAppsTests.apk";

    private static final String TEST_LAUNCHER_PACKAGE = "com.android.cts.launchertests.support";
    private static final String TEST_LAUNCHER_APK = "CtsLauncherAppsTestsSupport.apk";

    private int mProfileId;
    private String mOriginalLauncher;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mHasFeature = mHasFeature & hasDeviceFeature("android.software.managed_users");

        if(mHasFeature) {
            mOriginalLauncher = getDefaultLauncher();

            installAppAsUser(TEST_APK, mPrimaryUserId);
            installAppAsUser(TEST_LAUNCHER_APK, mPrimaryUserId);

            createAndStartManagedProfile();
            installAppAsUser(TEST_APK, mProfileId);

            waitForBroadcastIdle();
            wakeupAndDismissKeyguard();
        }
    }

    @Override
    protected void tearDown() throws Exception {
        if (mHasFeature) {
            getDevice().uninstallPackage(TEST_PACKAGE);
            getDevice().uninstallPackage(TEST_LAUNCHER_PACKAGE);
        }
        super.tearDown();
    }

    public void testQuietMode() throws Exception {
        if (!mHasFeature) {
          return;
        }
        runDeviceTestsAsUser(
                TEST_PACKAGE,
                TEST_CLASS,
                null,
                mPrimaryUserId,
                createParams(mProfileId));
    }

    private void createAndStartManagedProfile() throws Exception {
        mProfileId = createManagedProfile(mPrimaryUserId);
        switchUser(mPrimaryUserId);
        startUser(mProfileId);
    }

    private Map<String, String> createParams(int targetUserId) throws Exception {
        Map<String, String> params = new HashMap<>();
        params.put(PARAM_TARGET_USER, Integer.toString(getUserSerialNumber(targetUserId)));
        params.put(PARAM_ORIGINAL_DEFAULT_LAUNCHER, mOriginalLauncher);
        return params;
    }
}
