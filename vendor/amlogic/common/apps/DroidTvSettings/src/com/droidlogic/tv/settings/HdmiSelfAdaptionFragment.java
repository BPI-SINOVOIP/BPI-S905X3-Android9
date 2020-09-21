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

import android.content.ContentResolver;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.UserHandle;
import android.provider.Settings;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;

import com.droidlogic.tv.settings.R;
import com.droidlogic.tv.settings.RadioPreference;
import com.droidlogic.tv.settings.dialog.old.Action;

import com.droidlogic.app.PlayBackManager;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class HdmiSelfAdaptionFragment extends LeanbackPreferenceFragment {
    private static final String TAG = "HdmiSelfAdaptionFragment";

    private static final String HDMI_SELFADAPTION_RADIO_GROUP = "hdmi_selfadaption";
    private static final String ACTION_PART = "near_adaption";
    private static final String ACTION_TOTAL = "all_adaption";
    private static final String ACTION_OFF = "off";

    private PlayBackManager mPlayBackManager;
    private String mHdmiSelfAdaptionMode;

    private static final int SET_DELAY_MS = 500;
    private final Handler mDelayHandler = new Handler();
    private final Runnable mSetHdmiSelfAdaptionRunnable = new Runnable() {
        @Override
            public void run() {
                if (ACTION_OFF.equals(mHdmiSelfAdaptionMode)) {
                    mPlayBackManager.setHdmiSelfadaption(PlayBackManager.MODE_OFF);
                } else if (ACTION_PART.equals(mHdmiSelfAdaptionMode)) {
                    mPlayBackManager.setHdmiSelfadaption(PlayBackManager.MODE_PART);
                } else if (ACTION_TOTAL.equals(mHdmiSelfAdaptionMode)) {
                    mPlayBackManager.setHdmiSelfadaption(PlayBackManager.MODE_TOTAL);
                }
            }
    };

    public static HdmiSelfAdaptionFragment newInstance() {
        return new HdmiSelfAdaptionFragment();
    }

    @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            mPlayBackManager = new PlayBackManager(getContext());

            final Context themedContext = getPreferenceManager().getContext();
            final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(themedContext);
            screen.setTitle(R.string.playback_hdmi_selfadaption);
            Preference activePref = null;
            final List<Action> infoList = getActions();
            for (final Action info : infoList) {
                final String tag = info.getKey();
                final RadioPreference radioPreference = new RadioPreference(themedContext);
                radioPreference.setKey(tag);
                radioPreference.setPersistent(false);
                radioPreference.setTitle(info.getTitle());
                radioPreference.setSummaryOn(info.getDescription());
                radioPreference.setRadioGroup(HDMI_SELFADAPTION_RADIO_GROUP);
                radioPreference.setLayoutResource(R.layout.preference_reversed_widget);

                if (info.isChecked()) {
                    radioPreference.setChecked(true);
                    activePref = radioPreference;
                }

                screen.addPreference(radioPreference);
            }
            if (activePref != null && savedInstanceState == null) {
                scrollToPreference(activePref);
            }

            setPreferenceScreen(screen);
        }

    private ArrayList<Action> getActions() {
        int mode = mPlayBackManager.getHdmiSelfAdaptionMode();

        ArrayList<Action> actions = new ArrayList<Action>();
        actions.add(new Action.Builder().key(ACTION_OFF).title(getString(R.string.off))
                .checked(mode == mPlayBackManager.MODE_OFF).build());
        actions.add(new Action.Builder().key(ACTION_PART).title(getString(R.string.playback_hdmi_selfadaption_part))
                .checked(mode == mPlayBackManager.MODE_PART)
                .description(getString(R.string.playback_hdmi_selfadaption_part_desc)).build());
        actions.add(new Action.Builder().key(ACTION_TOTAL).title(getString(R.string.playback_hdmi_selfadaption_total))
                .checked(mode == mPlayBackManager.MODE_TOTAL)
                .description(getString(R.string.playback_hdmi_selfadaption_total_desc)).build());
        return actions;
    }

    @Override
        public boolean onPreferenceTreeClick(Preference preference) {
            if (preference instanceof RadioPreference) {
                final RadioPreference radioPreference = (RadioPreference) preference;
                radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
                if (radioPreference.isChecked()) {
                    mHdmiSelfAdaptionMode = radioPreference.getKey().toString();
                    mDelayHandler.removeCallbacks(mSetHdmiSelfAdaptionRunnable);
                    mDelayHandler.postDelayed(mSetHdmiSelfAdaptionRunnable, SET_DELAY_MS);
                } else {
                    radioPreference.setChecked(true);
                }
            }
            return super.onPreferenceTreeClick(preference);
        }
}
