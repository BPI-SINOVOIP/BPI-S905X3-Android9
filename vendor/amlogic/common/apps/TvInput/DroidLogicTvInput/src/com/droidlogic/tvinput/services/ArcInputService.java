/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.services;

import java.io.IOException;

import org.xmlpull.v1.XmlPullParserException;

import com.droidlogic.tvinput.Utils;

import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.DroidLogicTvInputService;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvInputBaseSession;
import com.droidlogic.tvinput.R;

import android.content.Context;
import android.content.pm.ResolveInfo;
import android.media.tv.TvInputHardwareInfo;
import android.media.tv.TvInputInfo;
import android.media.tv.TvStreamConfig;
import android.media.tv.TvInputManager.Hardware;
import android.os.Bundle;
import android.text.TextUtils;
import java.util.HashMap;
import java.util.Map;
import android.view.Surface;
import android.net.Uri;
import android.media.AudioManager;
import android.media.tv.TvInputManager;

public class ArcInputService extends DroidLogicTvInputService {
    private static final String TAG = ArcInputService.class.getSimpleName();
    private static final String SYS_NODE_EARC = "/sys/class/extcon/earcrx/state";
    private ArcInputSession mCurrentSession;
    private int id = 0;
    private Map<Integer, ArcInputSession> sessionMap = new HashMap<>();
    private AudioManager mAudioManager;
    private SystemControlManager mSystemControlManager;

    @Override
    public Session onCreateSession(String inputId) {
        super.onCreateSession(inputId);
        Utils.logd(TAG, "onCreateSession:"+inputId);
        mCurrentSession = new ArcInputSession(getApplicationContext(), inputId, getHardwareDeviceId(inputId));
        registerInputSession(mCurrentSession);
        mCurrentSession.setSessionId(id);
        sessionMap.put(id, mCurrentSession);
        id++;
        if (mSystemControlManager == null) {
            mSystemControlManager =  SystemControlManager.getInstance();
        }
        if (mAudioManager == null)
            mAudioManager = (AudioManager)getApplicationContext().getSystemService (Context.AUDIO_SERVICE);
        return mCurrentSession;
    }

    @Override
    public void setCurrentSessionById(int sessionId) {
        Utils.logd(TAG, "setCurrentSessionById:"+sessionId);
        ArcInputSession session = sessionMap.get(sessionId);
        if (session != null) {
            mCurrentSession = session;
        }
    }

    @Override
    public void doTuneFinish(int result, Uri uri, int sessionId) {
        Utils.logd(TAG, "doTuneFinish,result:"+result+"sessionId:"+sessionId);
        if (result == ACTION_SUCCESS) {
            ArcInputSession session = sessionMap.get(sessionId);
            if (session != null) {
                if (TextUtils.isEmpty(mSystemControlManager.readSysFs(SYS_NODE_EARC))) {
                    mAudioManager.setParameters("spdifin/arcin switch=1");
                }
                //notifyVideoUnavailable for cts test
                session.notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_AUDIO_ONLY);
            }
        }
    }

    public class ArcInputSession extends TvInputBaseSession {
        public ArcInputSession(Context context, String inputId, int deviceId) {
            super(context, inputId, deviceId);
            Utils.logd(TAG, "=====new ArcInputSession=====");
            //initOverlayView(R.layout.layout_overlay);
            if (mOverlayView != null) {
                mOverlayView.setImage(R.drawable.spdifin);
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
        @Override
        public void doRelease() {
            super.doRelease();
            if (mCurrentSession != null && mCurrentSession.getSessionId() == getSessionId()) {
                mCurrentSession = null;
                registerInputSession(null);
            }
        }

        @Override
        public void doAppPrivateCmd(String action, Bundle bundle) {
            super.doAppPrivateCmd(action, bundle);
            if (TextUtils.equals(DroidLogicTvUtils.ACTION_STOP_TV, action)) {
                stopTv();
            }
        }
    }

    public TvInputInfo onHardwareAdded(TvInputHardwareInfo hardwareInfo) {
        if (hardwareInfo.getDeviceId() != DroidLogicTvUtils.DEVICE_ID_ARC
            || hasInfoExisted(hardwareInfo))
            return null;

        Utils.logd(TAG, "=====onHardwareAdded=====" + hardwareInfo.getDeviceId());

        TvInputInfo info = null;
        ResolveInfo rInfo = getResolveInfo(ArcInputService.class.getName());
        if (rInfo != null) {
            try {
                info = TvInputInfo.createTvInputInfo(
                           getApplicationContext(),
                           rInfo,
                           hardwareInfo,
                           getTvInputInfoLabel(hardwareInfo.getDeviceId()),
                           null);
            } catch (XmlPullParserException e) {
                // TODO: handle exception
            } catch (IOException e) {
                // TODO: handle exception
            }
        }
        updateInfoListIfNeededLocked(hardwareInfo, info, false);
        acquireHardware(info);
        return info;
    }

    public String onHardwareRemoved(TvInputHardwareInfo hardwareInfo) {
        if (hardwareInfo.getDeviceId() != DroidLogicTvUtils.DEVICE_ID_ARC
            || !hasInfoExisted(hardwareInfo))
            return null;

        TvInputInfo info = getTvInputInfo(hardwareInfo);
        String id = null;
        if (info != null)
            id = info.getId();
        updateInfoListIfNeededLocked(hardwareInfo, info, true);
        releaseHardware();
        Utils.logd(TAG, "=====onHardwareRemoved=====" + id);
        return id;
    }

}
