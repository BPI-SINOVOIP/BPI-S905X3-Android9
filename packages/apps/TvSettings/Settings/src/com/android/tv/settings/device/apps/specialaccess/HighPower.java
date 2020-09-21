/*
 * Copyright (C) 2017 The Android Open Source Project
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

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.Keep;
import android.support.annotation.NonNull;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceGroup;
import android.support.v7.preference.TwoStatePreference;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settingslib.applications.ApplicationsState;
import com.android.settingslib.fuelgauge.PowerWhitelistBackend;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;

/**
 * Fragment for managing power save whitelist
 */
@Keep
public class HighPower extends SettingsPreferenceFragment implements
        ManageApplicationsController.Callback {

    private PowerWhitelistBackend mPowerWhitelistBackend;
    private ManageApplicationsController mManageApplicationsController;
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
                            info.extraInfo =
                                    mPowerWhitelistBackend.isWhitelisted(info.info.packageName);
                            return !ManageAppOp.shouldIgnorePackage(getContext(),
                                    info.info.packageName);
                        }
                    });

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.APPLICATIONS_HIGH_POWER_APPS;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mPowerWhitelistBackend = PowerWhitelistBackend.getInstance(context);
        mManageApplicationsController = new ManageApplicationsController(context, this,
                getLifecycle(), mFilter, ApplicationsState.ALPHA_COMPARATOR);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.manage_high_power, null);
    }

    @Override
    public void onResume() {
        super.onResume();
        mManageApplicationsController.updateAppList();
    }

    @NonNull
    @Override
    public Preference bindPreference(@NonNull Preference preference,
            ApplicationsState.AppEntry entry) {
        final TwoStatePreference switchPref = (SwitchPreference) preference;
        switchPref.setTitle(entry.label);
        switchPref.setKey(entry.info.packageName);
        switchPref.setIcon(entry.icon);
        if (mPowerWhitelistBackend.isSysWhitelisted(entry.info.packageName)) {
            switchPref.setChecked(false);
            switchPref.setEnabled(false);
        } else {
            switchPref.setEnabled(true);
            switchPref.setChecked(!(Boolean) entry.extraInfo);
            switchPref.setOnPreferenceChangeListener((pref, newValue) -> {
                final String pkg = pref.getKey();
                if ((Boolean) newValue) {
                    mPowerWhitelistBackend.removeApp(pkg);
                } else {
                    mPowerWhitelistBackend.addApp(pkg);
                }
                updateSummary(pref);
                return true;
            });
        }
        updateSummary(switchPref);
        return switchPref;
    }

    private void updateSummary(Preference preference) {
        final String pkg = preference.getKey();
        if (mPowerWhitelistBackend.isSysWhitelisted(pkg)) {
            preference.setSummary(R.string.high_power_system);
        } else if (mPowerWhitelistBackend.isWhitelisted(pkg)) {
            preference.setSummary(R.string.high_power_on);
        } else {
            preference.setSummary(R.string.high_power_off);
        }
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
        empty.setTitle(R.string.high_power_apps_empty);
        empty.setEnabled(false);
        return empty;
    }

    @NonNull
    @Override
    public PreferenceGroup getAppPreferenceGroup() {
        return getPreferenceScreen();
    }
}
