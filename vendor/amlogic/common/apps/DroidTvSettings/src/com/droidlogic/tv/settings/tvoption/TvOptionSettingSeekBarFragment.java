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

package com.droidlogic.tv.settings.tvoption;

import android.os.Bundle;
import android.os.Handler;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;
import android.os.SystemProperties;
import android.util.Log;
import android.text.TextUtils;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;


import com.droidlogic.tv.settings.util.DroidUtils;
import com.droidlogic.tv.settings.SettingsConstant;
import com.droidlogic.tv.settings.R;

import com.droidlogic.app.tv.TvControlManager;

public class TvOptionSettingSeekBarFragment extends LeanbackPreferenceFragment implements SeekBar.OnSeekBarChangeListener {

    private static final String TAG = "TvOptionSettingSeekBarFragment";

    private SeekBar seekbar_audio_ad_mix_level;
    private TextView text_audio_ad_mix_level;

    private TvOptionSettingManager mTvOptionSettingManager;

    public static TvOptionSettingSeekBarFragment newInstance() {
        return new TvOptionSettingSeekBarFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView (LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.xml.tv_settings_seekbar, container, false);
        return view;
    }

    @Override
    public void onViewCreated (View view, Bundle savedInstanceState) {
        if (mTvOptionSettingManager == null) {
            mTvOptionSettingManager = new TvOptionSettingManager(getActivity(), false);
        }
        initSeekBar(view);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {

    }

    private void initSeekBar(View view) {
        int status = -1;
        boolean hasfocused = false;
        seekbar_audio_ad_mix_level = (SeekBar) view.findViewById(R.id.seekbar_tv_audio_ad_mix_level);
        text_audio_ad_mix_level = (TextView) view.findViewById(R.id.text_tv_audio_ad_mix_level);
        if (true) {
            status = mTvOptionSettingManager.getADMixStatus();
            seekbar_audio_ad_mix_level.setOnSeekBarChangeListener(this);
            seekbar_audio_ad_mix_level.setProgress(status);
            setShow(R.id.seekbar_tv_audio_ad_mix_level, status);
            seekbar_audio_ad_mix_level.requestFocus();
            hasfocused = true;
        } else {
            seekbar_audio_ad_mix_level.setVisibility(View.GONE);
            text_audio_ad_mix_level.setVisibility(View.GONE);
        }
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        switch (seekBar.getId()) {
            case R.id.seekbar_tv_audio_ad_mix_level:{
                setShow(R.id.seekbar_tv_audio_ad_mix_level, progress);
                mTvOptionSettingManager.setADMix(progress - mTvOptionSettingManager.getADMixStatus());
                break;
            }
            default:
                break;
        }
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    private void setShow(int id, int value) {
        switch (id) {
            case R.id.seekbar_tv_audio_ad_mix_level:{
                text_audio_ad_mix_level.setText(getShowString(R.string.tv_audio_ad_mix_level, value));
                break;
            }
            default:
                break;
        }
    }

    private String getShowString(int resid, int value) {
        return getActivity().getResources().getString(resid) + ": " + value + "%";
    }
}
