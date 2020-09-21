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

public class SPDIFInputService extends DroidLogicTvInputService {
    private static final String TAG = SPDIFInputService.class.getSimpleName();;
    private SPDIFInputSession mCurrentSession;
    private int id = 0;
    private Map<Integer, SPDIFInputSession> sessionMap = new HashMap<>();
    private AudioManager mAudioManager;

    @Override
    public void onCreate() {
        super.onCreate();
        initInputService(DroidLogicTvUtils.DEVICE_ID_SPDIF, SPDIFInputService.class.getName());
    }

    @Override
    public Session onCreateSession(String inputId) {
        super.onCreateSession(inputId);

        mCurrentSession = new SPDIFInputSession(getApplicationContext(), inputId, getHardwareDeviceId(inputId));
        mCurrentSession.setSessionId(id);
        registerInputSession(mCurrentSession);
        sessionMap.put(id, mCurrentSession);
        id++;

        if (mAudioManager == null)
            mAudioManager = (AudioManager)getApplicationContext().getSystemService (Context.AUDIO_SERVICE);

        return mCurrentSession;
    }

    @Override
    public void setCurrentSessionById(int sessionId) {
        Utils.logd(TAG, "setCurrentSessionById:"+sessionId);
        SPDIFInputSession session = sessionMap.get(sessionId);
        if (session != null) {
            mCurrentSession = session;
        }
    }

    @Override
    public void doTuneFinish(int result, Uri uri, int sessionId) {
        Utils.logd(TAG, "doTuneFinish,result:"+result+"sessionId:"+sessionId);
        if (result == ACTION_SUCCESS) {
            SPDIFInputSession session = sessionMap.get(sessionId);
            if (session != null) {
                mAudioManager.setParameters("spdifin/arcin switch=0");
                //notifyVideoUnavailable for cts test
                session.notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_AUDIO_ONLY);
            }
        }
    }

    public class SPDIFInputSession extends TvInputBaseSession {
        public SPDIFInputSession(Context context, String inputId, int deviceId) {
            super(context, inputId, deviceId);
            Utils.logd(TAG, "=====new SPDIFInputSession=====");
            initOverlayView(R.layout.layout_overlay_no_subtitle);
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
             doTuneInService(channelUri, getSessionId());
             if (mOverlayView != null) {
                mOverlayView.setImageVisibility(true);
             }
             return false;
        }

        @Override
        public void doRelease() {
            super.doRelease();
            if (sessionMap.containsKey(getSessionId())) {
                mCurrentSession = null;
                sessionMap.remove(getSessionId());
                registerInputSession(null);
            }
        }

        @Override
        public void doAppPrivateCmd(String action, Bundle bundle) {
            super.doAppPrivateCmd(action, bundle);
            if (TextUtils.equals(DroidLogicTvUtils.ACTION_STOP_TV, action)) {
                if (mHardware != null) {
                    mHardware.setSurface(null, null);
                }
            }
        }
    }

    public TvInputInfo onHardwareAdded(TvInputHardwareInfo hardwareInfo) {
        if (hardwareInfo.getDeviceId() != DroidLogicTvUtils.DEVICE_ID_SPDIF
            || hasInfoExisted(hardwareInfo))
            return null;

        Utils.logd(TAG, "=====onHardwareAdded=====" + hardwareInfo.getDeviceId());

        TvInputInfo info = null;
        ResolveInfo rInfo = getResolveInfo(SPDIFInputService.class.getName());
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
        if (hardwareInfo.getDeviceId() != DroidLogicTvUtils.DEVICE_ID_SPDIF
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
