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

import android.app.ActivityManager;
import android.content.Context;
import android.os.Bundle;
import android.support.annotation.Keep;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.Preference;

import com.android.internal.logging.nano.MetricsProto;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;

/**
 * Screen for Special App Access items
 */
@Keep
public class SpecialAppAccess extends SettingsPreferenceFragment {

    @VisibleForTesting
    static final String KEY_FEATURE_PIP = "picture_in_picture";
    private static final String[] DISABLED_FEATURES_LOW_RAM_TV =
            new String[]{KEY_FEATURE_PIP};

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.special_app_access, null);

        updatePreferenceStates();
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.SPECIAL_ACCESS;
    }

    @VisibleForTesting
    void updatePreferenceStates() {
        ActivityManager activityManager = (ActivityManager) getContext()
                .getSystemService(Context.ACTIVITY_SERVICE);
        if (activityManager.isLowRamDevice()) {
            for (String disabledFeature : DISABLED_FEATURES_LOW_RAM_TV) {
                removePreference(disabledFeature);
            }
        }
    }

    @VisibleForTesting
    void removePreference(String key) {
        final Preference preference = findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
        }
    }
}
