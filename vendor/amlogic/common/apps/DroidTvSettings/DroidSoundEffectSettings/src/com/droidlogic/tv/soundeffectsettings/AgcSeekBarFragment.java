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
import android.os.Handler;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;
import android.util.Log;
import android.text.TextUtils;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;

import com.droidlogic.tv.soundeffectsettings.R;
import com.droidlogic.app.tv.AudioEffectManager;

public class AgcSeekBarFragment extends LeanbackPreferenceFragment implements SeekBar.OnSeekBarChangeListener {

    private static final String TAG = "AgcSeekBarFragment";

    private CheckBox checkbox_enable;

    private SeekBar seekbar_max_level;
    private SeekBar seekbar_attrack_time;
    private SeekBar seekbar_release_time;

    private TextView text_max_level;
    private TextView text_attrack_time;
    private TextView text_release_time;

    private AudioEffectManager mAudioEffectManager;
    private boolean isSeekBarInited = false;

    public static AgcSeekBarFragment newInstance() {
        return new AgcSeekBarFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView (LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.xml.tv_sound_agc, container, false);
        return view;
    }

    @Override
    public void onViewCreated (View view, Bundle savedInstanceState) {
        if (mAudioEffectManager == null) {
            mAudioEffectManager = ((TvSettingsActivity)getActivity()).getAudioEffectManager();
        }
        if (mAudioEffectManager == null) {
            Log.e(TAG, "onViewCreated mAudioEffectManager == null");
            return;
        }
        initSeekBar(view);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {

    }

    private void enableAgcView(boolean bool) {
        seekbar_max_level.setEnabled(bool);
        text_max_level.setEnabled(bool);
        seekbar_attrack_time.setEnabled(bool);
        text_attrack_time.setEnabled(bool);
        seekbar_release_time.setEnabled(bool);
        text_release_time.setEnabled(bool);
    }

    private void initSeekBar(View view) {
        int status = -1;
        boolean agcEnable = mAudioEffectManager.getAgcEnableStatus();
        checkbox_enable = (CheckBox) view.findViewById(R.id.checkbox_enable);
        seekbar_max_level = (SeekBar) view.findViewById(R.id.seekbar_max_level);
        text_max_level = (TextView) view.findViewById(R.id.text_max_level);
        seekbar_attrack_time = (SeekBar) view.findViewById(R.id.seekbar_attrack_time);
        text_attrack_time = (TextView) view.findViewById(R.id.text_attrack_time);
        seekbar_release_time = (SeekBar) view.findViewById(R.id.seekbar_release_time);
        text_release_time = (TextView) view.findViewById(R.id.text_release_time);
        checkbox_enable.setChecked(agcEnable);
        checkbox_enable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                mAudioEffectManager.setAgcEnable(isChecked);
                enableAgcView(isChecked);
            }
        });
        enableAgcView(agcEnable);
        status = mAudioEffectManager.getAgcMaxLevelStatus();
        seekbar_max_level.setOnSeekBarChangeListener(this);
        seekbar_max_level.setProgress(status + 30);//-30 ~ 0 --> 0 ~ 30
        setShow(R.id.seekbar_max_level, status);
        seekbar_max_level.requestFocus();
        status = mAudioEffectManager.getAgcAttrackTimeStatus();
        seekbar_attrack_time.setOnSeekBarChangeListener(this);
        seekbar_attrack_time.setProgress(status / 10 - 1);//10ms~200ms --> 0 ~ 19
        setShow(R.id.seekbar_attrack_time, status);
        status = mAudioEffectManager.getAgcReleaseTimeStatus();
        seekbar_release_time.setOnSeekBarChangeListener(this);
        seekbar_release_time.setProgress(status - 2);// 2s~8s --> 0 ~6
        setShow(R.id.seekbar_release_time, status);
        isSeekBarInited = true;
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (!isSeekBarInited) {
            return;
        }
        switch (seekBar.getId()) {
            case R.id.seekbar_max_level:{
                setShow(R.id.seekbar_max_level, progress - 30);
                mAudioEffectManager.setAgcMaxLevel(progress - 30); //0 ~ 30 -->-30 ~ 0
                break;
            }
            case R.id.seekbar_attrack_time:{
                setShow(R.id.seekbar_attrack_time, (progress + 1) * 10);
                mAudioEffectManager.setAgcAttrackTime((progress + 1) * 10);//0 ~ 19 --> 10ms~200ms
                break;
            }
            case R.id.seekbar_release_time:{
                setShow(R.id.seekbar_release_time, progress + 2);
                mAudioEffectManager.setAgcReleaseTime(progress + 2);//  0 ~6 -->2s~8s
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
            case R.id.seekbar_max_level:{
                text_max_level.setText(getShowString(R.id.seekbar_max_level, value));
                break;
            }
            case R.id.seekbar_attrack_time:{
                text_attrack_time.setText(getShowString(R.id.seekbar_attrack_time, value));
                break;
            }
            case R.id.seekbar_release_time:{
                text_release_time.setText(getShowString(R.id.seekbar_release_time, value));
                break;
            }
            default:
                break;
        }
    }

    private String getShowString(int resid, int value) {
        String result = "";
        switch (resid) {
            case R.id.seekbar_max_level:{
                result =  getActivity().getResources().getString(R.string.tv_sound_agc_max_level) + ": " + value + "dB";
                break;
            }
            case R.id.seekbar_attrack_time:{
                result = getActivity().getResources().getString(R.string.tv_sound_agc_attrack_time) + ": " + value + "ms";
                break;
            }
            case R.id.seekbar_release_time:{
                result = getActivity().getResources().getString(R.string.tv_sound_agc_release_time) + ": " + value + "s";
                break;
            }
            default:
                break;
        }
        return result;
    }
}
