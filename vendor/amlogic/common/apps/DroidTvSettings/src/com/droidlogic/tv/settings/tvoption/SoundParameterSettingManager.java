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

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.text.TextUtils;
import android.util.Log;
import android.os.SystemProperties;
import android.media.AudioManager;
import android.app.ActivityManager;
import android.provider.Settings;
import android.content.SharedPreferences;
import android.content.ContentResolver;

import com.droidlogic.tv.settings.R;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.AudioSettingManager;

import com.droidlogic.tv.settings.SettingsConstant;

import java.util.Arrays;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.HashMap;
import java.util.HashSet;
import java.util.TimeZone;
import java.util.SimpleTimeZone;

public class SoundParameterSettingManager {

    public static final String TAG = "SoundParameterSettingManager";
    public static final String DIGITAL_SOUND_PCM = "pcm";
    public static final String DIGITAL_SOUND_SPDIF = "spdif";
    public static final String DIGITAL_SOUND_AUTO = "auto";
    public static final String DIGITAL_SOUND_MANUAL = "manual";
    public static final String TV_KEY_AD_SWITCH = "ad_switch";

    private Resources mResources;
    private Context mContext;
    private AudioManager mAudioManager;
    private OutputModeManager mOutputModeManager;
    private AudioSettingManager mAudioSettingManager;

    public SoundParameterSettingManager (Context context) {
        mContext = context;
        mResources = mContext.getResources();
        mAudioManager = (AudioManager) context.getSystemService(context.AUDIO_SERVICE);
        mOutputModeManager = new OutputModeManager(context);
        mAudioSettingManager = new AudioSettingManager(context);
    }

    static public boolean CanDebug() {
        SystemControlManager mSystemControlManager = SystemControlManager.getInstance();
        return mSystemControlManager.getPropertyBoolean("vendor.soundparameter.debug", false);
    }

    // 0 1 ~ off on
    public int getVirtualSurroundStatus() {
        final int itemPosition =  Settings.Global.getInt(mContext.getContentResolver(),
                OutputModeManager.VIRTUAL_SURROUND, OutputModeManager.VIRTUAL_SURROUND_OFF);
        if (CanDebug()) Log.d(TAG, "getVirtualSurroundStatus = " + itemPosition);
        return itemPosition;
    }

    public int getSoundOutputStatus () {
        final int itemPosition =  Settings.Global.getInt(mContext.getContentResolver(),
                OutputModeManager.SOUND_OUTPUT_DEVICE, OutputModeManager.SOUND_OUTPUT_DEVICE_SPEAKER);
        if (CanDebug()) Log.d(TAG, "getSoundOutputStatus = " + itemPosition);
        return itemPosition;
    }

    public void setVirtualSurround (int mode) {
        if (CanDebug()) Log.d(TAG, "setVirtualSurround = " + mode);
        mOutputModeManager.setVirtualSurround(mode);
        Settings.Global.putInt(mContext.getContentResolver(), OutputModeManager.VIRTUAL_SURROUND, mode);
    }

    public void setSoundOutputStatus (int mode) {
        if (CanDebug()) Log.d(TAG, "setSoundOutputStatus = " + mode);
        mOutputModeManager.setSoundOutputStatus(mode);
        Settings.Global.putInt(mContext.getContentResolver(), OutputModeManager.SOUND_OUTPUT_DEVICE, mode);
        Settings.Global.putInt(mContext.getContentResolver(),
                "hdmi_system_audio_status_enabled" /* Settings.Global.HDMI_SYSTEM_AUDIO_STATUS_ENABLED */,
                mode == OutputModeManager.SOUND_OUTPUT_DEVICE_ARC ? OutputModeManager.TV_ARC_ON : OutputModeManager.TV_ARC_OFF);
    }

    public void setDigitalAudioFormat (String mode) {
        if (CanDebug()) Log.d(TAG, "setDigitalAudioFormat = " + mode);
        switch (mode) {
            case DIGITAL_SOUND_PCM:
                mOutputModeManager.setDigitalAudioFormatOut(OutputModeManager.DIGITAL_PCM);
                break;
            case DIGITAL_SOUND_SPDIF:
                mOutputModeManager.setDigitalAudioFormatOut(OutputModeManager.DIGITAL_SPDIF);
                break;
            case DIGITAL_SOUND_MANUAL:
                mOutputModeManager.setDigitalAudioFormatOut(OutputModeManager.DIGITAL_MANUAL,
                        getAudioManualFormats());
                break;
            case DIGITAL_SOUND_AUTO:
            default:
                mOutputModeManager.setDigitalAudioFormatOut(OutputModeManager.DIGITAL_AUTO);
                break;
        }
    }

    public String getDigitalAudioFormat() {
        final int value = Settings.Global.getInt(mContext.getContentResolver(),
                OutputModeManager.DIGITAL_AUDIO_FORMAT, OutputModeManager.DIGITAL_AUTO);
        if (CanDebug()) Log.d(TAG, "getDigitalAudioFormat value = " + value);

        switch (value) {
        case OutputModeManager.DIGITAL_PCM:
            return DIGITAL_SOUND_PCM;
        case OutputModeManager.DIGITAL_SPDIF:
            return DIGITAL_SOUND_SPDIF;
        case OutputModeManager.DIGITAL_MANUAL:
            return DIGITAL_SOUND_MANUAL;
        case OutputModeManager.DIGITAL_AUTO:
        default:
            return DIGITAL_SOUND_AUTO;
        }
    }

    public void setAudioManualFormats(int id, boolean enabled) {
        HashSet<Integer> fmts = new HashSet<>();
        String enable = getAudioManualFormats();
        if (!enable.isEmpty()) {
            try {
                Arrays.stream(enable.split(",")).mapToInt(Integer::parseInt)
                    .forEach(fmts::add);
            } catch (NumberFormatException e) {
                Log.w(TAG, "DIGITAL_AUDIO_SUBFORMAT misformatted.", e);
            }
        }
        if (enabled) {
            fmts.add(id);
        } else {
            fmts.remove(id);
        }
        mOutputModeManager.setDigitalAudioFormatOut(
                OutputModeManager.DIGITAL_MANUAL, TextUtils.join(",", fmts));
    }

    public String getAudioManualFormats() {
        return mAudioSettingManager.getAudioManualFormats();
    }

    public void setAudioMixingEnable(boolean newVal) {
        mAudioSettingManager.setAudioMixingEnable(newVal);
    }

    public boolean getAudioMixingEnable() {
        return mAudioSettingManager.getAudioMixingEnable();
    }

    public void setARCLatency(int newVal) {
        mAudioSettingManager.setARCLatency(newVal);
    }

    public int getARCLatency() {
        return mAudioSettingManager.getARCLatency();
    }

    public void setAudioOutputLatency(int newVal) {
        Settings.Global.putInt(mContext.getContentResolver(), OutputModeManager.DB_FIELD_AUDIO_OUTPUT_LATENCY, newVal);
        mOutputModeManager.setAudioOutputLatency(newVal);
    }

    public int getAudioOutputLatency() {
        return Settings.Global.getInt(mContext.getContentResolver(), OutputModeManager.DB_FIELD_AUDIO_OUTPUT_LATENCY,
                OutputModeManager.AUDIO_OUTPUT_LATENCY_DEFAULT);
    }

    public void setDrcModePassthroughSetting(int newVal) {
        mAudioSettingManager.setDrcModePassthroughSetting(newVal);
    }

    public boolean getAdSurportStatus() {
        int result = Settings.System.getInt(mContext.getContentResolver(),
                TV_KEY_AD_SWITCH, 0);
        return result > 0;
    }

    public void setAdSurportStatus(boolean newVal) {
        Settings.System.putInt(mContext.getContentResolver(),
                TV_KEY_AD_SWITCH, newVal ? 1 : 0);
    }

    public static boolean getSoundEffectsEnabled(ContentResolver contentResolver) {
        return Settings.System.getInt(contentResolver, Settings.System.SOUND_EFFECTS_ENABLED, 1) != 0;
    }

    public static final String DRC_OFF = "off";
    public static final String DRC_LINE = "line";
    public static final String DRC_RF = "rf";

    public String getDrcModePassthroughSetting() {
        boolean tvflag = SettingsConstant.needDroidlogicTvFeature(mContext)
              && (!SystemProperties.getBoolean("tv.soc.as.mbox", false));
        int value;
        if (tvflag) {
            value = Settings.Global.getInt(mContext.getContentResolver(),
                OutputModeManager.DRC_MODE, OutputModeManager.IS_DRC_RF);
        } else {
            value = Settings.Global.getInt(mContext.getContentResolver(),
                OutputModeManager.DRC_MODE, OutputModeManager.IS_DRC_LINE);
        }

        switch (value) {
        case OutputModeManager.IS_DRC_OFF:
            return DRC_OFF;
        case OutputModeManager.IS_DRC_LINE:
        default:
            return DRC_LINE;
        case OutputModeManager.IS_DRC_RF:
            return DRC_RF;
        }
    }

    public void resetParameter() {
        Log.d(TAG, "resetParameter");
        mOutputModeManager.resetSoundParameters();
    }
}

