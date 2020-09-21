/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.settings.applications;

import android.app.Application;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.UserHandle;
import android.provider.Settings;
import android.provider.Settings.Global;
import android.support.annotation.VisibleForTesting;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.Preference.OnPreferenceChangeListener;
import android.support.v7.preference.Preference.OnPreferenceClickListener;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.PreferenceGroup;
import android.support.v7.preference.PreferenceViewHolder;
import android.util.ArraySet;
import android.view.View;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.SettingsPreferenceFragment;
import com.android.settings.Utils;
import com.android.settings.widget.AppPreference;
import com.android.settingslib.applications.ApplicationsState;
import com.android.settingslib.applications.ApplicationsState.AppEntry;

import java.util.ArrayList;

/**
 * Activity to manage how Android handles URL resolution. Includes both per-app
 * handling as well as system handling for Web Actions.
 */
public class ManageDomainUrls extends SettingsPreferenceFragment
        implements ApplicationsState.Callbacks, OnPreferenceChangeListener,
        OnPreferenceClickListener {

    // constant value that can be used to check return code from sub activity.
    private static final int INSTALLED_APP_DETAILS = 1;

    private ApplicationsState mApplicationsState;
    private ApplicationsState.Session mSession;
    private PreferenceGroup mDomainAppList;
    private SwitchPreference mWebAction;
    private Preference mInstantAppAccountPreference;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setAnimationAllowed(true);
        mApplicationsState = ApplicationsState.getInstance(
                (Application) getContext().getApplicationContext());
        mSession = mApplicationsState.newSession(this, getLifecycle());
        setHasOptionsMenu(true);
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.manage_domain_url_settings;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
    }

    @Override
    public void onRunningStateChanged(boolean running) {
    }

    @Override
    public void onPackageListChanged() {
    }

    @Override
    public void onRebuildComplete(ArrayList<AppEntry> apps) {
        if (getContext() == null) {
            return;
        }

        final boolean disableWebActions = Global.getInt(getContext().getContentResolver(),
                Global.ENABLE_EPHEMERAL_FEATURE, 1) == 0;
        if (disableWebActions) {
            mDomainAppList = getPreferenceScreen();
        } else {
            final PreferenceGroup preferenceScreen = getPreferenceScreen();
            if (preferenceScreen.getPreferenceCount() == 0) {
                // add preferences
                final PreferenceCategory webActionCategory =
                        new PreferenceCategory(getPrefContext());
                webActionCategory.setTitle(R.string.web_action_section_title);
                preferenceScreen.addPreference(webActionCategory);

                // toggle to enable / disable Web Actions [aka Instant Apps]
                mWebAction = new SwitchPreference(getPrefContext());
                mWebAction.setTitle(R.string.web_action_enable_title);
                mWebAction.setSummary(R.string.web_action_enable_summary);
                mWebAction.setChecked(Settings.Secure.getInt(getContentResolver(),
                        Settings.Secure.INSTANT_APPS_ENABLED, 1) != 0);
                mWebAction.setOnPreferenceChangeListener(this);
                webActionCategory.addPreference(mWebAction);

                // Determine whether we should show the instant apps account chooser setting
                ComponentName instantAppSettingsComponent = getActivity().getPackageManager()
                        .getInstantAppResolverSettingsComponent();
                Intent instantAppSettingsIntent = null;
                if (instantAppSettingsComponent != null) {
                    instantAppSettingsIntent =
                            new Intent().setComponent(instantAppSettingsComponent);
                }
                if (instantAppSettingsIntent != null) {
                    final Intent launchIntent = instantAppSettingsIntent;
                    // TODO: Make this button actually launch the account chooser.
                    mInstantAppAccountPreference = new Preference(getPrefContext());
                    mInstantAppAccountPreference.setTitle(R.string.instant_apps_settings);
                    mInstantAppAccountPreference.setOnPreferenceClickListener(pref -> {
                        startActivity(launchIntent);
                        return true;
                    });
                    webActionCategory.addPreference(mInstantAppAccountPreference);
                }

                // list to manage link handling per app
                mDomainAppList = new PreferenceCategory(getPrefContext());
                mDomainAppList.setTitle(R.string.domain_url_section_title);
                preferenceScreen.addPreference(mDomainAppList);
            }
        }
        rebuildAppList(mDomainAppList, apps);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (preference == mWebAction) {
            boolean checked = (boolean) newValue;
            Settings.Secure.putInt(
                    getContentResolver(),
                    Settings.Secure.INSTANT_APPS_ENABLED, checked ? 1 : 0);
            return true;
        }
        return false;
    }

    private void rebuild() {
        final ArrayList<AppEntry> apps = mSession.rebuild(
                ApplicationsState.FILTER_WITH_DOMAIN_URLS, ApplicationsState.ALPHA_COMPARATOR);
        if (apps != null) {
            onRebuildComplete(apps);
        }
    }

    private void rebuildAppList(PreferenceGroup group, ArrayList<AppEntry> apps) {
        cacheRemoveAllPrefs(group);
        final int N = apps.size();
        for (int i = 0; i < N; i++) {
            AppEntry entry = apps.get(i);
            String key = entry.info.packageName + "|" + entry.info.uid;
            DomainAppPreference preference = (DomainAppPreference) getCachedPreference(key);
            if (preference == null) {
                preference = new DomainAppPreference(getPrefContext(), mApplicationsState, entry);
                preference.setKey(key);
                preference.setOnPreferenceClickListener(this);
                group.addPreference(preference);
            } else {
                preference.reuse();
            }
            preference.setOrder(i);
        }
        removeCachedPrefs(group);
    }

    @Override
    public void onPackageIconChanged() {
    }

    @Override
    public void onPackageSizeChanged(String packageName) {
    }

    @Override
    public void onAllSizesComputed() {
    }

    @Override
    public void onLauncherInfoChanged() {
    }

    @Override
    public void onLoadEntriesCompleted() {
        rebuild();
    }

    @Override
    public int getMetricsCategory() {
        return MetricsEvent.MANAGE_DOMAIN_URLS;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        if (preference.getClass() == DomainAppPreference.class) {
            ApplicationsState.AppEntry entry = ((DomainAppPreference) preference).mEntry;
            AppInfoBase.startAppInfoFragment(AppLaunchSettings.class, R.string.auto_launch_label,
                    entry.info.packageName, entry.info.uid, this,
                    INSTALLED_APP_DETAILS, getMetricsCategory());
            return true;
        }
        return false;
    }

    @VisibleForTesting
    static class DomainAppPreference extends AppPreference {
        private final AppEntry mEntry;
        private final PackageManager mPm;
        private final ApplicationsState mApplicationsState;

        public DomainAppPreference(final Context context, ApplicationsState applicationsState,
                AppEntry entry) {
            super(context);
            mApplicationsState = applicationsState;
            mPm = context.getPackageManager();
            mEntry = entry;
            mEntry.ensureLabel(getContext());
            setState();
            if (mEntry.icon != null) {
                setIcon(mEntry.icon);
            }
        }

        private void setState() {
            setTitle(mEntry.label);
            setSummary(getDomainsSummary(mEntry.info.packageName));
        }

        public void reuse() {
            setState();
            notifyChanged();
        }

        @Override
        public void onBindViewHolder(PreferenceViewHolder holder) {
            if (mEntry.icon == null) {
                holder.itemView.post(new Runnable() {
                    @Override
                    public void run() {
                        // Ensure we have an icon before binding.
                        mApplicationsState.ensureIcon(mEntry);
                        // This might trigger us to bind again, but it gives an easy way to only
                        // load the icon once its needed, so its probably worth it.
                        setIcon(mEntry.icon);
                    }
                });
            }
            super.onBindViewHolder(holder);
        }

        private CharSequence getDomainsSummary(String packageName) {
            // If the user has explicitly said "no" for this package, that's the
            // string we should show.
            int domainStatus =
                    mPm.getIntentVerificationStatusAsUser(packageName, UserHandle.myUserId());
            if (domainStatus == PackageManager.INTENT_FILTER_DOMAIN_VERIFICATION_STATUS_NEVER) {
                return getContext().getString(R.string.domain_urls_summary_none);
            }
            // Otherwise, ask package manager for the domains for this package,
            // and show the first one (or none if there aren't any).
            ArraySet<String> result = Utils.getHandledDomains(mPm, packageName);
            if (result.size() == 0) {
                return getContext().getString(R.string.domain_urls_summary_none);
            } else if (result.size() == 1) {
                return getContext().getString(R.string.domain_urls_summary_one, result.valueAt(0));
            } else {
                return getContext().getString(R.string.domain_urls_summary_some, result.valueAt(0));
            }
        }
    }
}
