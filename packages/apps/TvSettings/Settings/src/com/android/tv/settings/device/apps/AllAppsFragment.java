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

import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceGroup;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.Log;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settingslib.applications.ApplicationsState;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;

import java.util.ArrayList;
import java.util.Map;
import java.util.Set;

/**
 * Fragment for listing and managing all apps on the device.
 */
public class AllAppsFragment extends SettingsPreferenceFragment implements
        Preference.OnPreferenceClickListener {

    private static final String TAG = "AllAppsFragment";
    private static final String KEY_SHOW_OTHER_APPS = "ShowOtherApps";

    private static final @ApplicationsState.SessionFlags int SESSION_FLAGS =
            ApplicationsState.FLAG_SESSION_REQUEST_HOME_APP
            | ApplicationsState.FLAG_SESSION_REQUEST_ICONS
            | ApplicationsState.FLAG_SESSION_REQUEST_SIZES
            | ApplicationsState.FLAG_SESSION_REQUEST_LEANBACK_LAUNCHER;

    private ApplicationsState mApplicationsState;
    private ApplicationsState.Session mSessionInstalled;
    private ApplicationsState.AppFilter mFilterInstalled;
    private ApplicationsState.Session mSessionDisabled;
    private ApplicationsState.AppFilter mFilterDisabled;
    private ApplicationsState.Session mSessionOther;
    private ApplicationsState.AppFilter mFilterOther;

    private PreferenceGroup mInstalledPreferenceGroup;
    private PreferenceGroup mDisabledPreferenceGroup;
    private PreferenceGroup mOtherPreferenceGroup;
    private Preference mShowOtherApps;

    private final Handler mHandler = new Handler();
    private final Map<PreferenceGroup,
            ArrayList<ApplicationsState.AppEntry>> mUpdateMap = new ArrayMap<>(3);
    private long mRunAt = Long.MIN_VALUE;
    private final Runnable mUpdateRunnable = new Runnable() {
        @Override
        public void run() {
            for (final PreferenceGroup group : mUpdateMap.keySet()) {
                final ArrayList<ApplicationsState.AppEntry> entries = mUpdateMap.get(group);
                updateAppListInternal(group, entries);
            }
            mUpdateMap.clear();
            mRunAt = 0;
        }
    };

    /** Prepares arguments for the fragment. */
    public static void prepareArgs(Bundle b, String volumeUuid, String volumeName) {
        b.putString(AppsActivity.EXTRA_VOLUME_UUID, volumeUuid);
        b.putString(AppsActivity.EXTRA_VOLUME_NAME, volumeName);
    }

    /** Creates a new instance of the fragment. */
    public static AllAppsFragment newInstance(String volumeUuid, String volumeName) {
        final Bundle b = new Bundle(2);
        prepareArgs(b, volumeUuid, volumeName);
        final AllAppsFragment f = new AllAppsFragment();
        f.setArguments(b);
        return f;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mApplicationsState = ApplicationsState.getInstance(getActivity().getApplication());

        final String volumeUuid = getArguments().getString(AppsActivity.EXTRA_VOLUME_UUID);
        final String volumeName = getArguments().getString(AppsActivity.EXTRA_VOLUME_NAME);

        // The UUID of internal storage is null, so we check if there's a volume name to see if we
        // should only be showing the apps on the internal storage or all apps.
        if (!TextUtils.isEmpty(volumeUuid) || !TextUtils.isEmpty(volumeName)) {
            ApplicationsState.AppFilter volumeFilter =
                    new ApplicationsState.VolumeFilter(volumeUuid);

            mFilterInstalled =
                    new ApplicationsState.CompoundFilter(FILTER_INSTALLED, volumeFilter);
            mFilterDisabled =
                    new ApplicationsState.CompoundFilter(FILTER_DISABLED, volumeFilter);
            mFilterOther =
                    new ApplicationsState.CompoundFilter(FILTER_OTHER, volumeFilter);
        } else {
            mFilterInstalled = FILTER_INSTALLED;
            mFilterDisabled = FILTER_DISABLED;
            mFilterOther = FILTER_OTHER;
        }

        mSessionInstalled = mApplicationsState.newSession(new RowUpdateCallbacks() {
            @Override
            protected void doRebuild() {
                rebuildInstalled();
            }

            @Override
            public void onRebuildComplete(ArrayList<ApplicationsState.AppEntry> apps) {
                updateAppList(mInstalledPreferenceGroup, apps);
            }
        }, getLifecycle());
        mSessionInstalled.setSessionFlags(SESSION_FLAGS);

        mSessionDisabled = mApplicationsState.newSession(new RowUpdateCallbacks() {
            @Override
            protected void doRebuild() {
                rebuildDisabled();
            }

            @Override
            public void onRebuildComplete(ArrayList<ApplicationsState.AppEntry> apps) {
                updateAppList(mDisabledPreferenceGroup, apps);
            }
        }, getLifecycle());
        mSessionDisabled.setSessionFlags(SESSION_FLAGS);

        mSessionOther = mApplicationsState.newSession(new RowUpdateCallbacks() {
            @Override
            protected void doRebuild() {
                if (!mShowOtherApps.isVisible()) {
                    rebuildOther();
                }
            }

            @Override
            public void onRebuildComplete(ArrayList<ApplicationsState.AppEntry> apps) {
                updateAppList(mOtherPreferenceGroup, apps);
            }
        }, getLifecycle());
        mSessionOther.setSessionFlags(SESSION_FLAGS);


        rebuildInstalled();
        rebuildDisabled();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.all_apps, null);
        mInstalledPreferenceGroup = (PreferenceGroup) findPreference("InstalledPreferenceGroup");
        mDisabledPreferenceGroup = (PreferenceGroup) findPreference("DisabledPreferenceGroup");
        mOtherPreferenceGroup = (PreferenceGroup) findPreference("OtherPreferenceGroup");
        mOtherPreferenceGroup.setVisible(false);
        mShowOtherApps = findPreference(KEY_SHOW_OTHER_APPS);
        mShowOtherApps.setOnPreferenceClickListener(this);
        final String volumeUuid = getArguments().getString(AppsActivity.EXTRA_VOLUME_UUID);
        mShowOtherApps.setVisible(TextUtils.isEmpty(volumeUuid));
    }

    private void rebuildInstalled() {
        ArrayList<ApplicationsState.AppEntry> apps =
                mSessionInstalled.rebuild(mFilterInstalled, ApplicationsState.ALPHA_COMPARATOR);
        if (apps != null) {
            updateAppList(mInstalledPreferenceGroup, apps);
        }
    }

    private void rebuildDisabled() {
        ArrayList<ApplicationsState.AppEntry> apps =
                mSessionDisabled.rebuild(mFilterDisabled, ApplicationsState.ALPHA_COMPARATOR);
        if (apps != null) {
            updateAppList(mDisabledPreferenceGroup, apps);
        }
    }

    private void rebuildOther() {
        ArrayList<ApplicationsState.AppEntry> apps =
                mSessionOther.rebuild(mFilterOther, ApplicationsState.ALPHA_COMPARATOR);
        if (apps != null) {
            updateAppList(mOtherPreferenceGroup, apps);
        }
    }

    private void updateAppList(PreferenceGroup group,
            ArrayList<ApplicationsState.AppEntry> entries) {
        if (group == null) {
            Log.d(TAG, "Not updating list for null group");
            return;
        }
        mUpdateMap.put(group, entries);

        // We can get spammed with updates, so coalesce them to reduce jank and flicker
        if (mRunAt == Long.MIN_VALUE) {
            // First run, no delay
            mHandler.removeCallbacks(mUpdateRunnable);
            mHandler.post(mUpdateRunnable);
        } else {
            if (mRunAt == 0) {
                mRunAt = SystemClock.uptimeMillis() + 1000;
            }
            int delay = (int) (mRunAt - SystemClock.uptimeMillis());
            delay = delay < 0 ? 0 : delay;

            mHandler.removeCallbacks(mUpdateRunnable);
            mHandler.postDelayed(mUpdateRunnable, delay);
        }
    }

    private void updateAppListInternal(PreferenceGroup group,
            ArrayList<ApplicationsState.AppEntry> entries) {
        if (entries != null) {
            final Set<String> touched = new ArraySet<>(entries.size());
            for (final ApplicationsState.AppEntry entry : entries) {
                final String packageName = entry.info.packageName;
                Preference recycle = group.findPreference(packageName);
                if (recycle == null) {
                    recycle = new Preference(getPreferenceManager().getContext());
                }
                final Preference newPref = bindPreference(recycle, entry);
                group.addPreference(newPref);
                touched.add(packageName);
            }
            for (int i = 0; i < group.getPreferenceCount();) {
                final Preference pref = group.getPreference(i);
                if (touched.contains(pref.getKey())) {
                    i++;
                } else {
                    group.removePreference(pref);
                }
            }
        }
        mDisabledPreferenceGroup.setVisible(mDisabledPreferenceGroup.getPreferenceCount() > 0);
    }

    /**
     * Creates or updates a preference according to an {@link ApplicationsState.AppEntry} object
     * @param preference If non-null, updates this preference object, otherwise creates a new one
     * @param entry Info to populate preference
     * @return Updated preference entry
     */
    private Preference bindPreference(@NonNull Preference preference,
            ApplicationsState.AppEntry entry) {
        preference.setKey(entry.info.packageName);
        entry.ensureLabel(getContext());
        preference.setTitle(entry.label);
        preference.setSummary(entry.sizeStr);
        preference.setFragment(AppManagementFragment.class.getName());
        AppManagementFragment.prepareArgs(preference.getExtras(), entry.info.packageName);
        preference.setIcon(entry.icon);
        return preference;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        if  (KEY_SHOW_OTHER_APPS.equals(preference.getKey())) {
            showOtherApps();
            return true;
        }
        return false;
    }

    private void showOtherApps() {
        mShowOtherApps.setVisible(false);
        mOtherPreferenceGroup.setVisible(true);
        rebuildOther();
    }

    private abstract class RowUpdateCallbacks implements ApplicationsState.Callbacks {

        protected abstract void doRebuild();

        @Override
        public void onRunningStateChanged(boolean running) {
            doRebuild();
        }

        @Override
        public void onPackageListChanged() {
            doRebuild();
        }

        @Override
        public void onPackageIconChanged() {
            doRebuild();
        }

        @Override
        public void onPackageSizeChanged(String packageName) {
            doRebuild();
        }

        @Override
        public void onAllSizesComputed() {
            doRebuild();
        }

        @Override
        public void onLauncherInfoChanged() {
            doRebuild();
        }

        @Override
        public void onLoadEntriesCompleted() {
            doRebuild();
        }
    }

    private static final ApplicationsState.AppFilter FILTER_INSTALLED =
            new ApplicationsState.AppFilter() {

                @Override
                public void init() {}

                @Override
                public boolean filterApp(ApplicationsState.AppEntry info) {
                    return !FILTER_DISABLED.filterApp(info)
                            && info.info != null
                            && info.info.enabled
                            && info.hasLauncherEntry
                            && info.launcherEntryEnabled;
                }
            };

    private static final ApplicationsState.AppFilter FILTER_DISABLED =
            new ApplicationsState.AppFilter() {

                @Override
                public void init() {
                }

                @Override
                public boolean filterApp(ApplicationsState.AppEntry info) {
                    return info.info != null
                            && (info.info.enabledSetting
                            == PackageManager.COMPONENT_ENABLED_STATE_DISABLED
                            || info.info.enabledSetting
                            == PackageManager.COMPONENT_ENABLED_STATE_DISABLED_USER
                            || (info.info.enabledSetting
                            == PackageManager.COMPONENT_ENABLED_STATE_DEFAULT
                            && !info.info.enabled));
                }
            };

    private static final ApplicationsState.AppFilter FILTER_OTHER =
            new ApplicationsState.AppFilter() {

                @Override
                public void init() {}

                @Override
                public boolean filterApp(ApplicationsState.AppEntry info) {
                    return !FILTER_INSTALLED.filterApp(info) && !FILTER_DISABLED.filterApp(info);
                }
            };

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.MANAGE_APPLICATIONS;
    }
}
