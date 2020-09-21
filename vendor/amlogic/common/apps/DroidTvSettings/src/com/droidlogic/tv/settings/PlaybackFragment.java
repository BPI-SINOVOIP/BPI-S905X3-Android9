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

import android.os.Bundle;
import android.os.Handler;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;

import com.droidlogic.tv.settings.R;

import com.droidlogic.app.PlayBackManager;

public class PlaybackFragment extends LeanbackPreferenceFragment {
    private static final String TAG = "PlaybackFragment";

    private static final String KEY_PLAYBACK_HDMI_SELFADAPTION = "playback_hdmi_selfadaption";

    private PlayBackManager mPlayBackManager;
    private Preference hdmiSelfAdaptionPref = null;

    public static PlaybackFragment newInstance() {
        return new PlaybackFragment();
    }

    @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
        }

    @Override
        public void onResume() {
            super.onResume();
            refreshStatus();
        }

    @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            setPreferencesFromResource(R.xml.playback_settings, null);

            mPlayBackManager = new PlayBackManager(getContext());

            hdmiSelfAdaptionPref = findPreference(KEY_PLAYBACK_HDMI_SELFADAPTION);

            refreshStatus();
        }

    private String getHdmiSelfAdaptionStatus() {
        int mode = mPlayBackManager.getHdmiSelfAdaptionMode();
        switch (mode) {
            case PlayBackManager.MODE_OFF:
                return getString(R.string.off);
            case PlayBackManager.MODE_PART:
                return getString(R.string.playback_hdmi_selfadaption_part);
            case PlayBackManager.MODE_TOTAL:
                return getString(R.string.playback_hdmi_selfadaption_total);
            default:
                return getString(R.string.off);
        }
    }

    private void refreshStatus() {
        hdmiSelfAdaptionPref.setSummary(getHdmiSelfAdaptionStatus());
    }
}
