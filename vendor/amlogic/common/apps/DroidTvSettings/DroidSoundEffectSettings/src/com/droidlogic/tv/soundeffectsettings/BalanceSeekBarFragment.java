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


import com.droidlogic.tv.soundeffectsettings.R;
import com.droidlogic.app.tv.AudioEffectManager;

public class BalanceSeekBarFragment extends LeanbackPreferenceFragment implements SeekBar.OnSeekBarChangeListener {

    private static final String TAG = "BalanceSeekBarFragment";

    private SeekBar seekbar_balance;
    private TextView text_balance;

    private AudioEffectManager mAudioEffectManager;
    private boolean isSeekBarInited = false;

    public static BalanceSeekBarFragment newInstance() {
        return new BalanceSeekBarFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView (LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.xml.tv_sound_balance_seekbar, container, false);
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

    private void initSeekBar(View view) {
        int status = -1;
        boolean hasfocused = false;
        seekbar_balance= (SeekBar) view.findViewById(R.id.seekbar_tv_balance);
        text_balance = (TextView) view.findViewById(R.id.text_tv_balance);
        if (true) {
            status = mAudioEffectManager.getBalanceStatus();
            seekbar_balance.setOnSeekBarChangeListener(this);
            seekbar_balance.setProgress(status);
            setShow(R.id.seekbar_tv_balance, status);
            if (!hasfocused) {
                seekbar_balance.requestFocus();
                hasfocused = true;
            }
        } else {
            seekbar_balance.setVisibility(View.GONE);
            text_balance.setVisibility(View.GONE);
        }
        isSeekBarInited = true;
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (!isSeekBarInited) {
            return;
        }
        switch (seekBar.getId()) {
            case R.id.seekbar_tv_balance:{
                setShow(R.id.seekbar_tv_balance, progress);
                mAudioEffectManager.setBalance(progress/* - mTvOptionSettingManager.getBalanceStatus()*/);
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
            case R.id.seekbar_tv_balance:{
                text_balance.setText(getShowString(R.string.tv_balance_effect, value));
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
