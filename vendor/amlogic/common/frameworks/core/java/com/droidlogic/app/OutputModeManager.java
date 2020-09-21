/*
 * Copyright (C) 2009 The Android Open Source Project
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
 * limitations under the License.
 */

package com.droidlogic.app;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.Method;
import java.lang.reflect.Constructor;
import java.util.UUID;

import android.content.Context;
import android.content.Intent;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiHotplugEvent;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;
import android.media.AudioManager;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.media.audiofx.AudioEffect;
import android.content.ContentResolver;

public class OutputModeManager {
    private static final String TAG                         = "OutputModeManager";
    private static final boolean DEBUG                      = false;

    /**
     * The saved value for Outputmode auto-detection.
     * One integer
     * @hide
     */
    public static final String DISPLAY_OUTPUTMODE_AUTO      = "display_outputmode_auto";

    /**
     *  broadcast of the current HDMI output mode changed.
     */
    public static final String ACTION_HDMI_MODE_CHANGED     = "droidlogic.intent.action.HDMI_MODE_CHANGED";

    /**
     * Extra in {@link #ACTION_HDMI_MODE_CHANGED} indicating the mode:
     */
    public static final String EXTRA_HDMI_MODE              = "mode";

    public static final String SYS_DIGITAL_RAW              = "/sys/class/audiodsp/digital_raw";
    public static final String SYS_AUDIO_CAP                = "/sys/class/amhdmitx/amhdmitx0/aud_cap";
    public static final String SYS_AUIDO_HDMI               = "/sys/class/amhdmitx/amhdmitx0/config";
    public static final String SYS_AUIDO_SPDIF              = "/sys/devices/platform/spdif-dit.0/spdif_mute";

    public static final String AUIDO_DSP_AC3_DRC            = "/sys/class/audiodsp/ac3_drc_control";
    public static final String AUIDO_DSP_DTS_DEC            = "/sys/class/audiodsp/dts_dec_control";

    public static final String HDMI_STATE                   = "/sys/class/amhdmitx/amhdmitx0/hpd_state";
    public static final String HDMI_SUPPORT_LIST            = "/sys/class/amhdmitx/amhdmitx0/disp_cap";
    public static final String HDMI_COLOR_SUPPORT_LIST      = "/sys/class/amhdmitx/amhdmitx0/dc_cap";

    public static final String COLOR_ATTRIBUTE              = "/sys/class/amhdmitx/amhdmitx0/attr";
    public static final String DISPLAY_HDMI_VALID_MODE      = "/sys/class/amhdmitx/amhdmitx0/valid_mode";//test if tv support this mode

    public static final String DISPLAY_MODE                 = "/sys/class/display/mode";
    public static final String DISPLAY_AXIS                 = "/sys/class/display/axis";

    public static final String VIDEO_AXIS                   = "/sys/class/video/axis";

    public static final String FB0_FREE_SCALE_AXIS          = "/sys/class/graphics/fb0/free_scale_axis";
    public static final String FB0_FREE_SCALE_MODE          = "/sys/class/graphics/fb0/freescale_mode";
    public static final String FB0_FREE_SCALE               = "/sys/class/graphics/fb0/free_scale";
    public static final String FB1_FREE_SCALE               = "/sys/class/graphics/fb1/free_scale";

    public static final String FB0_WINDOW_AXIS              = "/sys/class/graphics/fb0/window_axis";
    public static final String FB0_BLANK                    = "/sys/class/graphics/fb0/blank";

    public  static final String AUDIO_MS12LIB_PATH          = "/vendor/lib/libdolbyms12.so";

    public static final String ENV_CVBS_MODE                = "ubootenv.var.cvbsmode";
    public static final String ENV_HDMI_MODE                = "ubootenv.var.hdmimode";
    public static final String ENV_OUTPUT_MODE              = "ubootenv.var.outputmode";
    public static final String ENV_DIGIT_AUDIO              = "ubootenv.var.digitaudiooutput";
    public static final String ENV_IS_BEST_MODE             = "ubootenv.var.is.bestmode";
    public static final String ENV_IS_BEST_DOLBYVISION      = "ubootenv.var.bestdolbyvision";
    public static final String ENV_COLORATTRIBUTE           = "ubootenv.var.colorattribute";
    public static final String ENV_HDR_POLICY               = "ubootenv.var.hdr_policy";
    public static final String ENV_HDR_PRIORITY             = "ubootenv.var.hdr_priority";

    public static final String PROP_BEST_OUTPUT_MODE        = "ro.vendor.platform.best_outputmode";
    public static final String PROP_HDMI_ONLY               = "ro.vendor.platform.hdmionly";
    public static final String PROP_SUPPORT_4K              = "ro.vendor.platform.support.4k";
    public static final String PROP_SUPPORT_OVER_4K30       = "ro.vendor.platform.support.over.4k30";
    public static final String PROP_DEEPCOLOR               = "vendor.sys.open.deepcolor";
    public static final String PROP_DTSDRCSCALE             = "persist.vendor.sys.dtsdrcscale";
    public static final String PROP_DTSEDID                 = "persist.vendor.sys.dts.edid";
    public static final String PROP_HDMI_FRAMERATE_PRIORITY = "persist.vendor.sys.framerate.priority";

    public static final String FULL_WIDTH_480               = "720";
    public static final String FULL_HEIGHT_480              = "480";
    public static final String FULL_WIDTH_576               = "720";
    public static final String FULL_HEIGHT_576              = "576";
    public static final String FULL_WIDTH_720               = "1280";
    public static final String FULL_HEIGHT_720              = "720";
    public static final String FULL_WIDTH_1080              = "1920";
    public static final String FULL_HEIGHT_1080             = "1080";
    public static final String FULL_WIDTH_4K2K              = "3840";
    public static final String FULL_HEIGHT_4K2K             = "2160";
    public static final String FULL_WIDTH_4K2KSMPTE         = "4096";
    public static final String FULL_HEIGHT_4K2KSMPTE        = "2160";

    public static final String AUDIO_MIXING                 = "audio_mixing";
    private static final String PARA_AUDIO_MIXING_ON        = "disable_pcm_mixing=0";
    private static final String PARA_AUDIO_MIXING_OFF       = "disable_pcm_mixing=1";
    public static final int AUDIO_MIXING_OFF                = 0;
    public static final int AUDIO_MIXING_ON                 = 1;
    public static final int AUDIO_MIXING_DEFAULT            = AUDIO_MIXING_ON;

    private static final UUID EFFECT_TYPE_DAP               = UUID.fromString("3337b21d-c8e6-4bbd-8f24-698ade8491b9");
    private static final UUID EFFECT_TYPE_NULL              = UUID.fromString("ec7178ec-e5e1-4432-a3f4-4657e6795210");
    /**
    * dap AudioEffect, must sync with DAPparams in libms12dap.so
    */
    public static final int CMD_DAP_ENABLE                  = 0;
    public static final int CMD_DAP_EFFECT_MODE             = 1;
    public static final int CMD_DAP_GEQ_GAINS               = 2;
    public static final int CMD_DAP_GEQ_ENABLE              = 3;
    public static final int CMD_DAP_POST_GAIN               = 4;
    public static final int CMD_DAP_VL_ENABLE               = 5;
    public static final int CMD_DAP_VL_AMOUNT               = 6;
    public static final int CMD_DAP_DE_ENABLE               = 7;
    public static final int CMD_DAP_DE_AMOUNT               = 8;
    public static final int CMD_DAP_SURROUND_ENABLE         = 9;
    public static final int CMD_DAP_SURROUND_BOOST          = 10;
    public static final int CMD_DAP_VIRTUALIZER_ENABLE      = 11;
    private static final int DAP_CPDP_OUTPUT_2_SPEAKER      = 7;
    private static final int DAP_CPDP_OUTPUT_2_HEADPHONE    = 6;

    public static final int SUBCMD_DAP_GEQ_BAND1            = 0x100;
    public static final int SUBCMD_DAP_GEQ_BAND2            = 0x101;
    public static final int SUBCMD_DAP_GEQ_BAND3            = 0x102;
    public static final int SUBCMD_DAP_GEQ_BAND4            = 0x103;
    public static final int SUBCMD_DAP_GEQ_BAND5            = 0x104;

    public static final String PARAM_DAP_SAVED              = "dap_saved";
    public static final String PARAM_DAP_MODE               = "dap_mode";
    public static final String PARAM_DAP_GEQ_ENABLE         = "dap_geq";
    public static final String PARAM_DAP_POST_GAIN          = "dap_post_gain";
    public static final String PARAM_DAP_VL_ENABLE          = "dap_vl";
    public static final String PARAM_DAP_VL_AMOUNT          = "dap_vl_amount";
    public static final String PARAM_DAP_DE_ENABLE          = "dap_de";
    public static final String PARAM_DAP_DE_AMOUNT          = "dap_de_amount";
    public static final String PARAM_DAP_SURROUND_ENABLE    = "dap_surround";
    public static final String PARAM_DAP_SURROUND_BOOST     = "dap_surround_boost";
    public static final String PARAM_DAP_GEQ_BAND1          = "dap_geq_band1";
    public static final String PARAM_DAP_GEQ_BAND2          = "dap_geq_band2";
    public static final String PARAM_DAP_GEQ_BAND3          = "dap_geq_band3";
    public static final String PARAM_DAP_GEQ_BAND4          = "dap_geq_band4";
    public static final String PARAM_DAP_GEQ_BAND5          = "dap_geq_band5";

    public static final int DAP_MODE_OFF                    = 0;
    public static final int DAP_MODE_MOVIE                  = 1;
    public static final int DAP_MODE_MUSIC                  = 2;
    public static final int DAP_MODE_NIGHT                  = 3;
    public static final int DAP_MODE_USER                   = 4;
    public static final int DAP_MODE_DEFAULT                = DAP_MODE_MUSIC;
    public static final int DAP_SURROUND_SPEAKER            = 0;
    public static final int DAP_SURROUND_HEADPHONE          = 1;
    public static final int DAP_GEQ_OFF                     = 0;
    public static final int DAP_GEQ_INIT                    = 1;
    public static final int DAP_GEQ_OPEN                    = 2;
    public static final int DAP_GEQ_RICH                    = 3;
    public static final int DAP_GEQ_FOCUSED                 = 4;
    public static final int DAP_GEQ_USER                    = 5;
    public static final int DAP_GEQ_DEFAULT                 = DAP_GEQ_INIT;
    public static final int DAP_OFF                         = 0;
    public static final int DAP_ON                          = 1;
    public static final int DAP_VL_DEFAULT                  = DAP_ON;
    public static final int DAP_VL_AMOUNT_DEFAULT           = 0;
    public static final int DAP_DE_DEFAULT                  = DAP_OFF;
    public static final int DAP_DE_AMOUNT_DEFAULT           = 0;
    public static final int DAP_SURROUND_DEFAULT            = DAP_SURROUND_SPEAKER;
    public static final int DAP_SURROUND_BOOST_DEFAULT      = 0;
    public static final int DAP_POST_GAIN_DEFAULT           = 0;
    public static final int DAP_GEQ_GAIN_DEFAULT            = 0;

    public static final String DIGITAL_AUDIO_FORMAT         = "digital_audio_format";
    public static final String DIGITAL_AUDIO_SUBFORMAT      = "digital_audio_subformat";
    public static final String PARA_PCM                     = "hdmi_format=0";
    public static final String PARA_SPDIF                   = "hdmi_format=4";
    public static final String PARA_AUTO                    = "hdmi_format=5";
    public static final int DIGITAL_PCM                     = 0;
    public static final int DIGITAL_SPDIF                   = 1;
    public static final int DIGITAL_AUTO                    = 2;
    public static final int DIGITAL_MANUAL                  = 3;
    // DD/DD+/DTS
    public static final String DIGITAL_AUDIO_SUBFORMAT_SPDIF            = "5,6,7";

    private static final String NRDP_EXTERNAL_SURROUND                  = "nrdp_external_surround_sound_enabled";
    private static final int NRDP_ENABLE                                = 1;
    private static final int NRDP_DISABLE                               = 0;

    public static final String BOX_LINE_OUT                             = "box_line_out";
    public static final String PARA_BOX_LINE_OUT_OFF                    = "enable_line_out=false";
    public static final String PARA_BOX_LINE_OUT_ON                     = "enable_line_out=true";
    public static final int BOX_LINE_OUT_OFF                            = 0;
    public static final int BOX_LINE_OUT_ON                             = 1;

    public static final String BOX_HDMI                                 = "box_hdmi";
    public static final String PARA_BOX_HDMI_OFF                        = "Audio hdmi-out mute=1";
    public static final String PARA_BOX_HDMI_ON                         = "Audio hdmi-out mute=0";
    public static final int BOX_HDMI_OFF                                = 0;
    public static final int BOX_HDMI_ON                                 = 1;

    public static final String TV_SPEAKER                               = "tv_speaker";
    public static final String PARA_TV_SPEAKER_OFF                      = "speaker_mute=1";
    public static final String PARA_TV_SPEAKER_ON                       = "speaker_mute=0";
    public static final int TV_SPEAKER_OFF                              = 0;
    public static final int TV_SPEAKER_ON                               = 1;

    public static final String TV_ARC                                   = "tv_arc";
    public static final String PARA_TV_ARC_OFF                          = "HDMI ARC Switch=0";
    public static final String PARA_TV_ARC_ON                           = "HDMI ARC Switch=1";
    public static final int TV_ARC_OFF                                  = 0;
    public static final int TV_ARC_ON                                   = 1;
    public static final String TV_ARC_LATENCY                           = "tv_arc_latency";
    public static final String PROPERTY_LOCAL_ARC_LATENCY               = "media.amnuplayer.audio.delayus";
    public static final int TV_ARC_LATENCY_MIN                          = -200;
    public static final int TV_ARC_LATENCY_MAX                          = 200;
    public static final int TV_ARC_LATENCY_DEFAULT                      = -40;

    public static final String DB_FIELD_AUDIO_OUTPUT_LATENCY            = "audio_output_latency";
    public static final String HAL_FIELD_AUDIO_OUTPUT_LATENCY           = "delay_time=";
    public static final int AUDIO_OUTPUT_LATENCY_MIN                    = 0;
    public static final int AUDIO_OUTPUT_LATENCY_MAX                    = 200;
    public static final int AUDIO_OUTPUT_LATENCY_DEFAULT                = 0;

    public static final String VIRTUAL_SURROUND                         = "virtual_surround";
    public static final String PARA_VIRTUAL_SURROUND_OFF                = "enable_virtual_surround=false";
    public static final String PARA_VIRTUAL_SURROUND_ON                 = "enable_virtual_surround=true";
    public static final int VIRTUAL_SURROUND_OFF                        = 0;
    public static final int VIRTUAL_SURROUND_ON                         = 1;

    //surround sound formats, must sync with Settings.Global
    public static final String ENCODED_SURROUND_OUTPUT                  = "encoded_surround_output";
    public static final String ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS  = "encoded_surround_output_enabled_formats";
    public static final int ENCODED_SURROUND_OUTPUT_AUTO                = 0;
    public static final int ENCODED_SURROUND_OUTPUT_NEVER               = 1;
    public static final int ENCODED_SURROUND_OUTPUT_ALWAYS              = 2;
    public static final int ENCODED_SURROUND_OUTPUT_MANUAL              = 3;

    public static final String SOUND_OUTPUT_DEVICE                      = "sound_output_device";
    public static final String PARA_SOUND_OUTPUT_DEVICE_SPEAKER         = "sound_output_device=speak";
    public static final String PARA_SOUND_OUTPUT_DEVICE_ARC             = "sound_output_device=arc";
    public static final int SOUND_OUTPUT_DEVICE_SPEAKER                 = 0;
    public static final int SOUND_OUTPUT_DEVICE_ARC                     = 1;

    public static final String DIGITAL_SOUND                = "digital_sound";
    public static final String PCM                          = "PCM";
    public static final String RAW                          = "RAW";
    public static final String HDMI                         = "HDMI";
    public static final String SPDIF                        = "SPDIF";
    public static final String HDMI_RAW                     = "HDMI passthrough";
    public static final String SPDIF_RAW                    = "SPDIF passthrough";
    public static final int IS_PCM                          = 0;
    public static final int IS_SPDIF_RAW                    = 1;
    public static final int IS_HDMI_RAW                     = 2;

    public static final String DRC_MODE                     = "drc_mode";
    public static final String DTSDRC_MODE                  = "dtsdrc_mode";
    public static final String CUSTOM_0_DRCMODE             = "0";
    public static final String CUSTOM_1_DRCMODE             = "1";
    public static final String LINE_DRCMODE                 = "2";
    public static final String RF_DRCMODE                   = "3";
    public static final String DEFAULT_DRCMODE              = LINE_DRCMODE;
    public static final String MIN_DRC_SCALE                = "0";
    public static final String MAX_DRC_SCALE                = "100";
    public static final String DEFAULT_DRC_SCALE            = MIN_DRC_SCALE;
    public static final int IS_DRC_OFF                      = 0;
    public static final int IS_DRC_LINE                     = 1;
    public static final int IS_DRC_RF                       = 2;

    public static final String REAL_OUTPUT_SOC              = "meson8,meson8b,meson8m2,meson9b";
    public static final String UI_720P                      = "720p";
    public static final String UI_1080P                     = "1080p";
    public static final String UI_2160P                     = "2160p";
    public static final String HDMI_480                     = "480";
    public static final String HDMI_576                     = "576";
    public static final String HDMI_720                     = "720p";
    public static final String HDMI_1080                    = "1080";
    public static final String HDMI_4K2K                    = "2160p";
    public static final String HDMI_SMPTE                   = "smpte";

    private static final String HDR_POLICY_SOURCE           = "1";
    private static final String HDR_POLICY_SINK             = "0";

    private String DEFAULT_OUTPUT_MODE                      = "720p60hz";
    private String DEFAULT_COLOR_ATTRIBUTE                  = "444,8bit";

    public static final int DOLBY_VISION                    = 0;
    public static final int HDR10                           = 1;

    private static final String[] MODE_RESOLUTION_FIRST = {
        "480i60hz",
        "576i50hz",
        "480p60hz",
        "576p50hz",
        "720p50hz",
        "720p60hz",
        "1080p50hz",
        "1080p60hz",
        "2160p24hz",
        "2160p25hz",
        "2160p30hz",
        "2160p50hz",
        "2160p60hz",
    };
    private static final String[] MODE_FRAMERATE_FIRST = {
        "480i60hz",
        "576i50hz",
        "480p60hz",
        "576p50hz",
        "720p50hz",
        "720p60hz",
        "2160p24hz",
        "2160p25hz",
        "2160p30hz",
        "1080p50hz",
        "1080p60hz",
        "2160p50hz",
        "2160p60hz",
    };
    private static String currentColorAttribute = null;
    private static String currentOutputmode = null;
    private boolean ifModeSetting = false;
    private final Context mContext;
    private final ContentResolver mResolver;
    final Object mLock = new Object[0];

    private SystemControlManager mSystenControl;
    private AudioManager mAudioManager;
    private static AudioEffect mDapAudioEffect;
    /*only system/priv process can binder HDMI_CONTROL_SERVICE*/
   // private HdmiControlManager mHdmiControlManager;

    public OutputModeManager(Context context) {
        mContext = context;

        mSystenControl = SystemControlManager.getInstance();
        mResolver = mContext.getContentResolver();
        currentOutputmode = readSysfs(DISPLAY_MODE);
        mAudioManager = (AudioManager) context.getSystemService(context.AUDIO_SERVICE);

       /* mHdmiControlManager = (HdmiControlManager) context.getSystemService(Context.HDMI_CONTROL_SERVICE);
        if (mHdmiControlManager != null) {
            mHdmiControlManager.addHotplugEventListener(new HdmiControlManager.HotplugEventListener() {

                @Override
                public void onReceived(HdmiHotplugEvent event) {
                    if (Settings.Global.getInt(context.getContentResolver(), DIGITAL_SOUND, IS_PCM) == IS_HDMI_RAW) {
                        autoSwitchHdmiPassthough();
                    }
                }
            });
        }*/
    }

    private void setOutputMode(final String mode) {
        setOutputModeNowLocked(mode);
    }

    public void setBestMode(String mode) {
        if (mode == null) {
            if (!isBestOutputmode()) {
                mSystenControl.setBootenv(ENV_IS_BEST_MODE, "true");
                if (SystemControlManager.USE_BEST_MODE) {
                    setOutputMode(getBestMatchResolution());
                } else {
                    setOutputMode(getHighestMatchResolution());
                }
            } else {
                mSystenControl.setBootenv(ENV_IS_BEST_MODE, "false");
            }
        } else {
            mSystenControl.setBootenv(ENV_IS_BEST_MODE, "false");
            mSystenControl.setBootenv(ENV_IS_BEST_DOLBYVISION, "false");
            setOutputModeNowLocked(mode);
        }
    }

    public void setDeepColorMode() {
        if (isDeepColor()) {
            mSystenControl.setProperty(PROP_DEEPCOLOR, "false");
        } else {
            mSystenControl.setProperty(PROP_DEEPCOLOR, "true");
        }
        setOutputModeNowLocked(getCurrentOutputMode());
    }

    public void setDeepColorAttribute(final String colorValue) {
        mSystenControl.setBootenv(ENV_IS_BEST_MODE, "false");
        mSystenControl.setBootenv(ENV_COLORATTRIBUTE, colorValue);
        setOutputModeNowLocked(getCurrentOutputMode());
    }

    public String getCurrentColorAttribute(){
       String colorValue = getBootenv(ENV_COLORATTRIBUTE, DEFAULT_COLOR_ATTRIBUTE);
       return colorValue;
    }

    public String getHdmiColorSupportList() {
        String list = readSupportList(HDMI_COLOR_SUPPORT_LIST);

        if (DEBUG)
            Log.d(TAG, "getHdmiColorSupportList :" + list);
        return list;
    }

    public boolean isModeSupportColor(final String curMode, final String curValue){
         return mSystenControl.GetModeSupportDeepColorAttr(curMode,curValue);
    }

    private void setOutputModeNowLocked(final String newMode){
        synchronized (mLock) {
            String oldMode = currentOutputmode;
            currentOutputmode = newMode;

            if (oldMode == null || oldMode.length() < 4) {
                Log.e(TAG, "get display mode error, oldMode:" + oldMode + " set to default " + DEFAULT_OUTPUT_MODE);
                oldMode = DEFAULT_OUTPUT_MODE;
            }

            if (DEBUG)
                Log.d(TAG, "change mode from " + oldMode + " -> " + newMode);

            mSystenControl.setMboxOutputMode(newMode);

            Intent intent = new Intent(ACTION_HDMI_MODE_CHANGED);
            //intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
            intent.putExtra(EXTRA_HDMI_MODE, newMode);
            mContext.sendStickyBroadcast(intent);
        }
    }

    public void setOsdMouse(String curMode) {
        if (DEBUG)
            Log.d(TAG, "set osd mouse curMode " + curMode);
        mSystenControl.setOsdMouseMode(curMode);
    }

    public void setOsdMouse(int x, int y, int w, int h) {
        mSystenControl.setOsdMousePara(x, y, w, h);
    }

    public String getCurrentOutputMode(){
        return readSysfs(DISPLAY_MODE);
    }

    public int getHdrPriority() {
        String curType = getBootenv(ENV_HDR_PRIORITY, Integer.toString(DOLBY_VISION));
        if (curType.contains(Integer.toString(HDR10))) {
            return HDR10;
        } else {
            return DOLBY_VISION;
        }
    }
    public void setHdrPriority(int type) {
        if (type == HDR10) {
            mSystenControl.setBootenv(ENV_HDR_PRIORITY, Integer.toString(HDR10));
        } else {
            mSystenControl.setBootenv(ENV_HDR_PRIORITY, Integer.toString(DOLBY_VISION));
        }
    }
    public int[] getPosition(String mode) {
        return mSystenControl.getPosition(mode);
    }

    public void savePosition(int left, int top, int width, int height) {
        mSystenControl.setPosition(left, top, width, height);
    }

    public String getHdmiSupportList() {
        String list = readSupportList(HDMI_SUPPORT_LIST).replaceAll("[*]", "");

        if (DEBUG)
            Log.d(TAG, "getHdmiSupportList :" + list);
        return list;
    }

    public String getHighestMatchResolution() {
        String value = readSupportList(HDMI_SUPPORT_LIST);
        if (getPropertyBoolean(PROP_HDMI_FRAMERATE_PRIORITY, false)) {
            for (int i = MODE_FRAMERATE_FIRST.length - 1; i >= 0 ; i--) {
                if (value.contains(MODE_FRAMERATE_FIRST[i])) {
                    return MODE_FRAMERATE_FIRST[i];
                }
            }
        } else {
            for (int i = MODE_RESOLUTION_FIRST.length - 1; i >= 0 ; i--) {
                if (value.contains(MODE_RESOLUTION_FIRST[i])) {
                    return MODE_RESOLUTION_FIRST[i];
                }
            }
        }

        return getPropertyString(PROP_BEST_OUTPUT_MODE, DEFAULT_OUTPUT_MODE);
    }

    public String getBestMatchResolution() {
        if (DEBUG)
            Log.d(TAG, "get best mode, if support mode contains *, that is best mode, otherwise use:" + PROP_BEST_OUTPUT_MODE);

        String[] supportList = null;
        String value = readSupportList(HDMI_SUPPORT_LIST);
        if (value.indexOf(HDMI_480) >= 0 || value.indexOf(HDMI_576) >= 0
            || value.indexOf(HDMI_720) >= 0 || value.indexOf(HDMI_1080) >= 0
            || value.indexOf(HDMI_4K2K) >= 0 || value.indexOf(HDMI_SMPTE) >= 0) {
            supportList = (value.substring(0, value.length()-1)).split(",");
        }

        if (supportList != null) {
            for (int i = 0; i < supportList.length; i++) {
                if (supportList[i].contains("*")) {
                    return supportList[i].substring(0, supportList[i].length()-1);
                }
            }
        }

        return getPropertyString(PROP_BEST_OUTPUT_MODE, DEFAULT_OUTPUT_MODE);
    }

    public String getSupportedResolution() {
        String curMode = getBootenv(ENV_HDMI_MODE, DEFAULT_OUTPUT_MODE);

        if (DEBUG)
            Log.d(TAG, "get supported resolution curMode:" + curMode);

        String value = readSupportList(HDMI_SUPPORT_LIST);
        String[] supportList = null;

        if (value.indexOf(HDMI_480) >= 0 || value.indexOf(HDMI_576) >= 0
            || value.indexOf(HDMI_720) >= 0 || value.indexOf(HDMI_1080) >= 0
            || value.indexOf(HDMI_4K2K) >= 0 || value.indexOf(HDMI_SMPTE) >= 0) {
            supportList = (value.substring(0, value.length()-1)).split(",");
        }

        if (supportList == null) {
            return curMode;
        }

        for (int i = 0; i < supportList.length; i++) {
            if (supportList[i].equals(curMode)) {
                return curMode;
            }
        }

        if (SystemControlManager.USE_BEST_MODE) {
            return getBestMatchResolution();
        }
        return getHighestMatchResolution();
    }

    private String readSupportList(String path) {
        String str = null;
        String value = "";
        try {
            BufferedReader br = new BufferedReader(new FileReader(path));
            while ((str = br.readLine()) != null) {
                if (str != null) {
                    if (!getPropertyBoolean(PROP_SUPPORT_4K, true)
                        && (str.contains("2160") || str.contains("smpte"))) {
                        continue;
                    }
                    if (!getPropertyBoolean(PROP_SUPPORT_OVER_4K30, true)
                        && (str.contains("2160p50") || str.contains("2160p60") || str.contains("smpte"))) {
                        continue;
                    }
                    value += str + ",";
                }
            }
            br.close();

            Log.d(TAG, "TV support list is :" + value);
        } catch (Exception e) {
            e.printStackTrace();
        }

        return value;
    }

    public void initOutputMode(){
        if (isHDMIPlugged()) {
            setHdmiPlugged();
        } else {
            if (!currentOutputmode.contains("cvbs"))
                setHdmiUnPlugged();
        }

        //there can not set osd mouse parameter, otherwise bootanimation logo will shake
        //because set osd1 scaler will shake
    }

    public void setHdmiUnPlugged(){
        Log.d(TAG, "setHdmiUnPlugged");

        if (getPropertyBoolean(PROP_HDMI_ONLY, true)) {
            String cvbsmode = getBootenv(ENV_CVBS_MODE, "576cvbs");
            setOutputMode(cvbsmode);
        }
    }

    public void setHdmiPlugged() {
        boolean isAutoMode = isBestOutputmode() || readSupportList(HDMI_SUPPORT_LIST).contains("null edid");

        Log.d(TAG, "setHdmiPlugged auto mode: " + isAutoMode);
        if (getPropertyBoolean(PROP_HDMI_ONLY, true)) {
            if (isAutoMode) {
                if (SystemControlManager.USE_BEST_MODE) {
                    setOutputMode(getBestMatchResolution());
                } else {
                    setOutputMode(getHighestMatchResolution());
                }
            } else {
                String mode = getSupportedResolution();
                setOutputMode(mode);
            }
        }
    }

    public boolean isBestOutputmode() {
        String isBestOutputmode = mSystenControl.getBootenv(ENV_IS_BEST_MODE, "true");
        return Boolean.parseBoolean(isBestOutputmode.equals("") ? "true" : isBestOutputmode);
    }

    public void setBestDolbyVision(boolean enable) {
        mSystenControl.setBootenv(ENV_IS_BEST_DOLBYVISION, enable ? "true" : "false");
    }

    public boolean isBestDolbyVsion() {
        String isBestDolbyVsion = mSystenControl.getBootenv(ENV_IS_BEST_DOLBYVISION, "true");
        Log.e("TEST", "isBestDolbyVsion:" + isBestDolbyVsion);
        return Boolean.parseBoolean(isBestDolbyVsion.equals("") ? "true" : isBestDolbyVsion);
    }
    public void setHdrStrategy(final String HdrStrategy) {
          mSystenControl.setBootenv(ENV_HDR_POLICY, HdrStrategy);
          mSystenControl.setHdrStrategy(HdrStrategy);
    }

    public String getHdrStrategy(){
        String dolbyvisionType = getBootenv(ENV_HDR_POLICY, HDR_POLICY_SOURCE);
        return dolbyvisionType;
    }
    public boolean isDeepColor() {
        return getPropertyBoolean(PROP_DEEPCOLOR, false);
    }

    public boolean isHDMIPlugged() {
        String status = readSysfs(HDMI_STATE);
        if ("1".equals(status))
            return true;
        else
            return false;
    }

    public boolean ifModeIsSetting() {
        return ifModeSetting;
    }

    private void shadowScreen() {
        writeSysfs(FB0_BLANK, "1");
        Thread task = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    ifModeSetting = true;
                    Thread.sleep(1000);
                    writeSysfs(FB0_BLANK, "0");
                    ifModeSetting = false;
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        });
        task.start();
    }

    public String getDigitalVoiceMode(){
        return getBootenv(ENV_DIGIT_AUDIO, PCM);
    }

    public int autoSwitchHdmiPassthough () {
        String mAudioCapInfo = readSysfsTotal(SYS_AUDIO_CAP);
        if (mAudioCapInfo.contains("Dobly_Digital+")) {
            setDigitalMode(HDMI_RAW);
            return IS_HDMI_RAW;
        } else if (mAudioCapInfo.contains("AC-3")
                || (getPropertyBoolean(PROP_DTSEDID, false) && mAudioCapInfo.contains("DTS"))) {
            setDigitalMode(SPDIF_RAW);
            return IS_SPDIF_RAW;
        } else {
            setDigitalMode(PCM);
            return IS_PCM;
        }
    }

    public void setDigitalMode(String mode) {
        // value : "PCM" ,"RAW","SPDIF passthrough","HDMI passthrough"
        setBootenv(ENV_DIGIT_AUDIO, mode);
        mSystenControl.setDigitalMode(mode);
    }

    public void enableDobly_DRC (boolean enable) {
        if (enable) {       //open DRC
            writeSysfs(AUIDO_DSP_AC3_DRC, "drchighcutscale 0x64");
            writeSysfs(AUIDO_DSP_AC3_DRC, "drclowboostscale 0x64");
        } else {           //close DRC
            writeSysfs(AUIDO_DSP_AC3_DRC, "drchighcutscale 0");
            writeSysfs(AUIDO_DSP_AC3_DRC, "drclowboostscale 0");
        }
    }

    public void setDoblyMode (String mode) {
        //"CUSTOM_0","CUSTOM_1","LINE","RF"; default use "LINE"
        int i = Integer.parseInt(mode);
        if (i >= 0 && i <= 3) {
            writeSysfs(AUIDO_DSP_AC3_DRC, "drcmode" + " " + mode);
        } else {
            writeSysfs(AUIDO_DSP_AC3_DRC, "drcmode" + " " + DEFAULT_DRCMODE);
        }
    }

    public void setDtsDrcScale (String drcscale) {
        //10 one step,100 highest; default use "0"
        int i = Integer.parseInt(drcscale);
        if (i >= 0 && i <= 100) {
            setProperty(PROP_DTSDRCSCALE, drcscale);
        } else {
            setProperty(PROP_DTSDRCSCALE, DEFAULT_DRC_SCALE);
        }
        setDtsDrcScaleSysfs();
    }

    public void setDtsDrcScaleSysfs() {
        String prop = getPropertyString(PROP_DTSDRCSCALE, DEFAULT_DRC_SCALE);
        int val = Integer.parseInt(prop);
        writeSysfs(AUIDO_DSP_DTS_DEC, String.format("0x%02x", val));
    }
    /**
    * @Deprecated
    **/
    public void setDTS_DownmixMode(String mode) {
        // 0: Lo/Ro;   1: Lt/Rt;  default 0
        int i = Integer.parseInt(mode);
        if (i >= 0 && i <= 1) {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdmxmode" + " " + mode);
        } else {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdmxmode" + " " + "0");
        }
    }
    /**
    * @Deprecated
    **/
    public void enableDTS_DRC_scale_control (boolean enable) {
        if (enable) {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdrcscale 0x64");
        } else {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdrcscale 0");
        }
    }
    /**
    * @Deprecated
    **/
    public void enableDTS_Dial_Norm_control (boolean enable) {
        if (enable) {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdialnorm 1");
        } else {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdialnorm 0");
        }
    }

    private String getProperty(String key) {
        if (DEBUG)
            Log.i(TAG, "getProperty key:" + key);
        return mSystenControl.getProperty(key);
    }

    private String getPropertyString(String key, String def) {
        if (DEBUG)
            Log.i(TAG, "getPropertyString key:" + key + " def:" + def);
        return mSystenControl.getPropertyString(key, def);
    }

    private int getPropertyInt(String key,int def) {
        if (DEBUG)
            Log.i(TAG, "getPropertyInt key:" + key + " def:" + def);
        return mSystenControl.getPropertyInt(key, def);
    }

    private long getPropertyLong(String key,long def) {
        if (DEBUG)
            Log.i(TAG, "getPropertyLong key:" + key + " def:" + def);
        return mSystenControl.getPropertyLong(key, def);
    }

    private boolean getPropertyBoolean(String key,boolean def) {
        if (DEBUG)
            Log.i(TAG, "getPropertyBoolean key:" + key + " def:" + def);
        return mSystenControl.getPropertyBoolean(key, def);
    }

    private void setProperty(String key, String value) {
        if (DEBUG)
            Log.i(TAG, "setProperty key:" + key + " value:" + value);
        mSystenControl.setProperty(key, value);
    }

    private String getBootenv(String key, String value) {
        if (DEBUG)
            Log.i(TAG, "getBootenv key:" + key + " def value:" + value);
        return mSystenControl.getBootenv(key, value);
    }

    private int getBootenvInt(String key, String value) {
        if (DEBUG)
            Log.i(TAG, "getBootenvInt key:" + key + " def value:" + value);
        return Integer.parseInt(mSystenControl.getBootenv(key, value));
    }

    private void setBootenv(String key, String value) {
        if (DEBUG)
            Log.i(TAG, "setBootenv key:" + key + " value:" + value);
        mSystenControl.setBootenv(key, value);
    }

    private String readSysfsTotal(String path) {
        return mSystenControl.readSysFs(path).replaceAll("\n", "");
    }

    private String readSysfs(String path) {

        return mSystenControl.readSysFs(path).replaceAll("\n", "");
        /*
        if (!new File(path).exists()) {
            Log.e(TAG, "File not found: " + path);
            return null;
        }

        String str = null;
        StringBuilder value = new StringBuilder();

        if (DEBUG)
            Log.i(TAG, "readSysfs path:" + path);

        try {
            FileReader fr = new FileReader(path);
            BufferedReader br = new BufferedReader(fr);
            try {
                while ((str = br.readLine()) != null) {
                    if (str != null)
                        value.append(str);
                };
                fr.close();
                br.close();
                if (value != null)
                    return value.toString();
                else
                    return null;
            } catch (IOException e) {
                e.printStackTrace();
                return null;
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return null;
        }
        */
    }

    private boolean writeSysfs(String path, String value) {
        if (DEBUG)
            Log.i(TAG, "writeSysfs path:" + path + " value:" + value);

        return mSystenControl.writeSysFs(path, value);
        /*
        if (!new File(path).exists()) {
            Log.e(TAG, "File not found: " + path);
            return false;
        }

        try {
            BufferedWriter writer = new BufferedWriter(new FileWriter(path), 64);
            try {
                writer.write(value);
            } finally {
                writer.close();
            }
            return true;

        } catch (IOException e) {
            Log.e(TAG, "IO Exception when write: " + path, e);
            return false;
        }
        */
    }

    public void saveDigitalAudioFormatMode(int mode, String submode) {
        String tmp;
        boolean isTv;
        // trigger AudioService retrieve support audio format value
        Settings.Global.putInt(mResolver,
                ENCODED_SURROUND_OUTPUT/*Settings.Global.ENCODED_SURROUND_OUTPUT*/, -1);
        int surround = -1;
        switch (mode) {
            case DIGITAL_SPDIF:
                Settings.Global.putInt(mResolver,
                        NRDP_EXTERNAL_SURROUND, NRDP_ENABLE);
                Settings.Global.putInt(mResolver,
                        DIGITAL_AUDIO_FORMAT, DIGITAL_SPDIF);
                Settings.Global.putString(mResolver,
                        DIGITAL_AUDIO_SUBFORMAT, DIGITAL_AUDIO_SUBFORMAT_SPDIF);
                if (surround != ENCODED_SURROUND_OUTPUT_MANUAL)
                    Settings.Global.putInt(mResolver,
                            ENCODED_SURROUND_OUTPUT/*Settings.Global.ENCODED_SURROUND_OUTPUT*/,
                            ENCODED_SURROUND_OUTPUT_MANUAL/*Settings.Global.ENCODED_SURROUND_OUTPUT_MANUAL*/);
                tmp = Settings.Global.getString(mResolver,
                        OutputModeManager.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS);
                if (!DIGITAL_AUDIO_SUBFORMAT_SPDIF.equals(tmp))
                    Settings.Global.putString(mResolver,
                            ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS,
                            DIGITAL_AUDIO_SUBFORMAT_SPDIF);
                break;
            case DIGITAL_MANUAL:
                if (submode == null)
                    submode = "";
                isTv = mSystenControl.getPropertyBoolean("ro.vendor.platform.has.tvuimode", false);
                Settings.Global.putInt(mResolver,
                        NRDP_EXTERNAL_SURROUND, NRDP_DISABLE);
                if (isTv) {
                    Settings.Global.putInt(mResolver,
                            DIGITAL_AUDIO_FORMAT, DIGITAL_AUTO);
                    break;
                } else
                    Settings.Global.putInt(mResolver,
                            DIGITAL_AUDIO_FORMAT, DIGITAL_MANUAL);
                Settings.Global.putString(mResolver,
                        DIGITAL_AUDIO_SUBFORMAT, submode);
                if (surround != ENCODED_SURROUND_OUTPUT_MANUAL)
                    Settings.Global.putInt(mResolver,
                            ENCODED_SURROUND_OUTPUT/*Settings.Global.ENCODED_SURROUND_OUTPUT*/,
                            ENCODED_SURROUND_OUTPUT_MANUAL/*Settings.Global.ENCODED_SURROUND_OUTPUT_MANUAL*/);
                tmp = Settings.Global.getString(mResolver,
                        OutputModeManager.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS);
                if (!submode.equals(tmp))
                    Settings.Global.putString(mResolver,
                            ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS, submode);
                break;
            case DIGITAL_AUTO:
                isTv = mSystenControl.getPropertyBoolean("ro.vendor.platform.has.tvuimode", false);
                boolean isDolbyMS12 = new File(AUDIO_MS12LIB_PATH).exists();
                if (isTv && isDolbyMS12)
                    Settings.Global.putInt(mResolver,
                        NRDP_EXTERNAL_SURROUND, NRDP_ENABLE);
                else
                    Settings.Global.putInt(mResolver,
                        NRDP_EXTERNAL_SURROUND, NRDP_DISABLE);
                Settings.Global.putInt(mResolver,
                        DIGITAL_AUDIO_FORMAT, DIGITAL_AUTO);
                if (surround != ENCODED_SURROUND_OUTPUT_AUTO)
                    Settings.Global.putInt(mResolver,
                            ENCODED_SURROUND_OUTPUT/*Settings.Global.ENCODED_SURROUND_OUTPUT*/,
                            ENCODED_SURROUND_OUTPUT_AUTO/*Settings.Global.ENCODED_SURROUND_OUTPUT_AUTO*/);
                break;
            case DIGITAL_PCM:
            default:
                Settings.Global.putInt(mResolver,
                        NRDP_EXTERNAL_SURROUND, NRDP_DISABLE);
                Settings.Global.putInt(mResolver,
                        DIGITAL_AUDIO_FORMAT, DIGITAL_PCM);
                if (surround != ENCODED_SURROUND_OUTPUT_NEVER)
                    Settings.Global.putInt(mResolver,
                            ENCODED_SURROUND_OUTPUT/*Settings.Global.ENCODED_SURROUND_OUTPUT*/,
                            ENCODED_SURROUND_OUTPUT_NEVER/*Settings.Global.ENCODED_SURROUND_OUTPUT_NEVER*/);
                break;
        }
    }

    public void setDigitalAudioFormatOut(int mode) {
        setDigitalAudioFormatOut(mode, "");
    }

    public void setDigitalAudioFormatOut(int mode, String submode) {
        Log.d(TAG, "setDigitalAudioFormatOut: mode="+mode+", submode="+submode);
        saveDigitalAudioFormatMode(mode, submode);
        switch (mode) {
            case DIGITAL_SPDIF:
                mAudioManager.setParameters(PARA_SPDIF);
                break;
            case DIGITAL_AUTO:
                mAudioManager.setParameters(PARA_AUTO);
                break;
            case DIGITAL_MANUAL:
                mAudioManager.setParameters(PARA_AUTO);
                break;
            case DIGITAL_PCM:
            default:
                mAudioManager.setParameters(PARA_PCM);
                break;
        }
    }

    public void setAudioMixingEnable(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_AUDIO_MIXING_ON);
        } else {
            mAudioManager.setParameters(PARA_AUDIO_MIXING_OFF);
        }
    }

    private boolean creatDapAudioEffect() {
        try {
            if (mDapAudioEffect == null) {
                Class audioeffect = Class.forName("android.media.audiofx.AudioEffect");
                Class[] param = new Class[]{ Class.forName("java.util.UUID"),
                        Class.forName("java.util.UUID"), int.class, int.class };
                Constructor ctor = audioeffect.getConstructor(param);
                Object[] obj = new Object[] { EFFECT_TYPE_DAP, EFFECT_TYPE_NULL, 0, 0 };
                mDapAudioEffect = (AudioEffect)ctor.newInstance(obj);
                //mDapAudioEffect = new AudioEffect(EFFECT_TYPE_DAP, EFFECT_TYPE_NULL, 0, 0);
                int result = mDapAudioEffect.setEnabled(true);
                if (result == AudioEffect.SUCCESS) {
                    Log.d(TAG, "creatDapAudioEffect enable dap");
                }
            }
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Unable to create Dap audio effect", e);
        }
        return false;
    }

    public boolean isDapValid() {
        File fl = new File("/vendor/lib/libms12dap.so");
        return fl.exists();
    }

    public void initDapAudioEffect() {
        Log.d(TAG, "initDapAudioEffect");
        if (!creatDapAudioEffect()) {
            Log.e(TAG, "initDapAudioEffect dap creat fail");
            return;
        }

        int result = mDapAudioEffect.setEnabled(true);
        if (result != AudioEffect.SUCCESS) {
            Log.d(TAG, "initDapAudioEffect dap setEnabled error: "+result);
            return;
        }

        int mode = getDapParam(CMD_DAP_EFFECT_MODE);
        if (Settings.Global.getInt(mResolver, PARAM_DAP_SAVED, 0) == 0) {
            int id = 0;
            //the first time, use the param from so load from ini file
            setDapParam(CMD_DAP_EFFECT_MODE, DAP_MODE_USER);
            for (id = CMD_DAP_GEQ_ENABLE; id <= CMD_DAP_VIRTUALIZER_ENABLE; id++)
                saveDapParam(id, getDapParamInternal(id));
            for (id = SUBCMD_DAP_GEQ_BAND1; id <= SUBCMD_DAP_GEQ_BAND5; id++)
                saveDapParam(id, getDapParamInternal(id));
            Settings.Global.putInt(mResolver, PARAM_DAP_SAVED, 1);
        } else {
            saveDapParam(CMD_DAP_EFFECT_MODE, DAP_MODE_USER);
            setDapParam(CMD_DAP_VL_ENABLE, getDapParam(CMD_DAP_VL_ENABLE));
            setDapParam(CMD_DAP_VL_AMOUNT, getDapParam(CMD_DAP_VL_AMOUNT));
            setDapParam(CMD_DAP_DE_ENABLE, getDapParam(CMD_DAP_DE_ENABLE));
            setDapParam(CMD_DAP_DE_AMOUNT, getDapParam(CMD_DAP_DE_AMOUNT));
            setDapParam(CMD_DAP_SURROUND_BOOST, getDapParam(CMD_DAP_SURROUND_BOOST));
            setDapParam(CMD_DAP_GEQ_ENABLE, getDapParam(CMD_DAP_GEQ_ENABLE));
            setDapParam(SUBCMD_DAP_GEQ_BAND1, getDapParam(SUBCMD_DAP_GEQ_BAND1));
            saveDapParam(CMD_DAP_EFFECT_MODE, mode);
        }
        setDapParam(CMD_DAP_EFFECT_MODE, mode);
        applyDapAudioEffectByPlayEmptyTrack();
    }

    private void setDapParamInternal(int id, int value) {
        try {
            Class audioEffect = Class.forName("android.media.audiofx.AudioEffect");
            Method setParameter = audioEffect.getMethod("setParameter", int.class, int.class);
            setParameter.invoke(mDapAudioEffect, id, value);
        } catch(Exception e) {
            Log.d(TAG, "setDapParamInternal: "+e);
        }
    }

    private void setDapParamInternal(int id, byte[] value) {
        try {
            Class audioEffect = Class.forName("android.media.audiofx.AudioEffect");
            Method setParameter = audioEffect.getMethod("setParameter", int.class, byte[].class);
            Object[] param = new Object[2];
            param[0] = id;
            param[1] = value;
            setParameter.invoke(mDapAudioEffect, param);
        } catch(Exception e) {
            Log.d(TAG, "setDapParamInternal: "+e);
        }
    }

    private int getDapParamInternal (int id) {
        try {
            Class audioEffect = Class.forName("android.media.audiofx.AudioEffect");
            switch (id) {
                case SUBCMD_DAP_GEQ_BAND1:
                case SUBCMD_DAP_GEQ_BAND2:
                case SUBCMD_DAP_GEQ_BAND3:
                case SUBCMD_DAP_GEQ_BAND4:
                case SUBCMD_DAP_GEQ_BAND5:
                {
                    Method getParameter = audioEffect.getMethod("getParameter", int.class, byte[].class);
                    byte[] value = new byte[5];
                    Object[] param = new Object[2];
                    param[0] = CMD_DAP_GEQ_GAINS;
                    param[1] = value;
                    getParameter.invoke(mDapAudioEffect, param);
                    return value[id-SUBCMD_DAP_GEQ_BAND1];
                }
                default:
                {
                    Method getParameter = audioEffect.getMethod("getParameter", int.class, int[].class);
                    int[] value = new int[1];
                    Object[] param = new Object[2];
                    param[0] = id;
                    param[1] = value;
                    getParameter.invoke(mDapAudioEffect, param);
                    return value[0];
                }
            }
        } catch(Exception e) {
            Log.d(TAG, "getDapParamInternal: "+e);
        }
        return 0;
    }

    public void setDapParam (int id, int value) {
        byte[] fiveband = new byte[5];
        switch (id) {
            case CMD_DAP_ENABLE:
            case CMD_DAP_EFFECT_MODE:
            case CMD_DAP_VL_ENABLE:
            case CMD_DAP_VL_AMOUNT:
            case CMD_DAP_DE_ENABLE:
            case CMD_DAP_DE_AMOUNT:
            case CMD_DAP_POST_GAIN:
            case CMD_DAP_GEQ_ENABLE:
            case CMD_DAP_SURROUND_ENABLE:
            case CMD_DAP_SURROUND_BOOST:
                setDapParamInternal(id, value);
                break;
            case CMD_DAP_VIRTUALIZER_ENABLE:
                if (value == DAP_SURROUND_SPEAKER)
                    setDapParamInternal(id, DAP_CPDP_OUTPUT_2_SPEAKER);
                else if (value == DAP_SURROUND_HEADPHONE)
                    setDapParamInternal(id, DAP_CPDP_OUTPUT_2_HEADPHONE);
                break;
            case SUBCMD_DAP_GEQ_BAND1:
            case SUBCMD_DAP_GEQ_BAND2:
            case SUBCMD_DAP_GEQ_BAND3:
            case SUBCMD_DAP_GEQ_BAND4:
            case SUBCMD_DAP_GEQ_BAND5:
                fiveband[0] = (byte)getDapParam(SUBCMD_DAP_GEQ_BAND1);
                fiveband[1] = (byte)getDapParam(SUBCMD_DAP_GEQ_BAND2);
                fiveband[2] = (byte)getDapParam(SUBCMD_DAP_GEQ_BAND3);
                fiveband[3] = (byte)getDapParam(SUBCMD_DAP_GEQ_BAND4);
                fiveband[4] = (byte)getDapParam(SUBCMD_DAP_GEQ_BAND5);
                fiveband[id-SUBCMD_DAP_GEQ_BAND1] = (byte)value;
                setDapParamInternal(CMD_DAP_GEQ_GAINS, fiveband);
                break;
        }
    }

    public void saveDapParam (int id, int value) {
        int param = 0;
        if ((id != CMD_DAP_EFFECT_MODE) && (getDapParam(CMD_DAP_EFFECT_MODE) != DAP_MODE_USER))
            return;

        switch (id) {
            case CMD_DAP_EFFECT_MODE:
                Settings.Global.putInt(mResolver, PARAM_DAP_MODE, value);
                break;
            case CMD_DAP_VL_ENABLE:
                Settings.Global.putInt(mResolver, PARAM_DAP_VL_ENABLE, value);
                break;
            case CMD_DAP_VL_AMOUNT:
                Settings.Global.putInt(mResolver, PARAM_DAP_VL_AMOUNT, value);
                break;
            case CMD_DAP_DE_ENABLE:
                Settings.Global.putInt(mResolver, PARAM_DAP_DE_ENABLE, value);
                break;
            case CMD_DAP_DE_AMOUNT:
                Settings.Global.putInt(mResolver, PARAM_DAP_DE_AMOUNT, value);
                break;
            case CMD_DAP_SURROUND_ENABLE:
                Settings.Global.putInt(mResolver, PARAM_DAP_SURROUND_ENABLE, value);
                break;
            case CMD_DAP_SURROUND_BOOST:
                Settings.Global.putInt(mResolver, PARAM_DAP_SURROUND_BOOST, value);
                break;
            case CMD_DAP_POST_GAIN:
                Settings.Global.putInt(mResolver, PARAM_DAP_POST_GAIN, value);
                break;
            case CMD_DAP_GEQ_ENABLE:
                Settings.Global.putInt(mResolver, PARAM_DAP_GEQ_ENABLE, value);
                break;
            case SUBCMD_DAP_GEQ_BAND1:
                param = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_ENABLE, DAP_GEQ_DEFAULT);
                if (param == DAP_GEQ_USER)
                    Settings.Global.putInt(mResolver, PARAM_DAP_GEQ_BAND1, value);
                break;
            case SUBCMD_DAP_GEQ_BAND2:
                param = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_ENABLE, DAP_GEQ_DEFAULT);
                if (param == DAP_GEQ_USER)
                    Settings.Global.putInt(mResolver, PARAM_DAP_GEQ_BAND2, value);
                break;
            case SUBCMD_DAP_GEQ_BAND3:
                param = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_ENABLE, DAP_GEQ_DEFAULT);
                if (param == DAP_GEQ_USER)
                    Settings.Global.putInt(mResolver, PARAM_DAP_GEQ_BAND3, value);
                break;
            case SUBCMD_DAP_GEQ_BAND4:
                param = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_ENABLE, DAP_GEQ_DEFAULT);
                if (param == DAP_GEQ_USER)
                    Settings.Global.putInt(mResolver, PARAM_DAP_GEQ_BAND4, value);
                break;
            case SUBCMD_DAP_GEQ_BAND5:
                param = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_ENABLE, DAP_GEQ_DEFAULT);
                if (param == DAP_GEQ_USER)
                    Settings.Global.putInt(mResolver, PARAM_DAP_GEQ_BAND5, value);
                break;
        }
    }

    public int getDapParam (int id) {
        int value = -1, param = 0;

        if (id == CMD_DAP_EFFECT_MODE) {
            value = Settings.Global.getInt(mResolver, PARAM_DAP_MODE, -1);
            if (value < 0) {
                value = getDapParamInternal(id);
                saveDapParam(id, value);
            }
            return value;
        }
        if (getDapParam(CMD_DAP_EFFECT_MODE) != DAP_MODE_USER)
            return getDapParamInternal(id);

        switch (id) {
            case CMD_DAP_EFFECT_MODE:
                value = Settings.Global.getInt(mResolver, PARAM_DAP_MODE, DAP_MODE_DEFAULT);
                break;
            case CMD_DAP_VL_ENABLE:
                value = Settings.Global.getInt(mResolver, PARAM_DAP_VL_ENABLE, DAP_VL_DEFAULT);
                break;
            case CMD_DAP_VL_AMOUNT:
                value = Settings.Global.getInt(mResolver, PARAM_DAP_VL_AMOUNT, DAP_VL_AMOUNT_DEFAULT);
                break;
            case CMD_DAP_DE_ENABLE:
                value = Settings.Global.getInt(mResolver, PARAM_DAP_DE_ENABLE, DAP_DE_DEFAULT);
                break;
            case CMD_DAP_SURROUND_ENABLE:
                value = Settings.Global.getInt(mResolver, PARAM_DAP_SURROUND_ENABLE, DAP_SURROUND_DEFAULT);
                break;
            case CMD_DAP_SURROUND_BOOST:
                value = Settings.Global.getInt(mResolver, PARAM_DAP_SURROUND_BOOST, DAP_SURROUND_BOOST_DEFAULT);
                break;
            case CMD_DAP_DE_AMOUNT:
                value = Settings.Global.getInt(mResolver, PARAM_DAP_DE_AMOUNT, DAP_DE_AMOUNT_DEFAULT);
                break;
            case CMD_DAP_POST_GAIN:
                value = Settings.Global.getInt(mResolver, PARAM_DAP_POST_GAIN, DAP_POST_GAIN_DEFAULT);
                break;
            case CMD_DAP_GEQ_ENABLE:
                value = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_ENABLE, DAP_GEQ_DEFAULT);
                break;
            case SUBCMD_DAP_GEQ_BAND1:
                param = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_ENABLE, DAP_GEQ_DEFAULT);
                if (param == DAP_GEQ_USER)
                    value = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_BAND1, DAP_GEQ_GAIN_DEFAULT);
                else
                    value = getDapParamInternal(id);
                break;
            case SUBCMD_DAP_GEQ_BAND2:
                param = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_ENABLE, DAP_GEQ_DEFAULT);
                if (param == DAP_GEQ_USER)
                    value = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_BAND2, DAP_GEQ_GAIN_DEFAULT);
                else
                    value = getDapParamInternal(id);
                break;
            case SUBCMD_DAP_GEQ_BAND3:
                param = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_ENABLE, DAP_GEQ_DEFAULT);
                if (param == DAP_GEQ_USER)
                    value = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_BAND3, DAP_GEQ_GAIN_DEFAULT);
                else
                    value = getDapParamInternal(id);
                break;
            case SUBCMD_DAP_GEQ_BAND4:
                param = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_ENABLE, DAP_GEQ_DEFAULT);
                if (param == DAP_GEQ_USER)
                    value = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_BAND4, DAP_GEQ_GAIN_DEFAULT);
                else
                    value = getDapParamInternal(id);
                break;
            case SUBCMD_DAP_GEQ_BAND5:
                param = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_ENABLE, DAP_GEQ_DEFAULT);
                if (param == DAP_GEQ_USER)
                    value = Settings.Global.getInt(mResolver, PARAM_DAP_GEQ_BAND5, DAP_GEQ_GAIN_DEFAULT);
                else
                    value = getDapParamInternal(id);
                break;
        }
        return value;
    }

    public void enableBoxLineOutAudio(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_BOX_LINE_OUT_ON);
        } else {
            mAudioManager.setParameters(PARA_BOX_LINE_OUT_OFF);
        }
    }

    public void enableBoxHdmiAudio(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_BOX_HDMI_ON);
        } else {
            mAudioManager.setParameters(PARA_BOX_HDMI_OFF);
        }
    }

    public void enableTvSpeakerAudio(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_TV_SPEAKER_ON);
        } else {
            mAudioManager.setParameters(PARA_TV_SPEAKER_OFF);
        }
    }

    public void enableTvArcAudio(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_TV_ARC_ON);
        } else {
            mAudioManager.setParameters(PARA_TV_ARC_OFF);
        }
    }

    public void setARCLatency(int value) {
        if (value > TV_ARC_LATENCY_MAX)
            value = TV_ARC_LATENCY_MAX;
        else if (value < TV_ARC_LATENCY_MIN)
            value = TV_ARC_LATENCY_MIN;
        mSystenControl.setProperty(PROPERTY_LOCAL_ARC_LATENCY, ""+(value*1000));
    }

    public void setAudioOutputLatency(int value) {
        if (value > AUDIO_OUTPUT_LATENCY_MAX)
            value = AUDIO_OUTPUT_LATENCY_MAX;
        else if (value < AUDIO_OUTPUT_LATENCY_MIN)
            value = AUDIO_OUTPUT_LATENCY_MIN;
        mAudioManager.setParameters(HAL_FIELD_AUDIO_OUTPUT_LATENCY + value);
    }

    public void setVirtualSurround (int value) {
        if (value == VIRTUAL_SURROUND_ON) {
            mAudioManager.setParameters(PARA_VIRTUAL_SURROUND_ON);
        } else {
            mAudioManager.setParameters(PARA_VIRTUAL_SURROUND_OFF);
        }
    }

    public void setSoundOutputStatus (int mode) {
        switch (mode) {
            case SOUND_OUTPUT_DEVICE_SPEAKER:
                enableTvSpeakerAudio(true);
                enableTvArcAudio(false);
                break;
            case SOUND_OUTPUT_DEVICE_ARC:
                enableTvSpeakerAudio(false);
                enableTvArcAudio(true);
                break;
        }
    }

    public void initSoundParametersAfterBoot() {
        final boolean istv = mSystenControl.getPropertyBoolean("ro.vendor.platform.has.tvuimode", false);
        if (!istv) {
            final int boxlineout = Settings.Global.getInt(mResolver, BOX_LINE_OUT, BOX_LINE_OUT_OFF);
            enableBoxLineOutAudio(boxlineout == BOX_LINE_OUT_ON);
            final int boxhdmi = Settings.Global.getInt(mResolver, BOX_HDMI, BOX_HDMI_ON);
            enableBoxHdmiAudio(boxhdmi == BOX_HDMI_ON);
        } else {
            final int virtualsurround = Settings.Global.getInt(mResolver, VIRTUAL_SURROUND, VIRTUAL_SURROUND_OFF);
            setVirtualSurround(virtualsurround);
            /*int device = mAudioManager.getDevicesForStream(AudioManager.STREAM_MUSIC);
            if ((device & AudioSystem.DEVICE_OUT_SPEAKER) != 0) {
                final int soundoutput = Settings.Global.getInt(mResolver, SOUND_OUTPUT_DEVICE, SOUND_OUTPUT_DEVICE_SPEAKER);
                setSoundOutputStatus(soundoutput);
            }*/
        }
    }

    public void resetSoundParameters() {
        final boolean istv = mSystenControl.getPropertyBoolean("ro.vendor.platform.has.tvuimode", false);
        if (!istv) {
            enableBoxLineOutAudio(false);
            enableBoxHdmiAudio(false);
        } else {
            enableTvSpeakerAudio(false);
            enableTvArcAudio(false);
            setVirtualSurround(VIRTUAL_SURROUND_OFF);
            setSoundOutputStatus(SOUND_OUTPUT_DEVICE_SPEAKER);
        }
    }

    private void applyDapAudioEffectByPlayEmptyTrack() {
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

