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

package com.droidlogic.tv.settings.pqsettings;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.text.TextUtils;
import android.util.Log;
import android.os.SystemProperties;
import android.provider.Settings;
import android.widget.Toast;

import com.droidlogic.tv.settings.R;
import com.droidlogic.tv.settings.SettingsConstant;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.TvInSignalInfo;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.TvDataBaseManager;
import android.media.tv.TvContract;

import com.droidlogic.tv.settings.TvSettingsActivity;

public class PQSettingsManager {
    public static final String TAG = "PQSettingsManager";
    public static final String CURRENT_DEVICE_ID = "current_device_id";
    public static final String CURRENT_CHANNEL_ID = "current_channel_id";
    public static final String TV_CURRENT_DEVICE_ID = "tv_current_device_id";

    public static final String KEY_PICTURE                          = "picture";
    public static final String KEY_PICTURE_MODE                     = "picture_mode";
    public static final String KEY_BRIGHTNESS                       = "brightness";
    public static final String KEY_CONTRAST                         = "contrast";
    public static final String KEY_COLOR                            = "color";
    public static final String KEY_SHARPNESS                        = "sharpness";
    public static final String KEY_BACKLIGHT                        = "backlight";
    public static final String KEY_TONE                             = "tone";
    public static final String KEY_COLOR_TEMPERATURE                = "color_temperature";
    public static final String KEY_ASPECT_RATIO                     = "aspect_ratio";
    public static final String KEY_DNR                              = "dnr";
    public static final String KEY_3D_SETTINGS                      = "settings_3d";

    public static final String STATUS_STANDARD                      = "standard";
    public static final String STATUS_VIVID                         = "vivid";
    public static final String STATUS_SOFT                          = "soft";
    public static final String STATUS_SPORT                         = "sport";
    public static final String STATUS_MONITOR                    = "monitor";
    public static final String STATUS_GAME                          = "game";
    public static final String STATUS_USER                          = "user";
    public static final String STATUS_WARM                          = "warm";
    public static final String STATUS_MUSIC                          = "music";
    public static final String STATUS_NEWS                          = "news";
    public static final String STATUS_MOVIE                          = "movie";
    public static final String STATUS_COOL                          = "cool";
    public static final String STATUS_ON                            = "on";
    public static final String STATUS_OFF                           = "off";
    public static final String STATUS_AUTO                          = "auto";
    public static final String STATUS_4_TO_3                        = "4:3";
    public static final String STATUS_PANORAMA                      = "panorama";
    public static final String STATUS_FULL_SCREEN                   = "full_screen";
    public static final String STATUS_MEDIUM                        = "medium";
    public static final String STATUS_HIGH                          = "high";
    public static final String STATUS_LOW                           = "low";
    public static final String STATUS_3D_LR_MODE                    = "left right mode";
    public static final String STATUS_3D_RL_MODE                    = "right left mode";
    public static final String STATUS_3D_UD_MODE                    = "up down mode";
    public static final String STATUS_3D_DU_MODE                    = "down up mode";
    public static final String STATUS_3D_TO_2D                      = "3D to 2D";
    public static final String STATUS_PCM                            = "pcm";
    public static final String STATUS_STEREO                         = "stereo";
    public static final String STATUS_LEFT_CHANNEL                  = "left channel";
    public static final String STATUS_RIGHT_CHANNEL                 = "right channel";
    public static final String STATUS_RAW                            = "raw";

    public static final int PERCENT_INCREASE                        = 1;
    public static final int PERCENT_DECREASE                        = -1;
    public static String currentTag = null;

    private Resources mResources;
    private Context mContext;
    private SystemControlManager mSystemControlManager;
    private TvControlManager mTvControlManager;
    private TvDataBaseManager mTvDataBaseManager;
    private TvControlManager.SourceInput mTvSourceInput;
    private TvControlManager.SourceInput_Type mVirtualTvSource;
    private TvControlManager.SourceInput_Type mTvSource;
    private int mDeviceId;
    private long mChannelId;
    private int mVideoStd;
    private TvSettingsActivity mTvSettingsActivity;

    public PQSettingsManager (Context context) {
        mContext = context;
        mTvSettingsActivity = (TvSettingsActivity)context;
        mDeviceId = mTvSettingsActivity.getIntent().getIntExtra(TV_CURRENT_DEVICE_ID, -1);
        mChannelId = mTvSettingsActivity.getIntent().getLongExtra(CURRENT_CHANNEL_ID, -1);
        mResources = mContext.getResources();
        mSystemControlManager = SystemControlManager.getInstance();
        if (SettingsConstant.needDroidlogicTvFeature(mContext)) {
            ChannelInfo currentChannel;
            if (mTvControlManager == null) {
                mTvControlManager = TvControlManager.getInstance();
            }
            mTvDataBaseManager = new TvDataBaseManager(context);
            mTvSource = DroidLogicTvUtils.parseTvSourceTypeFromDeviceId(mDeviceId);
            mTvSourceInput = DroidLogicTvUtils.parseTvSourceInputFromDeviceId(mDeviceId);
            mVirtualTvSource = mTvSource;

            if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
                Log.d(TAG, "channelId: " + mChannelId);
                currentChannel = mTvDataBaseManager.getChannelInfo(TvContract.buildChannelUri(mChannelId));
                if (currentChannel != null) {
                    mVideoStd = currentChannel.getVideoStd();
                    mTvSource = DroidLogicTvUtils.parseTvSourceTypeFromSigType(DroidLogicTvUtils.getSigType(currentChannel));
                    mTvSourceInput = DroidLogicTvUtils.parseTvSourceInputFromSigType(DroidLogicTvUtils.getSigType(currentChannel));
                    Log.d(TAG, "currentChannel != null");
                } else {
                    mVideoStd = -1;
                    Log.d(TAG, "currentChannel == null");
                }
                if (mVirtualTvSource == mTvSource) {//no channels in adtv input, DTV for default.
                    mTvSource = TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV;
                    mTvSourceInput = TvControlManager.SourceInput.DTV;
                    Log.d(TAG, "no channels in adtv input, DTV for default.");
                }
            }
        }
        Log.d(TAG, "mDeviceId: " + mDeviceId);
    }

    static public boolean CanDebug() {
        return SystemProperties.getBoolean("sys.pqsetting.debug", false);
    }

    public static final int PIC_STANDARD = 0;
    public static final int PIC_VIVID = 1;
    public static final int PIC_SOFT = 2;
    public static final int PIC_USER = 3;
    public static final int PIC_MOVIE = 4;
    public static final int PIC_MONITOR = 6;
    public static final int PIC_GAME = 7;
    public static final int PIC_SPORT = 8;

    public String getPictureModeStatus () {
        int pictureModeIndex = mSystemControlManager.GetPQMode();
        if (CanDebug()) Log.d(TAG, "getPictureModeStatus : " + pictureModeIndex);
        switch (pictureModeIndex) {
            case PIC_STANDARD:
                return STATUS_STANDARD;
            case PIC_VIVID:
                return STATUS_VIVID;
            case PIC_SOFT:
                return STATUS_SOFT;
            case PIC_USER:
                return STATUS_USER;
            case PIC_MONITOR:
                return STATUS_MONITOR;
            case PIC_SPORT:
                return STATUS_SPORT;
            case PIC_MOVIE:
                return STATUS_MOVIE;
            case PIC_GAME:
                return STATUS_GAME;
            default:
                return STATUS_STANDARD;
        }
    }

    public int getBrightnessStatus () {
        int value = mSystemControlManager.GetBrightness();
        if (CanDebug()) Log.d(TAG, "getBrightnessStatus : " + value);
        return value;
    }

    public int getContrastStatus () {
        int value = mSystemControlManager.GetContrast();
        if (CanDebug()) Log.d(TAG, "getContrastStatus : " + value);
        return value;
    }

    public int getColorStatus () {
        int value = mSystemControlManager.GetSaturation();
        if (CanDebug()) Log.d(TAG, "getColorStatus : " + value);
        return value;
    }

    public int getSharpnessStatus () {
        int value = mSystemControlManager.GetSharpness();
        if (CanDebug()) Log.d(TAG, "getSharpnessStatus : " + value);
        return value;
    }

    public int getToneStatus () {
        int value = mSystemControlManager.GetHue();
        if (CanDebug()) Log.d(TAG, "getTintStatus : " + value);
        return value;
    }

    public int getAspectRatioStatus () {
        int itemPosition = mSystemControlManager.GetDisplayMode(SystemControlManager.SourceInput.XXXX.toInt());
        if (CanDebug()) Log.d(TAG, "getAspectRatioStatus:" + itemPosition);
        if (itemPosition == SystemControlManager.Display_Mode.DISPLAY_MODE_MODE43.toInt())
            return 1;
        else if (itemPosition == SystemControlManager.Display_Mode.DISPLAY_MODE_FULL.toInt())
            return 2;
        else if (itemPosition == SystemControlManager.Display_Mode.DISPLAY_MODE_169.toInt())
            return 3;
        else
            return 0;
    }

    public int getColorTemperatureStatus () {
        int itemPosition = mSystemControlManager.GetColorTemperature();
        if (CanDebug()) Log.d(TAG, "getColorTemperatureStatus : " + itemPosition);
        return itemPosition;
    }

    public int getDnrStatus () {
        int itemPosition = mSystemControlManager.GetNoiseReductionMode();
        if (CanDebug()) Log.d(TAG, "getDnrStatus : " + itemPosition);
        return itemPosition;
    }

    public void setPictureMode (String mode) {
        if (CanDebug()) Log.d(TAG, "setPictureMode : " + mode);
        if (mode.equals(STATUS_STANDARD)) {
            mSystemControlManager.SetPQMode(PIC_STANDARD, 1, 0);
        } else if (mode.equals(STATUS_VIVID)) {
            mSystemControlManager.SetPQMode(PIC_VIVID, 1, 0);
        } else if (mode.equals(STATUS_SOFT)) {
            mSystemControlManager.SetPQMode(PIC_SOFT, 1, 0);
        } else if (mode.equals(STATUS_USER)) {
            mSystemControlManager.SetPQMode(PIC_USER, 1, 0);
        } else if (mode.equals(STATUS_MONITOR)) {
            mSystemControlManager.SetPQMode(PIC_MONITOR, 1, 0);
        } else if (mode.equals(STATUS_SPORT)) {
            mSystemControlManager.SetPQMode(PIC_SPORT, 1, 0);
        } else if (mode.equals(STATUS_MOVIE)) {
            mSystemControlManager.SetPQMode(PIC_MOVIE, 1, 0);
        } else if (mode.equals(STATUS_GAME)) {
            mSystemControlManager.SetPQMode(PIC_GAME, 1, 0);
        }
    }

    public void setBrightness (int step) {
        if (CanDebug())  Log.d(TAG, "setBrightness step : " + step );
        int PQMode = mSystemControlManager.GetPQMode();
        if (PQMode == 3) {
            int tmp = mSystemControlManager.GetBrightness();
            mSystemControlManager.SetBrightness(tmp + step, 1);
        } else {
            mSystemControlManager.SetBrightness(setPictureUserMode(KEY_BRIGHTNESS) + step, 1);
        }
    }

    public void setContrast (int step) {
        if (CanDebug())  Log.d(TAG, "setContrast step : " + step);
        int PQMode = mSystemControlManager.GetPQMode();
        if (PQMode == 3) {
            int tmp = mSystemControlManager.GetContrast();
            mSystemControlManager.SetContrast(tmp + step, 1);
        } else {
            mSystemControlManager.SetContrast(setPictureUserMode(KEY_CONTRAST) + step, 1);
        }
    }

    public void setColor (int step) {
        if (CanDebug())  Log.d(TAG, "setColor step : " + step);
        int PQMode = mSystemControlManager.GetPQMode();
        if (PQMode == 3) {
            int tmp = mSystemControlManager.GetSaturation();
            mSystemControlManager.SetSaturation(tmp + step, 1);
        } else {
            mSystemControlManager.SetSaturation(setPictureUserMode(KEY_COLOR) + step, 1);
        }
    }

    public void setSharpness (int step) {
        if (CanDebug())  Log.d(TAG, "setSharpness step : " + step);
        int PQMode = mSystemControlManager.GetPQMode();
        if (PQMode == 3) {
            int tmp = mSystemControlManager.GetSharpness();
            mSystemControlManager.SetSharpness(tmp + step , 1 , 1);
        } else
            mSystemControlManager.SetSharpness(setPictureUserMode(KEY_SHARPNESS) + step, 1 , 1);
    }

    public void setTone(int step) {
        if (CanDebug())  Log.d(TAG, "setTint step : " + step);
        int PQMode = mSystemControlManager.GetPQMode();
        if (PQMode == 3) {
            int tmp = mSystemControlManager.GetHue();
            mSystemControlManager.SetHue(tmp + step, 1);
        } else {
            mSystemControlManager.SetHue(setPictureUserMode(KEY_TONE) + step, 1);
        }
    }

    public String getVideoStd () {
        if (mVideoStd != -1) {
            switch (mVideoStd) {
                case 1:
                    return mResources.getString(R.string.pal);
                case 2:
                    return mResources.getString(R.string.ntsc);
                default:
                    return mResources.getString(R.string.pal);
            }
        }
        return null;
    }

    public boolean isNtscSignalOrNot() {
        if (!SettingsConstant.needDroidlogicTvFeature(mContext)) {
            return false;
        }
        TvInSignalInfo info;
        String videoStd = getVideoStd();
        if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV
            && videoStd != null && videoStd.equals(mResources.getString(R.string.ntsc))) {
            if (mTvControlManager == null) {
                mTvControlManager = TvControlManager.getInstance();
            }
            info = mTvControlManager.GetCurrentSignalInfo();
            if (info.sigStatus == TvInSignalInfo.SignalStatus.TVIN_SIG_STATUS_STABLE) {
                if (CanDebug()) Log.d(TAG, "ATV NTSC mode signal is stable, show Tint");
                return true;
            }
        } else if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_AV) {
            if (mTvControlManager == null) {
                mTvControlManager = TvControlManager.getInstance();
            }
            info = mTvControlManager.GetCurrentSignalInfo();
            if (info.sigStatus == TvInSignalInfo.SignalStatus.TVIN_SIG_STATUS_STABLE) {
                String[] strings = info.sigFmt.toString().split("_");
                if (strings[4].contains("NTSC")) {
                    if (CanDebug()) Log.d(TAG, "AV NTSC mode signal is stable, show Tint");
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isNtscSignal() {
        final int DEFAULT_VALUE = -1;
        final int NTSC = 2;
        return Settings.System.getInt(mContext.getContentResolver(), "curent_video_std", DEFAULT_VALUE) == NTSC;
    }

    public void setAspectRatio(int mode) {
        if (CanDebug()) Log.d(TAG, "setAspectRatio:" + mode);
        int source = TvControlManager.SourceInput.XXXX.toInt();
        if (mode == 0) {
            mSystemControlManager.SetDisplayMode(source, SystemControlManager.Display_Mode.DISPLAY_MODE_NORMAL, 1);
        } else if (mode == 1) {
            mSystemControlManager.SetDisplayMode(source, SystemControlManager.Display_Mode.DISPLAY_MODE_MODE43, 1);
        } else if (mode == 2) {
            mSystemControlManager.SetDisplayMode(source, SystemControlManager.Display_Mode.DISPLAY_MODE_FULL, 1);
        } else if (mode == 3) {
            mSystemControlManager.SetDisplayMode(source, SystemControlManager.Display_Mode.DISPLAY_MODE_169, 1);
        }
    }

    // 0 1 2 ~ standard warm cool
    public void setColorTemperature(int mode) {
        if (CanDebug())  Log.d(TAG, "setColorTemperature : " + mode);
        mSystemControlManager.SetColorTemperature(mode, 1);
    }

    //0 1 2 3 4 ~ off low medium high auto
    public void setDnr (int mode) {
        if (CanDebug()) Log.d(TAG, "setDnr : "+ mode);
        mSystemControlManager.SetNoiseReductionMode(mode, 1);
    }

    private int setPictureUserMode(String key) {
        if (CanDebug()) Log.d(TAG, "setPictureUserMode : "+ key);
        int brightness = mSystemControlManager.GetBrightness();
        int contrast = mSystemControlManager.GetContrast();
        int color = mSystemControlManager.GetSaturation();
        int sharpness = mSystemControlManager.GetSharpness();
        int tint = -1;
        tint = mSystemControlManager.GetHue();
        int ret = -1;

        switch (mSystemControlManager.GetPQMode()) {
            case PIC_STANDARD:
            case PIC_VIVID:
            case PIC_SOFT:
            case PIC_MONITOR:
            case PIC_SPORT:
            case PIC_MOVIE:
            case PIC_GAME:
                setPictureMode(STATUS_USER);
                break;
        }

        if (CanDebug()) Log.d(TAG, " brightness=" + brightness + " contrast=" + contrast + " color=" + color + " sharp=" + sharpness);
        if (!key.equals(KEY_BRIGHTNESS))
            mSystemControlManager.SetBrightness(brightness, 1);
        else
            ret = brightness;

        if (!key.equals(KEY_CONTRAST))
            mSystemControlManager.SetContrast(contrast, 1);
        else
            ret = contrast;

        if (!key.equals(KEY_COLOR))
            mSystemControlManager.SetSaturation(color, 1);
        else
            ret = color;

        if (!key.equals(KEY_SHARPNESS))
            mSystemControlManager.SetSharpness(sharpness, 1 , 1);
        else
            ret = sharpness;

        if (!key.equals(KEY_TONE))
            mSystemControlManager.SetHue(tint, 1);
        else
            ret = tint;
        return ret;
    }

    public void setBacklightValue (int value) {
        if (CanDebug()) Log.d(TAG, "setBacklightValue : "+ value);
        mSystemControlManager.SetBacklight(getBacklightStatus() + value, 1);
    }

    public int getBacklightStatus () {
        int value = mSystemControlManager.GetBacklight();
        if (CanDebug()) Log.d(TAG, "getBacklightStatus : " + value);
        return value;
    }

    private static final int AUTO_RANGE = 0;
    private static final int FULL_RANGE = 1;
    private static final int LIMIT_RANGE = 2;

    public void setHdmiColorRangeValue (int value) {
        if (CanDebug()) Log.d(TAG, "setHdmiColorRangeValue : "+ value);
        TvControlManager.HdmiColorRangeMode hdmicolor = TvControlManager.HdmiColorRangeMode.AUTO_RANGE;
        switch (value) {
            case AUTO_RANGE :
                hdmicolor = TvControlManager.HdmiColorRangeMode.AUTO_RANGE;
                break;
            case FULL_RANGE :
                hdmicolor = TvControlManager.HdmiColorRangeMode.FULL_RANGE;
                break;
            case LIMIT_RANGE :
                hdmicolor = TvControlManager.HdmiColorRangeMode.LIMIT_RANGE;
                break;
            default:
                hdmicolor = TvControlManager.HdmiColorRangeMode.AUTO_RANGE;
                break;
        }
        if (mTvControlManager == null) {
            mTvControlManager = TvControlManager.getInstance();
        }
        if (mTvControlManager != null) {
            mTvControlManager.SetHdmiColorRangeMode(hdmicolor);
        }
    }

    public int getHdmiColorRangeStatus () {
        int value = 0;
        if (mTvControlManager == null) {
            mTvControlManager = TvControlManager.getInstance();
        }
        if (mTvControlManager != null) {
            value = mTvControlManager.GetHdmiColorRangeMode();
        }
        if (CanDebug()) Log.d(TAG, "getHdmiColorRangeStatus : " + value);
        return value;
    }

    public boolean isHdmiSource() {
        if (mTvSourceInput != null) {
            return mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_HDMI;
        }
        return false;
    }

    public boolean isAvSource() {
        if (mTvSourceInput != null) {
            return mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_AV;
        }
        return false;
    }
}
