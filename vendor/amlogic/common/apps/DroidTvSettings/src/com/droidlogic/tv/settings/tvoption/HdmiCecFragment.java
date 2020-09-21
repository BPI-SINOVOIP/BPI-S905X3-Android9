/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC HdmiCecFragment
 */

package com.droidlogic.tv.settings.tvoption;

import android.content.Context;
import android.content.ContentResolver;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiTvClient;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.UserHandle;
import android.provider.Settings;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceGroup;
import android.support.v7.preference.SeekBarPreference;
import android.support.v7.preference.TwoStatePreference;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import com.droidlogic.app.HdmiCecManager;
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.tv.settings.R;
import com.droidlogic.tv.settings.SettingsConstant;
import com.droidlogic.tv.settings.SoundFragment;

import java.util.Map;
import java.util.Set;
import android.os.SystemProperties;
/**
 * Fragment to control HDMI Cec settings.
 */
public class HdmiCecFragment extends LeanbackPreferenceFragment implements Preference.OnPreferenceChangeListener{

    private static final String TAG = "HdmiCecFragment";
    private static HdmiCecFragment mHdmiCecFragment = null;

    private static final String KEY_CEC_SWITCH                  = "key_cec_switch";
    private static final String KEY_CEC_ONE_KEY_PLAY            = "key_cec_one_key_play";
    private static final String KEY_CEC_AUTO_POWER_OFF          = "key_cec_auto_power_off";
    private static final String KEY_CEC_AUTO_WAKE_UP            = "key_cec_auto_wake_up";
    private static final String KEY_CEC_AUTO_CHANGE_LANGUAGE    = "key_cec_auto_change_language";
    private static final String KEY_CEC_ARC_SWITCH              = "key_cec_arc_switch";
    private static final String KEY_CEC_DEVICE_LIST             = "key_cec_device_list";

    private TwoStatePreference mCecSwitchPref;
    private TwoStatePreference mCecOnekeyPlayPref;
    private TwoStatePreference mCecDeviceAutoPoweroffPref;
    private TwoStatePreference mCecAutoWakeupPref;
    private TwoStatePreference mCecAutoChangeLanguagePref;
    private TwoStatePreference mArcSwitchPref;

    private SystemControlManager mSystemControlManager = SystemControlManager.getInstance();
    private SoundParameterSettingManager mSoundParameterSettingManager;
    private HdmiCecManager mHdmiCecManager;
    private static long lastObserveredTime = 0;

    private HdmiControlManager mHdmiControlManager;
    private HdmiTvClient mHdmiTvClient;
    private HdmiControlManager.VendorCommandListener mVendorCommandListener;

    class SettingsVendorCommandListener implements HdmiControlManager.VendorCommandListener {
        @Override
        public void onReceived(int srcAddress, int destAddress, byte[] params, boolean hasVendorId) {
        }

        @Override
        public void onControlStateChanged(boolean enabled, int reason) {
            if (HdmiControlManager.CONTROL_STATE_CHANGED_REASON_SETTING == reason) {
                Log.d(TAG, "onControlStateChanged hdmi cec settings " + enabled);
                mHandler.sendMessage(Message.obtain(mHandler, MSG_ENABLE_CEC_SWITCH, enabled));
            }
        }
    }

    public static HdmiCecFragment newInstance() {
        if (mHdmiCecFragment == null) {
            mHdmiCecFragment = new HdmiCecFragment();
        }
        return mHdmiCecFragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mHdmiCecManager = new HdmiCecManager(getContext());
        mHdmiControlManager = (HdmiControlManager)getContext().getSystemService(Context.HDMI_CONTROL_SERVICE);
        if (mHdmiControlManager != null) {
            mHdmiTvClient = mHdmiControlManager.getTvClient();
        }
        if (mHdmiTvClient != null) {
            // it means the device is tv.
            mVendorCommandListener = new SettingsVendorCommandListener();
            mHdmiTvClient.setVendorCommandListener(mVendorCommandListener);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        refresh();
    }

    @Override
    public void onPause() {
        super.onPause();
        mHandler.removeCallbacksAndMessages(null);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        // When exit need to make sure the hdmi listener could be removed normally.
    }


    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        if (mSoundParameterSettingManager == null) {
            mSoundParameterSettingManager = new SoundParameterSettingManager(getActivity());
        }
        setPreferencesFromResource(R.xml.hdmicec, null);
        boolean tvFlag = SettingsConstant.needDroidlogicTvFeature(getContext())
                    && (SystemProperties.getBoolean("tv.soc.as.mbox", false) == false);
        mCecSwitchPref = (TwoStatePreference) findPreference(KEY_CEC_SWITCH);
        mCecOnekeyPlayPref = (TwoStatePreference) findPreference(KEY_CEC_ONE_KEY_PLAY);
        mCecDeviceAutoPoweroffPref = (TwoStatePreference) findPreference(KEY_CEC_AUTO_POWER_OFF);
        mCecAutoWakeupPref = (TwoStatePreference) findPreference(KEY_CEC_AUTO_WAKE_UP);
        mCecAutoChangeLanguagePref = (TwoStatePreference) findPreference(KEY_CEC_AUTO_CHANGE_LANGUAGE);
        mArcSwitchPref = (TwoStatePreference) findPreference(KEY_CEC_ARC_SWITCH);

        final Preference hdmiDeviceSelectPref = findPreference(KEY_CEC_DEVICE_LIST);
        if (mHdmiCecFragment == null) {
            mHdmiCecFragment = newInstance();
        }
        hdmiDeviceSelectPref.setOnPreferenceChangeListener(mHdmiCecFragment);

        final ListPreference digitalAudioFormat = (ListPreference) findPreference(SoundFragment.KEY_DIGITALSOUND_PASSTHROUGH);
        digitalAudioFormat.setEntries(getActivity().getResources().getStringArray(R.array.digital_sounds_tv_entries));
        digitalAudioFormat.setEntryValues(getActivity().getResources().getStringArray(R.array.digital_sounds_tv_entry_values));
        digitalAudioFormat.setValue(mSoundParameterSettingManager.getDigitalAudioFormat());
        digitalAudioFormat.setOnPreferenceChangeListener(this);

        final SeekBarPreference audioOutputLatencyPref = (SeekBarPreference) findPreference(SoundFragment.KEY_AUDIO_OUTPUT_LATENCY);
        audioOutputLatencyPref.setOnPreferenceChangeListener(this);
        audioOutputLatencyPref.setMax(OutputModeManager.AUDIO_OUTPUT_LATENCY_MAX);
        audioOutputLatencyPref.setMin(OutputModeManager.AUDIO_OUTPUT_LATENCY_MIN);
        audioOutputLatencyPref.setSeekBarIncrement(SoundFragment.KEY_AUDIO_OUTPUT_LATENCY_STEP);
        audioOutputLatencyPref.setValue(mSoundParameterSettingManager.getAudioOutputLatency());

        mCecOnekeyPlayPref.setVisible(!tvFlag);
        mCecAutoWakeupPref.setVisible(tvFlag);
        mCecSwitchPref.setVisible(true);
        mArcSwitchPref.setVisible(tvFlag);
        hdmiDeviceSelectPref.setVisible(tvFlag);
        audioOutputLatencyPref.setVisible(tvFlag);
        digitalAudioFormat.setVisible(tvFlag);
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        final String key = preference.getKey();
        if (key == null) {
            return super.onPreferenceTreeClick(preference);
        }

        switch (key) {
        case KEY_CEC_SWITCH:
            Log.d(TAG, "onPreferenceTreeClick cec switch " + mCecSwitchPref.isChecked());
            mHdmiCecManager.enableHdmiControl(mCecSwitchPref.isChecked());
            mCecSwitchPref.setEnabled(false);
            mCecOnekeyPlayPref.setEnabled(false);
            mCecDeviceAutoPoweroffPref.setEnabled(false);
            mCecAutoWakeupPref.setEnabled(false);
            mCecAutoChangeLanguagePref.setEnabled(false);
            mArcSwitchPref.setEnabled(false);
            return true;
        case KEY_CEC_ONE_KEY_PLAY:
            mHdmiCecManager.enableOneTouchPlay(mCecOnekeyPlayPref.isChecked());
            return true;
        case KEY_CEC_AUTO_POWER_OFF:
            mHdmiCecManager.enableAutoPowerOff(mCecDeviceAutoPoweroffPref.isChecked());
            return true;
        case KEY_CEC_AUTO_WAKE_UP:
            mHdmiCecManager.enableAutoWakeUp(mCecAutoWakeupPref.isChecked());
            return true;
        case KEY_CEC_AUTO_CHANGE_LANGUAGE:
            mHdmiCecManager.enableAutoChangeLanguage(mCecAutoChangeLanguagePref.isChecked());
            return true;
        case KEY_CEC_ARC_SWITCH:
            mHdmiCecManager.enableArc(mArcSwitchPref.isChecked());
            mHandler.sendEmptyMessageDelayed(MSG_ENABLE_ARC_SWITCH, TIME_DELAYED);
            mArcSwitchPref.setEnabled(false);
            return true;
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);
        if (TextUtils.equals(preference.getKey(), SoundFragment.KEY_DIGITALSOUND_PASSTHROUGH)) {
            mSoundParameterSettingManager.setDigitalAudioFormat((String)newValue);
        } else if (TextUtils.equals(preference.getKey(), SoundFragment.KEY_AUDIO_OUTPUT_LATENCY)) {
            mSoundParameterSettingManager.setAudioOutputLatency((int)newValue);
        }
        return true;
    }

    private void refresh() {
        boolean hdmiControlEnabled = mHdmiCecManager.isHdmiControlEnabled();
        mCecSwitchPref.setChecked(hdmiControlEnabled);
        mCecOnekeyPlayPref.setChecked(mHdmiCecManager.isOneTouchPlayEnabled());
        mCecOnekeyPlayPref.setEnabled(hdmiControlEnabled);
        mCecDeviceAutoPoweroffPref.setChecked(mHdmiCecManager.isAutoPowerOffEnabled());
        mCecDeviceAutoPoweroffPref.setEnabled(hdmiControlEnabled);
        mCecAutoWakeupPref.setChecked(mHdmiCecManager.isAutoWakeUpEnabled());
        mCecAutoWakeupPref.setEnabled(hdmiControlEnabled);
        mCecAutoChangeLanguagePref.setChecked(mHdmiCecManager.isAutoChangeLanguageEnabled());
        mCecAutoChangeLanguagePref.setEnabled(hdmiControlEnabled);

        boolean arcEnabled = mHdmiCecManager.isArcEnabled();
        mArcSwitchPref.setChecked(arcEnabled);
        mArcSwitchPref.setEnabled(hdmiControlEnabled);
    }

    private static final int MSG_ENABLE_CEC_SWITCH = 0;
    private static final int MSG_ENABLE_ARC_SWITCH = 1;
    private static final int TIME_DELAYED = 2000;//ms
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_ENABLE_CEC_SWITCH:
                    boolean enable = (boolean)msg.obj;
                    boolean hdmiControlEnabled = mCecSwitchPref.isChecked();
                    if (enable != hdmiControlEnabled) {
                        Log.e(TAG, "enable cec switch encounters confusion!");
                    }
                    // If the cec switch is on, then all the sub switches are enabled.
                    // If the cec switch is closed, then all the sub switches are disabled.
                    mCecSwitchPref.setEnabled(true);
                    mCecOnekeyPlayPref.setEnabled(hdmiControlEnabled);
                    mCecDeviceAutoPoweroffPref.setEnabled(hdmiControlEnabled);
                    mCecAutoWakeupPref.setEnabled(hdmiControlEnabled);
                    mCecAutoChangeLanguagePref.setEnabled(hdmiControlEnabled);
                    mArcSwitchPref.setEnabled(hdmiControlEnabled);
                    break;
                case MSG_ENABLE_ARC_SWITCH:
                    mArcSwitchPref.setEnabled(true);
                    break;
                default:
                    break;
            }
        }
    };
}
