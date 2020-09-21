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
import android.text.TextUtils;
import android.util.Log;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.TwoStatePreference;
import com.droidlogic.app.tv.AudioEffectManager;

public class DtsVirualXSettingFragment extends LeanbackPreferenceFragment implements Preference.OnPreferenceChangeListener{
    private static final String TAG = "DtsVirualXSettingFragment";

    private static final String KEY_TV_DTS_VIRTUALX_EFFECT            = "key_tv_dts_virtualx_effect";
    private static final String KEY_TV_DTS_TRUVOLUMEHD_EFFECT         = "key_tv_dts_truvolumexhd_effect";

    private AudioEffectManager mAudioEffectManager;
    private TwoStatePreference mTruVolumeHdPref;
    private ListPreference mVirtualXEffectPref;

    public static DtsVirualXSettingFragment newInstance() {
        return new DtsVirualXSettingFragment();
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
        mTruVolumeHdPref.setChecked(mAudioEffectManager.getDtsTruVolumeHdEnable());
        mVirtualXEffectPref.setValueIndex(mAudioEffectManager.getDtsVirtualXMode());
        mVirtualXEffectPref.setEnabled(mAudioEffectManager.isSupportVirtualX());
        mTruVolumeHdPref.setEnabled(mAudioEffectManager.isSupportVirtualX());
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.tv_sound_dts_virtualx_setting, null);
        if (mAudioEffectManager == null) {
            mAudioEffectManager = ((TvSettingsActivity)getActivity()).getAudioEffectManager();
        }

        mVirtualXEffectPref = (ListPreference) findPreference(KEY_TV_DTS_VIRTUALX_EFFECT);
        mVirtualXEffectPref.setValueIndex(mAudioEffectManager.getDtsVirtualXMode());
        mVirtualXEffectPref.setOnPreferenceChangeListener(this);
        mVirtualXEffectPref.setEnabled(mAudioEffectManager.isSupportVirtualX());

        mTruVolumeHdPref = (TwoStatePreference) findPreference(KEY_TV_DTS_TRUVOLUMEHD_EFFECT);
        mTruVolumeHdPref.setChecked(mAudioEffectManager.getDtsTruVolumeHdEnable());
        mTruVolumeHdPref.setEnabled(mAudioEffectManager.isSupportVirtualX());
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);
        final int selection = Integer.parseInt((String)newValue);
        if (TextUtils.equals(preference.getKey(), KEY_TV_DTS_VIRTUALX_EFFECT)) {
            mAudioEffectManager.setDtsVirtualXMode(selection);
        }
        return true;
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        final String key = preference.getKey();
        if (key != null) {
            switch (key) {
                case KEY_TV_DTS_TRUVOLUMEHD_EFFECT:
                    mAudioEffectManager.setDtsTruVolumeHdEnable(mTruVolumeHdPref.isChecked());
                    mTruVolumeHdPref.setChecked(mAudioEffectManager.getDtsTruVolumeHdEnable());
                    break;
            }
        }
        return super.onPreferenceTreeClick(preference);
    }



}
