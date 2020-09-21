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
 * limitations under the License
 */

package com.droidlogic.tv.settings.tvoption;

import android.content.ContentResolver;
import android.content.Context;
import android.os.Bundle;
import android.os.SystemProperties;
import android.provider.Settings;
import android.support.v14.preference.SwitchPreference;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.support.v7.preference.TwoStatePreference;
import android.text.TextUtils;
import android.util.Log;

import com.droidlogic.app.OutputModeManager;
import com.droidlogic.tv.settings.SettingsConstant;
import com.droidlogic.tv.settings.MainFragment;
import com.droidlogic.tv.settings.R;

public class TvOptionFragment extends LeanbackPreferenceFragment {

    private static final String TAG = "TvOptionFragment";
    private static final String KEY_SOUNDS = "tv_sound";

    public static TvOptionFragment newInstance() {
        return new TvOptionFragment();
    }

    static public boolean CanDebug() {
        return SystemProperties.getBoolean("sys.tvoption.debug", false);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.tv_option, null);
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        super.onPreferenceTreeClick(preference);
        if (TextUtils.equals(preference.getKey(), KEY_SOUNDS)) {
            MainFragment.startSoundEffectSettings(getActivity());
        }
        return false;
    }
}
