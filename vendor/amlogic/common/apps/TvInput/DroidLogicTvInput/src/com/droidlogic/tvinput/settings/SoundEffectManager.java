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

package com.droidlogic.tvinput.settings;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.text.TextUtils;
import android.util.Log;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.media.AudioManager;
import android.app.ActivityManager;
import android.content.pm.PackageManager;
import android.provider.Settings;
import android.widget.Toast;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.media.audiofx.AudioEffect;
import android.content.SharedPreferences;
import android.media.AudioFormat;
import android.media.AudioTrack;

import java.util.*;
import java.math.BigDecimal;
import java.text.DecimalFormat;
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.tv.AudioEffectManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;

public class SoundEffectManager {

    public static final String TAG = "SoundEffectManager";

    private static final UUID EFFECT_TYPE_TRUSURROUND           = UUID.fromString("1424f5a0-2457-11e6-9fe0-0002a5d5c51b");
    private static final UUID EFFECT_TYPE_BALANCE               = UUID.fromString("7cb34dc0-242e-11e6-bb63-0002a5d5c51b");
    private static final UUID EFFECT_TYPE_TREBLE_BASS           = UUID.fromString("7e282240-242e-11e6-bb63-0002a5d5c51b");
    private static final UUID EFFECT_TYPE_DAP                   = UUID.fromString("3337b21d-c8e6-4bbd-8f24-698ade8491b9");
    private static final UUID EFFECT_TYPE_EQ                    = UUID.fromString("ce2c14af-84df-4c36-acf5-87e428ed05fc");
    private static final UUID EFFECT_TYPE_AGC                   = UUID.fromString("4a959f5c-e33a-4df2-8c3f-3066f9275edf");
    private static final UUID EFFECT_TYPE_VIRTUAL_SURROUND      = UUID.fromString("c656ec6f-d6be-4e7f-854b-1218077f3915");
    private static final UUID EFFECT_TYPE_VIRTUAL_X             = UUID.fromString("5112a99e-b8b9-4c5e-91fd-a804d29c36b2");
    private static final UUID EFFECT_TYPE_DBX                   = UUID.fromString("a41cedc0-578e-11e5-9cb0-0002a5d5c51b");

    private static final UUID EFFECT_UUID_VIRTUAL_X             = UUID.fromString("61821587-ce3c-4aac-9122-86d874ea1fb1");
    private static final UUID EFFECT_UUID_DBX                   = UUID.fromString("07210842-7432-4624-8b97-35ac8782efa3");

    // defined index ID for DB storage
    public static final String DB_ID_SOUND_EFFECT_BASS                          = "db_id_sound_effect_bass";
    public static final String DB_ID_SOUND_EFFECT_TREBLE                        = "db_id_sound_effect_treble";
    public static final String DB_ID_SOUND_EFFECT_BALANCE                       = "db_id_sound_effect_balance";
    public static final String DB_ID_SOUND_EFFECT_DIALOG_CLARITY                = "db_id_sound_effect_dialog_clarity";
    public static final String DB_ID_SOUND_EFFECT_SURROUND                      = "db_id_sound_effect_surround";
    public static final String DB_ID_SOUND_EFFECT_TRUBASS                       = "db_id_sound_effect_tru_bass";
    public static final String DB_ID_SOUND_EFFECT_SOUND_MODE                    = "db_id_sound_effect_sound_mode";
    public static final String DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE               = "db_id_sound_effect_sound_mode_type";
    public static final String DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_DAP           = "db_id_sound_effect_sound_mode_type_dap";
    public static final String DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ            = "db_id_sound_effect_sound_mode_type_eq";
    public static final String DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE          = "db_id_sound_effect_sound_mode_dap";
    public static final String DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE           = "db_id_sound_effect_sound_mode_eq";
    public static final String DB_ID_SOUND_EFFECT_BAND1                         = "db_id_sound_effect_band1";
    public static final String DB_ID_SOUND_EFFECT_BAND2                         = "db_id_sound_effect_band2";
    public static final String DB_ID_SOUND_EFFECT_BAND3                         = "db_id_sound_effect_band3";
    public static final String DB_ID_SOUND_EFFECT_BAND4                         = "db_id_sound_effect_band4";
    public static final String DB_ID_SOUND_EFFECT_BAND5                         = "db_id_sound_effect_band5";
    public static final String DB_ID_SOUND_EFFECT_AGC_ENABLE                    = "db_id_sound_effect_agc_on";
    public static final String DB_ID_SOUND_EFFECT_AGC_MAX_LEVEL                 = "db_id_sound_effect_agc_level";
    public static final String DB_ID_SOUND_EFFECT_AGC_ATTRACK_TIME              = "db_id_sound_effect_agc_attrack";
    public static final String DB_ID_SOUND_EFFECT_AGC_RELEASE_TIME              = "db_id_sound_effect_agc_release";
    public static final String DB_ID_SOUND_EFFECT_AGC_SOURCE_ID                 = "db_id_sound_avl_source_id";
    public static final String DB_ID_SOUND_EFFECT_VIRTUALX_MODE                 = "db_id_sound_effect_virtualx_mode";
    public static final String DB_ID_SOUND_EFFECT_TREVOLUME_HD                  = "db_id_sound_effect_truvolume_hd";
    public static final String DB_ID_SOUND_EFFECT_DBX_ENABLE                    = "db_id_sound_effect_dbx_enable";
    public static final String DB_ID_SOUND_EFFECT_DBX_SOUND_MODE                = "db_id_sound_effect_dbx_sound_mode";
    public static final String DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SONICS      = "db_id_sound_effect_dbx_advanced_mode_sonics";
    public static final String DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_VOLUME      = "db_id_sound_effect_dbx_advanced_mode_volume";
    public static final String DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SURROUND    = "db_id_sound_effect_dbx_advanced_mode_surround";
    private static final String[] DB_ID_AUDIO_OUTPUT_SPEAKER_DELAY_ARRAY    = {
            "db_id_audio_output_speaker_delay_atv",
            "db_id_audio_output_speaker_delay_dtv",
            "db_id_audio_output_speaker_delay_av",
            "db_id_audio_output_speaker_delay_hdmi",
            "db_id_audio_output_speaker_delay_media",
    };
    private static final String[] DB_ID_AUDIO_OUTPUT_SPDIF_DELAY_ARRAY       = {
            "db_id_audio_output_spdif_delay_atv",
            "db_id_audio_output_spdif_delay_dtv",
            "db_id_audio_output_spdif_delay_av",
            "db_id_audio_output_spdif_delay_hdmi",
            "db_id_audio_output_spdif_delay_media",
    };
    private static final String[] DB_ID_AUDIO_PRESCALE_ARRAY       = {
            "db_id_audio_prescale_atv",
            "db_id_audio_prescale_dtv",
            "db_id_audio_prescale_av",
            "db_id_audio_prescale_hdmi",
            "db_id_audio_prescale_media",
    };

    //set id
    public static final int SET_BASS                                    = 0;
    public static final int SET_TREBLE                                  = 1;
    public static final int SET_BALANCE                                 = 2;
    public static final int SET_DIALOG_CLARITY_MODE                     = 3;
    public static final int SET_SURROUND_ENABLE                         = 4;
    public static final int SET_TRUBASS_ENABLE                          = 5;
    public static final int SET_SOUND_MODE                              = 6;
    public static final int SET_EFFECT_BAND1                            = 7;
    public static final int SET_EFFECT_BAND2                            = 8;
    public static final int SET_EFFECT_BAND3                            = 9;
    public static final int SET_EFFECT_BAND4                            = 10;
    public static final int SET_EFFECT_BAND5                            = 11;
    public static final int SET_AGC_ENABLE                              = 12;
    public static final int SET_AGC_MAX_LEVEL                           = 13;
    public static final int SET_AGC_ATTRACK_TIME                        = 14;
    public static final int SET_AGC_RELEASE_TIME                        = 15;
    public static final int SET_AGC_SOURCE_ID                           = 16;
    public static final int SET_VIRTUAL_SURROUND                        = 17;
    public static final int SET_VIRTUALX_MODE                           = 18;
    public static final int SET_TRUVOLUME_HD_ENABLE                     = 19;
    public static final int SET_DBX_ENABLE                              = 20;
    public static final int SET_DBX_SOUND_MODE                          = 21;
    public static final int SET_DBX_SOUND_MODE_ADVANCED_SONICS          = 22;
    public static final int SET_DBX_SOUND_MODE_ADVANCED_VOLUME          = 23;
    public static final int SET_DBX_SOUND_MODE_ADVANCED_SURROUND        = 24;

    //SoundMode mode.  Parameter ID
    public static final int PARAM_SRS_PARAM_DIALOGCLARTY_MODE           = 1;
    public static final int PARAM_SRS_PARAM_SURROUND_MODE               = 2;
    public static final int PARAM_SRS_PARAM_VOLUME_MODE                 = 3;
    public static final int PARAM_SRS_PARAM_TRUEBASS_ENABLE             = 5;

    //Balance level.  Parameter ID
    public static final int PARAM_BALANCE_LEVEL                         = 0;

    //Tone level.  Parameter ID for
    public static final int PARAM_BASS_LEVEL                            = 0;
    public static final int PARAM_TREBLE_LEVEL                          = 1;

    //dap AudioEffect, [ HPEQparams ] enumeration alignment in Hpeq.cpp
    public static final int PARAM_EQ_ENABLE                             = 0;
    public static final int PARAM_EQ_EFFECT                             = 1;
    public static final int PARAM_EQ_CUSTOM                             = 2;
    //agc effect define
    public static final int PARAM_AGC_ENABLE                            = 0;
    public static final int PARAM_AGC_MAX_LEVEL                         = 1;
    public static final int PARAM_AGC_ATTRACK_TIME                      = 4;
    public static final int PARAM_AGC_RELEASE_TIME                      = 5;
    public static final int PARAM_AGC_SOURCE_ID                         = 6;

    public static final boolean DEFAULT_AGC_ENABLE                      = true; //enable 1, disable 0
    public static final int DEFAULT_AGC_MAX_LEVEL                       = -18;  //db
    public static final int DEFAULT_AGC_ATTRACK_TIME                    = 10;   //ms
    public static final int DEFAULT_AGC_RELEASE_TIME                    = 2;    //s
    public static final int DEFAULT_AGC_SOURCE_ID                       = 3;
    //virtual surround
    public static final int PARAM_VIRTUALSURROUND                       = 0;

    //definition off and on
    private static final int PARAMETERS_SWITCH_OFF                      = 1;
    private static final int PARAMETERS_SWITCH_ON                       = 0;

    private static final int UI_SWITCH_OFF                              = 0;
    private static final int UI_SWITCH_ON                               = 1;

    private static final int PARAMETERS_DAP_ENABLE                      = 1;
    private static final int PARAMETERS_DAP_DISABLE                     = 0;
    //band 1, band 2, band 3, band 4, band 5  need transfer 0~100 to -10~10
    private static final int[] EFFECT_SOUND_MODE_USER_BAND              = {50, 50, 50, 50, 50};
    private static final int EFFECT_SOUND_TYPE_NUM = 6;

    // Virtual X effect param type
    private static final int PARAM_DTS_PARAM_MBHL_ENABLE_I32            = 0;
    private static final int PARAM_DTS_PARAM_TBHDX_ENABLE_I32           = 35;
    private static final int PARAM_DTS_PARAM_VX_ENABLE_I32              = 46;
    private static final int PARAM_DTS_PARAM_LOUDNESS_CONTROL_ENABLE_I32= 67;

    // DBX effect param type
    private static final int PARAM_DBX_PARAM_ENABLE                     = 0;
    private static final int PARAM_DBX_SET_MODE                         = 1;

    private static final String PARAM_HAL_AUDIO_OUTPUT_SPEAKER_DELAY    = "hal_param_speaker_delay_time_ms";
    private static final String PARAM_HAL_AUDIO_OUTPUT_SPDIF_DELAY      = "hal_param_spdif_delay_time_ms";
    private static final String PARAM_HAL_AUDIO_PRESCALE                = "SOURCE_GAIN";

    private static final int AUDIO_OUTPUT_SPEAKER_DELAY_MIN             = 0;       // ms
    private static final int AUDIO_OUTPUT_SPEAKER_DELAY_MAX             = 200;     // ms
    private static final int AUDIO_OUTPUT_SPEAKER_DELAY_DEFAULT         = 0;       // ms

    private static final int AUDIO_OUTPUT_SPDIF_DELAY_MIN               = 0;       // ms
    private static final int AUDIO_OUTPUT_SPDIF_DELAY_MAX               = 200;     // ms
    private static final int AUDIO_OUTPUT_SPPDIF_DELAY_DEFAULT          = 0;       // ms

    private static final int AUDIO_PRESCALE_MIN                         = -150;    // -15 dB
    private static final int AUDIO_PRESCALE_MAX                         = 150;     // 15 dB
    private static final int[] AUDIO_PRESCALE_DEFAULT_ARRAY             = {0, 0, 0, 0, 0}; // ATV, DTV, AV, HDMI, MEDIA, range: [-150 - 150]

    private int mSoundModule = AudioEffectManager.DAP_MODULE;
    // Prefix to append to audio preferences file
    private Context mContext;
    private AudioManager mAudioManager;

    //sound effects
    private AudioEffect mVirtualX;
    private AudioEffect mTruSurround;
    private AudioEffect mBalance;
    private AudioEffect mTrebleBass;
    private AudioEffect mSoundMode;
    private AudioEffect mAgc;
    private AudioEffect mVirtualSurround;
    private AudioEffect mDbx;

    private boolean mSupportVirtualX;

    private static SoundEffectManager mInstance;

    public static synchronized SoundEffectManager getInstance(Context context) {
        if (null == mInstance) {
            mInstance = new SoundEffectManager(context);
        }
        return mInstance;
    }
    private SoundEffectManager (Context context) {
        mContext = context;
        mAudioManager = (AudioManager) context.getSystemService(context.AUDIO_SERVICE);
    }

    static public boolean CanDebug() {
        return SystemProperties.getBoolean("sys.droidlogic.tvinput.debug", false);
    }

    public void createAudioEffects() {
        if (CanDebug()) Log.d(TAG, "Create Audio Effects");
        if (creatVirtualXAudioEffects()) {
            mSupportVirtualX = true;
        } else {
            mSupportVirtualX = false;
            Log.i(TAG, "current not support Virtual X, begin to create TruSurround effect");
            creatTruSurroundAudioEffects();
        }
        creatTrebleBassAudioEffects();
        creatSoundModeAudioEffects();
        creatVirtualSurroundAudioEffects();
        creatBalanceAudioEffects();
        creatDbxAudioEffects();
    }
    private boolean creatVirtualXAudioEffects() {
        try {
            if (mVirtualX == null) {
                if (CanDebug()) Log.d(TAG, "begin to create VirtualX effect");
                mVirtualX = new AudioEffect(EFFECT_TYPE_VIRTUAL_X, EFFECT_UUID_VIRTUAL_X, 0, 0);
            }
            int result = mVirtualX.setEnabled(true);
            if (result != AudioEffect.SUCCESS) {
                Log.e(TAG, "enable VirtualX effect fail, ret:" + result);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "create VirtualX effect fail", e);
            return false;
        }
    }

    private boolean creatTruSurroundAudioEffects() {
        try {
            if (mTruSurround == null) {
                if (CanDebug()) Log.d(TAG, "begin to create TruSurround effect");
                mTruSurround = new AudioEffect(EFFECT_TYPE_TRUSURROUND, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
            }
            int result = mTruSurround.setEnabled(true);
            if (result != AudioEffect.SUCCESS) {
                Log.e(TAG, "enable TruSurround effect fail, ret:" + result);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create mTruSurround audio effect", e);
            return false;
        }
    }

    private boolean creatBalanceAudioEffects() {
        try {
            if (mBalance == null) {
                if (CanDebug()) Log.d(TAG, "creatBalanceAudioEffects");
                mBalance = new AudioEffect(EFFECT_TYPE_BALANCE, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create mBalance audio effect", e);
            return false;
        }
    }

    private boolean creatTrebleBassAudioEffects() {
        try {
            if (mTrebleBass == null) {
                if (CanDebug()) Log.d(TAG, "creatTrebleBassAudioEffects");
                mTrebleBass = new AudioEffect(EFFECT_TYPE_TREBLE_BASS, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create mTrebleBass audio effect", e);
            return false;
        }
    }

    private boolean creatSoundModeAudioEffects() {
        //dap both enable on mbox and tv, so move dap to public code
        //effect num has limit in hal, so if dap valid, ignore eq
        OutputModeManager opm = new OutputModeManager(mContext);
        if (!opm.isDapValid())
            return creatEqAudioEffects();
        return false;
    }

    private boolean creatEqAudioEffects() {
        try {
            if (mSoundMode == null) {
                if (CanDebug()) Log.d(TAG, "creatEqAudioEffects");
                mSoundMode = new AudioEffect(EFFECT_TYPE_EQ, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
                int result = mSoundMode.setEnabled(true);
                if (result == AudioEffect.SUCCESS) {
                    if (CanDebug()) Log.d(TAG, "creatEqAudioEffects enable eq");
                    mSoundMode.setParameter(PARAM_EQ_ENABLE, PARAMETERS_DAP_ENABLE);
                    mSoundModule = AudioEffectManager.EQ_MODULE;
                    Settings.Global.putString(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE,
                            DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ);
                }
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create Eq audio effect", e);
            return false;
        }
    }

    private boolean creatDapAudioEffects() {
        try {
            if (mSoundMode == null) {
                if (CanDebug()) Log.d(TAG, "creatDapAudioEffects");
                mSoundMode = new AudioEffect(EFFECT_TYPE_DAP, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
                int result = mSoundMode.setEnabled(true);
                if (result == AudioEffect.SUCCESS) {
                    if (CanDebug()) Log.d(TAG, "creatDapAudioEffects enable dap");
                    mSoundMode.setParameter(PARAM_EQ_ENABLE, PARAMETERS_DAP_ENABLE);
                    mSoundModule = AudioEffectManager.DAP_MODULE;
                    Settings.Global.putString(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE,
                            DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_DAP);
                }
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create Dap audio effect", e);
            return false;
        }
    }

    private boolean creatAgcAudioEffects() {
        try {
            if (mAgc == null) {
                if (CanDebug()) Log.d(TAG, "creatAgcAudioEffects");
                mAgc = new AudioEffect(EFFECT_TYPE_AGC, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create Agc audio effect", e);
            return false;
        }
    }

    private boolean creatVirtualSurroundAudioEffects() {
        try {
            if (mVirtualSurround == null) {
                if (CanDebug()) Log.d(TAG, "creatVirtualSurroundAudioEffects");
                mVirtualSurround = new AudioEffect(EFFECT_TYPE_VIRTUAL_SURROUND, AudioEffect.EFFECT_TYPE_NULL, 0, 0);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to create VirtualSurround audio effect", e);
            return false;
        }
    }

    private boolean creatDbxAudioEffects() {
        try {
            if (mDbx == null) {
                if (CanDebug()) Log.d(TAG, "begin to create DBX effect");
                mDbx = new AudioEffect(EFFECT_TYPE_DBX, EFFECT_UUID_DBX, 0, 0);
            }
            int result = mDbx.setEnabled(true);
            if (result != AudioEffect.SUCCESS) {
                Log.e(TAG, "enable DBX effect fail, ret:" + result);
            }
            return true;
        } catch (RuntimeException e) {
            Log.e(TAG, "create DBX effect fail", e);
            return false;
        }
    }

    public boolean isSupportVirtualX() {
        return mSupportVirtualX;
    }

    public void setDtsVirtualXMode(int virtalXMode) {
        if (null == mVirtualX) {
            Log.e(TAG, "The VirtualX effect is not created, the mode cannot be setDtsVirtualXMode.");
            return;
        }
        if (CanDebug()) Log.d(TAG, "setDtsVirtualXMode = " + virtalXMode);
        switch (virtalXMode) {
            case AudioEffectManager.SOUND_EFFECT_VIRTUALX_MODE_OFF:
                mVirtualX.setParameter(PARAM_DTS_PARAM_MBHL_ENABLE_I32, 0);
                mVirtualX.setParameter(PARAM_DTS_PARAM_TBHDX_ENABLE_I32, 0);
                mVirtualX.setParameter(PARAM_DTS_PARAM_VX_ENABLE_I32, 0);
                break;
            case AudioEffectManager.SOUND_EFFECT_VIRTUALX_MODE_BASS:
                mVirtualX.setParameter(PARAM_DTS_PARAM_MBHL_ENABLE_I32, 1);
                mVirtualX.setParameter(PARAM_DTS_PARAM_TBHDX_ENABLE_I32, 1);
                mVirtualX.setParameter(PARAM_DTS_PARAM_VX_ENABLE_I32, 0);
                break;
            case AudioEffectManager.SOUND_EFFECT_VIRTUALX_MODE_FULL:
                mVirtualX.setParameter(PARAM_DTS_PARAM_MBHL_ENABLE_I32, 1);
                mVirtualX.setParameter(PARAM_DTS_PARAM_TBHDX_ENABLE_I32, 1);
                mVirtualX.setParameter(PARAM_DTS_PARAM_VX_ENABLE_I32, 1);
                break;
            default:
                Log.w(TAG, "VirtualX effect mode invalid, mode:" + virtalXMode);
                return;
        }
        saveAudioParameters(SET_VIRTUALX_MODE, virtalXMode);
    }

    public int getDtsVirtualXMode() {
        return getSavedAudioParameters(SET_VIRTUALX_MODE);
    }

    public void setDtsTruVolumeHdEnable(boolean enable) {
        if (null == mVirtualX) {
            Log.e(TAG, "The VirtualX effect is not created, the mode cannot be setDtsTruVolumeHdEnable.");
            return;
        }
        if (CanDebug()) Log.d(TAG, "setDtsTruVolumeHdEnable = " + enable);
        int dbSwitch = enable ? 1 : 0;
        mVirtualX.setParameter(PARAM_DTS_PARAM_LOUDNESS_CONTROL_ENABLE_I32, dbSwitch);
        saveAudioParameters(SET_TRUVOLUME_HD_ENABLE, dbSwitch);
    }

    public boolean getDtsTruVolumeHdEnable() {
        int dbSwitch = getSavedAudioParameters(SET_TRUVOLUME_HD_ENABLE);
        boolean enable = (1 == dbSwitch);
        if (dbSwitch != 1 && dbSwitch != 0) {
            Log.w(TAG, "DTS Tru Volume HD db value invalid, db:" + dbSwitch + ", return default false");
        }
        return enable;
    }

    public int getSoundModeStatus () {
        int saveresult = -1;
        if (!creatSoundModeAudioEffects()) {
            Log.e(TAG, "getSoundModeStatus creat fail");
            return AudioEffectManager.EQ_SOUND_MODE_STANDARD;
        }
        int[] value = new int[1];
        mSoundMode.getParameter(PARAM_EQ_EFFECT, value);
        saveresult = getSavedAudioParameters(SET_SOUND_MODE);
        if (saveresult != value[0]) {
            Log.w(TAG, "getSoundModeStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (CanDebug()) {
            Log.d(TAG, "getSoundModeStatus = " + saveresult);
        }
        return saveresult;
    }

    //return current is eq or dap
    public int getSoundModule() {
        return mSoundModule;
    }

    public int getTrebleStatus () {
        int saveresult = -1;
        if (!creatTrebleBassAudioEffects()) {
            Log.e(TAG, "getTrebleStatus mTrebleBass creat fail");
            return AudioEffectManager.EFFECT_TREBLE_DEFAULT;
        }
        int[] value = new int[1];
        mTrebleBass.getParameter(PARAM_TREBLE_LEVEL, value);
        saveresult = getSavedAudioParameters(SET_TREBLE);
        if (saveresult != value[0]) {
            Log.w(TAG, "getTrebleStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (CanDebug()) {
            Log.d(TAG, "getTrebleStatus = " + saveresult);
        }
        return saveresult;
    }

    public int getBassStatus () {
        int saveresult = -1;
        if (!creatTrebleBassAudioEffects()) {
            Log.e(TAG, "getBassStatus mTrebleBass creat fail");
            return AudioEffectManager.EFFECT_BASS_DEFAULT;
        }
        int[] value = new int[1];
        mTrebleBass.getParameter(PARAM_BASS_LEVEL, value);
        saveresult = getSavedAudioParameters(SET_BASS);
        if (saveresult != value[0]) {
            Log.w(TAG, "getBassStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (CanDebug()) {
            Log.d(TAG, "getBassStatus = " + saveresult);
        }
        return saveresult;
    }

    public int getBalanceStatus () {
        int saveresult = -1;
        if (!creatBalanceAudioEffects()) {
            Log.e(TAG, "getBalanceStatus mBalance creat fail");
            return AudioEffectManager.EFFECT_BALANCE_DEFAULT;
        }
        int[] value = new int[1];
        mBalance.getParameter(PARAM_BALANCE_LEVEL, value);
        saveresult = getSavedAudioParameters(SET_BALANCE);
        if (saveresult != value[0]) {
            Log.w(TAG, "getBalanceStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (CanDebug()) {
            Log.d(TAG, "getBalanceStatus = " + saveresult);
        }
        return saveresult;
    }

    public boolean getAgcEnableStatus () {
        int saveresult = -1;
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "getAgcEnableStatus mAgc creat fail");
            return DEFAULT_AGC_ENABLE;
        }
        int[] value = new int[1];
        mAgc.getParameter(PARAM_AGC_ENABLE, value);
        saveresult = getSavedAudioParameters(SET_AGC_ENABLE);
        if (saveresult != value[0]) {
            Log.w(TAG, "getAgcEnableStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (CanDebug()) {
            Log.d(TAG, "getAgcEnableStatus = " + saveresult);
        }
        return saveresult == 1;
    }

    public int getAgcMaxLevelStatus () {
        int saveresult = -1;
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "getAgcEnableStatus mAgc creat fail");
            return DEFAULT_AGC_MAX_LEVEL;
        }
        int[] value = new int[1];
        mAgc.getParameter(PARAM_AGC_MAX_LEVEL, value);
        saveresult = getSavedAudioParameters(SET_AGC_MAX_LEVEL);
        if (saveresult != value[0]) {
            Log.w(TAG, "getAgcMaxLevelStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (CanDebug()) {
            Log.d(TAG, "getAgcMaxLevelStatus = " + saveresult);
        }
        return value[0];
    }

    public int getAgcAttrackTimeStatus () {
        int saveresult = -1;
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "getAgcAttrackTimeStatus mAgc creat fail");
            return DEFAULT_AGC_ATTRACK_TIME;
        }
        int[] value = new int[1];
        mAgc.getParameter(PARAM_AGC_ATTRACK_TIME, value);
        saveresult = getSavedAudioParameters(SET_AGC_ATTRACK_TIME);
        if (saveresult != value[0] / 48) {
            Log.w(TAG, "getAgcAttrackTimeStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (CanDebug()) {
            Log.d(TAG, "getAgcAttrackTimeStatus = " + saveresult);
        }
        //value may be changed realtime
        return value[0] / 48;
    }

    public int getAgcReleaseTimeStatus () {
        int saveresult = -1;
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "getAgcReleaseTimeStatus mAgc creat fail");
            return DEFAULT_AGC_RELEASE_TIME;
        }
        int[] value = new int[1];
        mAgc.getParameter(PARAM_AGC_RELEASE_TIME, value);
        saveresult = getSavedAudioParameters(SET_AGC_RELEASE_TIME);
        if (saveresult != value[0]) {
            Log.w(TAG, "getAgcReleaseTimeStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (CanDebug()) {
            Log.d(TAG, "getAgcReleaseTimeStatus = " + saveresult);
        }
        //value may be changed realtime
        return value[0];
    }

    public int getAgcSourceIdStatus () {
        int saveresult = -1;
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "getAgcSourceIdStatus mAgc creat fail");
            return DEFAULT_AGC_RELEASE_TIME;
        }
        int[] value = new int[1];
        mAgc.getParameter(PARAM_AGC_SOURCE_ID, value);
        saveresult = getSavedAudioParameters(SET_AGC_SOURCE_ID);
        if (saveresult != value[0]) {
            Log.w(TAG, "getAgcSourceIdStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (CanDebug()) {
            Log.d(TAG, "getAgcSourceIdStatus = " + saveresult);
        }
        //value may be changed realtime
        return value[0];
    }

    // 0 1 ~ off on
    public int getVirtualSurroundStatus() {
        int saveresult = -1;
        if (!creatVirtualSurroundAudioEffects()) {
            Log.e(TAG, "getVirtualSurroundStatus mVirtualSurround creat fail");
            return OutputModeManager.VIRTUAL_SURROUND_OFF;
        }
        int[] value = new int[1];
        mVirtualSurround.getParameter(PARAM_VIRTUALSURROUND, value);
        saveresult = getSavedAudioParameters(SET_VIRTUAL_SURROUND);
        if (saveresult != value[0]) {
            Log.w(TAG, "getVirtualSurroundStatus erro get: " + value[0] + ", saved: " + saveresult);
        } else if (CanDebug()) {
            Log.d(TAG, "getVirtualSurroundStatus = " + saveresult);
        }
        return saveresult;
    }

    //set sound mode except customed one
    public void setSoundMode (int mode) {
        //need to set sound mode by observer listener
        saveAudioParameters(SET_SOUND_MODE, mode);
    }

    public void setSoundModeByObserver (int mode) {
        if (!creatSoundModeAudioEffects()) {
            Log.e(TAG, "setSoundMode creat fail");
            return;
        }
        int result = mSoundMode.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (CanDebug()) Log.d(TAG, "setSoundMode = " + mode);
            mSoundMode.setParameter(PARAM_EQ_EFFECT, mode);
            if (mode == AudioEffectManager.EQ_SOUND_MODE_CUSTOM && (mSoundModule == AudioEffectManager.EQ_MODULE || mSoundModule == AudioEffectManager.DAP_MODULE)) {
                //set one band, at the same time the others will be set
                setDifferentBandEffects(AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1, getSavedAudioParameters(SET_EFFECT_BAND1), false);
            }
            //need to set sound mode by observer listener
            //saveAudioParameters(SET_SOUND_MODE, mode);
        }
    }

    public void setUserSoundModeParam(int bandNumber, int value) {
        if (null == mSoundMode) {
            Log.e(TAG, "The EQ effect is not created, the mode cannot be setUserSoundModeParam.");
            return;
        }
        if (bandNumber > AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5 || bandNumber < AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1) {
            Log.e(TAG, "the EQ band number:" + bandNumber + " invalid, set failed");
            return;
        }
        Log.e(TAG, "setUserSoundModeParam bandNumber:"+ bandNumber+", value:"+value);
        setDifferentBandEffects(bandNumber, value, true);
    }

    public int getUserSoundModeParam(int bandNumber) {
        if (bandNumber > AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5 || bandNumber < AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1) {
            Log.e(TAG, "the EQ band number:" + bandNumber + " invalid, set failed");
            return 0;
        }
        int value = 0;
        value = getSavedAudioParameters(bandNumber + SET_EFFECT_BAND1);
        if (CanDebug()) Log.d(TAG, "getUserSoundModeParam band number:" + bandNumber+ ", value:" + value);
        return value;
    }

    private void setDifferentBandEffects(int bandnum, int value, boolean needsave) {
        if (!creatSoundModeAudioEffects()) {
            Log.e(TAG, "setDifferentBandEffects create fail");
            return;
        }
        int result = mSoundMode.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (CanDebug()) Log.d(TAG, "setDifferentBandEffects: NO." + bandnum + " = " + value);
            byte[] fiveband = new byte[5];
            for (int i = AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1; i <= AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5; i++) {
                if (bandnum == i) {
                    fiveband[i] = (byte)MappingLine(value, true);
                } else {
                    fiveband[i] = (byte) MappingLine(getSavedAudioParameters(i + SET_EFFECT_BAND1), true);
                }
            }
            Log.i(TAG, "set eq custom effect band value: " + Arrays.toString(fiveband));
            mSoundMode.setParameter(PARAM_EQ_CUSTOM, fiveband);
            if (needsave) {
                saveAudioParameters(bandnum + SET_EFFECT_BAND1, value);
            }
        }
    }
    //convert -10~10 to 0~100 controled by need or not
    private int unMappingLine(int mapval, boolean need) {
        if (!need) {
            return mapval;
        }

        final int MIN_UI_VAL = -10;
        final int MAX_UI_VAL = 10;
        final int MIN_VAL = 0;
        final int MAX_VAL = 100;
        if (mapval > MAX_UI_VAL || mapval < MIN_UI_VAL) {
            Log.e(TAG, "unMappingLine: map value:" + mapval + " invalid. set default value:" + (MAX_VAL - MIN_VAL) / 2);
            return (MAX_VAL - MIN_VAL) / 2;
        }
        return (mapval - MIN_UI_VAL) * (MAX_VAL - MIN_VAL) / (MAX_UI_VAL - MIN_UI_VAL);
    }

    //convert 0~100 to -10~10 controled by need or not
    private int MappingLine(int mapval, boolean need) {
        if (!need) {
            return mapval;
        }
        final int MIN_UI_VAL = 0;
        final int MAX_UI_VAL = 100;
        final int MIN_VAL = -10;
        final int MAX_VAL = 10;
        if (MIN_VAL < 0) {
            return (mapval - (MAX_UI_VAL + MIN_UI_VAL) / 2) * (MAX_VAL - MIN_VAL)
                   / (MAX_UI_VAL - MIN_UI_VAL);
        } else {
            return (mapval - MIN_UI_VAL) * (MAX_VAL - MIN_VAL) / (MAX_UI_VAL - MIN_UI_VAL);
        }
    }

    public void setTreble (int step) {
        if (!creatTrebleBassAudioEffects()) {
            Log.e(TAG, "setTreble mTrebleBass creat fail");
            return;
        }
        int result = mTrebleBass.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (CanDebug()) Log.d(TAG, "setTreble = " + step);
            mTrebleBass.setParameter(PARAM_TREBLE_LEVEL, step);
            saveAudioParameters(SET_TREBLE, step);
        }
    }

    public void setBass (int step) {
        if (!creatTrebleBassAudioEffects()) {
            Log.e(TAG, "setBass mTrebleBass creat fail");
            return;
        }
        int result = mTrebleBass.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (CanDebug()) Log.d(TAG, "setBass = " + step);
            mTrebleBass.setParameter(PARAM_BASS_LEVEL, step);
            saveAudioParameters(SET_BASS, step);
        }
    }

    public void setBalance (int step) {
        if (!creatBalanceAudioEffects()) {
            Log.e(TAG, "setBalance mBalance creat fail");
            return;
        }
        int result = mBalance.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (CanDebug()) Log.d(TAG, "setBalance = " + step);
            mBalance.setParameter(PARAM_BALANCE_LEVEL, step);
            saveAudioParameters(SET_BALANCE, step);
        }
    }

    public void setSurroundEnable(boolean enable) {
        if (null == mTruSurround) {
            Log.e(TAG, "The Dts TruSurround effect is not created, the mode cannot be setSurroundEnable.");
            return;
        }
        if (CanDebug()) Log.d(TAG, "setSurroundEnable = " + enable);
        int dbSwitch = enable ? 1 : 0;
        mTruSurround.setParameter(PARAM_SRS_PARAM_SURROUND_MODE, dbSwitch);
        saveAudioParameters(SET_SURROUND_ENABLE, dbSwitch);
    }

    public boolean getSurroundEnable() {
        int dbSwitch = getSavedAudioParameters(SET_SURROUND_ENABLE);
        boolean enable = (1 == dbSwitch);
        if (dbSwitch != 1 && dbSwitch != 0) {
            Log.w(TAG, "DTS Surround enable db value invalid, db:" + dbSwitch + ", return default false");
        }
        return enable;
    }

    public void setDialogClarityMode(int mode) {
        if (null == mTruSurround) {
            Log.e(TAG, "The DTS TruSurround effect is not created, the mode cannot be setDialogClarityMode.");
            return;
        }
        if (CanDebug()) Log.d(TAG, "setDialogClarityMode = " + mode);
        mTruSurround.setParameter(PARAM_SRS_PARAM_DIALOGCLARTY_MODE, mode);
        saveAudioParameters(SET_DIALOG_CLARITY_MODE, mode);
    }

    public int getDialogClarityMode() {
        return getSavedAudioParameters(SET_DIALOG_CLARITY_MODE);
    }

    public void setTruBassEnable(boolean enable) {
        if (null == mTruSurround) {
            Log.e(TAG, "The DTS TruSurround effect is not created, the mode cannot be setTruBassEnable.");
            return;
        }
        if (CanDebug()) Log.d(TAG, "setTruBassEnable = " + enable);
        int dbSwitch = enable ? 1 : 0;
        mTruSurround.setParameter(PARAM_SRS_PARAM_TRUEBASS_ENABLE, dbSwitch);
        saveAudioParameters(SET_TRUBASS_ENABLE, dbSwitch);
    }

    public boolean getTruBassEnable() {
        int dbSwitch = getSavedAudioParameters(SET_TRUBASS_ENABLE);
        boolean enable = (1 == dbSwitch);
        if (dbSwitch != 1 && dbSwitch != 0) {
            Log.w(TAG, "DTS TreBass db value invalid, db:" + dbSwitch + ", return default false");
        }
        return enable;
    }

    public void setAgcEnable (boolean enable) {
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "setAgcEnable mAgc creat fail");
            return;
        }
        int dbSwitch = enable ? 1 : 0;
        int result = mAgc.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (CanDebug()) Log.d(TAG, "setAgcEnable = " + dbSwitch);
            mAgc.setParameter(PARAM_AGC_ENABLE, dbSwitch);
            saveAudioParameters(SET_AGC_ENABLE, dbSwitch);
        }
    }

    public void setAgcMaxLevel (int step) {
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "setAgcMaxLevel mAgc creat fail");
            return;
        }
        int result = mAgc.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (CanDebug()) Log.d(TAG, "setAgcMaxLevel = " + step);
            mAgc.setParameter(PARAM_AGC_MAX_LEVEL, step);
            saveAudioParameters(SET_AGC_MAX_LEVEL, step);
        }
    }

    public void setAgcAttrackTime (int step) {
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "setAgcAttrackTime mAgc creat fail");
            return;
        }
        int result = mAgc.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (CanDebug()) Log.d(TAG, "setAgcAttrackTime = " + step);
            mAgc.setParameter(PARAM_AGC_ATTRACK_TIME, step * 48);
            saveAudioParameters(SET_AGC_ATTRACK_TIME, step);
        }
    }

    public void setAgcReleaseTime (int step) {
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "setAgcReleaseTime mAgc creat fail");
            return;
        }
        int result = mAgc.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (CanDebug()) Log.d(TAG, "setAgcReleaseTime = " + step);
            mAgc.setParameter(PARAM_AGC_RELEASE_TIME, step);
            saveAudioParameters(SET_AGC_RELEASE_TIME, step);
        }
    }

    public void setSourceIdForAvl (int step) {
        if (!creatAgcAudioEffects()) {
            Log.e(TAG, "setSourceIdForAvl mAgc creat fail");
            return;
        }
        int result = mAgc.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (CanDebug()) Log.d(TAG, "setSourceIdForAvl = " + step);
            mAgc.setParameter(PARAM_AGC_SOURCE_ID, step);
            saveAudioParameters(SET_AGC_SOURCE_ID, step);
        }
    }

    public void setVirtualSurround (int mode) {
        if (!creatVirtualSurroundAudioEffects()) {
            Log.e(TAG, "setVirtualSurround mVirtualSurround creat fail");
            return;
        }
        int result = mVirtualSurround.setEnabled(true);
        if (result == AudioEffect.SUCCESS) {
            if (CanDebug()) Log.d(TAG, "setVirtualSurround = " + mode);
            mVirtualSurround.setParameter(PARAM_VIRTUALSURROUND, mode);
            saveAudioParameters(SET_VIRTUAL_SURROUND, mode);
        }
    }

    public void setDbxEnable(boolean enable) {
        if (null == mDbx) {
            Log.e(TAG, "The DBX effect is not created, the mode cannot be setDbxEnable.");
            return;
        }
        if (CanDebug()) Log.d(TAG, "setDbxEnable = " + enable);
        int dbSwitch = enable ? 1 : 0;
        mDbx.setParameter(PARAM_DBX_PARAM_ENABLE, dbSwitch);
        saveAudioParameters(SET_DBX_ENABLE, dbSwitch);
    }

    public boolean getDbxEnable() {
        int dbSwitch = getSavedAudioParameters(SET_DBX_ENABLE);
        boolean enable = (1 == dbSwitch);
        if (dbSwitch != 1 && dbSwitch != 0) {
            Log.w(TAG, "DBX enable db value invalid, db:" + dbSwitch + ", return default false");
        }
        if (CanDebug()) Log.d(TAG, "DBX get enable: " + enable);
        return enable;
    }

    public void setDbxSoundMode(int dbxMode) {
        if (null == mDbx) {
            Log.e(TAG, "The DBX effect is not created, the mode cannot be setDbxSoundMode.");
            return;
        }
        if (dbxMode > AudioEffectManager.DBX_SOUND_MODE_ADVANCED || dbxMode < AudioEffectManager.DBX_SOUND_MODE_STANDARD) {
            Log.w(TAG, "DBX sound mode invalid, mode:" + dbxMode + "setDbxSoundMode failed");
            return;
        }
        if (AudioEffectManager.DBX_SOUND_MODE_ADVANCED == dbxMode) {
            byte[] param = new byte[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND + 1];
            param[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS] = (byte) getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SONICS);
            param[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME] = (byte) getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_VOLUME);
            param[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND] = (byte) getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SURROUND);
            Log.i(TAG, "DBX set sound mode:" + dbxMode + ", param: " + Arrays.toString(param));
            mDbx.setParameter(PARAM_DBX_SET_MODE, param);
        } else {
            Log.i(TAG, "DBX set sound mode:" + dbxMode + ", param: " + Arrays.toString(AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[dbxMode]));
            mDbx.setParameter(PARAM_DBX_SET_MODE, AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[dbxMode]);
        }
        saveAudioParameters(SET_DBX_SOUND_MODE, dbxMode);
    }

    public int getDbxSoundMode() {
        int soundMode = 0;
        soundMode = getSavedAudioParameters(SET_DBX_SOUND_MODE);
        if (CanDebug()) Log.d(TAG, "DBX get sound mode: " + soundMode);
        return soundMode;
    }

    public void setDbxAdvancedModeParam(int paramType, int value) {
        if (null == mDbx) {
            Log.e(TAG, "The DBX effect is not created, the mode cannot be setDbxAdvancedModeParam.");
            return;
        }
        switch (paramType) {
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS:
                if (value > 4 || value < 0) {
                    Log.e(TAG, "setDbxAdvancedModeParam invalid sonics value:" + value);
                    return;
                }
                saveAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SONICS, value);
                break;
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME:
                if (value > 2 || value < 0) {
                    Log.e(TAG, "setDbxAdvancedModeParam invalid volume value:" + value);
                    return;
                }
                saveAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_VOLUME, value);
                break;
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND:
                if (value > 2 || value < 0) {
                    Log.e(TAG, "setDbxAdvancedModeParam invalid surround value:" + value);
                    return;
                }
                saveAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SURROUND, value);
                break;
            default:
                Log.e(TAG, "setDbxAdvancedModeParam invalid type:" + paramType);
                return;
        }
        byte[] param = new byte[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND + 1];
        param[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS] = (byte) getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SONICS);
        param[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME] = (byte) getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_VOLUME);
        param[AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND] = (byte) getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SURROUND);
        if (CanDebug()) Log.d(TAG, "DBX set advanced sound mode param: " + Arrays.toString(param));
        mDbx.setParameter(PARAM_DBX_SET_MODE, param);
    }

    public int getDbxAdvancedModeParam(int paramType) {
        int value = 0;
        switch (paramType) {
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS:
                value = getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SONICS);
                break;
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME:
                value = getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_VOLUME);
                break;
            case AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND:
                value = getSavedAudioParameters(SET_DBX_SOUND_MODE_ADVANCED_SURROUND);
                break;
            default:
                Log.e(TAG, "getDbxAdvancedModeParam invalid type:" + paramType + ", return 0");
                break;
        }
        if (CanDebug()) Log.d(TAG, "getDbxAdvancedModeParam paramType:" + paramType+ ", value:" + value);
        return value;
    }

    public void setAudioOutputSpeakerDelay(int source, int delayMs) {
        if (source < AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "setAudioOutputSpeakerDelay: unsupport delay source:" + source + ", min:" + AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return;
        }
        if (delayMs < AUDIO_OUTPUT_SPEAKER_DELAY_MIN || delayMs > AUDIO_OUTPUT_SPEAKER_DELAY_MAX) {
            Log.w(TAG, "unsupport speaker delay time:" + delayMs + "ms, min:" + AUDIO_OUTPUT_SPEAKER_DELAY_MIN + "ms, max:"
                    + AUDIO_OUTPUT_SPEAKER_DELAY_MAX + "ms, now use max value");
            delayMs = AUDIO_OUTPUT_SPEAKER_DELAY_MAX;
        }
        if (CanDebug()) Log.d(TAG, "setAudioOutputSpeakerDelay delay source:" + source + ", delayMs:" + delayMs);
        int currentTvSource = DroidLogicTvUtils.getTvSourceType(mContext);
        if (currentTvSource == source) {
            mAudioManager.setParameters(PARAM_HAL_AUDIO_OUTPUT_SPEAKER_DELAY + "=" + delayMs);
        } else {
            Log.i(TAG, "setAudioOutputSpeakerDelay current source:" + currentTvSource + " is not the same as the set source:" + source + ", only save to DB");
        }
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_SPEAKER_DELAY_ARRAY[source], delayMs);
    }

    public int getAudioOutputSpeakerDelay(int source) {
        if (source < AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "getAudioOutputSpeakerDelay unsupport delay source:" + source + ", min:" + AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return -1;
        }
        int delayMs = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_SPEAKER_DELAY_ARRAY[source], AUDIO_OUTPUT_SPEAKER_DELAY_DEFAULT);
        if (CanDebug()) Log.d(TAG, "getAudioOutputSpeakerDelay source:" + source + ", delayMs:" + delayMs);
        return delayMs;
    }

    public void setAudioOutputSpdifDelay(int source, int delayMs) {
        if (source < AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "setAudioOutputSpdifDelay unsupport delay source:" + source + ", min:" + AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return;
        }
        if (delayMs < AUDIO_OUTPUT_SPDIF_DELAY_MIN || delayMs > AUDIO_OUTPUT_SPDIF_DELAY_MAX) {
            Log.w(TAG, "unsupport spdif delay time:" + delayMs + "ms, min:" + AUDIO_OUTPUT_SPDIF_DELAY_MIN + "ms, max:"
                    + AUDIO_OUTPUT_SPDIF_DELAY_MAX + "ms, now use max value");
            delayMs = AUDIO_OUTPUT_SPDIF_DELAY_MAX;
        }
        if (CanDebug()) Log.d(TAG, "setAudioOutputSpdifDelay delay source:" + source + ", delayMs:" + delayMs);
        int currentTvSource = DroidLogicTvUtils.getTvSourceType(mContext);
        if (currentTvSource == source) {
            mAudioManager.setParameters(PARAM_HAL_AUDIO_OUTPUT_SPDIF_DELAY + "=" + delayMs);
        } else {
            Log.i(TAG, "setAudioOutputSpdifDelay current source:" + currentTvSource + " is not the same as the set source:" + source + ", only save to DB");
        }
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_SPDIF_DELAY_ARRAY[source], delayMs);
    }

    public int getAudioOutputSpdifDelay(int source) {
        if (source < AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "getAudioOutputSpdifDelay unsupport delay source:" + source + ", min:" + AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return -1;
        }
        int delayMs = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_OUTPUT_SPDIF_DELAY_ARRAY[source], AUDIO_OUTPUT_SPPDIF_DELAY_DEFAULT);
        if (CanDebug()) Log.d(TAG, "getAudioOutputSpdifDelay source:" + source + ", delayMs:" + delayMs);
        return delayMs;
    }

    public void setAudioPrescale(int source,int value) {
        if (source < AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "setAudioPrescale unsupport delay source:" + source + ", min:" + AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return;
        }
        if (value < AUDIO_PRESCALE_MIN || value > AUDIO_PRESCALE_MAX) {
            Log.w(TAG, "unsupport audio prescale:" + value + ", min:" + AUDIO_PRESCALE_MIN + ", max:" + AUDIO_PRESCALE_MAX);
            return;
        }
        if (CanDebug()) Log.d(TAG, "setAudioPrescale source:" + source + ", value:" + value);
        try {
            StringBuffer parameter;
            String realValue = "";
            DecimalFormat decimalFormat = new DecimalFormat("0.0");
            Settings.Global.putInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[source], value);

            // packgeing "SOURCE_GAIN=1.0 1.0 1.0 1.0 1.0" [atv,dtv,hdmi,av,media]
            parameter = new StringBuffer(PARAM_HAL_AUDIO_PRESCALE + "=");
            //UI -150 - 150, audio_hal -15 - 15 db
            int tempParamter = 1;
            tempParamter = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV],
                    AUDIO_PRESCALE_DEFAULT_ARRAY[AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV]);
            realValue = decimalFormat.format((float) tempParamter / 10);
            parameter.append(realValue + " ");

            tempParamter = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_DTV],
                    AUDIO_PRESCALE_DEFAULT_ARRAY[AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_DTV]);
            realValue = decimalFormat.format((float) tempParamter / 10);
            parameter.append(realValue + " ");

            tempParamter = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_HDMI],
                    AUDIO_PRESCALE_DEFAULT_ARRAY[AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_HDMI]);
            realValue = decimalFormat.format((float) tempParamter / 10);
            parameter.append(realValue + " ");

            tempParamter = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_AV],
                    AUDIO_PRESCALE_DEFAULT_ARRAY[AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_AV]);
            realValue = decimalFormat.format((float) tempParamter / 10);
            parameter.append(realValue + " ");

            tempParamter = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MEDIA],
                    AUDIO_PRESCALE_DEFAULT_ARRAY[AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MEDIA]);
            realValue = decimalFormat.format((float) tempParamter / 10);
                      parameter.append(realValue + " ");
            if (CanDebug()) Log.d(TAG, "setAudioPrescale setParameters:" + parameter.toString());
            mAudioManager.setParameters(parameter.toString());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public int getAudioPrescale(int source) {
        if (source < AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV || source >= AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MAX) {
            Log.w(TAG, "getAudioPrescaleStatus: unsupport delay source:" + source + ", min:" + AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV
                    + ", max:" + (AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MAX-1));
            return -1;
        }

        int saveResult = 0;
        BigDecimal mBigDecimalBase = new BigDecimal(Float.toString(10.00f));
        BigDecimal mBigDecimal = new BigDecimal(0.0f);
        String value = mAudioManager.getParameters("SOURCE_GAIN");//atv,dtv,hdmi,av,media
        value.trim();
        if (CanDebug()) Log.d(TAG, "getAudioPrescale hal param:" + value);
        saveResult = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_AUDIO_PRESCALE_ARRAY[source],
                AUDIO_PRESCALE_DEFAULT_ARRAY[AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV]);
        String[] subStrings = value.split(" ");//"source_gain = 1.0 1.0 1.0 1.0 1.0"
        if (subStrings.length == 7) {
            int driverValue = 0;
            mBigDecimal = new BigDecimal(subStrings[subStrings.length - 5].substring(0,3));
            driverValue = (int) mBigDecimal.multiply(mBigDecimalBase).floatValue();
            if (driverValue != saveResult) {
                Log.w(TAG, "getAudioPrescaleStatus driverValue:" + driverValue + ", saveResult:" + saveResult);
            }
        } else {
            Log.w(TAG, "getAudioPrescaleStatus param length:" + subStrings.length + " invalid");
        }
        if (CanDebug()) Log.d(TAG, "getAudioPrescale source:" + source + ", value:" + saveResult);
        return saveResult;
    }

    private void saveAudioParameters(int id, int value) {
        if (CanDebug()) Log.d(TAG, "saveAudioParameters id:" + id+ ", value:" + value);
        switch (id) {
            case SET_BASS:
                int soundModeBass = getSoundModeFromDb();
                if (AudioEffectManager.EQ_SOUND_MODE_CUSTOM == soundModeBass) {
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BASS, value);
                }
                break;
            case SET_TREBLE:
                int soundModeTreble = getSoundModeFromDb();
                if (AudioEffectManager.EQ_SOUND_MODE_CUSTOM == soundModeTreble) {
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREBLE, value);
                }
                break;
            case SET_BALANCE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BALANCE, value);
                break;
            case SET_DIALOG_CLARITY_MODE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DIALOG_CLARITY, value);
                break;
            case SET_SURROUND_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SURROUND, value);
                break;
            case SET_TRUBASS_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TRUBASS, value);
                break;
            case SET_SOUND_MODE:
                String soundmodetype = Settings.Global.getString(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE);
                if (soundmodetype == null || DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ.equals(soundmodetype)) {
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE, value);
                } else if ((DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_DAP.equals(soundmodetype))) {
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE, value);
                } else {
                    Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE, value);
                }
                break;
            case SET_EFFECT_BAND1:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND1, value);
                break;
            case SET_EFFECT_BAND2:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND2, value);
                break;
            case SET_EFFECT_BAND3:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND3, value);
                break;
            case SET_EFFECT_BAND4:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND4, value);
                break;
            case SET_EFFECT_BAND5:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND5, value);
                break;
            case SET_AGC_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_ENABLE, value);
                break;
            case SET_AGC_MAX_LEVEL:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_MAX_LEVEL, value);
                break;
            case SET_AGC_ATTRACK_TIME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_ATTRACK_TIME, value);
                break;
            case SET_AGC_RELEASE_TIME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_RELEASE_TIME, value);
                break;
            case SET_AGC_SOURCE_ID:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_SOURCE_ID, value);
                break;
            case SET_VIRTUAL_SURROUND:
                Settings.Global.putInt(mContext.getContentResolver(), OutputModeManager.VIRTUAL_SURROUND, value);
                break;
            case SET_VIRTUALX_MODE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_VIRTUALX_MODE, value);
                break;
            case SET_TRUVOLUME_HD_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREVOLUME_HD, value);
                break;
            case SET_DBX_ENABLE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ENABLE, value);
                break;
            case SET_DBX_SOUND_MODE:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_SOUND_MODE, value);
                break;
            case SET_DBX_SOUND_MODE_ADVANCED_SONICS:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SONICS, value);
                break;
            case SET_DBX_SOUND_MODE_ADVANCED_VOLUME:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_VOLUME, value);
                break;
            case SET_DBX_SOUND_MODE_ADVANCED_SURROUND:
                Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SURROUND, value);
                break;
            default:
                break;
        }
    }

    private int getSoundModeFromDb() {
        String soundmodetype = Settings.Global.getString(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE);
        if (soundmodetype == null || DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ.equals(soundmodetype)) {
            return Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        } else if ((DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_DAP.equals(soundmodetype))) {
            return Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        } else {
            return Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        }
    }

    private int getSavedAudioParameters(int id) {
        if (CanDebug()) Log.d(TAG, "getSavedAudioParameters id:" + id);
        int result = -1;
        switch (id) {
            case SET_BASS:
                int soundModeBass = getSoundModeFromDb();
                if (AudioEffectManager.EQ_SOUND_MODE_CUSTOM == soundModeBass) {
                    result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BASS, AudioEffectManager.EFFECT_BASS_DEFAULT);
                } else {
                    result = AudioEffectManager.EFFECT_BASS_DEFAULT;
                }
                break;
            case SET_TREBLE:
                int soundModeTreble = getSoundModeFromDb();
                if (AudioEffectManager.EQ_SOUND_MODE_CUSTOM == soundModeTreble) {
                    result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREBLE, AudioEffectManager.EFFECT_TREBLE_DEFAULT);
                } else {
                    result = AudioEffectManager.EFFECT_TREBLE_DEFAULT;
                }
                break;
            case SET_BALANCE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BALANCE, AudioEffectManager.EFFECT_BALANCE_DEFAULT);
                break;
            case SET_DIALOG_CLARITY_MODE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DIALOG_CLARITY, AudioEffectManager.SOUND_EFFECT_DIALOG_CLARITY_ENABLE_DEFAULT);
                break;
            case SET_SURROUND_ENABLE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SURROUND, AudioEffectManager.SOUND_EFFECT_SURROUND_ENABLE_DEFAULT);
                break;
            case SET_TRUBASS_ENABLE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TRUBASS, AudioEffectManager.SOUND_EFFECT_TRUBASS_ENABLE_DEFAULT);
                break;
            case SET_SOUND_MODE:
                result = getSoundModeFromDb();
                Log.d(TAG, "getSavedAudioParameters SET_SOUND_MODE = " + result);
                break;
            case SET_EFFECT_BAND1:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND1, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1]);
                break;
            case SET_EFFECT_BAND2:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND2, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND2]);
                break;
            case SET_EFFECT_BAND3:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND3, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND3]);
                break;
            case SET_EFFECT_BAND4:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND4, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND4]);
                break;
            case SET_EFFECT_BAND5:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND5, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5]);
                break;
            case SET_AGC_ENABLE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_ENABLE, DEFAULT_AGC_ENABLE ? 1 : 0);
                break;
            case SET_AGC_MAX_LEVEL:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_MAX_LEVEL, DEFAULT_AGC_MAX_LEVEL);
                break;
            case SET_AGC_ATTRACK_TIME:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_ATTRACK_TIME, DEFAULT_AGC_ATTRACK_TIME);
                break;
            case SET_AGC_RELEASE_TIME:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_RELEASE_TIME, DEFAULT_AGC_RELEASE_TIME);
                break;
            case SET_AGC_SOURCE_ID:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_SOURCE_ID, DEFAULT_AGC_SOURCE_ID);
                break;
            case SET_VIRTUAL_SURROUND:
                result = Settings.Global.getInt(mContext.getContentResolver(), OutputModeManager.VIRTUAL_SURROUND, OutputModeManager.VIRTUAL_SURROUND_OFF);
                break;
            case SET_VIRTUALX_MODE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_VIRTUALX_MODE, AudioEffectManager.SOUND_EFFECT_VIRTUALX_MODE_DEFAULT);
                break;
            case SET_TRUVOLUME_HD_ENABLE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREVOLUME_HD, AudioEffectManager.SOUND_EFFECT_TRUVOLUME_HD_ENABLE_DEFAULT);
                break;
            case SET_DBX_ENABLE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ENABLE, AudioEffectManager.SOUND_EFFECT_DBX_ENABLE_DEFAULT);
                break;
            case SET_DBX_SOUND_MODE:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_SOUND_MODE, AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_DEFAULT);
                break;
            case SET_DBX_SOUND_MODE_ADVANCED_SONICS:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SONICS,
                        AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[AudioEffectManager.DBX_SOUND_MODE_ADVANCED][AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS]);
                break;
            case SET_DBX_SOUND_MODE_ADVANCED_VOLUME:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_VOLUME,
                        AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[AudioEffectManager.DBX_SOUND_MODE_ADVANCED][AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME]);
                break;
            case SET_DBX_SOUND_MODE_ADVANCED_SURROUND:
                result = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SURROUND,
                        AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[AudioEffectManager.DBX_SOUND_MODE_ADVANCED][AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND]);
                break;
            default:
                break;
        }
        return result;
    }

    public void cleanupAudioEffects() {
        if (mBalance!= null) {
            mBalance.setEnabled(false);
            mBalance.release();
            mBalance = null;
        }
        if (mTruSurround!= null) {
            mTruSurround.setEnabled(false);
            mTruSurround.release();
            mTruSurround = null;
        }
        if (mTrebleBass!= null) {
            mTrebleBass.setEnabled(false);
            mTrebleBass.release();
            mTrebleBass = null;
        }
        if (mSoundMode!= null) {
            mSoundMode.setEnabled(false);
            mSoundMode.release();
            mSoundMode = null;
        }
        if (mAgc!= null) {
            mAgc.setEnabled(false);
            mAgc.release();
            mAgc = null;
        }
        if (mVirtualSurround != null) {
            mVirtualSurround.setEnabled(false);
            mVirtualSurround.release();
            mVirtualSurround = null;
        }
        if (mVirtualX!= null) {
            mVirtualX.setEnabled(false);
            mVirtualX.release();
            mVirtualX = null;
        }
        if (mDbx!= null) {
            mDbx.setEnabled(false);
            mDbx.release();
            mDbx = null;
        }
    }

    public void initSoundEffectSettings() {
        if (Settings.Global.getInt(mContext.getContentResolver(), "set_five_band", 0) == 0) {
            if (mSoundMode != null) {
                byte[] fiveBandNum = new byte[5];
                mSoundMode.getParameter(PARAM_EQ_CUSTOM, fiveBandNum);
                for (int i = SET_EFFECT_BAND1; i <= SET_EFFECT_BAND5; i++) {
                    saveAudioParameters(i, unMappingLine(fiveBandNum[i - SET_EFFECT_BAND1], true));
                }
            } else {
                for (int i = SET_EFFECT_BAND1; i <= SET_EFFECT_BAND5; i++) {
                    saveAudioParameters(i, EFFECT_SOUND_MODE_USER_BAND[i - SET_EFFECT_BAND1]);
                }
                Log.w(TAG, "get default band value fail, set default value, mSoundMode == null");
            }
            Settings.Global.putInt(mContext.getContentResolver(), "set_five_band", 1);
        }

        int soundMode = getSavedAudioParameters(SET_SOUND_MODE);
        setSoundModeByObserver(soundMode);
        setBass(getSavedAudioParameters(SET_BASS));
        setTreble(getSavedAudioParameters(SET_TREBLE));
        setAgcEnable(getSavedAudioParameters(SET_AGC_ENABLE) != 0);
        setAgcMaxLevel(getSavedAudioParameters(SET_AGC_MAX_LEVEL));
        setAgcAttrackTime (getSavedAudioParameters(SET_AGC_ATTRACK_TIME));
        setAgcReleaseTime(getSavedAudioParameters(SET_AGC_RELEASE_TIME));
        setSourceIdForAvl(getSavedAudioParameters(SET_AGC_SOURCE_ID));
        setVirtualSurround(getSavedAudioParameters(SET_VIRTUAL_SURROUND));

        OutputModeManager opm = new OutputModeManager(mContext);
        int audioOutPutLatency = Settings.Global.getInt(mContext.getContentResolver(), OutputModeManager.DB_FIELD_AUDIO_OUTPUT_LATENCY,
                OutputModeManager.AUDIO_OUTPUT_LATENCY_DEFAULT);
        opm.setAudioOutputLatency(audioOutPutLatency);
        //init sound parameter at the same time
        /*SoundParameterSettingManager soundparameter = new SoundParameterSettingManager(mContext);
        if (soundparameter != null) {
            soundparameter.initParameterAfterBoot();
        }
        soundparameter = null;*/
        applyAudioEffectByPlayEmptyTrack();

        if (isSupportVirtualX()) {
            setDtsVirtualXMode(getDtsVirtualXMode());
            setDtsTruVolumeHdEnable(getDtsTruVolumeHdEnable());
        } else {
            setSurroundEnable(getSurroundEnable());
            setDialogClarityMode(getDialogClarityMode());
            setTruBassEnable(getTruBassEnable());
        }

        boolean dbxStatus = getDbxEnable();
        if (dbxStatus) {
            int dbxMode = getSavedAudioParameters(SET_DBX_SOUND_MODE);
            setDbxEnable(dbxStatus);
            setDbxSoundMode(dbxMode);
        }

        // refresh db delay of media to hal
        setAudioOutputSpeakerDelay(AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MEDIA, getAudioOutputSpeakerDelay(AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MEDIA));
        setAudioOutputSpdifDelay(AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MEDIA, getAudioOutputSpdifDelay(AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MEDIA));
        // refresh db prescale of all source to hal (set one prescale, at the same time the others will be set)
        setAudioPrescale(AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV, getAudioPrescale(AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV));
    }

    public void resetSoundEffectSettings() {
        Log.d(TAG, "resetSoundEffectSettings");
        cleanupAudioEffects();
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BASS, AudioEffectManager.EFFECT_BASS_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREBLE, AudioEffectManager.EFFECT_TREBLE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BALANCE, AudioEffectManager.EFFECT_BALANCE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DIALOG_CLARITY, AudioEffectManager.SOUND_EFFECT_DIALOG_CLARITY_ENABLE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SURROUND, AudioEffectManager.SOUND_EFFECT_SURROUND_ENABLE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TRUBASS, AudioEffectManager.SOUND_EFFECT_TRUBASS_ENABLE_DEFAULT);
        Settings.Global.putString(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE, DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE, AudioEffectManager.EQ_SOUND_MODE_STANDARD);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND1, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND1]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND2, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND2]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND3, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND3]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND4, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND4]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_BAND5, EFFECT_SOUND_MODE_USER_BAND[AudioEffectManager.EQ_SOUND_MODE_EFFECT_BAND5]);
        Settings.Global.putInt(mContext.getContentResolver(), "set_five_band", 0);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_ENABLE, DEFAULT_AGC_ENABLE ? 1 : 0);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_MAX_LEVEL, DEFAULT_AGC_MAX_LEVEL);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_ATTRACK_TIME, DEFAULT_AGC_ATTRACK_TIME);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_RELEASE_TIME, DEFAULT_AGC_RELEASE_TIME);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_AGC_SOURCE_ID, DEFAULT_AGC_SOURCE_ID);
        Settings.Global.putInt(mContext.getContentResolver(), OutputModeManager.VIRTUAL_SURROUND, OutputModeManager.VIRTUAL_SURROUND_OFF);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_VIRTUALX_MODE, AudioEffectManager.SOUND_EFFECT_VIRTUALX_MODE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_TREVOLUME_HD, AudioEffectManager.SOUND_EFFECT_TRUVOLUME_HD_ENABLE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ENABLE, AudioEffectManager.SOUND_EFFECT_DBX_ENABLE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_SOUND_MODE, AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_DEFAULT);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SONICS,
                AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[AudioEffectManager.DBX_SOUND_MODE_ADVANCED][AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SONICS]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_VOLUME,
                AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[AudioEffectManager.DBX_SOUND_MODE_ADVANCED][AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_VOLUME]);
        Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_DBX_ADVANCED_MODE_SURROUND,
                AudioEffectManager.SOUND_EFFECT_DBX_SOUND_MODE_ARRAY_DEFAULT[AudioEffectManager.DBX_SOUND_MODE_ADVANCED][AudioEffectManager.DBX_ADVANCED_MODE_PRARM_TYPE_SURROUND]);
        initSoundEffectSettings();
    }

    private void applyAudioEffectByPlayEmptyTrack() {
        int bufsize = AudioTrack.getMinBufferSize(8000, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
        byte data[] = new byte[bufsize];
        AudioTrack trackplayer = new AudioTrack(AudioManager.STREAM_MUSIC, 8000, AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT, bufsize, AudioTrack.MODE_STREAM);
        trackplayer.play();
        trackplayer.write(data, 0, data.length);
        trackplayer.stop();
        trackplayer.release();
    }
}

