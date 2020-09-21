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
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.SeekBarPreference;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.TwoStatePreference;
import android.util.Log;

import com.droidlogic.tv.settings.tvoption.SoundParameterSettingManager;
import com.droidlogic.app.OutputModeManager;

public class DolbyAudioEffectFragment extends LeanbackPreferenceFragment implements Preference.OnPreferenceChangeListener {

    private static final String TAG = "DapModeFragment";

    private static final String KEY_DAP_MODE = "dap_mode";
    private static final String KEY_DAP_DETAIL = "dap_detail";
    private static final String KEY_VOLUME_LEVELER = "dap_volume_leveler";
    private static final String KEY_VOLUME_LEVELER_AMOUNT = "dap_volume_leveler_amount";
    private static final String KEY_DIALOG_ENHANCER = "dap_dialog_enhancer";
    private static final String KEY_DIALOG_ENHANCER_AMOUNT = "dap_dialog_enhancer_amount";
    private static final String KEY_VIRTUALIZER_SURROUND = "dap_virtualizer_surround";
    private static final String KEY_SURROUND_BOOST = "dap_surround_boost";
    private static final String KEY_GEQ_MODE = "dap_geq";
    private static final String KEY_GEQ_BAND1 = "effect_band1";
    private static final String KEY_GEQ_BAND2 = "effect_band2";
    private static final String KEY_GEQ_BAND3 = "effect_band3";
    private static final String KEY_GEQ_BAND4 = "effect_band4";
    private static final String KEY_GEQ_BAND5 = "effect_band5";
    private static final String KEY_VL_INFO = "dap_vl_info";
    private static final String KEY_DE_INFO = "dap_de_info";
    private static final String KEY_GEQ_INFO = "dap_geq_info";

    private PreferenceCategory mDapDetailPref;
    private TwoStatePreference mVlPref;
    private SeekBarPreference mVlAmountPref;
    private TwoStatePreference mDePref;
    private SeekBarPreference mDeAmountPref;
    private SeekBarPreference mSurroundBoostPref;
    private ListPreference mGeqPref;
    private SeekBarPreference mSeekbar1;
    private SeekBarPreference mSeekbar2;
    private SeekBarPreference mSeekbar3;
    private SeekBarPreference mSeekbar4;
    private SeekBarPreference mSeekbar5;
    private Preference mVlInfoPref;
    private Preference mDeInfoPref;
    private Preference mGeqInfoPref;

    private DolbyAudioEffectManager mDapManager;

    public static DolbyAudioEffectFragment newInstance() {
        return new DolbyAudioEffectFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
        updateDetail();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        boolean enable = false;
        int progress = 0;

        if (mDapManager == null)
            mDapManager = DolbyAudioEffectManager.getInstance(getActivity());

        setPreferencesFromResource(R.xml.dolby_audioeffect, null);

        final ListPreference dapPref = (ListPreference) findPreference(KEY_DAP_MODE);
        mDapDetailPref = (PreferenceCategory) findPreference(KEY_DAP_DETAIL);
        mVlPref = (TwoStatePreference) findPreference(KEY_VOLUME_LEVELER);
        mVlAmountPref = (SeekBarPreference) findPreference(KEY_VOLUME_LEVELER_AMOUNT);
        mDePref = (TwoStatePreference) findPreference(KEY_DIALOG_ENHANCER);
        mDeAmountPref = (SeekBarPreference) findPreference(KEY_DIALOG_ENHANCER_AMOUNT);
        mSurroundBoostPref = (SeekBarPreference) findPreference(KEY_SURROUND_BOOST);
        mGeqPref = (ListPreference) findPreference(KEY_GEQ_MODE);
        mSeekbar1 = (SeekBarPreference) findPreference(KEY_GEQ_BAND1);
        mSeekbar2 = (SeekBarPreference) findPreference(KEY_GEQ_BAND2);
        mSeekbar3 = (SeekBarPreference) findPreference(KEY_GEQ_BAND3);
        mSeekbar4 = (SeekBarPreference) findPreference(KEY_GEQ_BAND4);
        mSeekbar5 = (SeekBarPreference) findPreference(KEY_GEQ_BAND5);
        mVlInfoPref = (Preference) findPreference(KEY_VL_INFO);
        mDeInfoPref = (Preference) findPreference(KEY_DE_INFO);
        mGeqInfoPref = (Preference) findPreference(KEY_GEQ_INFO);

        progress = mDapManager.getDapParam(OutputModeManager.CMD_DAP_EFFECT_MODE);
        dapPref.setValueIndex(progress);
        dapPref.setOnPreferenceChangeListener(this);


        mVlAmountPref.setOnPreferenceChangeListener(this);
        mDeAmountPref.setOnPreferenceChangeListener(this);
        mSurroundBoostPref.setOnPreferenceChangeListener(this);

        mSeekbar1.setMin(-10);
        mSeekbar1.setMax(10);
        mSeekbar1.setOnPreferenceChangeListener(this);
        mSeekbar2.setMin(-10);
        mSeekbar2.setMax(10);
        mSeekbar2.setOnPreferenceChangeListener(this);
        mSeekbar3.setMin(-10);
        mSeekbar3.setMax(10);
        mSeekbar3.setOnPreferenceChangeListener(this);
        mSeekbar4.setMin(-10);
        mSeekbar4.setMax(10);
        mSeekbar4.setOnPreferenceChangeListener(this);
        mSeekbar5.setMin(-10);
        mSeekbar5.setMax(10);
        mSeekbar5.setOnPreferenceChangeListener(this);
        mGeqPref.setOnPreferenceChangeListener(this);
    }

    private void updateGeq(int mode) {
        if (mode == OutputModeManager.DAP_GEQ_OFF) {
            mSeekbar1.setVisible(false);
            mSeekbar2.setVisible(false);
            mSeekbar3.setVisible(false);
            mSeekbar4.setVisible(false);
            mSeekbar5.setVisible(false);
        } else {
            mSeekbar1.setValue(mDapManager.getDapParam(OutputModeManager.SUBCMD_DAP_GEQ_BAND1));
            mSeekbar2.setValue(mDapManager.getDapParam(OutputModeManager.SUBCMD_DAP_GEQ_BAND2));
            mSeekbar3.setValue(mDapManager.getDapParam(OutputModeManager.SUBCMD_DAP_GEQ_BAND3));
            mSeekbar4.setValue(mDapManager.getDapParam(OutputModeManager.SUBCMD_DAP_GEQ_BAND4));
            mSeekbar5.setValue(mDapManager.getDapParam(OutputModeManager.SUBCMD_DAP_GEQ_BAND5));
            mSeekbar1.setVisible(true);
            mSeekbar2.setVisible(true);
            mSeekbar3.setVisible(true);
            mSeekbar4.setVisible(true);
            mSeekbar5.setVisible(true);
            int val = mDapManager.getDapParam(OutputModeManager.CMD_DAP_EFFECT_MODE);
            if ((mode == OutputModeManager.DAP_GEQ_USER) && (val == OutputModeManager.DAP_MODE_USER)) {
                mSeekbar1.setAdjustable(true);
                mSeekbar2.setAdjustable(true);
                mSeekbar3.setAdjustable(true);
                mSeekbar4.setAdjustable(true);
                mSeekbar5.setAdjustable(true);
            } else {
                mSeekbar1.setAdjustable(false);
                mSeekbar2.setAdjustable(false);
                mSeekbar3.setAdjustable(false);
                mSeekbar4.setAdjustable(false);
                mSeekbar5.setAdjustable(false);
            }
        }
    }

    private String getShowString(int resid) {
        return getActivity().getResources().getString(resid);
    }

    private void updateDetail() {
        boolean enable = false;
        boolean isUserMode = false;
        int val = 0, progress = 0;
        int mode = 0;

        mode = mDapManager.getDapParam(OutputModeManager.CMD_DAP_EFFECT_MODE);
        if (mode == OutputModeManager.DAP_MODE_OFF) {
            mVlPref.setVisible(false);
            mVlAmountPref.setVisible(false);
            mVlInfoPref.setVisible(false);
            mDePref.setVisible(false);
            mDeAmountPref.setVisible(false);
            mDeInfoPref.setVisible(false);
            mSurroundBoostPref.setVisible(false);
            mGeqPref.setVisible(false);
            mGeqInfoPref.setVisible(false);
            mSeekbar1.setVisible(false);
            mSeekbar2.setVisible(false);
            mSeekbar3.setVisible(false);
            mSeekbar4.setVisible(false);
            mSeekbar5.setVisible(false);
            mDapDetailPref.setTitle("");
            return;
        } else if (mode == OutputModeManager.DAP_MODE_USER) {
            isUserMode = true;
        } else
            isUserMode = false;


        enable = (mDapManager.getDapParam(OutputModeManager.CMD_DAP_VL_ENABLE) != OutputModeManager.DAP_OFF);
        val = mDapManager.getDapParam(OutputModeManager.CMD_DAP_VL_AMOUNT);
        mVlPref.setChecked(enable);
        mVlAmountPref.setValue(val);
        mVlAmountPref.setAdjustable(isUserMode);
        mVlAmountPref.setTitle(isUserMode?"":getShowString(R.string.dap_volume_leveler));
        mVlInfoPref.setVisible((!isUserMode) && (!enable));
        mVlPref.setVisible(isUserMode);
        mVlAmountPref.setVisible(enable);

        enable = (mDapManager.getDapParam(OutputModeManager.CMD_DAP_DE_ENABLE) != OutputModeManager.DAP_OFF);
        val = mDapManager.getDapParam(OutputModeManager.CMD_DAP_DE_AMOUNT);
        mDePref.setChecked(enable);
        mDeAmountPref.setValue(val);
        mDeAmountPref.setAdjustable(isUserMode);
        mDeAmountPref.setTitle(isUserMode?"":getShowString(R.string.dap_dialog_enhancer));
        mDeInfoPref.setVisible((!isUserMode) && (!enable));
        mDePref.setVisible(isUserMode);
        mDeAmountPref.setVisible(enable);

        val = mDapManager.getDapParam(OutputModeManager.CMD_DAP_SURROUND_BOOST);
        mSurroundBoostPref.setValue(val);
        mSurroundBoostPref.setAdjustable(isUserMode);
        mSurroundBoostPref.setVisible(true);

        val = mDapManager.getDapParam(OutputModeManager.CMD_DAP_GEQ_ENABLE);
        mGeqPref.setValueIndex(val);
        mGeqInfoPref.setSummary(mGeqPref.getEntry());
        mGeqInfoPref.setVisible(!isUserMode);
        mGeqPref.setVisible(isUserMode);

        updateGeq(val);
        mDapDetailPref.setTitle(R.string.dap_detail);
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        boolean isChecked;
        switch (preference.getKey()) {
            case KEY_VOLUME_LEVELER:
                isChecked = mVlPref.isChecked();
                mDapManager.setDapParam(OutputModeManager.CMD_DAP_VL_ENABLE, isChecked?1:0);
                mVlAmountPref.setVisible(isChecked);
                break;
            case KEY_DIALOG_ENHANCER:
                isChecked = mDePref.isChecked();
                mDapManager.setDapParam(OutputModeManager.CMD_DAP_DE_ENABLE, isChecked?1:0);
                mDeAmountPref.setVisible(isChecked);
                break;
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        switch (preference.getKey()) {
            case KEY_DAP_MODE:
                final int selection = Integer.parseInt((String)newValue);
                mDapManager.setDapParam(OutputModeManager.CMD_DAP_EFFECT_MODE, selection);
                break;
            case KEY_VOLUME_LEVELER_AMOUNT:
                mDapManager.setDapParam(OutputModeManager.CMD_DAP_VL_AMOUNT, (int)newValue);
                break;
            case KEY_DIALOG_ENHANCER_AMOUNT:
                mDapManager.setDapParam(OutputModeManager.CMD_DAP_DE_AMOUNT, (int)newValue);
                break;
            case KEY_SURROUND_BOOST:
                mDapManager.setDapParam(OutputModeManager.CMD_DAP_SURROUND_BOOST, (int)newValue);
                break;
            case KEY_GEQ_MODE:
                final int progress = Integer.parseInt((String)newValue);
                mDapManager.setDapParam(OutputModeManager.CMD_DAP_GEQ_ENABLE, progress);
                updateGeq(progress);
                break;
            case KEY_GEQ_BAND1:
                mDapManager.setDapParam(OutputModeManager.SUBCMD_DAP_GEQ_BAND1, (int)newValue);
                break;
            case KEY_GEQ_BAND2:
                mDapManager.setDapParam(OutputModeManager.SUBCMD_DAP_GEQ_BAND2, (int)newValue);
                break;
            case KEY_GEQ_BAND3:
                mDapManager.setDapParam(OutputModeManager.SUBCMD_DAP_GEQ_BAND3, (int)newValue);
                break;
            case KEY_GEQ_BAND4:
                mDapManager.setDapParam(OutputModeManager.SUBCMD_DAP_GEQ_BAND4, (int)newValue);
                break;
            case KEY_GEQ_BAND5:
                mDapManager.setDapParam(OutputModeManager.SUBCMD_DAP_GEQ_BAND5, (int)newValue);
                break;
        }
        return true;
    }
}
