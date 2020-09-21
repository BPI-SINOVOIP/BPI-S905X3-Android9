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

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.provider.Settings;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.ListPreference;
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

import com.droidlogic.app.SystemControlManager;

import com.droidlogic.tv.soundeffectsettings.R;
import com.droidlogic.app.tv.AudioEffectManager;

public class AudioSettingsFragment extends LeanbackPreferenceFragment implements Preference.OnPreferenceChangeListener, SeekBar.OnSeekBarChangeListener {

    private static final String TAG = "AudioSettingsFragment";

    private static final String KEY_TV_SOUND_AUDIO_SOURCE_SELECT             = "key_tv_sound_audio_source_select";

    private static int mCurrentSettingSourceId = 0;

    private ListPreference mTvSourceSelectPref;

    private SeekBar mSeekBarAudioOutputDelaySpeaker;
    private SeekBar mSeekBarAudioOutputDelaySpdif;
    private SeekBar mSeekBarAudioPrescale;

    private TextView mTextAudioOutputDelaySpeaker;
    private TextView mTextAudioOutputDelaySpdif;
    private TextView mTextAudioPrescale;

    private AudioEffectManager mAudioEffectManager;
    private boolean mIsSeekBarInited = false;

    private static final int AUDIO_OUTPUT_DELAY_SOURCE_DEFAULT               = 0;


    public static AudioSettingsFragment newInstance() {
        return new AudioSettingsFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
        mTvSourceSelectPref.setValueIndex(mCurrentSettingSourceId);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.tv_sound_audio_source_select, null);
        if (mAudioEffectManager == null) {
            mAudioEffectManager = ((TvSettingsActivity)getActivity()).getAudioEffectManager();
        }

        mTvSourceSelectPref = (ListPreference) findPreference(KEY_TV_SOUND_AUDIO_SOURCE_SELECT);
        mTvSourceSelectPref.setValueIndex(getCurrentSource());
        mTvSourceSelectPref.setOnPreferenceChangeListener(this);
        mTvSourceSelectPref.setEnabled(true);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);
        final int selection = Integer.parseInt((String)newValue);
        if (TextUtils.equals(preference.getKey(), KEY_TV_SOUND_AUDIO_SOURCE_SELECT)) {
            setCurrentSource(selection);
            createParamSettingUiDialog();
        }
        return true;
    }

    private void setCurrentSource(int source) {
        mCurrentSettingSourceId = source;
    }

    private int getCurrentSource() {
        return mCurrentSettingSourceId;
    }

    private void createParamSettingUiDialog() {
        Context context = (Context) (getActivity());
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.xml.tv_sound_audio_settings_seekbar, null);//tv_sound_audio_settings_seekbar.xml
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        final AlertDialog mAlertDialog = builder.create();
        mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                mIsSeekBarInited = false;
            }
        });
        mAlertDialog.show();
        mAlertDialog.getWindow().setContentView(view);
        initSeekBar(view);
    }

    private void initSeekBar(View view) {
        int delayMs = 0;

        mSeekBarAudioOutputDelaySpeaker = (SeekBar) view.findViewById(R.id.id_seek_bar_audio_delay_speaker);
        mTextAudioOutputDelaySpeaker = (TextView) view.findViewById(R.id.id_text_view_audio_delay_speaker);
        delayMs = mAudioEffectManager.getAudioOutputSpeakerDelay(getCurrentSource());
        mSeekBarAudioOutputDelaySpeaker.setOnSeekBarChangeListener(this);
        mSeekBarAudioOutputDelaySpeaker.setProgress(delayMs);
        setShow(R.id.id_seek_bar_audio_delay_speaker, delayMs);
        mSeekBarAudioOutputDelaySpeaker.requestFocus();

        mSeekBarAudioOutputDelaySpdif = (SeekBar) view.findViewById(R.id.id_seek_bar_audio_delay_spdif);
        mTextAudioOutputDelaySpdif = (TextView) view.findViewById(R.id.id_text_view_audio_delay_spdif);
        delayMs = mAudioEffectManager.getAudioOutputSpdifDelay(getCurrentSource());
        mSeekBarAudioOutputDelaySpdif.setOnSeekBarChangeListener(this);
        mSeekBarAudioOutputDelaySpdif.setProgress(delayMs);
        setShow(R.id.id_seek_bar_audio_delay_spdif, delayMs);

        mSeekBarAudioPrescale = (SeekBar) view.findViewById(R.id.id_seek_bar_audio_prescale);
        mTextAudioPrescale = (TextView) view.findViewById(R.id.id_text_view_audio_prescale);
        int value = mAudioEffectManager.getAudioPrescale(getCurrentSource());
        mSeekBarAudioPrescale.setOnSeekBarChangeListener(this);
        mSeekBarAudioPrescale.setProgress(value+150);
        setShow(R.id.id_seek_bar_audio_prescale, value);

        mIsSeekBarInited = true;
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (!mIsSeekBarInited) {
            return;
        }
        switch (seekBar.getId()) {
            case R.id.id_seek_bar_audio_delay_speaker:{
                setShow(R.id.id_seek_bar_audio_delay_speaker, progress);
                mAudioEffectManager.setAudioOutputSpeakerDelay(getCurrentSource(), progress);
                setDelayEnabled();
                break;
            }
            case R.id.id_seek_bar_audio_delay_spdif:{
                setShow(R.id.id_seek_bar_audio_delay_spdif, progress);
                mAudioEffectManager.setAudioOutputSpdifDelay(getCurrentSource(), progress);
                setDelayEnabled();
                break;
            }
            case R.id.id_seek_bar_audio_prescale:{
                // UI display -150 - 150
                setShow(R.id.id_seek_bar_audio_prescale, progress-150);
                mAudioEffectManager.setAudioPrescale(getCurrentSource(), progress-150);
                setDelayEnabled();
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
            case R.id.id_seek_bar_audio_delay_speaker:{
                mTextAudioOutputDelaySpeaker.setText(getShowString(R.string.title_tv_audio_delay_speaker, value));
                break;
            }
            case R.id.id_seek_bar_audio_delay_spdif:{
                mTextAudioOutputDelaySpdif.setText(getShowString(R.string.title_tv_audio_delay_spdif, value));
                break;
            }
            case R.id.id_seek_bar_audio_prescale:{
                mTextAudioPrescale.setText(getActivity().getResources().getString(R.string.title_tv_audio_prescale) + ": " + value/10.0 + " dB");
                break;
            }
            default:
                break;
        }
    }

    private String getShowString(int resid, int value) {
        return getActivity().getResources().getString(resid) + ": " + value + " ms";
    }

    private void setDelayEnabled () {
        SystemControlManager.getInstance().setProperty(AudioEffectManager.PROP_AUDIO_DELAY_ENABLED, "true");
    }
}
