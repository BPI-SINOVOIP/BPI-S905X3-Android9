/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.settings;

import android.R.integer;
import android.app.ActivityManager;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
//import android.content.pm.IPackageDataObserver;
import android.content.res.Resources;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;
import android.os.Bundle;
import android.os.SystemClock;
import android.os.SystemProperties;

import java.util.ArrayList;
import java.util.Locale;
import java.util.HashMap;
import java.lang.System;
import java.lang.reflect.Method;

import android.media.tv.TvInputInfo;
import com.droidlogic.app.SystemControlManager;

import android.media.AudioManager;
//import android.media.AudioSystem;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.TvControlDataManager;
import com.droidlogic.app.tv.TvInSignalInfo;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvMultilingualText;
import com.droidlogic.app.tv.TvScanConfig;
import com.droidlogic.tvinput.R;

public class SettingsManager {
    public static final String TAG = "SettingsManager";

    public static final String KEY_PICTURE                          = "picture";
    public static final String KEY_PICTURE_MODE                     = "picture_mode";
    public static final String KEY_BRIGHTNESS                       = "brightness";
    public static final String KEY_CONTRAST                         = "contrast";
    public static final String KEY_COLOR                            = "color";
    public static final String KEY_SHARPNESS                        = "sharpness";
    public static final String KEY_BACKLIGHT                        = "backlight";
    public static final String KEY_TINT                             = "tint";
    public static final String KEY_COLOR_TEMPERATURE                = "color_temperature";
    public static final String KEY_ASPECT_RATIO                     = "aspect_ratio";
    public static final String KEY_DNR                              = "dnr";
    public static final String KEY_3D_SETTINGS                      = "settings_3d";

    public static final String KEY_SOUND                            = "sound";
    public static final String KEY_SOUND_MODE                       = "sound_mode";
    public static final String KEY_TREBLE                           = "treble";
    public static final String KEY_BASS                             = "bass";
    public static final String KEY_BALANCE                          = "balance";
    public static final String KEY_SPDIF                            = "spdif";
    public static final String KEY_VIRTUAL_SURROUND                 = "virtual_surround";
    public static final String KEY_SURROUND                         = "surround";
    public static final String KEY_DIALOG_CLARITY                   = "dialog_clarity";
    public static final String KEY_BASS_BOOST                       = "bass_boost";

    public static final String KEY_CHANNEL                          = "channel";
    public static final String KEY_AUIDO_TRACK                      = "audio_track";
    public static final String KEY_SOUND_CHANNEL                    = "sound_channel";
    public static final String KEY_CHANNEL_INFO                     = "channel_info";
    public static final String KEY_COLOR_SYSTEM                     = "color_system";
    public static final String KEY_SOUND_SYSTEM                     = "sound_system";
    public static final String KEY_VOLUME_COMPENSATE                = "volume_compensate";
    public static final String KEY_FINE_TUNE                        = "fine_tune";
    public static final String KEY_MANUAL_SEARCH                    = "manual_search";
    public static final String KEY_AUTO_SEARCH                      = "auto_search";
    public static final String KEY_CHANNEL_EDIT                     = "channel_edit";
    public static final String KEY_SWITCH_CHANNEL                   = "switch_channel";
    public static final String KEY_MTS                              ="mts";

    public static final String KEY_SETTINGS                         = "settings";
    public static final String KEY_DTV_TYPE                         = "dtv_type";
    public static final String KEY_SLEEP_TIMER                      = "sleep_timer";
    public static final String KEY_MENU_TIME                        = "menu_time";
    public static final String KEY_STARTUP_SETTING                  = "startup_setting";
    public static final String KEY_DYNAMIC_BACKLIGHT                = "dynamic_backlight";
    public static final String KEY_RESTORE_FACTORY                  = "restore_factory";
    public static final String KEY_DEFAULT_LANGUAGE                 = "default_language";
    public static final String KEY_SUBTITLE_SWITCH                  = "sub_switch";
    public static final String KEY_AD_SWITCH                        = "ad_switch";
    public static final String KEY_AD_MIX                           = "ad_mix_level";
    public static final String KEY_AD_LIST                          = "ad_list";
    public static final String KEY_HDMI20                           = "hdmi20";
    public static final String KEY_FBC_UPGRADE                      ="fbc_upgrade";

    public static final String STATUS_STANDARD                      = "standard";
    public static final String STATUS_VIVID                         = "vivid";
    public static final String STATUS_SOFT                          = "soft";
    public static final String STATUS_MONITOR                       = "monitor";
    public static final String STATUS_USER                          = "user";
    public static final String STATUS_WARM                          = "warm";
    public static final String STATUS_MUSIC                         = "music";
    public static final String STATUS_NEWS                          = "news";
    public static final String STATUS_MOVIE                         = "movie";
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
    public static final String STATUS_PCM                           = "pcm";
    public static final String STATUS_STEREO                        = "stereo";
    public static final String STATUS_LEFT_CHANNEL                  = "left channel";
    public static final String STATUS_RIGHT_CHANNEL                 = "right channel";
    public static final String STATUS_RAW                           = "raw";

    public static final String STATUS_DEFAULT_PERCENT               = "50%";
    public static final double STATUS_DEFAUT_FREQUENCY              = 44250000;
    public static final int PERCENT_INCREASE                        = 1;
    public static final int PERCENT_DECREASE                        = -1;
    public static final int DEFAULT_SLEEP_TIMER                     = 0;
    public static final int DEFUALT_MENU_TIME                       = 10;
    public static final String LAUNCHER_NAME                        = "com.android.launcher";
    public static final String LAUNCHER_ACTIVITY                    = "com.android.launcher2.Launcher";
    public static final String TV_NAME                              = "com.droidlogic.tv";
    public static final String TV_ACTIVITY                          = "com.droidlogic.tv.DroidLogicTv";

    public static final String STRING_ICON                          = "icon";
    public static final String STRING_NAME                          = "name";
    public static final String STRING_STATUS                        = "status";
    public static final String STRING_PRIVATE                       = "private";

    public static final String ATSC_TV_SEARCH_SYS                   = "atsc_tv_search_sys";
    public static final String ATSC_TV_SEARCH_SOUND_SYS             = "atsc_tv_search_sound_sys";
    public static String currentTag = null;
    public static final int STREAM_MUSIC = 3;

    private Context mContext;
    private Resources mResources;
    private String mInputId;
    private TvControlManager.SourceInput_Type mVirtualTvSource;
    private TvControlManager.SourceInput_Type mTvSource;
    private TvControlManager.SourceInput mTvSourceInput;
    private ChannelInfo currentChannel;
    private TvControlManager mTvControlManager;
    private TvControlDataManager mTvControlDataManager = null;
    private TvDataBaseManager mTvDataBaseManager;
    private SystemControlManager mSystemControlManager;
    private ArrayList<ChannelInfo> videoChannelList;
    private ArrayList<ChannelInfo> radioChannelList;
    private boolean isRadioChannel = false;
    private int mResult = DroidLogicTvUtils.RESULT_OK;

    public SettingsManager (Context context, Intent intent) {
        mContext = context;
        mTvDataBaseManager = new TvDataBaseManager(mContext);
        mSystemControlManager = SystemControlManager.getInstance();

        setCurrentChannelData(intent);

        mTvControlManager = TvControlManager.getInstance();
        mResources = mContext.getResources();
        mTvControlDataManager = TvControlDataManager.getInstance(mContext);
    }

    public void setCurrentChannelData(Intent intent) {
        mInputId = intent.getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
        DroidLogicTvUtils.saveInputId(mContext, mInputId);
        isRadioChannel = intent.getBooleanExtra(DroidLogicTvUtils.EXTRA_IS_RADIO_CHANNEL, false);

        int deviceId = intent.getIntExtra(DroidLogicTvUtils.EXTRA_CHANNEL_DEVICE_ID, -1);
        if (deviceId == -1)//for TIF compatible
            deviceId = DroidLogicTvUtils.getHardwareDeviceId(mInputId);

        mTvSource = DroidLogicTvUtils.parseTvSourceTypeFromDeviceId(deviceId);
        mTvSourceInput = DroidLogicTvUtils.parseTvSourceInputFromDeviceId(deviceId);
        mVirtualTvSource = mTvSource;

        if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            long channelId = intent.getLongExtra(DroidLogicTvUtils.EXTRA_CHANNEL_NUMBER, -1);
            currentChannel = mTvDataBaseManager.getChannelInfo(TvContract.buildChannelUri(channelId));
            if (currentChannel != null) {
                mTvSource = DroidLogicTvUtils.parseTvSourceTypeFromSigType(DroidLogicTvUtils.getSigType(currentChannel));
                mTvSourceInput = DroidLogicTvUtils.parseTvSourceInputFromSigType(DroidLogicTvUtils.getSigType(currentChannel));
            }
            if (mVirtualTvSource == mTvSource) {//no channels in adtv input, DTV for default.
                mTvSource = TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV;
                mTvSourceInput = TvControlManager.SourceInput.DTV;
            }
        }
        Log.d(TAG, "init SettingsManager curSource=" + mTvSource + " isRadio=" + isRadioChannel);
        Log.d(TAG, "curVirtualSource=" + mVirtualTvSource);

        if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV
            || mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV) {
            long channelId = intent.getLongExtra(DroidLogicTvUtils.EXTRA_CHANNEL_NUMBER, -1);
            currentChannel = mTvDataBaseManager.getChannelInfo(TvContract.buildChannelUri(channelId));
            if (currentChannel != null) {
                Log.d(TAG, "current channel is: ");
                currentChannel.print();
            } else {
                Log.d(TAG, "current channel is null!!");
            }
        }
        new Thread(loadChannelsRunnable).start();
    }

    public void setTag (String tag) {
        currentTag = tag;
    }

    public String getTag () {
        return currentTag;
    }

    public TvControlManager.SourceInput_Type getCurentTvSource () {
        return mTvSource;
    }

    public TvControlManager.SourceInput_Type getCurentVirtualTvSource () {
        return mVirtualTvSource;
    }

    private int parseChannelEditType () {
        if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV)
            return ChannelEdit.TYPE_ATV;
        else if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV) {
            if (!isRadioChannel)
                return ChannelEdit.TYPE_DTV_TV;
            else
                return ChannelEdit.TYPE_DTV_RADIO;
        }

        return ChannelEdit.TYPE_ATV;
    }

    public void setActivityResult(int result) {
        mResult = result;
    }

    public int getActivityResult() {
        return mResult;
    }

    public String getStatus (String key) {
        Log.d(TAG, " current screen is :" + currentTag + ", item is :" + key);
        //picture
        if (!TextUtils.isEmpty(key)) {
            if (key.equals(KEY_PICTURE_MODE)) {
                return getPictureModeStatus();
            } else if (key.equals(KEY_BRIGHTNESS)) {
                return getBrightnessStatus();
            } else if (key.equals(KEY_CONTRAST)) {
                return getContrastStatus();
            } else if (key.equals(KEY_COLOR)) {
                return getColorStatus();
            } else if (key.equals(KEY_SHARPNESS)) {
                return getSharpnessStatus();
            } else if (key.equals(KEY_BACKLIGHT)) {
                return getBacklightStatus();
            } else if (key.equals(KEY_TINT)) {
                return getTintStatus();
            } else if (key.equals(KEY_COLOR_TEMPERATURE)) {
                return getColorTemperatureStatus();
            } else if (key.equals(KEY_ASPECT_RATIO)) {
                return getAspectRatioStatus();
            } else if (key.equals(KEY_DNR)) {
                return getDnrStatus();
            } else if (key.equals(KEY_3D_SETTINGS)) {
            }
            //sound
            else if (key.equals(KEY_SOUND_MODE)) {
                return getSoundModeStatus();
            } else if (key.equals(KEY_TREBLE)) {
                return getTrebleStatus();
            } else if (key.equals(KEY_BASS)) {
                return getBassStatus();
            } else if (key.equals(KEY_BALANCE)) {
                return getBalanceStatus();
            } else if (key.equals(KEY_SPDIF)) {
                return getSpdifStatus();
            } else if (key.equals(KEY_SURROUND)) {
                return getSurroundStatus();
            } else if (key.equals(KEY_VIRTUAL_SURROUND)) {
                return getVirtualSurroundLevel();
            } else if (key.equals(KEY_DIALOG_CLARITY)) {
                return getDialogClarityStatus();
            } else if (key.equals(KEY_BASS_BOOST)) {
                return getBassBoostStatus();
            }
            //channel
            else if (key.equals(KEY_CHANNEL_INFO)) {
                return getCurrentChannelStatus();
            } else if (key.equals(KEY_AUIDO_TRACK)) {
                return getAudioTrackStatus();
            } else if (key.equals(KEY_SOUND_CHANNEL)) {
                return getSoundChannelStatus();
            } else if (key.equals(KEY_DEFAULT_LANGUAGE)) {
                return getDefaultLanStatus();
            } else if (key.equals(KEY_SUBTITLE_SWITCH)) {
                return getSubtitleSwitchStatus();
            } else if (key.equals(KEY_COLOR_SYSTEM)) {
                return getColorSystemStatus();
            } else if (key.equals(KEY_SOUND_SYSTEM)) {
                return getSoundSystemStatus();
            } else if (key.equals(KEY_VOLUME_COMPENSATE)) {
                return getVolumeCompensateStatus();
            } else if (key.equals(KEY_SWITCH_CHANNEL)) {
                return getSwitchChannelStatus();
            } else if (key.equals(KEY_FINE_TUNE)) {
                return getFineTuneStatus();
            } else if (key.equals(KEY_MTS)) {
                return getAudioOutmode();
            }
            //settings
            else if (key.equals(KEY_DTV_TYPE)) {
                return getDtvTypeStatus(getDtvType());
            } else if (key.equals(KEY_SLEEP_TIMER)) {
                return getSleepTimerStatus();
            } else if (key.equals(KEY_MENU_TIME)) {
                return getMenuTimeStatus();
            } else if (key.equals(KEY_STARTUP_SETTING)) {
                return getStartupSettingStatus();
            } else if (key.equals(KEY_DYNAMIC_BACKLIGHT)) {
                return getDynamicBacklightStatus();
            } else if (key.equals(KEY_HDMI20)) {
                return getHdmi20Status();
            } else if (key.equals(KEY_AD_SWITCH)) {
                return getADSwitchStatus();
            } else if (key.equals(KEY_AD_MIX)) {
                return getADMixStatus();
            }
        }
        return null;
    }

    public  String getPictureModeStatus () {
        int pictureModeIndex = mSystemControlManager.GetPQMode();
        switch (pictureModeIndex) {
            case 0:
                return mResources.getString(R.string.standard);
            case 1:
                return mResources.getString(R.string.vivid);
            case 2:
                return mResources.getString(R.string.soft);
            case 3:
                return mResources.getString(R.string.user);
            case 6:
                return mResources.getString(R.string.monitor);
            default:
                return mResources.getString(R.string.standard);
        }
    }

    private String getBrightnessStatus () {
        return mSystemControlManager.GetBrightness() + "%";
    }

    private String getContrastStatus () {
        return mSystemControlManager.GetContrast() + "%";
    }

    private String getColorStatus () {
        return mSystemControlManager.GetSaturation() + "%";
    }

    private String getSharpnessStatus () {
        return mSystemControlManager.GetSharpness() + "%";
    }

    private String getBacklightStatus () {
        Log.d(TAG, " code temp removed");
        return null;//mSystemControlManager.GetBacklight() + "%"; tmp removed;
    }

    public boolean isShowTint() {
        TvInSignalInfo info;
        String colorSystem = getColorSystemStatus();
        if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV
            && colorSystem != null && colorSystem.equals(mResources.getString(R.string.ntsc))) {
            info = mTvControlManager.GetCurrentSignalInfo();
            if (info.sigStatus == TvInSignalInfo.SignalStatus.TVIN_SIG_STATUS_STABLE) {
                Log.d(TAG, "ATV NTSC mode signal is stable, show Tint");
                return true;
            }
        } else if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_AV) {
            info = mTvControlManager.GetCurrentSignalInfo();
            if (info.sigStatus == TvInSignalInfo.SignalStatus.TVIN_SIG_STATUS_STABLE) {
                String[] strings = info.sigFmt.toString().split("_");
                if (strings[4].contains("NTSC")) {
                    Log.d(TAG, "AV NTSC mode signal is stable, show Tint");
                    return true;
                }
            }
        }
        return false;
    }

    private String getTintStatus () {
        return mSystemControlManager.GetHue() + "%";
    }

    public String getColorTemperatureStatus () {
        int itemPosition = mSystemControlManager.GetColorTemperature();
        if (itemPosition == 0)
            return mResources.getString(R.string.standard);
        else if (itemPosition == 1)
            return mResources.getString(R.string.warm);
        else
            return mResources.getString(R.string.cool);
    }

    public String getAspectRatioStatus () {
        int itemPosition = mSystemControlManager.GetDisplayMode(mTvSourceInput.toInt());
        if (itemPosition == SystemControlManager.Display_Mode.DISPLAY_MODE_169.toInt())
            return mResources.getString(R.string.full_screen);
        else if (itemPosition == SystemControlManager.Display_Mode.DISPLAY_MODE_MODE43.toInt())
            return mResources.getString(R.string.four2three);
        else if (itemPosition == SystemControlManager.Display_Mode.DISPLAY_MODE_FULL.toInt())
            return mResources.getString(R.string.panorama);
        else
            return mResources.getString(R.string.auto);
    }

    public String getDnrStatus () {
        int itemPosition = mSystemControlManager.GetNoiseReductionMode();
        if (itemPosition == 0)
            return mResources.getString(R.string.off);
        else if (itemPosition == 1)
            return mResources.getString(R.string.low);
        else if (itemPosition == 2)
            return mResources.getString(R.string.medium);
        else if (itemPosition == 3)
            return mResources.getString(R.string.high);
        else
            return mResources.getString(R.string.auto);
    }

    public  String getSoundModeStatus () {
        int itemPosition = mTvControlManager.GetCurAudioSoundMode();
        if (itemPosition == 0)
            return mResources.getString(R.string.standard);
        else if (itemPosition == 1)
            return mResources.getString(R.string.music);
        else if (itemPosition == 2)
            return mResources.getString(R.string.news);
        else if (itemPosition == 3)
            return mResources.getString(R.string.movie);
        else if (itemPosition == 4)
            return mResources.getString(R.string.user);
        else
            return null;
    }

    private String getTrebleStatus () {
        return mTvControlManager.GetCurAudioTrebleVolume() + "%";
    }

    private String getBassStatus () {
        return mTvControlManager.GetCurAudioBassVolume() + "%";
    }

    private String getBalanceStatus () {
        return mTvControlManager.GetCurAudioBalance() + "%";
    }

    public String getSpdifStatus () {
        if (mTvControlManager.GetCurAudioSPDIFSwitch() == 0)
            return mResources.getString(R.string.off);
        int itemPosition = mTvControlManager.GetCurAudioSPDIFMode();
        if (itemPosition == 0)
            return mResources.getString(R.string.pcm);
        else if (itemPosition == 1)
            return mResources.getString(R.string.raw);
        else
            return null;
    }

    public String getSurroundStatus () {
        int itemPosition = mTvControlManager.GetCurAudioSrsSurround();
        if (itemPosition == 0)
            return mResources.getString(R.string.off);
        else
            return mResources.getString(R.string.on);
    }

    public String getVirtualSurroundStatus() {
        int itemPosition = mTvControlManager.GetAudioVirtualizerEnable();
        if (itemPosition == 0)
            return mResources.getString(R.string.off);
        else
            return mResources.getString(R.string.on);
    }

    public String getVirtualSurroundLevel() {
        if (mTvControlManager.GetAudioVirtualizerEnable() == 0) {
            return mResources.getString(R.string.off);
        } else {
            return mTvControlManager.GetAudioVirtualizerLevel() + "%";
        }
    }

    public String getDialogClarityStatus () {
        int itemPosition = mTvControlManager.GetCurAudioSrsDialogClarity();
        if (itemPosition == 0)
            return mResources.getString(R.string.off);
        else
            return mResources.getString(R.string.on);
    }

    public String getBassBoostStatus () {
        int itemPosition = mTvControlManager.GetCurAudioSrsTruBass();
        if (itemPosition == 0)
            return mResources.getString(R.string.off);
        else
            return mResources.getString(R.string.on);
    }

    public ChannelInfo getCurrentChannel() {
        return currentChannel;
    }

    public int getCurrentChannelNumber() {
        if (currentChannel != null)
            return currentChannel.getNumber();

        return -1;
    }

    private String getCurrentChannelStatus () {
        if (currentChannel != null)
            return currentChannel.getDisplayNameLocal();

        return null;
    }

    private String getAudioTrackStatus () {
        if (currentChannel != null &&  currentChannel.getAudioLangs() != null)
            return currentChannel.getAudioLangs()[currentChannel.getAudioTrackIndex()];
        else
            return null;
    }

    public String getSoundChannelStatus () {
        if (currentChannel != null) {
            switch (currentChannel.getAudioChannel()) {
                case 0:
                    return mResources.getString(R.string.stereo);
                case 1:
                    return mResources.getString(R.string.left_channel);
                case 2:
                    return mResources.getString(R.string.right_channel);
                default:
                    return mResources.getString(R.string.stereo);
            }
        }
        return null;
    }

    public ArrayList<HashMap<String, Object>> getChannelInfo () {
        ArrayList<HashMap<String, Object>> list =  new ArrayList<HashMap<String, Object>>();
        if (currentChannel != null) {
            HashMap<String, Object> item = new HashMap<String, Object>();
            item.put(STRING_NAME, mResources.getString(R.string.channel_l));
            item.put(STRING_STATUS, currentChannel.getDisplayNameLocal());
            list.add(item);

            item = new HashMap<String, Object>();
            item.put(STRING_NAME, mResources.getString(R.string.frequency_l));
            item.put(STRING_STATUS, Integer.toString(getFrequency() + getFineTune()));
            list.add(item);

            if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV) {
                item = new HashMap<String, Object>();
                item.put(STRING_NAME, mResources.getString(R.string.type));
                item.put(STRING_STATUS, currentChannel.getType());
                list.add(item);

                item = new HashMap<String, Object>();
                item.put(STRING_NAME, mResources.getString(R.string.service_id));
                item.put(STRING_STATUS, Integer.toString(currentChannel.getServiceId()));
                list.add(item);

                item = new HashMap<String, Object>();
                item.put(STRING_NAME, mResources.getString(R.string.pcr_id));
                item.put(STRING_STATUS, Integer.toString(currentChannel.getPcrPid()));
                list.add(item);
            }
        }
        return list;
    }

    public ArrayList<HashMap<String, Object>> getAudioTrackList () {
        ArrayList<HashMap<String, Object>> list =  new ArrayList<HashMap<String, Object>>();

        if (currentChannel != null && currentChannel.getAudioLangs() != null)
            for (int i = 0; i < currentChannel.getAudioLangs().length; i++) {
                HashMap<String, Object> item = new HashMap<String, Object>();
                item.put(STRING_NAME, currentChannel.getAudioLangs()[i]);
                list.add(item);
            }

        return list;
    }

    public ArrayList<HashMap<String, Object>> getAudioADList () {
        ArrayList<HashMap<String, Object>> list =  new ArrayList<HashMap<String, Object>>();

        if (currentChannel != null && currentChannel.getAudioLangs() != null && DroidLogicTvUtils.hasAudioADTracks(currentChannel)) {
            int mainTrackIndex = mSystemControlManager.getPropertyInt("tv.dtv.audio_track_idx", -1);
            Log.d(TAG, "mainTrackIndex["+mainTrackIndex+"]");
            if (mainTrackIndex < 0)
                return list;

            int[] adAudioIdx = DroidLogicTvUtils.getAudioADTracks(currentChannel, mainTrackIndex);
            if (adAudioIdx != null) {
                for (int i = 0; i < adAudioIdx.length; i++) {
                    HashMap<String, Object> item = new HashMap<String, Object>();
                    item.put(STRING_NAME, currentChannel.getAudioLangs()[adAudioIdx[i]]);
                    item.put(STRING_PRIVATE, String.valueOf(adAudioIdx[i]));
                    list.add(item);
                }
            }
        }

        return list;
    }

    public ArrayList<HashMap<String, Object>> getSoundChannelList () {
        ArrayList<HashMap<String, Object>> list =  new ArrayList<HashMap<String, Object>>();

        HashMap<String, Object> item = new HashMap<String, Object>();
        item.put(STRING_NAME, mResources.getString(R.string.stereo));
        list.add(item);

        item = new HashMap<String, Object>();
        item.put(STRING_NAME, mResources.getString(R.string.left_channel));
        list.add(item);

        item = new HashMap<String, Object>();
        item.put(STRING_NAME, mResources.getString(R.string.right_channel));
        list.add(item);

        return list;
    }

    public ArrayList<HashMap<String, Object>> getDefaultLanguageList () {
        ArrayList<HashMap<String, Object>> list =  new ArrayList<HashMap<String, Object>>();

        String[] def_lanArray = mResources.getStringArray(R.array.def_lan);
        for (String lanNameString : def_lanArray) {
            HashMap<String, Object> item = new HashMap<String, Object>();
            item.put(STRING_NAME, lanNameString);
            list.add(item);
        }

        return list;
    }

    private String getDefaultLanStatus () {
        String ret = mTvControlDataManager.getString(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_DEFAULT_LANGUAGE);
        String[] lanArray;
        if (ret == null) {
            lanArray =  mResources.getStringArray(R.array.def_lan);
            for (String lang : lanArray) {
                if (lang.equals(Locale.getDefault().getISO3Language()))
                    return lang;
            }
            ret = lanArray[0];
        } else {
            if (ret.equals("")) {
                lanArray =  mResources.getStringArray(R.array.def_lan);
                for (String lang : lanArray) {
                    if (lang.equals(Locale.getDefault().getISO3Language()))
                        return lang;
                }
                ret = lanArray[0];
            }
        }
        return ret;
    }

    public String getSubtitleSwitchStatus () {
        int switchVal = mTvControlDataManager.getInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_SUBTITLE_SWITCH, 0);
        switch (switchVal) {
            case 0:
                return mResources.getString(R.string.off);
            case 1:
                return mResources.getString(R.string.on);
            default:
                return mResources.getString(R.string.off);
        }
    }

    public String getADSwitchStatus () {
        int switchVal = mTvControlDataManager.getInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_SWITCH, 0);
        switch (switchVal) {
            case 0:
                return mResources.getString(R.string.off);
            case 1:
                return mResources.getString(R.string.on);
            default:
                return mResources.getString(R.string.off);
        }
    }

    public String getColorSystemStatus () {
        if (currentChannel != null) {
            switch (currentChannel.getVideoStd()) {
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

    public String getSoundSystemStatus () {
        if (currentChannel != null) {
            switch (currentChannel.getAudioStd()) {
                case 0:
                    return mResources.getString(R.string.sound_system_dk);
                case 1:
                    return mResources.getString(R.string.sound_system_i);
                case 2:
                    return mResources.getString(R.string.sound_system_bg);
                case 3:
                    return mResources.getString(R.string.sound_system_m);
                default:
                    return mResources.getString(R.string.sound_system_dk);
            }
        }

        return null;
    }

    private String getVolumeCompensateStatus () {
        return getVolumeCompensate() * 5 + "%";
    }

    public int getVolumeCompensate() {
        if (currentChannel != null)
            return currentChannel.getAudioCompensation();
        else
            return 0;
    }

    private String getADMixStatus () {
        return "" + getADMix() + "%";
    }

    public int getADMix() {
        int val = mTvControlDataManager.getInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_MIX, 50);
        return val;
    }


    private static final int FineTuneRange = 2500 * 1000; /*Hz*/
    private static final int FineTuneStepSize = 50 * 1000; /*Hz*/
    public int getFrequency () {
        if (currentChannel != null)
            return currentChannel.getFrequency();
        else {
            return -1;
        }
    }

    public String getFineTuneStatus() {
        return Integer.toString(2 * (getFineTuneProgress() - 50)) + "%";
    }

    public int getFineTune () {
        if (currentChannel != null)
            return currentChannel.getFineTune();
        return 0;
    }

    public int getFineTuneProgress () {
        if (currentChannel != null) {
            int finetune = currentChannel.getFineTune();
            return 50 + ((finetune * 50) / FineTuneRange);
        } else {
            return 50;
        }
    }


    public void setFineTuneStep(int step) {
        if (currentChannel != null) {
            int finetune = currentChannel.getFineTune() + (step * FineTuneStepSize);
            if ((finetune > FineTuneRange) || (finetune < -1 * FineTuneRange))
                return;
            setFineTune(finetune);
        }
    }

    public void setFineTune(int finetune) {
        if (currentChannel != null) {
            Log.d(TAG, "FineTune[" + finetune + "]");
            currentChannel.setFinetune(finetune);
            mTvDataBaseManager.updateChannelInfo(currentChannel);
            mTvControlManager.SetFrontendParms(TvControlManager.tv_fe_type_e.TV_FE_ANALOG,
                                               currentChannel.getFrequency() + currentChannel.getFineTune(),
                                               currentChannel.getVideoStd(), currentChannel.getAudioStd(),
                                               currentChannel.getVfmt(),
                                               currentChannel.getAudioOutPutMode(),
                                               0, 0);
        }
    }

    private int mSearchProgress;
    private int mSearchedNumber;

    public String getInputId() {
        return mInputId;
    }

    public int setManualSearchProgress (int progress) {
        mSearchProgress = progress;
        return progress;
    }

    public int setManualSearchSearchedNumber (int number) {
        mSearchedNumber = number;
        return number;
    }

    public int setAutoSearchProgress (int progress) {
        mSearchProgress = progress;
        return progress;
    }

    public int setAutoSearchSearchedNumber (int number) {
        mSearchedNumber = number;
        return number;
    }

    public int getManualSearchProgress () {
        return mSearchProgress;
    }

    public int getManualSearchSearchedNumber () {
        return mSearchedNumber;
    }

    public int getAutoSearchProgress () {
        return mSearchProgress;
    }

    public int getAutoSearchSearchedNumber () {
        return mSearchedNumber;
    }

    public ArrayList<HashMap<String, Object>> getChannelList (int type) {
        ArrayList<HashMap<String, Object>> list =  new ArrayList<HashMap<String, Object>>();
        ArrayList<ChannelInfo> channelList = getChannelInfoList(type);

        if (channelList.size() > 0) {
            for (int i = 0 ; i < channelList.size(); i++) {
                ChannelInfo info = channelList.get(i);
                HashMap<String, Object> item = new HashMap<String, Object>();
                if (!info.isBrowsable())
                    item.put(STRING_ICON, R.drawable.skip);
                else if (info.isFavourite())
                    item.put(STRING_ICON, R.drawable.favourite);
                else
                    item.put(STRING_ICON, null);

                item.put(STRING_NAME, info.getNumber() + ". " + info.getDisplayNameLocal());
                list.add(item);
            }
        } else {
            HashMap<String, Object> item = new HashMap<String, Object>();
            item.put(STRING_ICON, null);
            item.put(STRING_NAME, mResources.getString(R.string.error_no_channel));
            list.add(item);
        }

        return list;
    }

    private Runnable loadChannelsRunnable = new Runnable() {
        @Override
        public void run() {
            try {
                loadChannelInfoList();
            } catch (RuntimeException e) {
                e.printStackTrace();
            }
        }
    };

    private void loadChannelInfoList() {
        videoChannelList = mTvDataBaseManager.getChannelList(mInputId, Channels.SERVICE_TYPE_AUDIO_VIDEO);
        radioChannelList = mTvDataBaseManager.getChannelList(mInputId, Channels.SERVICE_TYPE_AUDIO);
    }

    private ArrayList<ChannelInfo> getChannelInfoList (int type) {
        ArrayList<ChannelInfo> channelList = null;
        switch (type) {
            case ChannelEdit.TYPE_ATV:
            case ChannelEdit.TYPE_DTV_TV:
                channelList = videoChannelList;
                break;
            case ChannelEdit.TYPE_DTV_RADIO:
                channelList = radioChannelList;
                break;
        }
        return channelList;
    }

    private ChannelInfo getChannelByIndex(int type, int index) {
        ArrayList<ChannelInfo> channelList = getChannelInfoList(type);
        if (channelList != null && index < channelList.size()) {
            return channelList.get(index);
        }
        return null;
    }

    public String getSwitchChannelStatus () {
        if (mTvControlManager.getBlackoutEnable() == 0)
            return mResources.getString(R.string.static_frame);
        else
            return mResources.getString(R.string.black_frame);
    }

    public String getDtvType() {
        int deviceId = DroidLogicTvUtils.getHardwareDeviceId(mInputId);
        String type = mTvControlDataManager.getString(mContext.getContentResolver(),
            DroidLogicTvUtils.TV_KEY_DTV_TYPE);
        if (type != null) {
            return type;
        } else {
            if (deviceId == DroidLogicTvUtils.DEVICE_ID_ADTV ) {
                return TvContract.Channels.TYPE_ATSC_T;
            }else {
                return TvContract.Channels.TYPE_DTMB;
            }
        }
    }

    public int getTvSearchTypeSys() {
        String searchColorSystem = DroidLogicTvUtils.getTvSearchTypeSys(mContext);
        int colorSystem = TvControlManager.ATV_VIDEO_STD_AUTO;
        if (searchColorSystem.equals(TvScanConfig.TV_COLOR_SYS.get(TvScanConfig.TV_COLOR_SYS_AUTO_INDEX))) {
            colorSystem = TvControlManager.ATV_VIDEO_STD_AUTO;
        } else if (searchColorSystem.equals(TvScanConfig.TV_COLOR_SYS.get(TvScanConfig.TV_COLOR_SYS_PAL_INDEX))) {
            colorSystem = TvControlManager.ATV_VIDEO_STD_PAL;
        } else if (searchColorSystem.equals(TvScanConfig.TV_COLOR_SYS.get(TvScanConfig.TV_COLOR_SYS_NTSC_INDEX))) {
            colorSystem = TvControlManager.ATV_VIDEO_STD_NTSC;
        } else if (searchColorSystem.equals(TvScanConfig.TV_COLOR_SYS.get(TvScanConfig.TV_COLOR_SYS_SECAM_INDEX))) {
            colorSystem = TvControlManager.ATV_VIDEO_STD_SECAM;
        } else {
            Log.w(TAG, "unsupport color System: " + searchColorSystem + ", set default: AUTO");
        }
        return colorSystem;
    }

    public int getTvSearchSoundSys() {
        String searchAudioSystem = DroidLogicTvUtils.getTvSearchSoundSys(mContext);
        int audioSystem = TvControlManager.ATV_AUDIO_STD_AUTO;
        if (searchAudioSystem.equals(TvScanConfig.TV_SOUND_SYS.get(TvScanConfig.TV_SOUND_SYS_AUTO_INDEX))) {
            audioSystem = TvControlManager.ATV_AUDIO_STD_AUTO;
        } else if (searchAudioSystem.equals(TvScanConfig.TV_SOUND_SYS.get(TvScanConfig.TV_SOUND_SYS_DK_INDEX))) {
            audioSystem = TvControlManager.ATV_AUDIO_STD_DK;
        } else if (searchAudioSystem.equals(TvScanConfig.TV_SOUND_SYS.get(TvScanConfig.TV_SOUND_SYS_I_INDEX))) {
            audioSystem = TvControlManager.ATV_AUDIO_STD_I;
        } else if (searchAudioSystem.equals(TvScanConfig.TV_SOUND_SYS.get(TvScanConfig.TV_SOUND_SYS_BG_INDEX))) {
            audioSystem = TvControlManager.ATV_AUDIO_STD_BG;
        } else if (searchAudioSystem.equals(TvScanConfig.TV_SOUND_SYS.get(TvScanConfig.TV_SOUND_SYS_M_INDEX))) {
            audioSystem = TvControlManager.ATV_AUDIO_STD_M;
        } else if (searchAudioSystem.equals(TvScanConfig.TV_SOUND_SYS.get(TvScanConfig.TV_SOUND_SYS_L_INDEX))) {
            audioSystem = TvControlManager.ATV_AUDIO_STD_L;
        } else {
            Log.w(TAG, "unsupport audio System: " + searchAudioSystem + ", set default: AUTO");
        }
        return audioSystem;
    }

    public String getDefaultDtvType() {
        /*int deviceId = DroidLogicTvUtils.getHardwareDeviceId(mInputId);
        if (deviceId == DroidLogicTvUtils.DEVICE_ID_ADTV ) {
            return TvContract.Channels.TYPE_ATSC_T;
        }else {
            return TvContract.Channels.TYPE_DTMB;
        }*/
        return TvContract.Channels.TYPE_ATSC_T;
    }

    public String getDtvTypeStatus(String type) {
        String ret = "";
        if (TextUtils.equals(type, TvContract.Channels.TYPE_DTMB)) {
                ret = mResources.getString(R.string.dtmb);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_C)) {
                ret = mResources.getString(R.string.dvbc);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_T)) {
                ret = mResources.getString(R.string.dvbt);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_S)) {
                ret = mResources.getString(R.string.dvbs);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_C2)) {
                ret = mResources.getString(R.string.dvbc2);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_T2)) {
                ret = mResources.getString(R.string.dvbt2);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_S2)) {
                ret = mResources.getString(R.string.dvbs2);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ATSC_T)) {
                ret = mResources.getString(R.string.atsc_t);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ATSC_C)) {
                ret = mResources.getString(R.string.atsc_c);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ISDB_T)) {
                ret = mResources.getString(R.string.isdb_t);
        }
        return ret;
    }

    public String getSleepTimerStatus () {
        String ret = "";
        int mode  = mSystemControlManager.getPropertyInt("tv.sleep_timer", 0);
        switch (mode) {
            case 0://off
                ret = mResources.getString(R.string.off);
                break;
            case 1://15min
                ret = mResources.getString(R.string.time_15min);
                break;
            case 2://30min
                ret = mResources.getString(R.string.time_30min);
                break;
            case 3://45min
                ret = mResources.getString(R.string.time_45min);
                break;
            case 4://60min
                ret = mResources.getString(R.string.time_60min);
                break;
            case 5://90min
                ret = mResources.getString(R.string.time_90min);
                break;
            case 6://120min
                ret = mResources.getString(R.string.time_120min);
                break;
            default:
                ret = mResources.getString(R.string.off);
                break;
        }
        return ret;
    }

    public String getMenuTimeStatus () {
        int seconds = mTvControlDataManager.getInt(mContext.getContentResolver(), KEY_MENU_TIME, DEFUALT_MENU_TIME);
        switch (seconds) {
            case 0:
                return mResources.getString(R.string.time_10s);
            case 1:
                return mResources.getString(R.string.time_20s);
            case 2:
                return mResources.getString(R.string.time_40s);
            case 3:
                return mResources.getString(R.string.time_60s);
            default:
                return mResources.getString(R.string.time_10s);
        }
    }

    public String getStartupSettingStatus () {
        int type = mTvControlDataManager.getInt(mContext.getContentResolver(), "tv_start_up_enter_app", 0);

        if (type == 0)
            return mResources.getString(R.string.launcher);
        else
            return mResources.getString(R.string.tv);
    }

    public String getDynamicBacklightStatus () {
        int itemPosition = mTvControlManager.isAutoBackLighting();
        if (itemPosition == 0)
            return mResources.getString(R.string.off);
        else
            return mResources.getString(R.string.on);
    }

    public TvControlManager.HdmiPortID getCurrHdmiPortID() {
        TvControlManager.HdmiPortID hdmiPortID;
        int device_id = mTvControlDataManager.getInt(mContext.getContentResolver(),
                DroidLogicTvUtils.TV_CURRENT_DEVICE_ID, -1);
        switch (device_id) {
            case DroidLogicTvUtils.DEVICE_ID_HDMI1:
                hdmiPortID = TvControlManager.HdmiPortID.HDMI_PORT_1;
                break;
            case DroidLogicTvUtils.DEVICE_ID_HDMI2:
                hdmiPortID = TvControlManager.HdmiPortID.HDMI_PORT_2;
                break;
            case DroidLogicTvUtils.DEVICE_ID_HDMI3:
                hdmiPortID = TvControlManager.HdmiPortID.HDMI_PORT_3;
                break;
            case DroidLogicTvUtils.DEVICE_ID_HDMI4:
                hdmiPortID = TvControlManager.HdmiPortID.HDMI_PORT_4;
                break;
            default:
                hdmiPortID = TvControlManager.HdmiPortID.HDMI_PORT_1;
                Log.w(TAG, "current source input is NOT HDMI");
                break;
        }
        return hdmiPortID;
    }

    public String getHdmi20Status() {
        TvControlManager.HdmiPortID hdmiPort = getCurrHdmiPortID();
        int hdmiVer = mTvControlManager.GetHdmiEdidVersion(hdmiPort);
        if (hdmiVer == TvControlManager.HdmiEdidVer.HDMI_EDID_VER_20.toInt()) {
            return mResources.getString(R.string.on);
        } else {
            return mResources.getString(R.string.off);
        }
    }

    public void setHdmi20Mode(String mode) {
        if (mode.equals(STATUS_ON)) {
            // set HDMI mode sequence: save than set
            mTvControlManager.SaveHdmiEdidVersion(getCurrHdmiPortID(),
                TvControlManager.HdmiEdidVer.HDMI_EDID_VER_20);
            mTvControlManager.SetHdmiEdidVersion(getCurrHdmiPortID(),
                TvControlManager.HdmiEdidVer.HDMI_EDID_VER_20);
        } else if (mode.equals(STATUS_OFF)) {
            mTvControlManager.SaveHdmiEdidVersion(getCurrHdmiPortID(),
                TvControlManager.HdmiEdidVer.HDMI_EDID_VER_14);
            mTvControlManager.SetHdmiEdidVersion(getCurrHdmiPortID(),
                TvControlManager.HdmiEdidVer.HDMI_EDID_VER_14);
        }
    }

    public void setPictureMode (String mode) {
        if (mode.equals(STATUS_STANDARD)) {
            mSystemControlManager.SetPQMode(SystemControlManager.PQMode.PQ_MODE_STANDARD.toInt(), 1, 0);
        } else if (mode.equals(STATUS_VIVID)) {
            mSystemControlManager.SetPQMode(SystemControlManager.PQMode.PQ_MODE_BRIGHT.toInt(), 1, 0);
        } else if (mode.equals(STATUS_SOFT)) {
            mSystemControlManager.SetPQMode(SystemControlManager.PQMode.PQ_MODE_SOFTNESS.toInt(), 1, 0);
        } else if (mode.equals(STATUS_MONITOR)) {
            mSystemControlManager.SetPQMode(SystemControlManager.PQMode.PQ_MODE_MONITOR.toInt(), 1, 0);
        } else if (mode.equals(STATUS_USER)) {
            mSystemControlManager.SetPQMode(SystemControlManager.PQMode.PQ_MODE_USER.toInt(), 1, 0);
        }
    }

    private int setPictureUserMode(String key) {
        int brightness = mSystemControlManager.GetBrightness();
        int contrast = mSystemControlManager.GetContrast();
        int color = mSystemControlManager.GetSaturation();
        int sharpness = mSystemControlManager.GetSharpness();
        int tint = -1;
        if (isShowTint())
            tint = mSystemControlManager.GetHue();
        int ret = -1;

        switch (mSystemControlManager.GetPQMode()) {
            case 0:
            case 1:
            case 2:
            case 6:
                setPictureMode(STATUS_USER);
                break;
        }

        //Log.d(TAG, " brightness=" + brightness + " contrast=" + contrast + " color=" + color + " sharp=" + sharpness);
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
            mSystemControlManager.SetSharpness(sharpness, 1, 1);
        else
            ret = sharpness;

        if (isShowTint()) {
            if (!key.equals(KEY_TINT))
                mSystemControlManager.SetHue(tint, 1);
            else
                ret = tint;
        }

        return ret;
    }

    public void setBrightness (int step) {
        if (mSystemControlManager.GetPQMode() == 3)
            mSystemControlManager.SetBrightness(mSystemControlManager.GetBrightness() + step, 1);
        else
            mSystemControlManager.SetBrightness(setPictureUserMode(KEY_BRIGHTNESS) + step, 1);
    }

    public void setContrast (int step) {
        if (mSystemControlManager.GetPQMode() == 3)
            mSystemControlManager.SetContrast(mSystemControlManager.GetContrast() + step, 1);
        else
            mSystemControlManager.SetContrast(setPictureUserMode(KEY_CONTRAST) + step, 1);
    }

    public void setColor (int step) {
        if (mSystemControlManager.GetPQMode() == 3)
            mSystemControlManager.SetSaturation(mSystemControlManager.GetSaturation() + step, 1);
        else
            mSystemControlManager.SetSaturation(setPictureUserMode(KEY_COLOR) + step, 1);
    }

    public void setSharpness (int step) {
        if (mSystemControlManager.GetPQMode() == 3)
            mSystemControlManager.SetSharpness(mSystemControlManager.GetSharpness() + step, 1, 1);
        else
            mSystemControlManager.SetSharpness(setPictureUserMode(KEY_SHARPNESS) + step, 1, 1);
    }

    public void setTint (int step) {
        if (isShowTint()) {
            if (mSystemControlManager.GetPQMode() == 3)
                mSystemControlManager.SetHue(mSystemControlManager.GetHue() + step, 1);
            else
                mSystemControlManager.SetHue(setPictureUserMode(KEY_TINT) + step, 1);
        }
    }

    public void setBacklight (int step) {
       // int value = mSystemControlManager.GetBacklight() + step;
       // if (value >= 0 && value <= 100) {
       //     mSystemControlManager.SetBacklight(value, 1);
       // }

       Log.d(TAG, "setBacklight code removed");
    }

    public void setColorTemperature(String mode) {
        if (mode.equals(STATUS_STANDARD))
            mSystemControlManager.SetColorTemperature(SystemControlManager.color_temperature.COLOR_TEMP_STANDARD.toInt(), 1);
        else if (mode.equals(STATUS_WARM))
            mSystemControlManager.SetColorTemperature(SystemControlManager.color_temperature.COLOR_TEMP_WARM.toInt(), 1);
        else if (mode.equals(STATUS_COOL))
            mSystemControlManager.SetColorTemperature(SystemControlManager.color_temperature.COLOR_TEMP_COLD.toInt(), 1);
    }

    public void setAspectRatio(String mode) {
        if (mode.equals(STATUS_AUTO)) {
            mSystemControlManager.SetDisplayMode(mTvSourceInput.toInt(), SystemControlManager.Display_Mode.DISPLAY_MODE_NORMAL, 1);
        } else if (mode.equals(STATUS_4_TO_3)) {
            mSystemControlManager.SetDisplayMode(mTvSourceInput.toInt(), SystemControlManager.Display_Mode.DISPLAY_MODE_MODE43, 1);
        } else if (mode.equals(STATUS_PANORAMA)) {
            mSystemControlManager.SetDisplayMode(mTvSourceInput.toInt(), SystemControlManager.Display_Mode.DISPLAY_MODE_FULL, 1);
        } else if (mode.equals(STATUS_FULL_SCREEN)) {
            mSystemControlManager.SetDisplayMode(mTvSourceInput.toInt(), SystemControlManager.Display_Mode.DISPLAY_MODE_169, 1);
        }
    }

    public void setDnr (String mode) {
        if (mode.equals(STATUS_OFF))
            mSystemControlManager.SetNoiseReductionMode(SystemControlManager.Noise_Reduction_Mode.REDUCE_NOISE_CLOSE.toInt(), 1);
        else if (mode.equals(STATUS_AUTO))
            mSystemControlManager.SetNoiseReductionMode(SystemControlManager.Noise_Reduction_Mode.REDUCTION_MODE_AUTO.toInt(), 1);
        else if (mode.equals(STATUS_MEDIUM))
            mSystemControlManager.SetNoiseReductionMode(SystemControlManager.Noise_Reduction_Mode.REDUCE_NOISE_MID.toInt(), 1);
        else if (mode.equals(STATUS_HIGH))
            mSystemControlManager.SetNoiseReductionMode(SystemControlManager.Noise_Reduction_Mode.REDUCE_NOISE_STRONG.toInt(), 1);
        else if (mode.equals(STATUS_LOW))
            mSystemControlManager.SetNoiseReductionMode(SystemControlManager.Noise_Reduction_Mode.REDUCE_NOISE_WEAK.toInt(), 1);
    }

    public void setSoundMode (String mode) {
        if (mode.equals(STATUS_STANDARD)) {
            mTvControlManager.SetAudioSoundMode(TvControlManager.Sound_Mode.SOUND_MODE_STD);
            mTvControlManager.SaveCurAudioSoundMode(TvControlManager.Sound_Mode.SOUND_MODE_STD.toInt());
        } else if (mode.equals(STATUS_MUSIC)) {
            mTvControlManager.SetAudioSoundMode(TvControlManager.Sound_Mode.SOUND_MODE_MUSIC);
            mTvControlManager.SaveCurAudioSoundMode(TvControlManager.Sound_Mode.SOUND_MODE_MUSIC.toInt());
        } else if (mode.equals(STATUS_NEWS)) {
            mTvControlManager.SetAudioSoundMode(TvControlManager.Sound_Mode.SOUND_MODE_NEWS);
            mTvControlManager.SaveCurAudioSoundMode(TvControlManager.Sound_Mode.SOUND_MODE_NEWS.toInt());
        } else if (mode.equals(STATUS_MOVIE)) {
            mTvControlManager.SetAudioSoundMode(TvControlManager.Sound_Mode.SOUND_MODE_THEATER);
            mTvControlManager.SaveCurAudioSoundMode(TvControlManager.Sound_Mode.SOUND_MODE_THEATER.toInt());
        } else if (mode.equals(STATUS_USER)) {
            mTvControlManager.SetAudioSoundMode(TvControlManager.Sound_Mode.SOUND_MODE_USER);
            mTvControlManager.SaveCurAudioSoundMode(TvControlManager.Sound_Mode.SOUND_MODE_USER.toInt());
        }
    }

    public void setTreble (int step) {
        int treble_value = mTvControlManager.GetCurAudioTrebleVolume() + step;

        int bass_value = -1;
        if (mTvControlManager.GetCurAudioSoundMode() != 4)
            bass_value = mTvControlManager.GetCurAudioBassVolume();

        if (treble_value >= 0 && treble_value <= 100) {
            mTvControlManager.SetAudioTrebleVolume(treble_value);
            mTvControlManager.SaveCurAudioTrebleVolume(treble_value);
        }

        if (bass_value != -1) {
            mTvControlManager.SetAudioBassVolume(bass_value);
            mTvControlManager.SaveCurAudioBassVolume(bass_value);
        }
    }

    public void setBass (int step) {
        int bass_value = mTvControlManager.GetCurAudioBassVolume() + step;

        int treble_value = -1;
        if (mTvControlManager.GetCurAudioSoundMode() != 4)
            treble_value = mTvControlManager.GetCurAudioTrebleVolume();

        if (bass_value >= 0 && bass_value <= 100) {
            mTvControlManager.SetAudioBassVolume(bass_value);
            mTvControlManager.SaveCurAudioBassVolume(bass_value);
        }

        if (treble_value != -1) {
            mTvControlManager.SetAudioTrebleVolume(treble_value);
            mTvControlManager.SaveCurAudioTrebleVolume(treble_value);
        }
    }

    public void setBalance (int step) {
        int balance_value = mTvControlManager.GetCurAudioBalance() + step;
        if (balance_value >= 0 && balance_value <= 100) {
            mTvControlManager.SetAudioBalance(balance_value);
            mTvControlManager.SaveCurAudioBalance(balance_value);
        }
    }

    public void setSpdif (String mode) {
        if (mode.equals(STATUS_OFF)) {
            mTvControlManager.SetAudioSPDIFSwitch(0);
            mTvControlManager.SaveCurAudioSPDIFSwitch(0);
        } else if (mode.equals(STATUS_PCM)) {
            mTvControlManager.SetAudioSPDIFSwitch(1);
            mTvControlManager.SaveCurAudioSPDIFSwitch(1);
            mTvControlManager.SetAudioSPDIFMode(0);
            mTvControlManager.SaveCurAudioSPDIFMode(0);
            sendBroadcastToTvapp("audio.replay");
        } else if (mode.equals(STATUS_RAW)) {
            mTvControlManager.SetAudioSPDIFSwitch(1);
            mTvControlManager.SaveCurAudioSPDIFSwitch(1);
            mTvControlManager.SetAudioSPDIFMode(1);
            mTvControlManager.SaveCurAudioSPDIFMode(1);
            sendBroadcastToTvapp("audio.replay");
        }
    }

    public void setSurround (String mode) {
        if (mode.equals(STATUS_ON)) {
            mTvControlManager.SetAudioSrsSurround(1);
            mTvControlManager.SaveCurAudioSrsSurround(1);
        } else if (mode.equals(STATUS_OFF)) {
            setDialogClarity(STATUS_OFF);
            setBassBoost(STATUS_OFF);
            mTvControlManager.SetAudioSrsSurround(0);
            mTvControlManager.SaveCurAudioSrsSurround(0);
        }
    }

    public void setVirtualSurround (String mode) {
        if (mode.equals(STATUS_ON)) {
            mTvControlManager.SetAudioVirtualizer(1,50);
        } else if (mode.equals(STATUS_OFF)) {
            mTvControlManager.SetAudioVirtualizer(0,50);
        }
    }

    public void setVirtualSurroundLevel(int step){
        int level = mTvControlManager.GetAudioVirtualizerLevel() + step;
        if (level >= 0 && level <= 100) {
            mTvControlManager.SetAudioVirtualizer(1, level);
        }
    }

    public void setDialogClarity (String mode) {
        if (mode.equals(STATUS_ON)) {
            setSurround(STATUS_ON);
            mTvControlManager.SetAudioSrsDialogClarity(1);
            mTvControlManager.SaveCurAudioSrsDialogClarity(1);
        } else if (mode.equals(STATUS_OFF)) {
            mTvControlManager.SetAudioSrsDialogClarity(0);
            mTvControlManager.SaveCurAudioSrsDialogClarity(0);
        }
    }

    public void setBassBoost (String mode) {
        if (mode.equals(STATUS_ON)) {
            setSurround(STATUS_ON);
            mTvControlManager.SetAudioSrsTruBass(1);
            mTvControlManager.SaveCurAudioSrsTruBass(1);
        } else if (mode.equals(STATUS_OFF)) {
            mTvControlManager.SetAudioSrsTruBass(0);
            mTvControlManager.SaveCurAudioSrsTruBass(0);;
        }
    }

    public void setAudioTrack (int position) {
        if (currentChannel != null) {
            mTvControlManager.DtvSwitchAudioTrack(currentChannel.getAudioPids()[position],
                                                  currentChannel.getAudioFormats()[position], 0);
            currentChannel.setAudioTrackIndex(position);
            mTvDataBaseManager.updateChannelInfo(currentChannel);
        }
    }

    public void setSoundChannel (int mode) {
        if (currentChannel != null) {
            currentChannel.setAudioChannel(mode);
            mTvDataBaseManager.updateChannelInfo(currentChannel);
            mTvControlManager.DtvSetAudioChannleMod(currentChannel.getAudioChannel());
        }
    }

    public void setDefAudioStreamVolume() {
        AudioManager audioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        int maxVolume = SystemProperties.getInt("ro.config.media_vol_steps", 100);
        int streamMaxVolume = audioManager.getStreamMaxVolume(STREAM_MUSIC);
        int defaultVolume = maxVolume == streamMaxVolume ? (maxVolume * 3) / 10 : (streamMaxVolume * 3) / 4;
        audioManager.setStreamVolume(STREAM_MUSIC, defaultVolume, 0);
    }

    public void setDefLanguage (int position) {
        String[] def_lanArray = mResources.getStringArray(R.array.def_lan);
        mTvControlDataManager.putString(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_DEFAULT_LANGUAGE, def_lanArray[position]);
        /*
                ArrayList<ChannelInfo> mVideoChannels = mTvDataBaseManager.getChannelList(mInputId, Channels.SERVICE_TYPE_AUDIO_VIDEO);
                int trackIdx = 0;
                for (ChannelInfo channel : mVideoChannels) {
                    if (channel.getAudioLangs() != null) {
                        String[] strArray = channel.getAudioLangs();
                        for (trackIdx = 0;trackIdx < strArray.length;trackIdx++) {
                            if (def_lanArray[position].equals(strArray[trackIdx]))
                                break;
                        }
                        trackIdx %= strArray.length;
                        channel.setAudioTrackIndex(trackIdx);
                        mTvDataBaseManager.updateChannelInfo(channel);
                    }
                }
        */
    }

    public void setColorSystem(int mode) {
        if (currentChannel != null) {
            currentChannel.setVideoStd(mode);
            mTvDataBaseManager.updateChannelInfo(currentChannel);
            mTvControlManager.SetFrontendParms(TvControlManager.tv_fe_type_e.TV_FE_ANALOG, currentChannel.getFrequency(),
                                               currentChannel.getVideoStd(), currentChannel.getAudioStd(), currentChannel.getVfmt(), currentChannel.getAudioOutPutMode(), 0, currentChannel.getFineTune());
        }
    }

    public void setSoundSystem(int mode) {
        if (currentChannel != null) {
            currentChannel.setAudioStd(mode);
            mTvDataBaseManager.updateChannelInfo(currentChannel);
            mTvControlManager.SetFrontendParms(TvControlManager.tv_fe_type_e.TV_FE_ANALOG, currentChannel.getFrequency(),
                                               currentChannel.getVideoStd(), currentChannel.getAudioStd(), currentChannel.getVfmt(), currentChannel.getAudioOutPutMode(), 0, currentChannel.getFineTune());
        }
    }

    public void setVolumeCompensate (int offset) {
        if (currentChannel != null) {
            if ((currentChannel.getAudioCompensation() < 20 && offset > 0)
                || (currentChannel.getAudioCompensation() > -20 && offset < 0)) {
                currentChannel.setAudioCompensation(currentChannel.getAudioCompensation() + offset);
                mTvDataBaseManager.updateChannelInfo(currentChannel);
                mTvControlManager.SetCurProgVolumeCompesition(currentChannel.getAudioCompensation());
            }
        }
    }

    public void setChannelName (int type, int channelNumber, String targetName) {
        ChannelInfo channel = getChannelByIndex(type, channelNumber);
        if (ChannelInfo.isSameChannel(channel, currentChannel)) {
            currentChannel.setDisplayNameLocal(targetName);
        }
        mTvDataBaseManager.setChannelName(channel, targetName);
    }

    public void swapChannelPosition (int type, int channelNumber, int targetNumber) {
        if (channelNumber == targetNumber)
            return;

        ChannelInfo sourceChannel = getChannelByIndex(type, channelNumber);
        ChannelInfo targetChannel = getChannelByIndex(type, targetNumber);
        mTvDataBaseManager.swapChannel(sourceChannel, targetChannel);
        loadChannelInfoList();
    }

    public void moveChannelPosition (int type, int channelNumber, int targetNumber) {
        if (channelNumber == targetNumber)
            return;

        ArrayList<ChannelInfo> channelList = getChannelInfoList(type);
        ChannelInfo sourceChannel = getChannelByIndex(type, channelNumber);
        ChannelInfo targetChannel = getChannelByIndex(type, targetNumber);
        mTvDataBaseManager.moveChannel(sourceChannel, targetChannel);
        loadChannelInfoList();
    }

    public void skipChannel (int type, int channelNumber) {
        ChannelInfo channel = getChannelByIndex(type, channelNumber);
        //if (ChannelInfo.isSameChannel(currentChannel, channel))
        //setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);

        mTvDataBaseManager.skipChannel(channel);
    }

    public  void deleteChannel (int type, int channelNumber) {
        ArrayList<ChannelInfo> channelList = getChannelInfoList(type);
        //if (ChannelInfo.isSameChannel(currentChannel,  channelList.get(channelNumber)))
        //setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);

        ChannelInfo channel = channelList.get(channelNumber);
        mTvDataBaseManager.deleteChannel(channel);
        channelList.remove(channelNumber);
    }

    public void setFavouriteChannel (int type, int channelNumber) {
        ChannelInfo channel = getChannelByIndex(type, channelNumber);
        mTvDataBaseManager.setFavouriteChannel(channel);
    }

    public void setDtvType(String type) {
        mTvControlDataManager.putString(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_DTV_TYPE, type);
    }

    public void setTvSearchTypeSys(int mode) {
        mTvControlDataManager.putInt(mContext.getContentResolver(), ATSC_TV_SEARCH_SYS, mode);
    }

    public void setSleepTimer (int mode) {
        AlarmManager alarm = (AlarmManager)mContext.getSystemService(Context.ALARM_SERVICE);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0,
                new Intent("android.intent.action.TIMER_SUSPEND"), 0);
        alarm.cancel(pendingIntent);

        mSystemControlManager.setProperty("tv.sleep_timer", mode+"");

        long timeout = 0;
        if (mode == 0) {
            return;
        } else if (mode < 5) {
            timeout = (mode * 15  - 1) * 60 * 1000;
        } else {
            timeout = ((mode - 4) * 30 + 4 * 15  - 1) * 60 * 1000;
        }

        alarm.setExact(AlarmManager.ELAPSED_REALTIME, SystemClock.elapsedRealtime() + timeout, pendingIntent);
        Log.d(TAG, "start time count down after " + timeout + " ms");
    }

    public void setMenuTime (int seconds) {
        int type = -1;
        if (seconds == 10) {
            type = 0;
        } else if (seconds == 20) {
            type = 1;
        } else if (seconds == 40) {
            type = 2;
        } else if (seconds == 60) {
            type = 3;
        }
        mTvControlDataManager.putInt(mContext.getContentResolver(), KEY_MENU_TIME, type);
        ((TvSettingsActivity)mContext).startShowActivityTimer();
    }

    public void setStartupSetting (int type) {
        mTvControlDataManager.putInt(mContext.getContentResolver(), "tv_start_up_enter_app", type);
    }

    public void setSubtitleSwitch (int switchVal) {
        mTvControlDataManager.putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_SUBTITLE_SWITCH, switchVal);
    }

    public void setAudioADSwitch (int switchVal) {
        mTvControlDataManager.putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_SWITCH, switchVal);
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_AD_SWITCH);
        intent.putExtra(DroidLogicTvUtils.EXTRA_SWITCH_VALUE, switchVal);
        mContext.sendBroadcast(intent);
    }

    public void setADMix (int step) {
        int level = getADMix() + step;
        if (level <= 100 && level >= 0) {
            mTvControlDataManager.putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_MIX, level);
            Intent intent = new Intent(DroidLogicTvUtils.ACTION_AD_MIXING_LEVEL);
            intent.putExtra(DroidLogicTvUtils.PARA_VALUE1, level);
            mContext.sendBroadcast(intent);
        }
    }

    public void setAudioADTrack (int track) {
        //mTvControlDataManager.putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_TRACK, track);
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_AD_TRACK);
        intent.putExtra(DroidLogicTvUtils.PARA_VALUE1, track);
        mContext.sendBroadcast(intent);
    }

    public void setAudioOutmode(int mode) {
        mTvControlManager.SetAudioOutmode(mode);
    }

    public String getAudioOutmode(){
        int mode = mTvControlManager.GetAudioOutmode();
        Log.d(TAG, "getAudioOutmode"+" mode = " + mode);
        switch (mode) {
            case TvControlManager.AUDIO_OUTMODE_MONO:
                return mResources.getString(R.string.audio_outmode_mono);
            case TvControlManager.AUDIO_OUTMODE_STEREO:
                return mResources.getString(R.string.audio_outmode_stereo);
            case TvControlManager.AUDIO_OUTMODE_SAP:
                return mResources.getString(R.string.audio_outmode_sap);
            default:
                return mResources.getString(R.string.audio_outmode_stereo);
        }
    }

    public void doFactoryReset() {
        mTvControlManager.StopTv();
        setSleepTimer(DEFAULT_SLEEP_TIMER);
        setMenuTime(DEFUALT_MENU_TIME);
        setStartupSetting(0);
        setDefLanguage(0);
        setSubtitleSwitch(0);
        setAudioADSwitch(0);
        setDefAudioStreamVolume();
        // SystemControlManager mSystemControlManager = new SystemControlManager(mContext);
        // mSystemControlManager.setBootenv("ubootenv.var.upgrade_step", "1");

        for (int i = 0; i < tvPackages.length; i++) {
            ClearPackageData(tvPackages[i]);
        }
        mTvControlManager.stopAutoBacklight();
        mTvControlManager.SSMInitDevice();
        mTvControlManager.FactoryCleanAllTableForProgram();
    }

    private String[] tvPackages = {
        "com.android.providers.tv",
    };

   /*
    private void deleteApplicationCacheFiles(PackageManager packageManager, String packageName, ClearUserDataObserver clearCacheObserver) {
        try {
            Class<?> cls = Class.forName("android.content.pm.PackageManager");
            Method method = cls.getMethod("deleteApplicationCacheFiles", String.class, ClearUserDataObserver.class);
            method.invoke(packageManager, packageName, clearCacheObserver);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private boolean clearApplicationUserData(ActivityManager am, String packageName, ClearUserDataObserver clearDataObserver) {
        try {
            Class<?> cls = Class.forName("android.app.ActivityManager");
            Method method = cls.getMethod("clearApplicationUserData", String.class, ClearUserDataObserver.class);
            Object objbool = method.invoke(am, packageName, clearDataObserver);
            return Boolean.parseBoolean(objbool.toString());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    */
    private  void ClearPackageData(String packageName) {
        Log.d(TAG, "ClearPackageData:" + packageName);
        /*
        //clear data
        ActivityManager am = (ActivityManager)mContext.getSystemService(Context.ACTIVITY_SERVICE);
        ClearUserDataObserver mClearDataObserver = new ClearUserDataObserver();
        boolean res = clearApplicationUserData(am, packageName, mClearDataObserver);
        if (!res) {
            Log.i(TAG, " clear " + packageName + " data failed");
        } else {
            Log.i(TAG, " clear " + packageName + " data succeed");
        }

        //clear cache
        PackageManager packageManager = mContext.getPackageManager();
        ClearUserDataObserver mClearCacheObserver = new ClearUserDataObserver();
        deleteApplicationCacheFiles(packageManager, packageName, mClearCacheObserver);

        //clear default
        packageManager.clearPackagePreferredActivities(packageName);
        */
    }

    /*
    private class ClearUserDataObserver extends IPackageDataObserver.Stub {
        public void onRemoveCompleted(final String packageName, final boolean succeeded) {
        }
    }
    */
    public void sendBroadcastToTvapp(String extra) {
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_UPDATE_TV_PLAY);
        intent.putExtra("tv_play_extra", extra);
        mContext.sendBroadcast(intent);
    }
    public void sendBroadcastToTvapp(String action, Bundle bundle) {
        Intent intent = new Intent(action);
        intent.putExtra(DroidLogicTvUtils.EXTRA_MORE, bundle);
        mContext.sendBroadcast(intent);
    }

    public void startTvPlayAndSetSourceInput() {
            mTvControlManager.StartTv();
            int deviceId = DroidLogicTvUtils.getHardwareDeviceId(mInputId);
            //Log.e(TAG,"deviceId:"+deviceId);
            mTvControlManager.SetSourceInput(mTvSourceInput, DroidLogicTvUtils.parseTvSourceInputFromDeviceId(deviceId));
        }

    public void deleteChannels(String type) {
            mTvDataBaseManager.deleteChannels(mInputId, type);
    }

    public void deleteAtvOrDtvChannels(boolean isatv) {
        mTvDataBaseManager.deleteAtvOrDtvChannels(isatv);
    }

    public void deleteOtherTypeAtvOrDtvChannels(String type, boolean isatv) {
        mTvDataBaseManager.deleteOtherTypeAtvOrDtvChannels(type, isatv);
    }
}
