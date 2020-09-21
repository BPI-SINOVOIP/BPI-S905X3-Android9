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
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.PreferenceCategory;
import android.os.SystemProperties;
import android.util.Log;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.content.Context;
import android.view.View;
import android.app.AlertDialog;
import android.widget.TextView;
import android.view.View.OnClickListener;
import android.os.PowerManager;

import com.droidlogic.tv.settings.util.DroidUtils;
import com.droidlogic.tv.settings.SettingsConstant;
import com.droidlogic.tv.settings.R;

public class SettingsModeFragment extends LeanbackPreferenceFragment implements Preference.OnPreferenceChangeListener {

    private static final String TAG = "SettingsModeFragment";

    private static final String SRATUP_SETTING = "tv_startup_setting";
    private static final String DYNAMIC_BACKLIGHT = "tv_dynamic_backlight";
    private static final String AUDIO_AD_SWITCH = "tv_audio_ad_switch";
    private static final String MULTI_SETTINGS = "multi_settings";
    private static final String RESTORE_FACTORY= "tv_restore_factory";
    private static final String FBC_UPGRADE = "tv_fbc_upgrade";

    private static final int RESTORE = 0;
    private static final int FBC = 1;

    private TvOptionSettingManager mTvOptionSettingManager;

    public static SettingsModeFragment newInstance() {
        return new SettingsModeFragment();
    }

    private boolean CanDebug() {
        return TvOptionFragment.CanDebug();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.tv_settings_mode, null);
        if (mTvOptionSettingManager == null) {
            mTvOptionSettingManager = new TvOptionSettingManager(getActivity(), false);
        }
        final ListPreference startupseting = (ListPreference) findPreference(SRATUP_SETTING);
        if (!DroidUtils.hasGtvsUiMode()) {
            startupseting.setValueIndex(mTvOptionSettingManager.getStartupSettingStatus());
            startupseting.setOnPreferenceChangeListener(this);
        } else {
            startupseting.setVisible(false);
        }
        final ListPreference dynamicbacklight = (ListPreference) findPreference(DYNAMIC_BACKLIGHT);
        dynamicbacklight.setEntries(initswitchEntries());
        dynamicbacklight.setEntryValues(initSwitchEntryValue());
        dynamicbacklight.setValueIndex(mTvOptionSettingManager.getDynamicBacklightStatus());
        dynamicbacklight.setOnPreferenceChangeListener(this);
        final ListPreference audioadswitch = (ListPreference) findPreference(AUDIO_AD_SWITCH);
        audioadswitch.setEntries(initswitchEntries());
        audioadswitch.setEntryValues(initSwitchEntryValue());
        audioadswitch.setValueIndex(mTvOptionSettingManager.getADSwitchStatus());
        audioadswitch.setOnPreferenceChangeListener(this);
        final Preference fbcupgrade = (Preference) findPreference(FBC_UPGRADE);
        fbcupgrade.setVisible(false);
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey());
        if (TextUtils.equals(preference.getKey(), RESTORE_FACTORY)) {
            createUiDialog(RESTORE);
        } else if (TextUtils.equals(preference.getKey(), FBC_UPGRADE)) {
            createUiDialog(FBC);
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);
        final int selection = Integer.parseInt((String)newValue);
        if (TextUtils.equals(preference.getKey(), SRATUP_SETTING)) {
            mTvOptionSettingManager.setStartupSetting(selection);
        } else if (TextUtils.equals(preference.getKey(), DYNAMIC_BACKLIGHT)) {
            mTvOptionSettingManager.setAutoBacklightStatus(selection);
        } else if (TextUtils.equals(preference.getKey(), AUDIO_AD_SWITCH)) {
            mTvOptionSettingManager.setAudioADSwitch(selection);
        }
        return true;
    }

    private String[] initswitchEntries() {
        String[] temp = new String[2];
        temp[0] = getActivity().getResources().getString(R.string.tv_settins_off);
        temp[1] = getActivity().getResources().getString(R.string.tv_settins_on);
        return temp;
    }

    private String[] initSwitchEntryValue() {
        String[] temp = new String[2];
        for (int i = 0; i < 2; i++) {
            temp[i] = String.valueOf(i);
        }
        return temp;
    }

    private void createUiDialog (int type) {
        Context context = (Context) (getActivity());
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.xml.layout_dialog, null);

        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        final AlertDialog mAlertDialog = builder.create();
        mAlertDialog.show();
        mAlertDialog.getWindow().setContentView(view);
        //mAlertDialog.getWindow().setLayout(150, 320);
        TextView button_cancel = (TextView)view.findViewById(R.id.dialog_cancel);
        TextView dialogtitle = (TextView)view.findViewById(R.id.dialog_title);
        TextView dialogdetails = (TextView)view.findViewById(R.id.dialog_details);
        if (RESTORE == type) {
            dialogtitle.setText(getActivity().getResources().getString(R.string.tv_ensure_restore));
            dialogdetails.setText(getActivity().getResources().getString(R.string.tv_prompt_def_set));
        } else if (FBC == type) {
            dialogtitle.setText(getActivity().getResources().getString(R.string.tv_ensure_fbc_update));
            dialogdetails.setText(getActivity().getResources().getString(R.string.tv_fbc_upgrade_detail));
        }
        button_cancel.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mAlertDialog != null)
                    mAlertDialog.dismiss();
            }
        });
        button_cancel.requestFocus();
        TextView button_ok = (TextView)view.findViewById(R.id.dialog_ok);
        button_ok.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                if (RESTORE == type) {
                    mTvOptionSettingManager.doFactoryReset();
                } else if (FBC == type) {
                    mTvOptionSettingManager.doFbcUpgrade();
                }
                mAlertDialog.dismiss();
                ((PowerManager) context.getSystemService(Context.POWER_SERVICE)).reboot("");
            }
        });
    }
}
