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

package com.android.settings.development;

import android.content.Context;
import android.provider.Settings;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.Preference;

import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.development.DeveloperOptionsPreferenceController;

/**
 * Base controller for Switch preference that maps to a specific value in Settings.System.
 */
public abstract class SystemSettingSwitchPreferenceController extends
        DeveloperOptionsPreferenceController implements Preference.OnPreferenceChangeListener,
        PreferenceControllerMixin {

    private static final int SETTING_VALUE_OFF = 0;
    private static final int SETTING_VALUE_ON = 1;

    private final String mSettingsKey;

    public SystemSettingSwitchPreferenceController(Context context, String systemSettingsKey) {
        super(context);
        mSettingsKey = systemSettingsKey;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        final boolean isEnabled = (Boolean) newValue;
        Settings.System.putInt(mContext.getContentResolver(), mSettingsKey,
            isEnabled ? SETTING_VALUE_ON : SETTING_VALUE_OFF);
        return true;
    }

    @Override
    public void updateState(Preference preference) {
        final int mode = Settings.System.getInt(
            mContext.getContentResolver(), mSettingsKey, SETTING_VALUE_OFF);
        ((SwitchPreference) mPreference).setChecked(mode != SETTING_VALUE_OFF);
    }

    @Override
    protected void onDeveloperOptionsSwitchDisabled() {
        super.onDeveloperOptionsSwitchDisabled();
        Settings.System.putInt(mContext.getContentResolver(), mSettingsKey, SETTING_VALUE_OFF);
        ((SwitchPreference) mPreference).setChecked(false);
    }
}
