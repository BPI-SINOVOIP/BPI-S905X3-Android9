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

package com.droidlogic.tv.soundeffectsettings;

import android.os.Bundle;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.TwoStatePreference;
import android.text.TextUtils;
import android.util.Log;


import com.droidlogic.tv.soundeffectsettings.R;
import com.droidlogic.app.tv.AudioEffectManager;

public class DtsSoundSettingFragment extends LeanbackPreferenceFragment implements Preference.OnPreferenceChangeListener {

    private static final String TAG = "DtsSoundSettingFragment";

    private static final String KEY_TV_DTS_TRU_SURROUND         = "key_tv_dts_tru_surround";
    private static final String KEY_TV_DTS_DIALOG_CLARITY       = "key_tv_dts_dialog_clarity";
    private static final String KEY_TV_DTS_TRU_BASS             = "key_tv_dts_tru_bass";

    private SoundParameterSettingManager mSoundParameterSettingManager;
    private AudioEffectManager mAudioEffectManager;

    private ListPreference mDialogClarityPref;
    private TwoStatePreference mTruSurroundPref;
    private TwoStatePreference mTruBassPref;

    public static DtsSoundSettingFragment newInstance() {
        return new DtsSoundSettingFragment();
    }

    private boolean CanDebug() {
        return OptionParameterManager.CanDebug();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
        mDialogClarityPref.setValueIndex(mAudioEffectManager.getDialogClarityMode());
        mTruSurroundPref.setChecked(mAudioEffectManager.getSurroundEnable());
        mTruBassPref.setChecked(mAudioEffectManager.getTruBassEnable());
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.tv_sound_dts_setting, null);
        if (mAudioEffectManager == null) {
            mAudioEffectManager = ((TvSettingsActivity)getActivity()).getAudioEffectManager();
        }
        if (mSoundParameterSettingManager == null) {
            mSoundParameterSettingManager = ((TvSettingsActivity)getActivity()).getSoundParameterSettingManager();
        }
        if (mAudioEffectManager == null) {
            Log.e(TAG, "onCreatePreferences mAudioEffectManager == null");
            return;
        }

        mDialogClarityPref = (ListPreference) findPreference(KEY_TV_DTS_DIALOG_CLARITY);
        mDialogClarityPref.setValueIndex(mAudioEffectManager.getDialogClarityMode());
        mDialogClarityPref.setOnPreferenceChangeListener(this);

        mTruSurroundPref = (TwoStatePreference) findPreference(KEY_TV_DTS_TRU_SURROUND);
        mTruSurroundPref.setChecked(mAudioEffectManager.getSurroundEnable());

        mTruBassPref = (TwoStatePreference) findPreference(KEY_TV_DTS_TRU_BASS);
        mTruBassPref.setChecked(mAudioEffectManager.getTruBassEnable());
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);

        final int selection = Integer.parseInt((String)newValue);
        if (TextUtils.equals(preference.getKey(), KEY_TV_DTS_DIALOG_CLARITY)) {
            mAudioEffectManager.setDialogClarityMode(selection);
        }
        return true;
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey());
        final String key = preference.getKey();
        if (key != null) {
            switch (key) {
                case KEY_TV_DTS_TRU_SURROUND:
                    mAudioEffectManager.setSurroundEnable(mTruSurroundPref.isChecked());
                    mTruSurroundPref.setChecked(mAudioEffectManager.getSurroundEnable());
                    break;
                case KEY_TV_DTS_TRU_BASS:
                    mAudioEffectManager.setTruBassEnable(mTruBassPref.isChecked());
                    mTruBassPref.setChecked(mAudioEffectManager.getTruBassEnable());
                    break;
            }
        }
        return super.onPreferenceTreeClick(preference);
    }
}
