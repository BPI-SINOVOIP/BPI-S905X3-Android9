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
import android.content.pm.ResolveInfo;
import android.media.tv.TvContentRating;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputHardwareInfo;
import android.media.tv.TvStreamConfig;
import android.media.tv.TvInputManager.Hardware;
import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;

import com.droidlogic.tvinput.Utils;

import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.DroidLogicTvInputService;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvInputBaseSession;
import com.droidlogic.tvinput.R;

import java.util.HashSet;
import java.util.Set;
import com.droidlogic.app.tv.TvControlManager;

import java.util.HashMap;
import java.util.Map;
import android.view.Surface;

public class ATVInputService extends DroidLogicTvInputService {

    private static final String TAG = "ATVInputService";

    private ATVSessionImpl mCurrentSession;
    private ChannelInfo mCurrentChannel = null;
    private int id = 0;
    private Map<Integer, ATVSessionImpl> sessionMap = new HashMap<>();


    private final BroadcastReceiver mParentalControlsBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (mCurrentSession != null) {
                mCurrentSession.checkContentBlockNeeded();
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        initInputService(DroidLogicTvUtils.DEVICE_ID_ATV, ATVInputService.class.getName());

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(TvInputManager.ACTION_BLOCKED_RATINGS_CHANGED);
        intentFilter.addAction(TvInputManager.ACTION_PARENTAL_CONTROLS_ENABLED_CHANGED);
        registerReceiver(mParentalControlsBroadcastReceiver, intentFilter);
        registerChannelScanStartReceiver();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mParentalControlsBroadcastReceiver);
        unRegisterChannelScanStartReceiver();
    }

    @Override
    public Session onCreateSession(String inputId) {
        super.onCreateSession(inputId);

        mCurrentSession = new ATVSessionImpl(this, inputId, getHardwareDeviceId(inputId));
        mCurrentSession.setSessionId(id);
        registerInputSession(mCurrentSession);
        sessionMap.put(id, mCurrentSession);
        id++;
        return mCurrentSession;
    }

    @Override
    public void setCurrentSessionById(int sessionId) {
        Utils.logd(TAG, "setCurrentSessionById:"+sessionId);
        ATVSessionImpl session = sessionMap.get(sessionId);
        if (session != null) {
            mCurrentSession = session;
        }
    }

    @Override
    public void doTuneFinish(int result, Uri uri, int sessionId) {
        Utils.logd(TAG, "doTuneFinish,result:"+result+"sessionId:"+sessionId);
        if (result == ACTION_SUCCESS) {
            ATVSessionImpl session = sessionMap.get(sessionId);
            if (session != null)
                session.switchToSourceInput(uri);
        }
    }

    public class ATVSessionImpl extends TvInputBaseSession {
        private final Context mContext;
        private TvInputManager mTvInputManager;
        private TvControlManager mTvControlManager;
        private TvDataBaseManager mTvDataBaseManager;
        private TvContentRating mLastBlockedRating;
        private TvContentRating mCurrentContentRating;
        private final Set<TvContentRating> mUnblockedRatingSet = new HashSet<>();

        protected ATVSessionImpl(Context context, String inputId, int deviceId) {
            super(context, inputId, deviceId);

            mContext = context;
            mTvDataBaseManager = new TvDataBaseManager(mContext);
            mTvControlManager = TvControlManager.getInstance();
            mLastBlockedRating = null;
            mCurrentChannel = null;

            initOverlayView(R.layout.layout_overlay);
            if (mOverlayView != null) {
                mOverlayView.setImage(R.drawable.bg_no_signal);
            }
        }
        @Override
        public boolean onSetSurface(Surface surface) {
            super.onSetSurface(surface);
            return setSurfaceInService(surface,this);
        }


        @Override
        public boolean onTune(Uri channelUri) {
            return doTuneInService(channelUri, getSessionId());
        }


        public void doRelease() {
            super.doRelease();
            if (sessionMap.containsKey(getSessionId())) {
                sessionMap.remove(getSessionId());
                mCurrentSession = null;
                registerInputSession(null);
            }
        }

        @Override
        public void doAppPrivateCmd(String action, Bundle bundle) {
            //super.doAppPrivateCmd(action, bundle);
            if (TextUtils.equals(DroidLogicTvUtils.ACTION_STOP_TV, action)) {
                if (mHardware != null) {
                    mHardware.setSurface(null, null);
                }
            } /*else if (DroidLogicTvUtils.ACTION_ATV_AUTO_SCAN.equals(action)
                || DroidLogicTvUtils.ACTION_ATV_MANUAL_SCAN.equals(action)) {
                resetScanStoreListener();
            }*/
        }

        @Override
        public void notifyVideoUnavailable(int reason) {
            super.notifyVideoAvailable();
            if (mOverlayView != null) {
                mOverlayView.setImage(R.drawable.bg_no_signal);
                mOverlayView.setImageVisibility(true);
                mOverlayView.setTextVisibility(true);
            }
        }

        @Override
        public void notifyVideoAvailable() {
            Log.d(TAG, "notifyVideoAvailable "+getSessionId());
            super.notifyVideoAvailable();

            if (mCurrentChannel !=null && mCurrentChannel.isAnalogChannel()) {
                openTvAudio(DroidLogicTvUtils.SOURCE_TYPE_ATV);
            }
        }

        @Override
        public void doUnblockContent(TvContentRating rating) {
            super.doUnblockContent(rating);
            if (rating != null) {
                unblockContent(rating);
            }
        }

        private void switchToSourceInput(Uri uri) {
            mUnblockedRatingSet.clear();
            Log.d(TAG, "switchToSourceInput  uri="+ uri);
            if (Utils.getChannelId(uri) < 0) {
                Log.d(TAG, "ChannelId < 0, stop Play");
                mTvControlManager.SetFrontendParms(TvControlManager.tv_fe_type_e.TV_FE_ANALOG,
                                     45250000,// try to get the tune into unlock status
                                     TvControlManager.tvin_color_system_e.COLOR_SYSTEM_PAL.toInt(),
                                     TvControlManager.ATV_AUDIO_STD_DK, 0, 0, 0, 0);
                releasePlayer();
                notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);
                mCurrentChannel = null;
                return;
            }
            mCurrentChannel = mTvDataBaseManager.getChannelInfo(uri);
            if (mCurrentChannel != null) {
                playProgram(mCurrentChannel);
            } else {
                Log.w(TAG, "Failed to get channel info for " + uri);
            }
        }

        private boolean playProgram(ChannelInfo info) {
            info.print();
            /* open atv audio and mute when notify Video Available */
            openTvAudio(DroidLogicTvUtils.SOURCE_TYPE_ATV);
            mTvControlManager.PlayATVProgram(info.getFrequency() + info.getFineTune(),
                info.getVideoStd(),
                info.getAudioStd(),
                info.getVfmt(),
                info.getAudioOutPutMode(),
                0,
                info.getAudioCompensation());
            checkContentBlockNeeded();
            return true;
        }

        private void checkContentBlockNeeded() {
            if (mCurrentContentRating == null || !mTvInputManager.isParentalControlsEnabled()
                || !mTvInputManager.isRatingBlocked(mCurrentContentRating)
                || mUnblockedRatingSet.contains(mCurrentContentRating)) {
                // Content rating is changed so we don't need to block anymore.
                // Unblock content here explicitly to resume playback.
                unblockContent(null);
                return;
            }

            mLastBlockedRating = mCurrentContentRating;
            // Children restricted content might be blocked by TV app as well,
            // but TIS should do its best not to show any single frame of blocked content.
            releasePlayer();

            Log.d(TAG, "notifyContentBlocked [rating:" + mCurrentContentRating + "]");
            notifyContentBlocked(mCurrentContentRating);
        }

        private void unblockContent(TvContentRating rating) {
            // TIS should unblock content only if unblock request is legitimate.
            if (rating == null || mLastBlockedRating == null
                    || (mLastBlockedRating != null && rating.equals(mLastBlockedRating))) {
                mLastBlockedRating = null;
                if (rating != null) {
                    mUnblockedRatingSet.add(rating);
                }
                Log.d(TAG, "notifyContentAllowed");
                notifyContentAllowed();
            }
        }
    }

    public static final class TvInput {
        public final String displayName;
        public final String name;
        public final String description;
        public final String logoThumbUrl;
        public final String logoBackgroundUrl;

        public TvInput(String displayName, String name, String description, String logoThumbUrl,
                String logoBackgroundUrl) {
            this.displayName = displayName;
            this.name = name;
            this.description = description;
            this.logoThumbUrl = logoThumbUrl;
            this.logoBackgroundUrl = logoBackgroundUrl;
        }
    }

    public TvInputInfo onHardwareAdded(TvInputHardwareInfo hardwareInfo) {
        if (hardwareInfo.getDeviceId() != DroidLogicTvUtils.DEVICE_ID_ATV
                || hasInfoExisted(hardwareInfo))
            return null;

        Log.d(TAG, "=====onHardwareAdded=====" + hardwareInfo.getDeviceId());

        TvInputInfo info = null;
        ResolveInfo rInfo = getResolveInfo(ATVInputService.class.getName());
        if (rInfo != null) {
            try {
                info = TvInputInfo.createTvInputInfo(this, rInfo, hardwareInfo,
                        getTvInputInfoLabel(hardwareInfo.getDeviceId()), null);
            } catch (Exception e) {
            }
        }
        updateInfoListIfNeededLocked(hardwareInfo, info, false);
        acquireHardware(info);
        return info;
    }

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

    @Override
    public void onUpdateCurrentChannel(ChannelInfo channel, boolean store) {
        if (store) {
            TvDataBaseManager mTvDataBaseManager = new TvDataBaseManager(this);
            mTvDataBaseManager.updateOrinsertAtvChannel(mCurrentChannel, channel);
        }
    }
}
