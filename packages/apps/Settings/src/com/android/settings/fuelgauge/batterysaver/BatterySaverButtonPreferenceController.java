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

package com.android.settings.fuelgauge.batterysaver;

import android.content.Context;
import android.os.PowerManager;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.core.TogglePreferenceController;
import com.android.settings.fuelgauge.BatterySaverReceiver;
import com.android.settings.widget.TwoStateButtonPreference;
import com.android.settingslib.core.lifecycle.LifecycleObserver;
import com.android.settingslib.core.lifecycle.events.OnStart;
import com.android.settingslib.core.lifecycle.events.OnStop;
import com.android.settingslib.fuelgauge.BatterySaverUtils;

/**
 * Controller to update the battery saver button
 */
public class BatterySaverButtonPreferenceController extends
        TogglePreferenceController implements
        LifecycleObserver, OnStart, OnStop, BatterySaverReceiver.BatterySaverListener {

    private final BatterySaverReceiver mBatterySaverReceiver;
    private final PowerManager mPowerManager;

    private TwoStateButtonPreference mPreference;

    public BatterySaverButtonPreferenceController(Context context, String key) {
        super(context, key);
        mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mBatterySaverReceiver = new BatterySaverReceiver(context);
        mBatterySaverReceiver.setBatterySaverListener(this);
    }

    @Override
    public int getAvailabilityStatus() {
        return AVAILABLE;
    }

    @Override
    public boolean isSliceable() {
        return true;
    }

    @Override
    public void onStart() {
        mBatterySaverReceiver.setListening(true);
    }

    @Override
    public void onStop() {
        mBatterySaverReceiver.setListening(false);
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        mPreference = (TwoStateButtonPreference) screen.findPreference(getPreferenceKey());
    }

    @Override
    public boolean isChecked() {
        return mPowerManager.isPowerSaveMode();
    }

    @Override
    public boolean setChecked(boolean stateOn) {
        // This screen already shows a warning, so we don't need another warning.
        return BatterySaverUtils.setPowerSaveMode(mContext, stateOn,
                false /* needFirstTimeWarning */);
    }

    @Override
    public void updateState(Preference preference) {
        super.updateState(preference);
        if (mPreference != null) {
            mPreference.setChecked(isChecked());
        }
    }

    @Override
    public void onPowerSaveModeChanged() {
        final boolean isChecked = isChecked();
        if (mPreference != null && mPreference.isChecked() != isChecked) {
            mPreference.setChecked(isChecked);
        }
    }

    @Override
    public void onBatteryChanged(boolean pluggedIn) {
        if (mPreference != null) {
            mPreference.setButtonEnabled(!pluggedIn);
        }
    }
}
