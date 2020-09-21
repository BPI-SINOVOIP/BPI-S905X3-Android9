/*
* opyright (C) 2015 The Android Open Source Project
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

import android.app.Activity;
import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.support.annotation.Keep;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.CheckBoxPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.text.TextUtils;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.hardware.hdmi.HdmiTvClient;
import android.hardware.hdmi.HdmiTvClient.SelectCallback;
import android.os.SystemProperties;
import com.droidlogic.tv.settings.SettingsConstant;
import android.support.v7.preference.Preference;
import com.droidlogic.tv.settings.R;


@Keep
public class HdmiCecDeviceSelectFragment extends LeanbackPreferenceFragment implements Preference.OnPreferenceClickListener {
    private static final String LOG_TAG = "HdmiCecDevControlFragment";
    HdmiControlManager hcm;
    HdmiTvClient tv;
    private static final int MSG_FRESH_UI = 0;
    private static final int ADDR_TV = 0;
    private static final int ADDR_SPECIFIC_USE = 14;
    private static final String[] DEFAULT_NAMES = {
        "TV",
        "Recorder_1",
        "Recorder_2",
        "Tuner_1",
        "Playback_1",
        "AudioSystem",
        "Tuner_2",
        "Tuner_3",
        "Playback_2",
        "Recorder_3",
        "Tuner_4",
        "Playback_3",
        "Reserved_1",
        "Reserved_2",
        "Secondary_TV",
    };

    public static final int POWER_STATUS_UNKNOWN = -1;
    public static final int POWER_STATUS_ON = 0;
    public static final int POWER_STATUS_STANDBY = 1;
    public static final int POWER_STATUS_TRANSIENT_TO_ON = 2;
    public static final int POWER_STATUS_TRANSIENT_TO_STANDBY = 3;
    public static final int DEVICE_EVENT_REMOVE_DEVICE = 3;

    public static final int CEC_MESSAGE_USER_CONTROL_PRESSED = 0x44;
    public static final int CEC_MESSAGE_USER_CONTROL_RELEASED = 0x45;

    public static final int CEC_KEYCODE_POWER = 0x40;
    public static final int CEC_KEYCODE_POWER_ON_FUNCTION = 0x6D;

    private ArrayList<HdmiDeviceInfo> mHdmiDeviceInfoList = new ArrayList();
    public static HdmiCecDeviceSelectFragment newInstance() {
        return new HdmiCecDeviceSelectFragment();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        boolean tvFlag = SettingsConstant.needDroidlogicTvFeature(getContext())
                    && (SystemProperties.getBoolean("tv.soc.as.mbox", false) == false);
        if (tvFlag) {
            hcm = (HdmiControlManager) getActivity().getSystemService(Context.HDMI_CONTROL_SERVICE);
            tv = hcm.getTvClient();
            updatePreferenceFragment();
        }
    }

    private void updatePreferenceFragment() {
        Log.d(LOG_TAG, "updatePreferenceFragment ");
        final Context themedContext = getPreferenceManager().getContext();
        final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(
                themedContext);
        screen.setTitle(R.string.title_cec_device_list);
        setPreferenceScreen(screen);
        int logicalAddress;
        int mDevicePowerStatus;
        String deviceName = "";
        mHdmiDeviceInfoList.clear();
        for (HdmiDeviceInfo info : tv.getDeviceList()) {
            if (info != null) {
                mHdmiDeviceInfoList.add(info);
                logicalAddress = info.getLogicalAddress();
                mDevicePowerStatus = info.getDevicePowerStatus();
                if (ADDR_TV <= logicalAddress && logicalAddress <= ADDR_SPECIFIC_USE) {
                    deviceName = (info.getDisplayName() != null ? info.getDisplayName() : DEFAULT_NAMES[logicalAddress]);
                }
                Log.d(LOG_TAG, "logicalAddress = " + logicalAddress + ", deviceName = " + deviceName);
                final Preference mPreference = new Preference(themedContext);
                mPreference.setTitle(deviceName);
                mPreference.setKey(String.valueOf(logicalAddress));
                mPreference.setOnPreferenceClickListener(this);
                screen.addPreference(mPreference);
            }
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        //mHandler.sendEmptyMessage(MSG_FRESH_UI);
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        Log.d(LOG_TAG, "[onPreferenceClick] preference.getKey() = " + preference.getKey());
        final int logicalAddress = Integer.parseInt((String)preference.getKey());
        /*
        if (tv != null) {
            byte[] mUCPbody = new byte[2];
            mUCPbody[0] = CEC_MESSAGE_USER_CONTROL_PRESSED;
            mUCPbody[1] = CEC_KEYCODE_POWER_ON_FUNCTION;
            tv.sendCommonCecCommand(ADDR_TV, logicalAddress, mUCPbody);
            byte[] mUCRbody = new byte[1];
            mUCRbody[0] = CEC_MESSAGE_USER_CONTROL_RELEASED;
            tv.sendCommonCecCommand(ADDR_TV, logicalAddress, mUCRbody);
        }
        */
        String msg  = "select device "+ preference.getTitle() + "!";
        Toast toast = Toast.makeText(getActivity(), msg, Toast.LENGTH_SHORT);
        toast.show();
        deviceSelect(logicalAddress);
        return true;
    }

    private void deviceSelect(int logicAddr) {
        if (tv == null) {
            return;
        }
        tv.deviceSelect(logicAddr, new SelectCallback() {
            @Override
            public void onComplete(int result) {
                if (result != HdmiControlManager.RESULT_SUCCESS) {
                    Log.d(LOG_TAG, "select device fail, onComplete result = " + result);
                } else {
                    Log.d(LOG_TAG, "select device success, onComplete result = " + result);
                }
            }
        });
    }

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_FRESH_UI:
                    updatePreferenceFragment();
                    break;
            }
        }
    };
}
