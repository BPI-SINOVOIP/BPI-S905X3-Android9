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
 * limitations under the License
 */

package com.android.server.telecom.settings;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.provider.BlockedNumberContract.SystemContract;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import com.android.server.telecom.R;

public class EnhancedCallBlockingFragment extends PreferenceFragment
        implements Preference.OnPreferenceChangeListener {

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.enhanced_call_blocking_settings);

        setOnPreferenceChangeListener(SystemContract.ENHANCED_SETTING_KEY_BLOCK_UNREGISTERED);
        setOnPreferenceChangeListener(SystemContract.ENHANCED_SETTING_KEY_BLOCK_PRIVATE);
        setOnPreferenceChangeListener(SystemContract.ENHANCED_SETTING_KEY_BLOCK_PAYPHONE);
        setOnPreferenceChangeListener(SystemContract.ENHANCED_SETTING_KEY_BLOCK_UNKNOWN);
    }

    /**
     * Set OnPreferenceChangeListener for the preference.
     */
    private void setOnPreferenceChangeListener(String key) {
        SwitchPreference pref = (SwitchPreference) findPreference(key);
        if (pref != null) {
            pref.setOnPreferenceChangeListener(this);
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        updateEnhancedBlockPref(SystemContract.ENHANCED_SETTING_KEY_BLOCK_UNREGISTERED);
        updateEnhancedBlockPref(SystemContract.ENHANCED_SETTING_KEY_BLOCK_PRIVATE);
        updateEnhancedBlockPref(SystemContract.ENHANCED_SETTING_KEY_BLOCK_PAYPHONE);
        updateEnhancedBlockPref(SystemContract.ENHANCED_SETTING_KEY_BLOCK_UNKNOWN);
    }

    /**
     * Update preference checked status.
     */
    private void updateEnhancedBlockPref(String key) {
        SwitchPreference pref = (SwitchPreference) findPreference(key);
        if (pref != null) {
            pref.setChecked(BlockedNumbersUtil.getEnhancedBlockSetting(getActivity(), key));
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object objValue) {
        BlockedNumbersUtil.setEnhancedBlockSetting(getActivity(), preference.getKey(),
                (boolean) objValue);
        return true;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return inflater.inflate(R.xml.layout_customized_listview,
                container, false);
    }
}
