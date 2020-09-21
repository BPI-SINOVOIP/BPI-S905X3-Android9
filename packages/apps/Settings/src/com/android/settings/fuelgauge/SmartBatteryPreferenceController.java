/*
 * Copyright (C) 2018 The Android Open Source Project
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
 *
 *
 */

package com.android.settings.fuelgauge;

import android.content.Context;
import android.provider.Settings;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.Preference;
import android.text.TextUtils;

import com.android.settings.core.BasePreferenceController;
import com.android.settings.overlay.FeatureFactory;

/**
 * Controller to change and update the smart battery toggle
 */
public class SmartBatteryPreferenceController extends BasePreferenceController implements
        Preference.OnPreferenceChangeListener {
    private static final String KEY_SMART_BATTERY = "smart_battery";
    private static final int ON = 1;
    private static final int OFF = 0;
    private PowerUsageFeatureProvider mPowerUsageFeatureProvider;

    public SmartBatteryPreferenceController(Context context) {
        super(context, KEY_SMART_BATTERY);
        mPowerUsageFeatureProvider = FeatureFactory.getFactory(context)
                .getPowerUsageFeatureProvider(context);
    }

    @Override
    public int getAvailabilityStatus() {
        return mPowerUsageFeatureProvider.isSmartBatterySupported()
                ? AVAILABLE
                : UNSUPPORTED_ON_DEVICE;
    }

    @Override
    public boolean isSliceable() {
        return TextUtils.equals(getPreferenceKey(), "smart_battery");
    }

    @Override
    public void updateState(Preference preference) {
        super.updateState(preference);
        final boolean smartBatteryOn = Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.ADAPTIVE_BATTERY_MANAGEMENT_ENABLED, ON) == ON;
        ((SwitchPreference) preference).setChecked(smartBatteryOn);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        final boolean smartBatteryOn = (Boolean) newValue;
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.ADAPTIVE_BATTERY_MANAGEMENT_ENABLED, smartBatteryOn ? ON : OFF);
        return true;
    }
}
