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
import android.text.TextUtils;
import android.util.Log;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.TwoStatePreference;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;
import com.droidlogic.app.tv.AudioEffectManager;

public class DbxSettingFragment extends LeanbackPreferenceFragment implements Preference.OnPreferenceChangeListener, SeekBar.OnSeekBarChangeListener{
    private static final String TAG = "DbxSettingFragment";

    private static final String KEY_TV_DBX_ENABLE               = "key_tv_dbx_enable";
    private static final String KEY_TV_DBX_SOUND_MODE           = "key_tv_dbx_sound_mode";

    private AudioEffectManager mAudioEffectManager;
    private TwoStatePreference mDbxSwitchPref;
    private ListPreference mDbxSoundModePref;
    private SeekBar mSeekBarSonics;
    private SeekBar mSeekBarVolume;
    private SeekBar mSeekBarSurround;
    private TextView mTextSonics;
    private TextView mTextVolume;
    private TextView mTextSurround;
    private boolean mIsSeekBarInited = false;

    public static DbxSettingFragment newInstance() {
        return new DbxSettingFragment();
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
        boolean dbxStatus = mAudioEffectManager.getDbxEnable();
        mDbxSoundModePref.setValueIndex(mAudioEffectManager.getDbxSoundMode());
        mDbxSoundModePref.setEnabled(dbxStatus);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.tv_sound_dbx_setting, null);
        if (mAudioEffectManager == null) {
            mAudioEffectManager = ((TvSettingsActivity)getActivity()).getAudioEffectManager();
        }
        boolean dbxStatus = mAudioEffectManager.getDbxEnable();

        mDbxSwitchPref = (TwoStatePreference) findPreference(KEY_TV_DBX_ENABLE);
        mDbxSwitchPref.setChecked(dbxStatus);
        mDbxSwitchPref.setEnabled(true);

        mDbxSoundModePref = (ListPreference) findPreference(KEY_TV_DBX_SOUND_MODE);
        mDbxSoundModePref.setValueIndex(mAudioEffectManager.getDbxSoundMode());
        mDbxSoundModePref.setOnPreferenceChangeListener(this);
        mDbxSoundModePref.setEnabled(dbxStatus);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);
        final int selection = Integer.parseInt((String)newValue);
        if (TextUtils.equals(preference.getKey(), KEY_TV_DBX_SOUND_MODE)) {
            mAudioEffectManager.setDbxSoundMode(selection);
            if (selection == AudioEffectManager.DBX_SOUND_MODE_ADVANCED) {
                createParamSettingUiDialog();
            }
        }
        return true;
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        final String key = preference.getKey();
        if (key != null) {
            switch (key) {
                case KEY_TV_DBX_ENABLE:
                    mAudioEffectManager.setDbxEnable(mDbxSwitchPref.isChecked());
                    boolean dbxStatus = mAudioEffectManager.getDbxEnable();
                    mDbxSoundModePref.setEnabled(dbxStatus);
                    break;
            }
        }
        return super.onPreferenceTreeClick(preference);
    }
    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
    }
    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (!mIsSeekBarInited) {
            return;
        }
        ((TvSettingsActivity)getActivity()).startShowActivityTimer();
        switch (seekBar.getId()) {
            case R.id.id_seek_bar_tv_dbx_param_sonics:{
                setShow(AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS, progress);
                mAudioEffectManager.setDbxAdvancedModeParam(AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS, progress);
                break;
            }
            case R.id.id_seek_bar_tv_dbx_param_volume:{
                setShow(AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME, progress);
                mAudioEffectManager.setDbxAdvancedModeParam(AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME, progress);
                break;
            }
            case R.id.id_seek_bar_tv_dbx_param_surround:{
                setShow(AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND, progress);
                mAudioEffectManager.setDbxAdvancedModeParam(AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND, progress);
                break;
            }
            default:
                break;
        }
    }

    private void initSeekBar(View view) {
        int status = 0;
        mSeekBarSonics = (SeekBar) view.findViewById(R.id.id_seek_bar_tv_dbx_param_sonics);
        mTextSonics = (TextView) view.findViewById(R.id.id_text_view_tv_dbx_param_sonics);
        status = mAudioEffectManager.getDbxAdvancedModeParam(AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS);
        mSeekBarSonics.setOnSeekBarChangeListener(this);
        mSeekBarSonics.setProgress(status);
        setShow(AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS, status);
        mSeekBarSonics.requestFocus();

        mSeekBarVolume = (SeekBar) view.findViewById(R.id.id_seek_bar_tv_dbx_param_volume);
        mTextVolume = (TextView) view.findViewById(R.id.id_text_view_tv_dbx_param_volume);
        status = mAudioEffectManager.getDbxAdvancedModeParam(AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME);
        mSeekBarVolume.setOnSeekBarChangeListener(this);
        mSeekBarVolume.setProgress(status);
        setShow(AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME, status);

        mSeekBarSurround = (SeekBar) view.findViewById(R.id.id_seek_bar_tv_dbx_param_surround);
        mTextSurround = (TextView) view.findViewById(R.id.id_text_view_tv_dbx_param_surround);
        status = mAudioEffectManager.getDbxAdvancedModeParam(AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND);
        mSeekBarSurround.setOnSeekBarChangeListener(this);
        mSeekBarSurround.setProgress(status);
        setShow(AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND, status);
        mIsSeekBarInited = true;
    }

    private void createParamSettingUiDialog() {
        Context context = (Context) (getActivity());
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.xml.tv_sound_dbx_advanced_param_setting_ui, null);//tv_sound_dbx_advanced_param_setting_ui.xml
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

    private void setShow(int id, int value) {
        switch (id) {
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS:{
                mTextSonics.setText(getShowString(R.string.title_tv_dbx_param_sonics, value));
                break;
            }
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME:{
                mTextVolume.setText(getShowString(R.string.title_tv_dbx_param_volume, value));
                break;
            }
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND:{
                mTextSurround.setText(getShowString(R.string.title_tv_dbx_param_surround, value));
                break;
            }
            default:
                break;
        }
    }

    private String getShowString(int resid, int value) {
        return getActivity().getResources().getString(resid) + ": " + value;
    }

}
