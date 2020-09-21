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

package com.android.settings.development;

import android.content.Context;
import android.provider.Settings;
import android.support.annotation.VisibleForTesting;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.Preference;

import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.development.DeveloperOptionsPreferenceController;

public class PointerLocationPreferenceController extends DeveloperOptionsPreferenceController
        implements Preference.OnPreferenceChangeListener, PreferenceControllerMixin {

    private static final String POINTER_LOCATION_KEY = "pointer_location";

    @VisibleForTesting
    static final int SETTING_VALUE_ON = 1;
    @VisibleForTesting
    static final int SETTING_VALUE_OFF = 0;

    public PointerLocationPreferenceController(Context context) {
        super(context);
    }

    @Override
    public String getPreferenceKey() {
        return POINTER_LOCATION_KEY;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        final boolean isEnabled = (Boolean) newValue;
        Settings.System.putInt(mContext.getContentResolver(),
                Settings.System.POINTER_LOCATION, isEnabled ? SETTING_VALUE_ON : SETTING_VALUE_OFF);
        return true;
    }

    @Override
    public void updateState(Preference preference) {
        final int pointerLocationMode = Settings.System.getInt(mContext.getContentResolver(),
                Settings.System.POINTER_LOCATION, SETTING_VALUE_OFF);
        ((SwitchPreference) mPreference).setChecked(pointerLocationMode != SETTING_VALUE_OFF);
    }

    @Override
    protected void onDeveloperOptionsSwitchDisabled() {
        super.onDeveloperOptionsSwitchDisabled();
        Settings.System.putInt(mContext.getContentResolver(), Settings.System.POINTER_LOCATION,
                SETTING_VALUE_OFF);
        ((SwitchPreference) mPreference).setChecked(false);
    }
}
