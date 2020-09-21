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

public class EnableGnssRawMeasFullTrackingPreferenceController extends
        DeveloperOptionsPreferenceController implements Preference.OnPreferenceChangeListener,
        PreferenceControllerMixin {

    private static final String ENABLE_GNSS_RAW_MEAS_FULL_TRACKING_KEY =
            "enable_gnss_raw_meas_full_tracking";

    static final int SETTING_VALUE_ON = 1;
    static final int SETTING_VALUE_OFF = 0;

    public EnableGnssRawMeasFullTrackingPreferenceController(Context context) {
        super(context);
    }

    @Override
    public String getPreferenceKey() {
        return ENABLE_GNSS_RAW_MEAS_FULL_TRACKING_KEY;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        final boolean isEnabled = (Boolean) newValue;
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.ENABLE_GNSS_RAW_MEAS_FULL_TRACKING,
                isEnabled ? SETTING_VALUE_ON : SETTING_VALUE_OFF);

        return true;
    }

    @Override
    public void updateState(Preference preference) {
        final int enableGnssRawMeasFullTrackingMode =
                Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.ENABLE_GNSS_RAW_MEAS_FULL_TRACKING, SETTING_VALUE_OFF);
        ((SwitchPreference) mPreference).setChecked(enableGnssRawMeasFullTrackingMode != SETTING_VALUE_OFF);
    }

    @Override
    protected void onDeveloperOptionsSwitchDisabled() {
        super.onDeveloperOptionsSwitchDisabled();
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.ENABLE_GNSS_RAW_MEAS_FULL_TRACKING, SETTING_VALUE_OFF);
        ((SwitchPreference) mPreference).setChecked(false);
    }
}
