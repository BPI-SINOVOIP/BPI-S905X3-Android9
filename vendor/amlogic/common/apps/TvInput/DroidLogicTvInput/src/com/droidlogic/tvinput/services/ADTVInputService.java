/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.services;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ContentUris;
import android.content.pm.ResolveInfo;
import android.media.AudioRoutesInfo;
import android.media.AudioSystem;
import android.media.IAudioRoutesObserver;
import android.media.IAudioService;
import android.media.tv.TvContentRating;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputHardwareInfo;
import android.media.tv.TvTrackInfo;
import android.media.tv.TvContract;
import android.media.tv.TvStreamConfig;
import android.media.tv.TvInputManager.Hardware;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.database.ContentObserver;
//import android.database.IContentObserver;
import android.provider.Settings;

import com.droidlogic.tvinput.Utils;

import com.droidlogic.app.tv.DroidLogicTvInputService;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.TvChannelParams;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.TvInputBaseSession;
import com.droidlogic.app.tv.Program;
import com.droidlogic.app.tv.TvMultilingualText;
import com.droidlogic.app.tv.TvTime;
import com.droidlogic.app.tv.TvStoreManager;
import com.droidlogic.app.tv.TvControlDataManager;
import com.droidlogic.app.SystemControlManager;

import java.util.HashSet;
import java.util.Set;
import java.util.Map;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Arrays;

import com.droidlogic.app.tv.TvControlManager;

import java.util.HashMap;
import java.util.Map;
import android.net.Uri;
import android.view.Surface;
import org.json.JSONArray;
import org.json.JSONObject;

public class ADTVInputService extends DTVInputService {

    private static final String TAG = "ADTVInputService";
    private IAudioService mAudioService;
    private AudioRoutesInfo mCurAudioRoutesInfo;
    private ChannelInfo.Audio mCurrentAudio;
    private Runnable mHandleTvAudioRunnable;
    private int mDelayTime;
    final IAudioRoutesObserver.Stub mAudioRoutesObserver = new IAudioRoutesObserver.Stub() {
        @Override
        public void dispatchAudioRoutesChanged(final AudioRoutesInfo newRoutes) {
            if ((mCurrentSession != null && mCurrentSession.mCurrentChannel != null &&
                    mCurrentSession.mHandler != null) &&
                    ((newRoutes.mainType != mCurAudioRoutesInfo.mainType) ||
                        (!newRoutes.toString().equals(mCurAudioRoutesInfo.toString())))) {
                mCurAudioRoutesInfo = newRoutes;
                mCurrentSession.mHandler.removeCallbacks(mHandleTvAudioRunnable);
                mDelayTime = newRoutes.mainType != AudioRoutesInfo.MAIN_HDMI ? 500 : 1500;
                mHandleTvAudioRunnable = new Runnable() {
                    public void run() {
                        if (mCurrentSession.mCurrentChannel.isAnalogChannel()) {
                            mCurrentSession.openTvAudio(DroidLogicTvUtils.SOURCE_TYPE_ATV);
                        } else {
                            mCurrentAudio = null;
                            mCurrentSession.openTvAudio(DroidLogicTvUtils.SOURCE_TYPE_DTV);
                            if ((mCurrentSession.getAudioAuto(mCurrentSession.mCurrentChannel) >= 0) &&
                                    (mCurrentSession.mCurrentAudios != null)) {
                                mCurrentAudio = mCurrentSession.mCurrentAudios.get(
                                        mCurrentSession.getAudioAuto(mCurrentSession.mCurrentChannel));
                            }
                            AudioSystem.setParameters("fmt=" +
                                    ((mCurrentAudio != null) ? String.valueOf(mCurrentAudio.mFormat) : "-1"));
                            AudioSystem.setParameters("has_dtv_video=" +
                                    (ChannelInfo.isVideoChannel(mCurrentSession.mCurrentChannel) ?
                                            "1" : "0"));
                            AudioSystem.setParameters("cmd=1");
                        }
                    }
                };
                try {
                    mCurrentSession.mHandler.postDelayed(mHandleTvAudioRunnable,
                            mAudioService.isBluetoothA2dpOn() ? mDelayTime + 2000 : mDelayTime);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            } else if (mCurrentSession == null) {
                mCurAudioRoutesInfo = newRoutes;
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        initInputService(DroidLogicTvUtils.DEVICE_ID_ADTV, ADTVInputService.class.getName());
        IBinder b = ServiceManager.getService(Context.AUDIO_SERVICE);
        mAudioService = IAudioService.Stub.asInterface(b);
        try {
            mCurAudioRoutesInfo = mAudioService.startWatchingRoutes(mAudioRoutesObserver);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    @Override
    public Session onCreateSession(String inputId) {
        registerInput(inputId);
        mCurrentSession = new ADTVSessionImpl(this, inputId, getHardwareDeviceId(inputId));
        mCurrentSession.setSessionId(id);
        registerInputSession(mCurrentSession);
        sessionMap.put(id, mCurrentSession);
        id++;

        return mCurrentSession;
    }

    public class ADTVSessionImpl extends DTVInputService.DTVSessionImpl implements TvControlManager.AVPlaybackListener, TvControlManager.AudioEventListener {

        protected ADTVSessionImpl(Context context, String inputId, int deviceId) {
            super(context, inputId, deviceId);
        }

        @Override
        protected void checkContentBlockNeeded(ChannelInfo channelInfo) {
            doParentalControls(channelInfo);
        }

        @Override
        protected boolean playProgram(ChannelInfo info) {
            if (info == null)
                return false;

            if (mSurface == null) {
                Log.d(TAG, "surface is null, drop playProgram");
                return true;
            }

            info.print();

            int audioAuto = getAudioAuto(info);
            ChannelInfo.Audio audio = null;
            if (mCurrentAudios != null && mCurrentAudios.size() > 0 && audioAuto >= 0)
                audio = mCurrentAudios.get(audioAuto);

            if (info.isAnalogChannel()) {
                if (info.isNtscChannel())
                    muteVideo(false);

                /* open atv audio and mute when notify Video Available */
                openTvAudio(DroidLogicTvUtils.SOURCE_TYPE_ATV);
                if (false) {
                    mTvControlManager.PlayATVProgram(info.getFrequency() + info.getFineTune(),
                        info.getVideoStd(),
                        info.getAudioStd(),
                        info.getVfmt(),
                        info.getAudioOutPutMode(),
                        0,
                        info.getAudioCompensation());
                } else {
                    TvControlManager.FEParas fe = new TvControlManager.FEParas();
                    fe.setFrequency(info.getFrequency() + info.getFineTune());
                    fe.setVideoStd(info.getVideoStd());
                    fe.setAudioStd(info.getAudioStd());
                    fe.setVfmt(info.getVfmt());
                    fe.setAudioOutPutMode(info.getAudioOutPutMode());
                    fe.setAfc(0);
                    StringBuilder param = new StringBuilder("{")
                        .append("\"type\":\"atv\"")
                        .append("," + fe.toString("fe"))
                        .append(",\"a\":{\"AudComp\":"+info.getAudioCompensation()+"}")
                        .append("}");

                    mTvControlManager.startPlay("ntsc", param.toString());
                }
            } else {
                String subPidString = new String("\"pid\":0");
                int subCount = (info.getSubtitlePids() == null)? 0 : info.getSubtitlePids().length;
                if (subCount != 0) {
                    subPidString = new String("");
                    for (int i = 0; i < subCount; i++) {
                        subPidString = subPidString + "\"pid" + i+ "\":" + info.getSubtitlePids()[i];
                        if (i != (subCount - 1 ))
                            subPidString = subPidString + ",";
                    }
                    Log.e(TAG, "subpid string: " + subPidString);
                }
                TvControlManager.FEParas fe = new TvControlManager.FEParas(info.getFEParas());
                int mixingLevel = mAudioADMixingLevel;
                if (mixingLevel < 0)
                    mixingLevel = mTvControlDataManager.getInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_MIX, AD_MIXING_LEVEL_DEF);

                Log.d(TAG, "v:"+info.getVideoPid()+" a:"+(audio!=null?audio.mPid:"null")+" p:"+info.getPcrPid());
                mTvControlManager.SetAVPlaybackListener(this);
                mTvControlManager.SetAudioEventListener(this);
                openTvAudio(DroidLogicTvUtils.SOURCE_TYPE_DTV);
                if (false) {
                    mTvControlManager.PlayDTVProgram(
                        fe,
                        info.getVideoPid(),
                        info.getVfmt(),
                        (audio != null) ? audio.mPid : -1,
                        (audio != null) ? audio.mFormat : -1,
                        info.getPcrPid(),
                        info.getAudioCompensation(),
                        DroidLogicTvUtils.hasAudioADTracks(info),
                        mixingLevel);
                } else {
                    //("{\"fe\":{\"mod\":7,\"freq\":785028615,\"mode\":16777219},\"v\":{\"pid\":33,\"fmt\":0},\"a\":{\"pid\":36,\"fmt\":3},\"p\":{\"pid\":33}}")
                    int timeshiftMaxTime = mSystemControlManager.getPropertyInt("tv.dtv.tf.max.time", 10*60);/*seconds*/
                    int timeshiftMaxSize = mSystemControlManager.getPropertyInt(MAX_CACHE_SIZE_KEY, MAX_CACHE_SIZE_DEF * 1024);/*bytes*/
                    String timeshiftPath = mSystemControlManager.getPropertyString("tv.dtv.tf.path", getCacheStoragePath());
                    StringBuilder param = new StringBuilder("{")
                        .append("\"fe\":" + info.getFEParas())
                        .append(",\"v\":{\"pid\":"+info.getVideoPid()+",\"fmt\":"+info.getVfmt()+"}")
                        .append(",\"a\":{\"pid\":"+(audio != null ? audio.mPid : -1)+",\"fmt\":"+(audio != null ? audio.mFormat : -1)+",\"AudComp\":"+info.getAudioCompensation()+"}")
                        .append(",\"p\":{\"pid\":"+info.getPcrPid()+"}")
                        //.append(",\"para\":{"+"\"disableTimeShifting\":1"+"}")
                        .append(",\"para\":{")
                        .append("\"max\":{"+"\"time\":"+timeshiftMaxTime+"}")//",\"size\":"+timeshiftMaxSize+
                        .append(",\"path\":\""+timeshiftPath+"\"")
                        .append(",\"subpid\":{" + subPidString)
                        .append("},\"subcnt\":" + subCount)
                        .append("}}");
                    Log.d(TAG, "playProgram adtvparam: " + param.toString());

                    mTvControlManager.startPlay("atsc", param.toString());
                    initTimeShiftStatus();
                }
                mTvControlManager.DtvSetAudioChannleMod(info.getAudioChannel());
            }

            mSystemControlManager.setProperty(DTV_AUDIO_TRACK_IDX,
                        ((audioAuto>=0)? String.valueOf(audioAuto) : "-1"));
            mSystemControlManager.setProperty(DTV_AUDIO_TRACK_ID, generateAudioIdString(audio));

            notifyTracks(info);

            tryStartSubtitle(info);
            if (!info.isAnalogChannel()) {
                startAudioADMainMix(info, audioAuto);
            }

            isTvPlaying = true;
            return true;
        }

        protected void muteVideo(boolean mute) {
            if (mute)
                mSystemControlManager.writeSysFs("/sys/class/deinterlace/di0/config", "hold_video 1");
            else
                mSystemControlManager.writeSysFs("/sys/class/deinterlace/di0/config", "hold_video 0");
        }

        @Override
        protected boolean startSubtitle(ChannelInfo channelInfo) {
            if (super.startSubtitle(channelInfo))
                return true;

            if (channelInfo != null && channelInfo.isNtscChannel()) {
                startSubtitleCCBackground(channelInfo);
                return true;
            }
            return false;
        }

        @Override
        protected void stopSubtitleBlock(ChannelInfo channel) {
            Log.d(TAG, "stopSubtitleBlock");

            if (channel.isNtscChannel())
                startSubtitleCCBackground(channel);
            else
                super.stopSubtitleBlock(channel);
        }

        private TvContentRating[] mATVContentRatings = null;

        @Override
        protected boolean isAtsc(ChannelInfo info) {
            return info.isNtscChannel() || super.isAtsc(info);
        }

        @Override
        protected boolean tryPlayProgram(ChannelInfo info) {
            mATVContentRatings = null;
            return super.tryPlayProgram(info);
        }

        @Override
        protected TvContentRating[] getContentRatingsOfCurrentProgram(ChannelInfo channelInfo) {
            if (channelInfo != null && channelInfo.isAnalogChannel())
                return Program.stringToContentRatings(channelInfo.getContentRatings());
                //return mATVContentRatings;
            else
                return super.getContentRatingsOfCurrentProgram(channelInfo);
        }

        @Override
        public void onSubtitleData(String json) {
//            Log.d(TAG, "onSubtitleData json: " + json);
//            Log.d(TAG, "onSubtitleData curchannel:"+(mCurrentChannel!=null?mCurrentChannel.getDisplayName():"null"));
//            if (mCurrentChannel != null && mCurrentChannel.isAnalogChannel()) {
//                mATVContentRatings = DroidLogicTvUtils.parseARatings(json);
//                if (/*mATVContentRatings != null && */json.contains("Aratings") && !TextUtils.equals(json, mCurrentChannel.getContentRatings())) {
//                    TvDataBaseManager tvdatabasemanager = new TvDataBaseManager(mContext);
//                    mCurrentChannel.setContentRatings(json);
//                    tvdatabasemanager.updateOrinsertAtvChannel(mCurrentChannel);
//                }
//
//                if (mHandler != null)
//                    mHandler.sendMessage(mHandler.obtainMessage(MSG_PARENTAL_CONTROL, this));
//            }
            super.onSubtitleData(json);
        }

        public String onReadSysFs(String node) {
            return super.onReadSysFs(node);
        }

        public void onWriteSysFs(String node, String value) {
            super.onWriteSysFs(node, value);
        }

        @Override
        protected void setMonitor(ChannelInfo channel) {
            if (channel == null || !channel.isAnalogChannel())
                super.setMonitor(channel);
        }

        @Override
        public void onEvent(int msgType, int programID) {
            Log.d(TAG, "AV evt:" + msgType);
            super.onEvent(msgType, programID);
        }

        @Override
        public void HandleAudioEvent(int cmd, int param1, int param2) {
            Log.d(TAG, "audio cmd" + cmd);
            super.HandleAudioEvent(cmd, param1, param2);
         }
    }

    @Override
    public TvInputInfo onHardwareAdded(TvInputHardwareInfo hardwareInfo) {
        if (hardwareInfo.getDeviceId() != DroidLogicTvUtils.DEVICE_ID_ADTV)
            return null;

        Log.d(TAG, "=====onHardwareAdded=====" + hardwareInfo.getDeviceId());

        TvInputInfo info = null;
        ResolveInfo rInfo = getResolveInfo(ADTVInputService.class.getName());
        if (rInfo != null) {
            try {
                info = TvInputInfo.createTvInputInfo(
                           getApplicationContext(),
                           rInfo,
                           hardwareInfo,
                           getTvInputInfoLabel(hardwareInfo.getDeviceId()),
                           null);
            } catch (Exception e) {
                // TODO: handle exception
                 e.printStackTrace();
            }
        }
        updateInfoListIfNeededLocked(hardwareInfo, info, false);
        acquireHardware(info);
        return info;
    }

    @Override
    public String onHardwareRemoved(TvInputHardwareInfo hardwareInfo) {
        if (hardwareInfo.getType() != TvInputHardwareInfo.TV_INPUT_TYPE_TUNER)
            return null;

        TvInputInfo info = getTvInputInfo(hardwareInfo);
        String id = null;
        if (info != null)
            id = info.getId();

        updateInfoListIfNeededLocked(hardwareInfo, info, true);
        releaseHardware();
        Log.d(TAG, "=====onHardwareRemoved===== " + id);
        return id;
    }


}
