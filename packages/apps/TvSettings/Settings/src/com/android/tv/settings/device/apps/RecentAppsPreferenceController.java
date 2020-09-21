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
 * limitations under the License.
 */

package com.android.tv.settings.device.apps;

import android.app.Application;
import android.app.usage.UsageStats;
import android.app.usage.UsageStatsManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.UserHandle;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.PreferenceScreen;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.IconDrawableFactory;
import android.util.Log;

import com.android.settingslib.applications.AppUtils;
import com.android.settingslib.applications.ApplicationsState;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.settingslib.utils.StringUtil;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * This controller displays a list of recently used apps and a "See all" button.
 */
public class RecentAppsPreferenceController extends AbstractPreferenceController
        implements Comparator<UsageStats> {

    private static final String TAG = "RecentAppsPreferenceController";
    private static final String KEY_PREF_CATEGORY = "recently_used_apps_category";
    @VisibleForTesting
    static final String KEY_SEE_ALL = "see_all_apps";
    private static final int SHOW_RECENT_APP_COUNT = 5;
    private static final Set<String> SKIP_SYSTEM_PACKAGES = new ArraySet<>();

    private final PackageManager mPm;
    private final UsageStatsManager mUsageStatsManager;
    private final ApplicationsState mApplicationsState;
    private final int mUserId;
    private final IconDrawableFactory mIconDrawableFactory;

    private Calendar mCal;
    private List<UsageStats> mStats;

    private PreferenceCategory mCategory;

    static {
        SKIP_SYSTEM_PACKAGES.addAll(Arrays.asList(
                "android",
                "com.android.tv.settings",
                "com.android.systemui",
                "com.android.providers.calendar",
                "com.android.providers.media"
        ));
    }

    public RecentAppsPreferenceController(Context context, Application app) {
        this(context, app == null ? null : ApplicationsState.getInstance(app));
    }

    @VisibleForTesting(otherwise = VisibleForTesting.NONE)
    RecentAppsPreferenceController(Context context, ApplicationsState appState) {
        super(context);
        mIconDrawableFactory = IconDrawableFactory.newInstance(context);
        mUserId = UserHandle.myUserId();
        mPm = context.getPackageManager();
        mUsageStatsManager =
                (UsageStatsManager) context.getSystemService(Context.USAGE_STATS_SERVICE);
        mApplicationsState = appState;
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_PREF_CATEGORY;
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        mCategory = (PreferenceCategory) screen.findPreference(getPreferenceKey());
        refreshUi(mCategory.getContext());
    }

    @VisibleForTesting
    void refreshUi(Context prefContext) {
        reloadData();
        final List<UsageStats> recentApps = getDisplayableRecentAppList();
        if (recentApps != null && !recentApps.isEmpty()) {
            displayRecentApps(prefContext, recentApps);
        } else {
            displayOnlyAllApps();
        }
    }

    private void displayOnlyAllApps() {
        mCategory.setVisible(false);
        int prefCount = mCategory.getPreferenceCount();
        for (int i = prefCount - 1; i >= 0; i--) {
            final Preference pref = mCategory.getPreference(i);
            if (!TextUtils.equals(pref.getKey(), KEY_SEE_ALL)) {
                mCategory.removePreference(pref);
            }
        }
    }

    private void displayRecentApps(Context prefContext, List<UsageStats> recentApps) {
        mCategory.setVisible(true);

        // Rebind prefs/avoid adding new prefs if possible. Adding/removing prefs causes jank.
        // Build a cached preference pool
        final Map<String, Preference> appPreferences = new ArrayMap<>();
        int prefCount = mCategory.getPreferenceCount();
        for (int i = 0; i < prefCount; i++) {
            final Preference pref = mCategory.getPreference(i);
            final String key = pref.getKey();
            if (!TextUtils.equals(key, KEY_SEE_ALL)) {
                appPreferences.put(key, pref);
            }
        }
        final int recentAppsCount = recentApps.size();
        for (int i = 0; i < recentAppsCount; i++) {
            final UsageStats stat = recentApps.get(i);
            // Bind recent apps to existing prefs if possible, or create a new pref.
            final String pkgName = stat.getPackageName();
            final ApplicationsState.AppEntry appEntry =
                    mApplicationsState.getEntry(pkgName, mUserId);
            if (appEntry == null) {
                continue;
            }

            boolean rebindPref = true;
            Preference pref = appPreferences.remove(pkgName);
            if (pref == null) {
                pref = new Preference(prefContext);
                rebindPref = false;
            }
            pref.setKey(pkgName);
            pref.setTitle(appEntry.label);
            pref.setIcon(mIconDrawableFactory.getBadgedIcon(appEntry.info));
            pref.setSummary(StringUtil.formatRelativeTime(mContext,
                    System.currentTimeMillis() - stat.getLastTimeUsed(), false));
            pref.setOrder(i);
            AppManagementFragment.prepareArgs(pref.getExtras(), pkgName);
            pref.setFragment(AppManagementFragment.class.getName());
            if (!rebindPref) {
                mCategory.addPreference(pref);
            }
        }
        // Remove unused prefs from pref cache pool
        for (Preference unusedPrefs : appPreferences.values()) {
            mCategory.removePreference(unusedPrefs);
        }
    }

    @Override
    public void updateState(Preference preference) {
        super.updateState(preference);
        refreshUi(mCategory.getContext());
    }

    @Override
    public final int compare(UsageStats a, UsageStats b) {
        // return by descending order
        return Long.compare(b.getLastTimeUsed(), a.getLastTimeUsed());
    }

    @VisibleForTesting
    void reloadData() {
        mCal = Calendar.getInstance();
        mCal.add(Calendar.DAY_OF_YEAR, -1);
        mStats = mUsageStatsManager.queryUsageStats(
                UsageStatsManager.INTERVAL_BEST, mCal.getTimeInMillis(),
                System.currentTimeMillis());
    }

    private List<UsageStats> getDisplayableRecentAppList() {
        final List<UsageStats> recentApps = new ArrayList<>();
        final Map<String, UsageStats> map = new ArrayMap<>();
        final int statCount = mStats.size();
        for (int i = 0; i < statCount; i++) {
            final UsageStats pkgStats = mStats.get(i);
            if (!shouldIncludePkgInRecents(pkgStats)) {
                continue;
            }
            final String pkgName = pkgStats.getPackageName();
            final UsageStats existingStats = map.get(pkgName);
            if (existingStats == null) {
                map.put(pkgName, pkgStats);
            } else {
                existingStats.add(pkgStats);
            }
        }
        final List<UsageStats> packageStats = new ArrayList<>();
        packageStats.addAll(map.values());
        Collections.sort(packageStats, this /* comparator */);
        int count = 0;
        for (UsageStats stat : packageStats) {
            final ApplicationsState.AppEntry appEntry = mApplicationsState.getEntry(
                    stat.getPackageName(), mUserId);
            if (appEntry == null) {
                continue;
            }
            recentApps.add(stat);
            count++;
            if (count >= SHOW_RECENT_APP_COUNT) {
                break;
            }
        }
        return recentApps;
    }

    /**
     * Whether or not the app should be included in recent list.
     */
    private boolean shouldIncludePkgInRecents(UsageStats stat) {
        final String pkgName = stat.getPackageName();
        if (stat.getLastTimeUsed() < mCal.getTimeInMillis()) {
            Log.d(TAG, "Invalid timestamp, skipping " + pkgName);
            return false;
        }

        if (SKIP_SYSTEM_PACKAGES.contains(pkgName)) {
            Log.d(TAG, "System package, skipping " + pkgName);
            return false;
        }
        final Intent launchIntent = new Intent().addCategory(Intent.CATEGORY_LEANBACK_LAUNCHER)
                .setPackage(pkgName);

        if (mPm.resolveActivity(launchIntent, 0) == null) {
            // Not visible on launcher -> likely not a user visible app, skip if non-instant.
            final ApplicationsState.AppEntry appEntry =
                    mApplicationsState.getEntry(pkgName, mUserId);
            if (appEntry == null || appEntry.info == null || !AppUtils.isInstant(appEntry.info)) {
                Log.d(TAG, "Not a user visible or instant app, skipping " + pkgName);
                return false;
            }
        }
        return true;
    }
}
