/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.settings.fuelgauge;

import static android.app.usage.UsageStatsManager.STANDBY_BUCKET_ACTIVE;
import static android.app.usage.UsageStatsManager.STANDBY_BUCKET_EXEMPTED;
import static android.app.usage.UsageStatsManager.STANDBY_BUCKET_FREQUENT;
import static android.app.usage.UsageStatsManager.STANDBY_BUCKET_NEVER;
import static android.app.usage.UsageStatsManager.STANDBY_BUCKET_RARE;
import static android.app.usage.UsageStatsManager.STANDBY_BUCKET_WORKING_SET;

import android.app.usage.UsageStatsManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.os.Bundle;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.Preference.OnPreferenceClickListener;
import android.support.v7.preference.PreferenceGroup;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.SettingsPreferenceFragment;
import com.android.settings.widget.RadioButtonPreference;

import java.util.List;

public class InactiveApps extends SettingsPreferenceFragment
        implements Preference.OnPreferenceChangeListener {

    private static final CharSequence[] SETTABLE_BUCKETS_NAMES =
            {"ACTIVE", "WORKING_SET", "FREQUENT", "RARE"};

    private static final CharSequence[] SETTABLE_BUCKETS_VALUES = {
            Integer.toString(STANDBY_BUCKET_ACTIVE),
            Integer.toString(STANDBY_BUCKET_WORKING_SET),
            Integer.toString(STANDBY_BUCKET_FREQUENT),
            Integer.toString(STANDBY_BUCKET_RARE)
    };

    private UsageStatsManager mUsageStats;

    @Override
    public int getMetricsCategory() {
        return MetricsEvent.FUELGAUGE_INACTIVE_APPS;
    }

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        mUsageStats = getActivity().getSystemService(UsageStatsManager.class);
        addPreferencesFromResource(R.xml.inactive_apps);
    }

    @Override
    public void onResume() {
        super.onResume();
        init();
    }

    private void init() {
        PreferenceGroup screen = getPreferenceScreen();
        screen.removeAll();
        screen.setOrderingAsAdded(false);
        final Context context = getActivity();
        final PackageManager pm = context.getPackageManager();
        final UsageStatsManager usm = context.getSystemService(UsageStatsManager.class);

        Intent launcherIntent = new Intent(Intent.ACTION_MAIN);
        launcherIntent.addCategory(Intent.CATEGORY_LAUNCHER);
        List<ResolveInfo> apps = pm.queryIntentActivities(launcherIntent, 0);
        for (ResolveInfo app : apps) {
            String packageName = app.activityInfo.applicationInfo.packageName;
            ListPreference p = new ListPreference(getPrefContext());
            p.setTitle(app.loadLabel(pm));
            p.setIcon(app.loadIcon(pm));
            p.setKey(packageName);
            p.setEntries(SETTABLE_BUCKETS_NAMES);
            p.setEntryValues(SETTABLE_BUCKETS_VALUES);
            updateSummary(p);
            p.setOnPreferenceChangeListener(this);

            screen.addPreference(p);
        }
    }

    static String bucketToName(int bucket) {
        switch (bucket) {
            case STANDBY_BUCKET_EXEMPTED: return "EXEMPTED";
            case STANDBY_BUCKET_ACTIVE: return "ACTIVE";
            case STANDBY_BUCKET_WORKING_SET: return "WORKING_SET";
            case STANDBY_BUCKET_FREQUENT: return "FREQUENT";
            case STANDBY_BUCKET_RARE: return "RARE";
            case STANDBY_BUCKET_NEVER: return "NEVER";
        }
        return "";
    }

    private void updateSummary(ListPreference p) {
        final Resources res = getActivity().getResources();
        final int appBucket = mUsageStats.getAppStandbyBucket(p.getKey());
        final String bucketName = bucketToName(appBucket);
        p.setSummary(res.getString(R.string.standby_bucket_summary, bucketName));
        // Buckets outside of the range of the dynamic ones are only used for special
        // purposes and can either not be changed out of, or might have undesirable
        // side-effects in combination with other assumptions.
        final boolean changeable = appBucket >= STANDBY_BUCKET_ACTIVE
                && appBucket <= STANDBY_BUCKET_RARE;
        if (changeable) {
            p.setValue(Integer.toString(appBucket));
        }
        p.setEnabled(changeable);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        mUsageStats.setAppStandbyBucket(preference.getKey(), Integer.parseInt((String) newValue));
        updateSummary((ListPreference) preference);
        return false;
    }
}
