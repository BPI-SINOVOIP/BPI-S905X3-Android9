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

package com.android.tv.settings.device.apps.specialaccess;

import android.app.AppOpsManager;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.annotation.Keep;
import android.support.annotation.NonNull;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceGroup;
import android.support.v7.preference.TwoStatePreference;
import android.util.Log;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settingslib.applications.ApplicationsState;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;

/**
 * Fragment for managing which apps are granted PIP access
 */
@Keep
public class PictureInPicture extends SettingsPreferenceFragment
        implements ManageApplicationsController.Callback {
    private static final String TAG = "PictureInPicture";

    private ManageApplicationsController mManageApplicationsController;
    private AppOpsManager mAppOpsManager;

    private final ApplicationsState.AppFilter mFilter =
            new ApplicationsState.CompoundFilter(
                    new ApplicationsState.CompoundFilter(
                            ApplicationsState.FILTER_WITHOUT_DISABLED_UNTIL_USED,
                            ApplicationsState.FILTER_ALL_ENABLED),

                    new ApplicationsState.AppFilter() {
                        @Override
                        public void init() {}

                        @Override
                        public boolean filterApp(ApplicationsState.AppEntry info) {
                            info.extraInfo = mAppOpsManager.checkOpNoThrow(
                                    AppOpsManager.OP_PICTURE_IN_PICTURE,
                                    info.info.uid,
                                    info.info.packageName) == AppOpsManager.MODE_ALLOWED;
                            return !ManageAppOp.shouldIgnorePackage(
                                    getContext(), info.info.packageName)
                                    && checkPackageHasPipActivities(info.info.packageName);
                        }
                    });

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mAppOpsManager = getContext().getSystemService(AppOpsManager.class);
        mManageApplicationsController = new ManageApplicationsController(getContext(), this,
                getLifecycle(), mFilter, ApplicationsState.ALPHA_COMPARATOR);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.picture_in_picture, null);
    }

    @Override
    public void onResume() {
        super.onResume();
        mManageApplicationsController.updateAppList();
    }

    private boolean checkPackageHasPipActivities(String packageName) {
        try {
            final PackageInfo packageInfo = getContext().getPackageManager().getPackageInfo(
                    packageName, PackageManager.GET_ACTIVITIES);
            if (packageInfo.activities == null) {
                return false;
            }
            for (ActivityInfo info : packageInfo.activities) {
                if (info.supportsPictureInPicture()) {
                    return true;
                }
            }
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Exception while fetching package info for " + packageName, e);
            return false;
        }
        return false;
    }

    @NonNull
    @Override
    public Preference bindPreference(@NonNull Preference preference,
            ApplicationsState.AppEntry entry) {
        final TwoStatePreference switchPref = (SwitchPreference) preference;
        switchPref.setTitle(entry.label);
        switchPref.setKey(entry.info.packageName);
        switchPref.setIcon(entry.icon);
        switchPref.setChecked((Boolean) entry.extraInfo);
        switchPref.setOnPreferenceChangeListener((pref, newValue) -> {
            mAppOpsManager.setMode(AppOpsManager.OP_PICTURE_IN_PICTURE,
                    entry.info.uid,
                    entry.info.packageName,
                    (Boolean) newValue ? AppOpsManager.MODE_ALLOWED : AppOpsManager.MODE_ERRORED);
            return true;
        });
        switchPref.setSummary((Boolean) entry.extraInfo
                ? R.string.app_permission_summary_allowed
                : R.string.app_permission_summary_not_allowed);
        return switchPref;
    }

    @NonNull
    @Override
    public Preference createAppPreference() {
        return new SwitchPreference(getPreferenceManager().getContext());
    }

    @NonNull
    @Override
    public Preference getEmptyPreference() {
        final Preference empty = new Preference(getPreferenceManager().getContext());
        empty.setKey("empty");
        empty.setTitle(R.string.picture_in_picture_empty_text);
        empty.setEnabled(false);
        return empty;
    }

    @NonNull
    @Override
    public PreferenceGroup getAppPreferenceGroup() {
        return getPreferenceScreen();
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.SETTINGS_MANAGE_PICTURE_IN_PICTURE;
    }
}
