package com.android.settingslib.location;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.isA;
import static org.mockito.Mockito.when;

import android.app.AppOpsManager;
import android.app.AppOpsManager.OpEntry;
import android.app.AppOpsManager.PackageOps;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.os.Process;
import android.os.UserHandle;
import android.os.UserManager;

import com.android.settingslib.SettingsLibRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.TimeUnit;

@RunWith(SettingsLibRobolectricTestRunner.class)
public class RecentLocationAppsTest {

    private static final int TEST_UID = 1234;
    private static final long NOW = System.currentTimeMillis();
    // App running duration in milliseconds
    private static final int DURATION = 10;
    private static final long ONE_MIN_AGO = NOW - TimeUnit.MINUTES.toMillis(1);
    private static final long TWENTY_THREE_HOURS_AGO = NOW - TimeUnit.HOURS.toMillis(23);
    private static final long TWO_DAYS_AGO = NOW - TimeUnit.DAYS.toMillis(2);
    private static final String[] TEST_PACKAGE_NAMES =
            {"package_1MinAgo", "package_14MinAgo", "package_20MinAgo"};

    @Mock
    private Context mContext;
    @Mock
    private PackageManager mPackageManager;
    @Mock
    private AppOpsManager mAppOpsManager;
    @Mock
    private Resources mResources;
    @Mock
    private UserManager mUserManager;
    private int mTestUserId;
    private RecentLocationApps mRecentLocationApps;



    @Before
    public void setUp() throws NameNotFoundException {
        MockitoAnnotations.initMocks(this);

        when(mContext.getPackageManager()).thenReturn(mPackageManager);
        when(mContext.getSystemService(Context.APP_OPS_SERVICE)).thenReturn(mAppOpsManager);
        when(mContext.getResources()).thenReturn(mResources);
        when(mContext.getSystemService(UserManager.class)).thenReturn(mUserManager);
        when(mContext.getSystemService(Context.USER_SERVICE)).thenReturn(mUserManager);
        when(mPackageManager.getApplicationLabel(isA(ApplicationInfo.class)))
                .thenReturn("testApplicationLabel");
        when(mPackageManager.getUserBadgedLabel(isA(CharSequence.class), isA(UserHandle.class)))
                .thenReturn("testUserBadgedLabel");
        mTestUserId = UserHandle.getUserId(TEST_UID);
        when(mUserManager.getUserProfiles())
                .thenReturn(Collections.singletonList(new UserHandle(mTestUserId)));

        long[] testRequestTime = {ONE_MIN_AGO, TWENTY_THREE_HOURS_AGO, TWO_DAYS_AGO};
        List<PackageOps> appOps = createTestPackageOpsList(TEST_PACKAGE_NAMES, testRequestTime);
        when(mAppOpsManager.getPackagesForOps(RecentLocationApps.LOCATION_OPS)).thenReturn(appOps);
        mockTestApplicationInfos(mTestUserId, TEST_PACKAGE_NAMES);

        mRecentLocationApps = new RecentLocationApps(mContext);
    }

    @Test
    public void testGetAppList_shouldFilterRecentApps() {
        List<RecentLocationApps.Request> requests = mRecentLocationApps.getAppList();
        // Only two of the apps have requested location within 15 min.
        assertThat(requests).hasSize(2);
        // Make sure apps are ordered by recency
        assertThat(requests.get(0).packageName).isEqualTo(TEST_PACKAGE_NAMES[0]);
        assertThat(requests.get(0).requestFinishTime).isEqualTo(ONE_MIN_AGO + DURATION);
        assertThat(requests.get(1).packageName).isEqualTo(TEST_PACKAGE_NAMES[1]);
        assertThat(requests.get(1).requestFinishTime).isEqualTo(TWENTY_THREE_HOURS_AGO + DURATION);
    }

    @Test
    public void testGetAppList_shouldNotShowAndroidOS() throws NameNotFoundException {
        // Add android OS to the list of apps.
        PackageOps androidSystemPackageOps =
                createPackageOps(
                        RecentLocationApps.ANDROID_SYSTEM_PACKAGE_NAME,
                        Process.SYSTEM_UID,
                        AppOpsManager.OP_MONITOR_HIGH_POWER_LOCATION,
                        ONE_MIN_AGO,
                        DURATION);
        long[] testRequestTime =
                {ONE_MIN_AGO, TWENTY_THREE_HOURS_AGO, TWO_DAYS_AGO, ONE_MIN_AGO};
        List<PackageOps> appOps = createTestPackageOpsList(TEST_PACKAGE_NAMES, testRequestTime);
        appOps.add(androidSystemPackageOps);
        when(mAppOpsManager.getPackagesForOps(RecentLocationApps.LOCATION_OPS)).thenReturn(appOps);
        mockTestApplicationInfos(
                Process.SYSTEM_UID, RecentLocationApps.ANDROID_SYSTEM_PACKAGE_NAME);

        List<RecentLocationApps.Request> requests = mRecentLocationApps.getAppList();
        // Android OS shouldn't show up in the list of apps.
        assertThat(requests).hasSize(2);
        // Make sure apps are ordered by recency
        assertThat(requests.get(0).packageName).isEqualTo(TEST_PACKAGE_NAMES[0]);
        assertThat(requests.get(0).requestFinishTime).isEqualTo(ONE_MIN_AGO + DURATION);
        assertThat(requests.get(1).packageName).isEqualTo(TEST_PACKAGE_NAMES[1]);
        assertThat(requests.get(1).requestFinishTime).isEqualTo(TWENTY_THREE_HOURS_AGO + DURATION);
    }

    private void mockTestApplicationInfos(int userId, String... packageNameList)
            throws NameNotFoundException {
        for (String packageName : packageNameList) {
            ApplicationInfo appInfo = new ApplicationInfo();
            appInfo.packageName = packageName;
            when(mPackageManager.getApplicationInfoAsUser(
                    packageName, PackageManager.GET_META_DATA, userId)).thenReturn(appInfo);
        }
    }

    private List<PackageOps> createTestPackageOpsList(String[] packageNameList, long[] time) {
        List<PackageOps> packageOpsList = new ArrayList<>();
        for (int i = 0; i < packageNameList.length ; i++) {
            PackageOps packageOps = createPackageOps(
                    packageNameList[i],
                    TEST_UID,
                    AppOpsManager.OP_MONITOR_LOCATION,
                    time[i],
                    DURATION);
            packageOpsList.add(packageOps);
        }
        return packageOpsList;
    }

    private PackageOps createPackageOps(
            String packageName, int uid, int op, long time, int duration) {
        return new PackageOps(
                packageName,
                uid,
                Collections.singletonList(createOpEntryWithTime(op, time, duration)));
    }

    private OpEntry createOpEntryWithTime(int op, long time, int duration) {
        return new OpEntry(op, AppOpsManager.MODE_ALLOWED, time, 0L, duration, 0, "");
    }
}
