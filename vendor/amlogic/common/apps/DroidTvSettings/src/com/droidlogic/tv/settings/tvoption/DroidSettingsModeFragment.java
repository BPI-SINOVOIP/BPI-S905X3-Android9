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
import android.content.Intent;
import android.content.DialogInterface;
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
import android.provider.Settings;
import android.media.tv.TvInputManager;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.view.ViewGroup;

import com.droidlogic.app.HdmiCecManager;
import com.droidlogic.tv.settings.util.DroidUtils;
import com.droidlogic.tv.settings.SettingsConstant;
import com.droidlogic.tv.settings.MainFragment;
import com.droidlogic.tv.settings.R;

import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;

public class DroidSettingsModeFragment extends LeanbackPreferenceFragment implements Preference.OnPreferenceChangeListener {

    private static final String TAG = "DroidSettingsModeFragment";

    private static final String STARTUP_SETTING = "tv_startup_setting";
    private static final String DYNAMIC_BACKLIGHT = "tv_dynamic_backlight";
    private static final String RESTORE_FACTORY= "tv_restore_factory";
    private static final String FBC_UPGRADE = "tv_fbc_upgrade";
    private static final String PIP= "tv_pip";
    private static final String CLOSED_CAPTIONS = "tv_closed_captions";
    private static final String AV_PARENTAL_CONTROLS = "parental_controls";
    private static final String MENU_TIME = "tv_menu_time";
    private static final String SLEEP_TIMER = "sleep_timer";
    private static final String DAYLIGHT_SAVING_TIME = "tv_daylight_saving_time";
    private static final String FACTORY_MENU =  "tv_factory_menu";
    private static final String HDMI_SWITCH =  "tv_hdmi_switch";
    private static final String KEY_HDMI_CEC_CONTROL = "hdmicec";
    private static final String KEY_HDMI_AUDIO_LATENCY = "box_hdmi_audio_latency";

    private static final String AV_PARENTAL_CONTROLS_ON = "On";
    private static final String AV_PARENTAL_CONTROLS_OFF = "Off";

    private static final int RESTORE = 0;
    private static final int FBC = 1;

    private static final String INTENT_ACTION_FINISH_FRAGMENT = "action.finish.droidsettingsmodefragment";

    private TvOptionSettingManager mTvOptionSettingManager;
    private TvControlManager mTvControlManager;
    private TvInputManager mTvInputManager;

    public static DroidSettingsModeFragment newInstance() {
        return new DroidSettingsModeFragment();
    }

    private boolean CanDebug() {
        return TvOptionFragment.CanDebug();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mTvOptionSettingManager == null) {
            mTvOptionSettingManager = new TvOptionSettingManager(getActivity(), false);
        }
        final Preference hdmiAudioLatency = (Preference) findPreference(KEY_HDMI_AUDIO_LATENCY);
        hdmiAudioLatency.setSummary(mTvOptionSettingManager.getHdmiAudioLatency() + "ms");
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.droid_settings_mode, null);
        if (mTvOptionSettingManager == null) {
            mTvOptionSettingManager = new TvOptionSettingManager(getActivity(), false);
        }
        if (mTvControlManager == null) {
            mTvControlManager = TvControlManager.getInstance();
        }
        if (mTvInputManager == null) {
            mTvInputManager = (TvInputManager)getActivity().getSystemService(Context.TV_INPUT_SERVICE);
        }
        boolean isTv = SettingsConstant.needDroidlogicTvFeature(getActivity());
        boolean hideStartUp = DroidUtils.VALUE_HIDE_STARTUP.equals(DroidUtils.read(getActivity(), DroidUtils.KEY_HIDE_STARTUP));

        final Preference pip = (Preference) findPreference(PIP);
        pip.setVisible(false);

        final Preference hdmiSwitch = (Preference) findPreference(HDMI_SWITCH);
        final Preference avParentalControls = (Preference) findPreference(AV_PARENTAL_CONTROLS);
        int deviceId = Settings.System.getInt(getContext().getContentResolver(),
                DroidLogicTvUtils.TV_CURRENT_DEVICE_ID, 0);
        Log.d(TAG, "mSourceInput: " + deviceId);
        boolean isParentControlEnabled = mTvInputManager.isParentalControlsEnabled();
        if (deviceId == DroidLogicTvUtils.DEVICE_ID_AV1
            || deviceId == DroidLogicTvUtils.DEVICE_ID_AV2) {
            avParentalControls.setVisible(true);
        } else {
            avParentalControls.setVisible(false);
        }
        if (deviceId != DroidLogicTvUtils.DEVICE_ID_HDMI1
            && deviceId != DroidLogicTvUtils.DEVICE_ID_HDMI2
            && deviceId != DroidLogicTvUtils.DEVICE_ID_HDMI3
            && deviceId != DroidLogicTvUtils.DEVICE_ID_HDMI4) {
            hdmiSwitch.setVisible(false);
        }
        if (isParentControlEnabled) {
            avParentalControls.setSummary(AV_PARENTAL_CONTROLS_ON);
        } else {
            avParentalControls.setSummary(AV_PARENTAL_CONTROLS_OFF);
        }
        final ListPreference startupseting = (ListPreference) findPreference(STARTUP_SETTING);
        if (!DroidUtils.hasGtvsUiMode() && !hideStartUp) {
            startupseting.setValueIndex(mTvOptionSettingManager.getStartupSettingStatus());
            startupseting.setOnPreferenceChangeListener(this);
        } else {
            startupseting.setVisible(false);
        }
        final ListPreference menutime = (ListPreference) findPreference(MENU_TIME);
        menutime.setValueIndex(mTvOptionSettingManager.getMenuTimeStatus());
        menutime.setOnPreferenceChangeListener(this);
        final ListPreference sleeptimer = (ListPreference) findPreference(SLEEP_TIMER);
        sleeptimer.setValueIndex(mTvOptionSettingManager.getSleepTimerStatus());
        sleeptimer.setOnPreferenceChangeListener(this);
        final ListPreference dynamicbacklightPref = (ListPreference) findPreference(DYNAMIC_BACKLIGHT);
        dynamicbacklightPref.setEntries(initswitchEntries());
        dynamicbacklightPref.setEntryValues(initSwitchEntryValue());
        dynamicbacklightPref.setValueIndex(mTvOptionSettingManager.getDynamicBacklightStatus());
        dynamicbacklightPref.setOnPreferenceChangeListener(this);
        final Preference fbcupgrade = (Preference) findPreference(FBC_UPGRADE);
        fbcupgrade.setVisible(false);

        final ListPreference daylightSavingTime = (ListPreference) findPreference(DAYLIGHT_SAVING_TIME);
        if (SystemProperties.getBoolean("persist.sys.daylight.control", false)) {
            daylightSavingTime.setEntries(R.array.tv_daylight_saving_time_entries);
            daylightSavingTime.setEntryValues(R.array.tv_daylight_saving_time_entry_values);
            daylightSavingTime.setValueIndex(mTvOptionSettingManager.getDaylightSavingTime());
            daylightSavingTime.setOnPreferenceChangeListener(this);
        } else {
            daylightSavingTime.setVisible(false);
        }
        final Preference hdmiCec = (Preference) findPreference(KEY_HDMI_CEC_CONTROL);
        if (!isTv) {
            hdmiCec.setVisible(false);
        }
        final Preference factoryRestore = (Preference) findPreference(RESTORE_FACTORY);
        factoryRestore.setVisible(false);
        final Preference hdmiAudioLatency = (Preference) findPreference(KEY_HDMI_AUDIO_LATENCY);
        if (isTv) {
            hdmiAudioLatency.setTitle(getActivity().getResources().getString(R.string.arc_hdmi_audio_latency));
        }
        hdmiAudioLatency.setSummary(mTvOptionSettingManager.getHdmiAudioLatency() + "ms");
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey());
        if (TextUtils.equals(preference.getKey(), RESTORE_FACTORY)) {
            createUiDialog(RESTORE);
        } else if (TextUtils.equals(preference.getKey(), FBC_UPGRADE)) {
            createUiDialog(FBC);
        } else if (TextUtils.equals(preference.getKey(), AV_PARENTAL_CONTROLS)) {
            startUiInLiveTv(AV_PARENTAL_CONTROLS);
        } else if (TextUtils.equals(preference.getKey(), CLOSED_CAPTIONS)) {
            startUiInLiveTv(CLOSED_CAPTIONS);
        } else if (TextUtils.equals(preference.getKey(), PIP)) {
            startUiInLiveTv(PIP);
        }
        return super.onPreferenceTreeClick(preference);
    }

    private void startUiInLiveTv(String value) {
        Intent intent = new Intent();
        intent.setAction("action.startlivetv.settingui");
        intent.putExtra(value, true);
        getActivity().sendBroadcast(intent);
        getActivity().finish();
    }

    private void startFactoryMenu() {
        Intent factory = new Intent("droidlogic.intent.action.FactoryMainActivity");
        getActivity().startActivity(factory);
        getActivity().finish();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);
        final int selection = Integer.parseInt((String)newValue);
        if (TextUtils.equals(preference.getKey(), STARTUP_SETTING)) {
            mTvOptionSettingManager.setStartupSetting(selection);
        } else if (TextUtils.equals(preference.getKey(), DYNAMIC_BACKLIGHT)) {
            mTvOptionSettingManager.setAutoBacklightStatus(selection);
        } else if (TextUtils.equals(preference.getKey(), MENU_TIME)) {
            mTvOptionSettingManager.setMenuTime(selection);
        } else if (TextUtils.equals(preference.getKey(), SLEEP_TIMER)) {
            mTvOptionSettingManager.setSleepTimer(selection);
        } else if (TextUtils.equals(preference.getKey(), DAYLIGHT_SAVING_TIME)) {
            mTvOptionSettingManager.setDaylightSavingTime(selection);
        }

        Intent intent = new Intent();
        intent.setAction(INTENT_ACTION_FINISH_FRAGMENT);
        getActivity().sendBroadcast(intent);
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
        AlertDialog.Builder uiDialog = new AlertDialog.Builder(getActivity());
        String dialogtitle = "";
        String dialogdetails = "";
        if (RESTORE == type) {
            dialogtitle = getActivity().getResources().getString(R.string.tv_ensure_restore);
            dialogdetails = getActivity().getResources().getString(R.string.tv_prompt_def_set);
        } else if (FBC == type) {
            dialogtitle = getActivity().getResources().getString(R.string.tv_ensure_fbc_update);
            dialogdetails = getActivity().getResources().getString(R.string.tv_fbc_upgrade_detail);
        }
        uiDialog.setTitle(dialogtitle);
        uiDialog.setMessage(dialogdetails);
        uiDialog.setPositiveButton(getString(R.string.tv_ok)
            , new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    if (RESTORE == type) {
                        mTvOptionSettingManager.doFactoryReset();
                    } else if (FBC == type) {
                        mTvOptionSettingManager.doFbcUpgrade();
                    }
                    dialog.dismiss();
                    ((PowerManager) context.getSystemService(Context.POWER_SERVICE)).reboot(null);
                }
            });
        uiDialog.setNegativeButton(getString(R.string.tv_cancel)
            , new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    dialog.dismiss();
                }
            });
        uiDialog.create().show();
    }

    /*private void createUiDialog (int type) {
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
    }*/
}
