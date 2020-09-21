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
package com.android.settings.dashboard.conditional;

import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.os.PowerManager;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.core.SubSettingLauncher;
import com.android.settings.fuelgauge.BatterySaverDrawable;
import com.android.settings.fuelgauge.BatterySaverReceiver;
import com.android.settings.fuelgauge.batterysaver.BatterySaverSettings;
import com.android.settingslib.fuelgauge.BatterySaverUtils;

public class BatterySaverCondition extends Condition implements
        BatterySaverReceiver.BatterySaverListener {

    private final BatterySaverReceiver mReceiver;

    public BatterySaverCondition(ConditionManager manager) {
        super(manager);

        mReceiver = new BatterySaverReceiver(manager.getContext());
        mReceiver.setBatterySaverListener(this);
    }

    @Override
    public void refreshState() {
        PowerManager powerManager = mManager.getContext().getSystemService(PowerManager.class);
        setActive(powerManager.isPowerSaveMode());
    }

    @Override
    public Drawable getIcon() {
        return mManager.getContext().getDrawable(R.drawable.ic_battery_saver_accent_24dp);
    }

    @Override
    public CharSequence getTitle() {
        return mManager.getContext().getString(R.string.condition_battery_title);
    }

    @Override
    public CharSequence getSummary() {
        return mManager.getContext().getString(R.string.condition_battery_summary);
    }

    @Override
    public CharSequence[] getActions() {
        return new CharSequence[]{mManager.getContext().getString(R.string.condition_turn_off)};
    }

    @Override
    public void onPrimaryClick() {
        new SubSettingLauncher(mManager.getContext())
                .setDestination(BatterySaverSettings.class.getName())
                .setSourceMetricsCategory(MetricsEvent.DASHBOARD_SUMMARY)
                .setTitle(R.string.battery_saver)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                .launch();
    }

    @Override
    public void onActionClick(int index) {
        if (index == 0) {
            BatterySaverUtils.setPowerSaveMode(mManager.getContext(), false,
                    /*needFirstTimeWarning*/ false);
            refreshState();
        } else {
            throw new IllegalArgumentException("Unexpected index " + index);
        }
    }

    @Override
    public int getMetricsConstant() {
        return MetricsEvent.SETTINGS_CONDITION_BATTERY_SAVER;
    }

    @Override
    public void onResume() {
        mReceiver.setListening(true);
    }

    @Override
    public void onPause() {
        mReceiver.setListening(false);
    }

    @Override
    public void onPowerSaveModeChanged() {
        ConditionManager.get(mManager.getContext()).getCondition(BatterySaverCondition.class)
                        .refreshState();
    }

    @Override
    public void onBatteryChanged(boolean pluggedIn) {
        // do nothing
    }
}
