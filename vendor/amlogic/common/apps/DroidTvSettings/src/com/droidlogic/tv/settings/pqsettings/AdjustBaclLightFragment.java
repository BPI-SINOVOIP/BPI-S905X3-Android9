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

package com.droidlogic.tv.settings.pqsettings;

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
import android.provider.Settings;

import com.droidlogic.tv.settings.util.DroidUtils;
import com.droidlogic.tv.settings.SettingsConstant;
import com.droidlogic.tv.settings.R;

public class AdjustBaclLightFragment extends LeanbackPreferenceFragment implements SeekBar.OnSeekBarChangeListener {

    private static final String TAG = "AdjustBaclLightFragment";

    private SeekBar seekbar_blacklight;
    private TextView text_blacklight;
    private PQSettingsManager mPQSettingsManager;
    private boolean isSeekBarInited = false;

    public static AdjustBaclLightFragment newInstance() {
        return new AdjustBaclLightFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView (LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.xml.backlight_seekbar, container, false);
        return view;
    }

    @Override
    public void onViewCreated (View view, Bundle savedInstanceState) {
        initSeekBar(view);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {

    }

    private void initSeekBar(View view) {
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }
        int status = -1;
        boolean hasfocused = false;
        boolean isTv = SettingsConstant.needDroidlogicTvFeature(getActivity());
        seekbar_blacklight = (SeekBar) view.findViewById(R.id.seekbar_backlight);
        text_blacklight = (TextView) view.findViewById(R.id.text_backlight);
        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_backlight)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_backlight))) {
            status = mPQSettingsManager.getBacklightStatus();
            seekbar_blacklight.setOnSeekBarChangeListener(this);
            seekbar_blacklight.setProgress(status);
            setShow(R.id.seekbar_backlight, status);
            seekbar_blacklight.requestFocus();
            hasfocused = true;
        } else {
            seekbar_blacklight.setVisibility(View.GONE);
            text_blacklight.setVisibility(View.GONE);
        }

        isSeekBarInited = true;
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (!isSeekBarInited) {
            return;
        }
        switch (seekBar.getId()) {
            case R.id.seekbar_backlight:{
                setShow(R.id.seekbar_backlight, progress);
                mPQSettingsManager.setBacklightValue(progress - mPQSettingsManager.getBacklightStatus());
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
            case R.id.seekbar_backlight:{
                text_blacklight.setText(getShowString(R.string.pq_backlight, value));
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
