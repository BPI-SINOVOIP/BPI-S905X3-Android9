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
import android.os.Message;
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

import com.droidlogic.app.SystemControlManager;

public class AdjustHdmiAudioLatencyFragment extends LeanbackPreferenceFragment implements SeekBar.OnSeekBarChangeListener {

    private static final String TAG = "AdjustHdmiAudioLatencyFragment";

    private SeekBar seekbar_hdmi_audio;
    private TextView text_hdmi_audio;
    private TextView title_hdmi_audio;
    private SystemControlManager mSystemControlManager = SystemControlManager.getInstance();
    private boolean isSeekBarInited = false;
    private TvOptionSettingManager mTvOptionSettingManager;

    private static final int MSG_SET_AUDIO_LATENCY = 1;

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_SET_AUDIO_LATENCY:
                    Log.d(TAG, "handleMessage MSG_SET_AUDIO_LATENCY " + msg.arg1 + "ms");
                    mTvOptionSettingManager.setHdmiAudioLatency(String.valueOf(msg.arg1));
                    break;
                default:
                    Log.d(TAG, "handleMessage default");
                    break;
            }
        }
    };

    public static AdjustHdmiAudioLatencyFragment newInstance() {
        return new AdjustHdmiAudioLatencyFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView (LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        if (mTvOptionSettingManager == null) {
            mTvOptionSettingManager = new TvOptionSettingManager(getActivity(), false);
        }
        View view = inflater.inflate(R.xml.hdmi_audio_latency_seekbar, container, false);
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
        boolean hasfocused = false;
        boolean isTv = SettingsConstant.needDroidlogicTvFeature(getActivity());
        int audioLatency = mTvOptionSettingManager.getHdmiAudioLatency();
        int progress = convertAudioLatencyToProgress(audioLatency);
        title_hdmi_audio = (TextView) view.findViewById(R.id.title_hdmi_audio_latency);
        seekbar_hdmi_audio = (SeekBar) view.findViewById(R.id.seekbar_audio_latency);
        text_hdmi_audio = (TextView) view.findViewById(R.id.text_audio_latency);
        if (isTv) {
            title_hdmi_audio.setText(R.string.arc_hdmi_audio_latency);
        }
        setShow(R.id.seekbar_audio_latency, audioLatency);
        seekbar_hdmi_audio.setOnSeekBarChangeListener(this);
        seekbar_hdmi_audio.setProgress(progress);
        seekbar_hdmi_audio.requestFocus();
        isSeekBarInited = true;
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (!isSeekBarInited) {
            return;
        }
        switch (seekBar.getId()) {
            case R.id.seekbar_audio_latency:{
                int latency = convertProgressToAudioLatency(progress);
                setShow(R.id.seekbar_audio_latency, latency);
                sendSetAudioLatencyMessage(latency);
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
        boolean isTv = SettingsConstant.needDroidlogicTvFeature(getActivity());
        int res = R.string.hdmi_audio_latency;
        if (isTv) {
            res = R.string.arc_hdmi_audio_latency;
        }
        switch (id) {
            case R.id.seekbar_audio_latency:{
                text_hdmi_audio.setText(getShowString(res, value));
                break;
            }
            default:
                break;
        }
    }

    private String getShowString(int resid, int value) {
        return getActivity().getResources().getString(resid) + ": " + value + "ms";
    }

    //convert -200~200 to 0~40
    private int convertAudioLatencyToProgress(int latency) {
        int result = 20;
        if (latency >= -200 && latency <= 200) {
            result = (latency + 200) / 10;
        }
        return result;
    }

    //convert 0~40 to -200~200
    private int convertProgressToAudioLatency(int progress) {
        int result = 0;
        if (progress >= 0 && progress <= 40) {
            result = -200 + progress * 10;
        }
        return result;
    }

    private void sendSetAudioLatencyMessage(int latency) {
        if (mHandler != null) {
            mHandler.removeMessages(MSG_SET_AUDIO_LATENCY);
            mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_SET_AUDIO_LATENCY, latency, 0), 500);
        } else {
            Log.e(TAG, "sendSetAudioLatencyMessage NULL handle");
        }
    }
}
