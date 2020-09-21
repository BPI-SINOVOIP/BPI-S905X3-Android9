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

package com.droidlogic.tv.settings;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.PowerManager;
import android.provider.Settings;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.util.ArrayMap;
import android.util.Log;

import com.droidlogic.app.SystemControlManager;

import com.droidlogic.tv.settings.R;
import com.droidlogic.tv.settings.RadioPreference;
import com.droidlogic.tv.settings.dialog.old.Action;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class PowerOnModeFragment extends LeanbackPreferenceFragment {
    private static final String TAG = "PowerOnModeFragment";
    private static final String POWERONMODE_RADIO_GROUP = "PowerOnModeAction";
    private static final String POWER_ON_MODE_ENV = "ubootenv.var.powermode";
    private static final String STRING_POWER_ON = "on";
    private static final String STRING_POWER_STANDBY = "standby";
    private static final String STRING_POWER_LAST = "last";
    private static final int POWER_ON = 0;
    private static final int POWER_STANDBY = 1;
    private static final int POWER_LAST = 2;
    private Context mContext;
    private static final int DELAY_MS = 500;
    private SystemControlManager mSystemControlManager = null;
    private final Handler mDelayHandler = new Handler();
    private String mNewPowerOnMode = null;
    private final Runnable mSetPowerOnModeRunnable = new Runnable() {
        @Override
        public void run() {
            if (STRING_POWER_ON.equals(mNewPowerOnMode)) {
                setPowerOnMode(POWER_ON);
            } else if (STRING_POWER_STANDBY.equals(mNewPowerOnMode)) {
                setPowerOnMode(POWER_STANDBY);
            } else if (STRING_POWER_LAST.equals(mNewPowerOnMode)) {
                setPowerOnMode(POWER_LAST);
            }
        }
    };

    public static PowerOnModeFragment newInstance() {
        return new PowerOnModeFragment();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        final Context themedContext = getPreferenceManager().getContext();
        mContext = themedContext;

        mSystemControlManager = SystemControlManager.getInstance();

        final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(themedContext);
        screen.setTitle(R.string.power_on_mode);
        String currentPowerOnMode = null;
        Preference activePref = null;

        final List<Action> powerOnModeInfoList = getActions();
        for (final Action powerOnModeInfo : powerOnModeInfoList) {
            final String powerOnModeTag = powerOnModeInfo.getKey();
            final RadioPreference radioPreference = new RadioPreference(themedContext);
            radioPreference.setKey(powerOnModeTag);
            radioPreference.setPersistent(false);
            radioPreference.setTitle(powerOnModeInfo.getTitle());
            radioPreference.setRadioGroup(POWERONMODE_RADIO_GROUP);
            radioPreference.setLayoutResource(R.layout.preference_reversed_widget);
            if (powerOnModeInfo.isChecked()) {
                currentPowerOnMode = powerOnModeTag;
                radioPreference.setChecked(true);
                activePref = radioPreference;
            }
            screen.addPreference(radioPreference);
        }
        if (activePref != null && savedInstanceState == null) {
            scrollToPreference(activePref);
        }
        setPreferenceScreen(screen);

        Log.d(TAG, "onCreatePreferences");
    }

    private ArrayList<Action> getActions() {
        ArrayList<Action> actions = new ArrayList<Action>();
        int mode = getPowerOnMode();
        actions.add(new Action.Builder().key(STRING_POWER_ON).title(getString(R.string.power_on))
                .checked(mode == POWER_ON).build());
        actions.add(new Action.Builder().key(STRING_POWER_STANDBY).title(getString(R.string.power_on_standby))
                .checked(mode == POWER_STANDBY).build());
        actions.add(new Action.Builder().key(STRING_POWER_LAST).title(getString(R.string.power_on_last))
                .checked(mode == POWER_LAST).build());

        return actions;
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (preference instanceof RadioPreference) {
            final RadioPreference radioPreference = (RadioPreference) preference;
            radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
            mNewPowerOnMode = radioPreference.getKey().toString();
            mDelayHandler.removeCallbacks(mSetPowerOnModeRunnable);
            mDelayHandler.postDelayed(mSetPowerOnModeRunnable, DELAY_MS);
            radioPreference.setChecked(true);
        }
        return super.onPreferenceTreeClick(preference);
    }

    private int getPowerOnMode() {
        int default_value = POWER_ON;
        String mode = mSystemControlManager.getBootenv(POWER_ON_MODE_ENV, STRING_POWER_ON);
        if (mode.equals(STRING_POWER_ON)) {
            default_value = POWER_ON;
        } else if (mode.equals(STRING_POWER_STANDBY)) {
            default_value = POWER_STANDBY;
        } else if (mode.equals(STRING_POWER_LAST)) {
            default_value = POWER_LAST;
        }

        Log.d(TAG, "getPowerOnMode: " + default_value);

        return default_value;
    }

    private void setPowerOnMode(int keyValue) {
        if (keyValue == POWER_ON) {
            mSystemControlManager.setBootenv(POWER_ON_MODE_ENV, STRING_POWER_ON);
        } else if (keyValue == POWER_STANDBY) {
            mSystemControlManager.setBootenv(POWER_ON_MODE_ENV, STRING_POWER_STANDBY);
        } else if (keyValue == POWER_LAST) {
            mSystemControlManager.setBootenv(POWER_ON_MODE_ENV, STRING_POWER_LAST);
        }

        Log.d(TAG, "setPowerOnMode: " + keyValue);
    }
}
