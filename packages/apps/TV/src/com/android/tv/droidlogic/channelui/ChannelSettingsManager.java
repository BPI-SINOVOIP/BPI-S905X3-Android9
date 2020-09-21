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
 * limitations under the License.
 */

package com.android.tv.droidlogic.channelui;

import android.media.tv.TvContract;
import android.content.res.Resources;
import android.util.Log;
import android.provider.Settings;
import android.os.Bundle;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.content.Context;
import android.net.Uri;
import android.text.TextUtils;
import android.content.Intent;
import android.widget.Toast;

import com.android.tv.R;
import com.android.tv.MainActivity;
import com.android.tv.TvApplication;
import com.android.tv.data.api.Channel;
import com.android.tv.TvSingletons;

import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvControlDataManager;
import com.droidlogic.app.tv.TvChannelSetting;

import java.util.ArrayList;
import java.util.HashMap;

public class ChannelSettingsManager {
    private final static String TAG = "ChannelSettingsManager";
    private final static boolean DEBUG = true;
    private Resources mResources;
    private Context mContext;

    public static final String DTVKIT_PACKAGE = "org.dtvkit.inputsource";
    public static final String KEY_INFO_ITEM = "item_key";
    public static final String VALUE_STRENGTH = "strength";
    public static final String VALUE_QUALITY = "quality";

    //channel info
    public static final String STRING_NAME = "name";
    public static final String STRING_STATUS = "status";
    public static final String STRING_PRIVATE = "private";

    private TvControlManager.SourceInput_Type mVirtualTvSource;
    private TvControlManager.SourceInput_Type mTvSource;
    private TvInputInfo mCurrentTvinputInfo = null;
    private String mInputId;
    private int mDeviceId = -1;
    private long mChannelId = -1;
    private MainActivity mMainActivity;
    private int mStrength;
    private int mQuality;

    private TvDataBaseManager mTvDataBaseManager;
    private SystemControlManager mSystemControlManager;
    private TvControlManager mTvControlManager;
    private ChannelInfo mCurrentChannel;

    public ChannelSettingsManager (Context context) {
        this.mMainActivity = (MainActivity) context;
        this.mContext = context;
        this.mResources = context.getResources();
        this.mCurrentTvinputInfo = mMainActivity.mQuickKeyInfo.getCurrentTvInputInfo();
        this.mDeviceId = mMainActivity.mQuickKeyInfo.getDeviceIdFromInfo(mCurrentTvinputInfo);
        this.mChannelId = mMainActivity.getCurrentChannelId();

        mTvDataBaseManager = new TvDataBaseManager(context);
        mSystemControlManager = SystemControlManager.getInstance();
        mTvControlManager = TvControlManager.getInstance();
        mStrength = mTvControlManager.DtvGetSignalStrength();

        initParameters();
    }

    private void initParameters() {
        RefreshTvSourceType();
    }

    public ArrayList<ChannelInfo> getVideoChannelList() {
        return  mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO, false);
    }

    public ArrayList<ChannelInfo> getRadioChannelList() {
        return mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_AUDIO, false);
    }

    public ArrayList<HashMap<String, String>> getChannelInfoStatus() {
        ArrayList<HashMap<String, String>> list =  new ArrayList<HashMap<String, String>>();
        TvControlManager.SourceInput_Type tvSource = DroidLogicTvUtils.parseTvSourceTypeFromDeviceId(mDeviceId);
        TvControlManager.SourceInput_Type virtualTvSource = tvSource;
        if (tvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            if (mCurrentChannel != null) {
                tvSource = DroidLogicTvUtils.parseTvSourceTypeFromSigType(DroidLogicTvUtils.getSigType(mCurrentChannel));
            }
            if (virtualTvSource == tvSource) {//no channels in adtv input, DTV for default.
                tvSource = TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV;
            }
        }
        if (mCurrentChannel != null) {
            String inputId = mCurrentChannel.getInputId();
            boolean isDtvKitChannel = inputId != null && inputId.startsWith(DTVKIT_PACKAGE);
            HashMap<String, String> item = new HashMap<String, String>();
            item.put(STRING_NAME, mResources.getString(R.string.channel_info_channel));
            item.put(STRING_STATUS, mCurrentChannel.getDisplayName());
            list.add(item);

            item = new HashMap<String, String>();
            item.put(STRING_NAME, mResources.getString(R.string.channel_info_frequency));
            item.put(STRING_STATUS, Integer.toString(mCurrentChannel.getFrequency() + mCurrentChannel.getFineTune()));
            list.add(item);

            if (mCurrentChannel.isDigitalChannel() || isDtvKitChannel) {
                if (mStrength > 0 && mStrength <= 100) {
                    item = new HashMap<String, String>();
                    item.put(STRING_NAME, mResources.getString(R.string.channel_info_strength));
                    item.put(STRING_STATUS, mStrength + "%");
                    item.put(KEY_INFO_ITEM, VALUE_STRENGTH);
                    list.add(item);
                } else {
                    item = new HashMap<String, String>();
                    item.put(STRING_NAME, mResources.getString(R.string.channel_info_strength));
                    item.put(STRING_STATUS, "0%");
                    item.put(KEY_INFO_ITEM, VALUE_STRENGTH);
                    list.add(item);
                }
                if (isDtvKitChannel) {
                    item = new HashMap<String, String>();
                    item.put(STRING_NAME, mResources.getString(R.string.channel_info_quality));
                    item.put(STRING_STATUS, mQuality + "%");
                    item.put(KEY_INFO_ITEM, VALUE_QUALITY);
                    list.add(item);
                }
            //Toast.makeText(mContext, Integer.toString(mStrength), Toast.LENGTH_SHORT).show();
            }
            if (tvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV || isDtvKitChannel) {
                item = new HashMap<String, String>();
                item.put(STRING_NAME, mResources.getString(R.string.channel_info_type));
                item.put(STRING_STATUS, mCurrentChannel.getType());
                list.add(item);

                item = new HashMap<String, String>();
                item.put(STRING_NAME, mResources.getString(R.string.channel_info_service_id));
                item.put(STRING_STATUS, Integer.toString(mCurrentChannel.getServiceId()));
                list.add(item);
                if (!isDtvKitChannel) {
                    item = new HashMap<String, String>();
                    item.put(STRING_NAME, mResources.getString(R.string.channel_info_pcr_id));
                    item.put(STRING_STATUS, Integer.toString(mCurrentChannel.getPcrPid()));
                    list.add(item);
                }
            }
        }
        return list;
    }

    public ArrayList<HashMap<String, String>> getAudioADList () {
        ArrayList<HashMap<String, String>> list =  new ArrayList<HashMap<String, String>>();
        if (mCurrentChannel != null && mCurrentChannel.getAudioLangs() != null && DroidLogicTvUtils.hasAudioADTracks(mCurrentChannel)) {
            int mainTrackIndex = mSystemControlManager.getPropertyInt("tv.dtv.audio_track_idx", -1);
            if (DEBUG) Log.d(TAG, "mainTrackIndex["+mainTrackIndex+"]");
            if (mainTrackIndex < 0)
                return list;

            int[] adAudioIdx = DroidLogicTvUtils.getAudioADTracks(mCurrentChannel, mainTrackIndex);
            if (adAudioIdx != null) {
                for (int i = 0; i < adAudioIdx.length; i++) {
                    HashMap<String, String> item = new HashMap<String, String>();
                    item.put(STRING_NAME, mCurrentChannel.getAudioLangs()[adAudioIdx[i]]);
                    item.put(STRING_PRIVATE, String.valueOf(adAudioIdx[i]));
                    list.add(item);
                }
            }
        }
        return list;
    }

    public ArrayList<HashMap<String, String>> getSoundChannelList () {
        ArrayList<HashMap<String, String>> list =  new ArrayList<HashMap<String, String>>();

        HashMap<String, String> item = new HashMap<String, String>();
        item.put(STRING_NAME, mResources.getString(R.string.channel_sound_channel_stereo));
        list.add(item);

        item = new HashMap<String, String>();
        item.put(STRING_NAME, mResources.getString(R.string.channel_sound_channel_left));
        list.add(item);

        item = new HashMap<String, String>();
        item.put(STRING_NAME, mResources.getString(R.string.channel_sound_channel_right));
        list.add(item);

        return list;
    }

    public ArrayList<HashMap<String, String>> getChannelVideoList () {
        ArrayList<HashMap<String, String>> list =  new ArrayList<HashMap<String, String>>();

        HashMap<String, String> item = new HashMap<String, String>();
        item.put(STRING_NAME, mResources.getString(R.string.channel_video_pal));
        list.add(item);

        item = new HashMap<String, String>();
        item.put(STRING_NAME, mResources.getString(R.string.channel_video_ntsc));
        list.add(item);

        item = new HashMap<String, String>();
        item.put(STRING_NAME, mResources.getString(R.string.channel_video_secam));
        list.add(item);

        return list;
    }

    public ArrayList<HashMap<String, String>> getChannelAudioList () {
        ArrayList<HashMap<String, String>> list =  new ArrayList<HashMap<String, String>>();

        HashMap<String, String> item = new HashMap<String, String>();
        item.put(STRING_NAME, mResources.getString(R.string.channel_audio_dk));
        list.add(item);

        item = new HashMap<String, String>();
        item.put(STRING_NAME, mResources.getString(R.string.channel_audio_i));
        list.add(item);

        item = new HashMap<String, String>();
        item.put(STRING_NAME, mResources.getString(R.string.channel_audio_bg));
        list.add(item);

        item = new HashMap<String, String>();
        item.put(STRING_NAME, mResources.getString(R.string.channel_audio_m));
        list.add(item);

        item = new HashMap<String, String>();
        item.put(STRING_NAME, mResources.getString(R.string.channel_audio_l));
        list.add(item);

        return list;
    }

    public int getSoundChannelStatus () {
        int value = 0;
        if (mCurrentChannel != null) {
            value = mCurrentChannel.getAudioChannel();
            /*
            switch (currentChannel.getAudioChannel()) {
                case 0:
                    return mResources.getString(R.string.stereo);
                case 1:
                    return mResources.getString(R.string.left_channel);
                case 2:
                    return mResources.getString(R.string.right_channel);
                default:
                    return mResources.getString(R.string.stereo);
            }*/
        }
        if (value < 0 || value > 2) {
            value = 0;
        }
        if (DEBUG) Log.d(TAG, "[getSoundChannelStatus] value = " + value);
        return value;
    }

    public int getChannelVideoStatus () {
        int value = 0;
        if (mCurrentChannel != null) {
            value = mCurrentChannel.getVideoStd();
        }

        if (value == TvControlManager.ATV_VIDEO_STD_PAL) {
            value = 0;
        } else if (value == TvControlManager.ATV_VIDEO_STD_NTSC) {
            value = 1;
        } else if (value == TvControlManager.ATV_VIDEO_STD_SECAM) {
            value = 2;
        } else {
            value = 0;
        }

        if (DEBUG) Log.d(TAG, "[getChannelVideoStatus] value = " + value);
        return value;
    }

    public int getChannelAudioStatus () {
        int value = 0;
        if (mCurrentChannel != null) {
            value = mCurrentChannel.getAudioStd();
        }

        if (value == TvControlManager.ATV_AUDIO_STD_DK) {
            value = 0;
        } else if (value == TvControlManager.ATV_AUDIO_STD_I) {
            value = 1;
        } else if (value == TvControlManager.ATV_AUDIO_STD_BG) {
            value = 2;
        } else if (value == TvControlManager.ATV_AUDIO_STD_M) {
            value = 3;
        } else if (value == TvControlManager.ATV_AUDIO_STD_L) {
            value = 4;
        } else {
            value = 0;
        }

        if (DEBUG) Log.d(TAG, "[getChannelAudioStatus] value = " + value);
        return value;
    }

    public int getVolumeCompensateStatus () {
        int value = 0;
        if (mCurrentChannel != null)
            value = mCurrentChannel.getAudioCompensation();
        else
            value = 0;
        return value;
    }

    public int getSwitchChannelStatus () {
        if (mTvControlManager.getBlackoutEnable() == 0)
            return 0;
        else
            return 1;
    }

    public String getDtvType() {
        //int deviceId = DroidLogicTvUtils.getHardwareDeviceId(mInputId);
        MainActivity mMainActivity = (MainActivity)mContext;
        TvInputInfo tunerinputinfo = mMainActivity.mQuickKeyInfo.getTunerInput();
        int deviceId = mMainActivity.mQuickKeyInfo.getDeviceIdFromInfo(tunerinputinfo);
        Log.d(TAG, "========== deviceId: " + deviceId);
        String type = TvSingletons.getSingletons(mContext).getTvControlDataManager().getString(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_DTV_TYPE);
        if (type != null) {
            return type;
        } else {
            if (deviceId == DroidLogicTvUtils.DEVICE_ID_ADTV ) {
                return TvContract.Channels.TYPE_ATSC_T;
            } else {
                return TvContract.Channels.TYPE_DTMB;
            }
        }
    }

    public String getDtvTypeStatus() {
        String type = getDtvType();
        String ret = "";
        if (TextUtils.equals(type, TvContract.Channels.TYPE_DTMB)) {
                ret = mResources.getString(R.string.channel_dtv_type_dtmb);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_C)) {
                ret = mResources.getString(R.string.channel_dtv_type_dvbc);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_T)) {
                ret = mResources.getString(R.string.channel_dtv_type_dvbt);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_S)) {
                ret = mResources.getString(R.string.channel_dtv_type_dvbs);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_C2)) {
                ret = mResources.getString(R.string.channel_dtv_type_dvbc2);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_T2)) {
                ret = mResources.getString(R.string.channel_dtv_type_dvbt2);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_S2)) {
                ret = mResources.getString(R.string.channel_dtv_type_dvbs2);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ATSC_T)) {
                ret = mResources.getString(R.string.channel_dtv_type_atsc_t);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ATSC_C)) {
                ret = mResources.getString(R.string.channel_dtv_type_atsc_c);
        } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ISDB_T)) {
                ret = mResources.getString(R.string.channel_dtv_type_isdb_t);
        }
        return ret;
    }

    public void setDtvType(String type) {
        TvSingletons.getSingletons(mContext).getTvControlDataManager().putString(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_DTV_TYPE, type);
    }

    public void setSoundChannel (int mode) {
        if (mCurrentChannel != null) {
            mCurrentChannel.setAudioChannel(mode);
            mTvDataBaseManager.updateChannelInfo(mCurrentChannel);
            mTvControlManager.DtvSetAudioChannleMod(mCurrentChannel.getAudioChannel());
        }
    }

    public void setChannelVideo (int mode) {
        if (mCurrentChannel != null) {
            TvChannelSetting.setAtvChannelVideo(mCurrentChannel, mode);
            mTvDataBaseManager.updateChannelInfo(mCurrentChannel);
        }
    }

    public void setChannelAudio (int mode) {
        if (mCurrentChannel != null) {
            TvChannelSetting.setAtvChannelAudio(mCurrentChannel, mode);
            mTvDataBaseManager.updateChannelInfo(mCurrentChannel);
        }
    }

    public void setVolumeCompensate (int value) {
        if (mCurrentChannel != null) {
            int current = mCurrentChannel.getAudioCompensation();
            int offset = 0;
            if (value > current) {
                offset = 1;
            } else if (value < current) {
                offset = -1;
            }
            if ((current < 20 && offset > 0)
                || (current > -20 && offset < 0)) {
                mCurrentChannel.setAudioCompensation(current + offset);
                mTvDataBaseManager.updateChannelInfo(mCurrentChannel);
                mTvControlManager.SetCurProgVolumeCompesition(mCurrentChannel.getAudioCompensation());
            }
        }
    }

    public void setChannelName (long channelid, String targetName) {
        if (targetName == null) {
            return;
        }
        ChannelInfo channel = getChannelInfoById(channelid);
        if (ChannelInfo.isSameChannel(channel, mCurrentChannel)) {
            mCurrentChannel.setDisplayName(targetName);
        }
        if (channel != null) {
            mTvDataBaseManager.updateSingleChannelInternalProviderData(channelid, ChannelInfo.KEY_SET_DISPLAYNAME, "1");
            mTvDataBaseManager.updateSingleColumn(channelid, TvContract.Channels.COLUMN_DISPLAY_NAME, targetName);
        }
    }

    public void skipChannel (long channelid) {
        ChannelInfo channel = getChannelInfoById(channelid);
        //if (ChannelInfo.isSameChannel(currentChannel, channel))
        //setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);
        if (channel != null && channel.isOtherChannel()) {
            int isHidden = channel.getHidden();
            boolean isFav = channel.isFavourite();
            mTvDataBaseManager.updateSingleChannelInternalProviderData(channelid, ChannelInfo.KEY_HIDDEN, isHidden == 1 ? "false" : "true");
            if (isHidden == 0 && isFav) {
                mTvDataBaseManager.updateSingleChannelInternalProviderData(channelid, ChannelInfo.KEY_IS_FAVOURITE, "0");
            }
        } else {
            mTvDataBaseManager.skipChannel(channel);
        }
    }

    public  void deleteChannel (long channelid) {
        //ArrayList<ChannelInfo> channelList = getChannelInfoList(mTvSource);
        //if (ChannelInfo.isSameChannel(currentChannel,  channelList.get(channelNumber)))
        //setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);

        ChannelInfo channel = getChannelInfoById(channelid);
        if (channel == null) {
            return;
        }
        mTvDataBaseManager.deleteChannel(channel);
    }

    public void setFavouriteChannel (long channelid) {
        ChannelInfo channel = getChannelInfoById(channelid);
        if (channel != null && channel.isOtherChannel()) {
            boolean isFav = channel.isFavourite();
            int isHidden = channel.getHidden();
            mTvDataBaseManager.updateSingleChannelInternalProviderData(channelid, ChannelInfo.KEY_IS_FAVOURITE, isFav ? "0" : "1");
            if (!isFav && isHidden == 1) {
                mTvDataBaseManager.updateSingleChannelInternalProviderData(channelid, ChannelInfo.KEY_HIDDEN, "false");
            }
        } else {
            mTvDataBaseManager.setFavouriteChannel(channel);
        }
    }

    public ChannelInfo getChannelInfoById(long id) {
        if (DEBUG) Log.d(TAG, "[getChannelInfoById] id = " + id);
        if (id == -1) {
            return null;
        }

        ArrayList<ChannelInfo> videoChannelList = mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO, false);
        ArrayList<ChannelInfo> radioChannelList = mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_AUDIO, false);
        if (videoChannelList != null) {
            for (int i =0; i < videoChannelList.size(); i++) {
                if (videoChannelList.get(i).getId() == id) {
                    ChannelInfo temp = videoChannelList.get(i);
                    return temp;
                }
            }
        }
        if (radioChannelList!= null) {
            for (int i =0; i < radioChannelList.size(); i++) {
                if (radioChannelList.get(i).getId() == id) {
                    ChannelInfo temp = radioChannelList.get(i);
                    return temp;
                }
            }
        }
        return null;
    }

    public void setBlackoutEnable(int status, int isSave) {
        mTvControlManager.setBlackoutEnable(status, isSave);
    }

    private void RefreshTvSourceType() {
        mTvSource = DroidLogicTvUtils.parseTvSourceTypeFromDeviceId(mDeviceId);
        mVirtualTvSource = mTvSource;
        mCurrentChannel = mTvDataBaseManager.getChannelInfo(TvContract.buildChannelUri(mChannelId));

        if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            //currentChannel = mTvDataBaseManager.getChannelInfo(TvContract.buildChannelUri(channelid));
            if (mCurrentChannel != null) {
                mTvSource = DroidLogicTvUtils.parseTvSourceTypeFromSigType(DroidLogicTvUtils.getSigType(mCurrentChannel));
            }
            if (mVirtualTvSource == mTvSource) {//no channels in adtv input, DTV for default.
                mTvSource = TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV;
            }
        }
        if (mCurrentTvinputInfo != null) {
            mInputId = mCurrentTvinputInfo.getId();
        }
    }

    public TvControlManager.SourceInput_Type getCurentTvSource () {
        return mTvSource;
    }

    public TvControlManager.SourceInput_Type getCurentVirtualTvSource () {
        return mVirtualTvSource;
    }

    public void setAudioOutmode(int mode) {
        mTvControlManager.SetAudioOutmode(mode);
    }

    public int getAudioOutmode(){
        int mode = mTvControlManager.GetAudioOutmode();
        if (DEBUG)  Log.d(TAG, "getAudioOutmode"+" mode = " + mode);
        if (mode < TvControlManager.AUDIO_OUTMODE_MONO || mode > TvControlManager.AUDIO_OUTMODE_SAP) {
            mode = TvControlManager.AUDIO_OUTMODE_STEREO;
        }
        return mode;
    }

    public int getADSwitchStatus () {
        int switchVal = TvSingletons.getSingletons(mContext).getTvControlDataManager().getInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_SWITCH, 0);
        if (DEBUG) Log.d(TAG, "getADSwitchStatus = " + switchVal);
        if (switchVal != 0) {
            switchVal = 1;
        }
        return switchVal;
    }

    public int getADMixStatus () {
        int val = TvSingletons.getSingletons(mContext).getTvControlDataManager().getInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_MIX, 50);
        if (DEBUG) Log.d(TAG, "getADMixStatus = " + val);
        return val;
    }

    public void setAudioADSwitch (int switchVal) {
        if (DEBUG) Log.d(TAG, "setAudioADSwitch = " + switchVal);
        TvSingletons.getSingletons(mContext).getTvControlDataManager().putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_SWITCH, switchVal);
        mMainActivity.getTvView().applyAudioADSwitch(-1);
    }

    public void setADMix (int step) {
        if (DEBUG) Log.d(TAG, "setADMix = " + step);
        int level = getADMixStatus() + step;
        if (level <= 100 && level >= 0) {
            TvSingletons.getSingletons(mContext).getTvControlDataManager().putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_MIX, level);
            mMainActivity.getTvView().applyAudioADMixLevel(level);
        }
    }

    private static final int FineTuneRange = 2500 * 1000; /*Hz*/
    private static final int FineTuneStepSize = 50 * 1000; /*Hz*/
    public int getFrequency () {
        if (mCurrentChannel != null)
            return mCurrentChannel.getFrequency();
        else {
            return -1;
        }
    }

    public int getFineTuneStatus() {
        //return Integer.toString(2 * (getFineTuneProgress() - 50)) + "%";
        return 2 * (getFineTuneProgress() - 50);
    }

    public int getFineTune () {
        int value = 0;
        if (mCurrentChannel != null)
            value = mCurrentChannel.getFineTune();
        if (DEBUG) Log.d(TAG, "getFineTune = " + value);
        return value;
    }

    public int getFineTuneProgress () {
        int value = 50;
        if (mCurrentChannel != null) {
            int finetune = mCurrentChannel.getFineTune();
            value = 50 + ((finetune * 50) / FineTuneRange);
        }
        if (DEBUG) Log.d(TAG, "getFineTuneProgress = " + value);
        return value;
    }

    public void setFineTuneStep(int step) {
        if (mCurrentChannel != null) {
            int oldtune = mCurrentChannel.getFineTune();
            int finetune = oldtune + (step * FineTuneStepSize);

            if ((oldtune == finetune) || (finetune > FineTuneRange) || (finetune < -1 * FineTuneRange))
                return;

            setFineTune(finetune);
        }
    }

    public void setFineTune(int finetune) {
        if (mCurrentChannel != null) {
            TvChannelSetting.setAtvFineTune(mCurrentChannel, finetune);
            mTvDataBaseManager.updateChannelInfo(mCurrentChannel);
        }
    }

    public void updateChannelOrder(ChannelInfo channel) {
        if (DEBUG) Log.d(TAG, "updateChannelOrder number: " + channel.getDisplayNumber() + " ,name: " + channel.getDisplayName());
        if (channel.isOtherChannel()) {
            mTvDataBaseManager.updateSingleChannelInternalProviderData(channel.getId(), ChannelInfo.KEY_SET_DISPLAYNUMBER, "1");
            mTvDataBaseManager.updateSingleColumn(channel.getId(), TvContract.Channels.COLUMN_DISPLAY_NUMBER, channel.getDisplayNumber());
        } else {
            mTvDataBaseManager.updateChannelInfo(channel);
        }
    }
}
