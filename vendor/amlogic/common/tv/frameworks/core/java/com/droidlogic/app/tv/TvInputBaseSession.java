/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.content.Context;
import android.media.AudioManager;
import android.media.tv.TvContentRating;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputService;
import android.media.tv.TvStreamConfig;
import android.media.tv.TvInputManager.Hardware;
import android.provider.Settings;
//import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.view.LayoutInflater;

import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.TvControlDataManager;

import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiTvClient;
import android.provider.Settings.Global;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.hardware.hdmi.HdmiTvClient.SelectCallback;

import com.droidlogic.app.tv.DroidLogicHdmiCecManager;
import android.media.tv.TvInputInfo;
//import android.hardware.hdmi.HdmiClient;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.content.Intent;
import java.util.List;
import android.view.KeyEvent;

public abstract class TvInputBaseSession extends TvInputService.Session implements Handler.Callback {
    private static final boolean DEBUG = true;
    private static final String TAG = "TvInputBaseSession";

    private static final int    MSG_REGISTER_BROADCAST = 8;
    private static final int    MSG_DO_PRI_CMD              = 9;
    protected static final int  MSG_SUBTITLE_SHOW           = 10;
    protected static final int  MSG_SUBTITLE_HIDE           = 11;
    protected static final int  MSG_DO_RELEASE              = 12;
    protected static final int  MSG_AUDIO_MUTE              = 13;
    protected static final int  MSG_IMAGETEXT_SET           = 14;

    protected static final int TVINPUT_BASE_DELAY_SEND_MSG  = 10; // Filter message within 10ms, only the last message is processed
    private Context mContext;
    public int mId;
    private String mInputId;
    private int mDeviceId;
    private AudioManager mAudioManager;
    private TvInputManager mTvInputManager;
    private boolean mHasRetuned = false;
    protected Handler mSessionHandler;
    private TvControlDataManager mTvControlDataManager = null;
    protected DroidLogicOverlayView mOverlayView = null;

    protected boolean isBlockNoRatingEnable = false;
    protected boolean isUnlockCurrent_NR = false;
    DroidLogicHdmiCecManager mDroidLogicHdmiCecManager = null;
    private int mKeyCodeMediaPlayPauseCount = 0;
    private boolean isSurfaceAlive = true;

    public TvInputBaseSession(Context context, String inputId, int deviceId) {
        super(context);
        mContext = context;
        mInputId = inputId;
        mDeviceId = deviceId;

        mAudioManager = (AudioManager)context.getSystemService (Context.AUDIO_SERVICE);
        mTvControlDataManager = TvControlDataManager.getInstance(mContext);
        mSessionHandler = new Handler(context.getMainLooper(), this);
        mTvInputManager = (TvInputManager)mContext.getSystemService(Context.TV_INPUT_SERVICE);
        mDroidLogicHdmiCecManager = DroidLogicHdmiCecManager.getInstance(mContext);
        sendSessionMessage(MSG_REGISTER_BROADCAST);
    }

    public void setSessionId(int id) {
        mId = id;
    }

    public int getSessionId() {
        return mId;
    }

    public String getInputId() {
        return mInputId;
    }

    public int getDeviceId() {
        return mDeviceId;
    }

    public void sendSessionMessage(int cmd) {
        Message msg = mSessionHandler.obtainMessage(cmd);
        mSessionHandler.removeMessages(msg.what);
        msg.sendToTarget();
    }
    public void doRelease() {
        Log.d(TAG, "doRelease,session:"+this);
        mContext.unregisterReceiver(mBroadcastReceiver);
    }

    public void doAppPrivateCmd(String action, Bundle bundle) {}
    public void doUnblockContent(TvContentRating rating) {}

    @Override
    public void onSurfaceChanged(int format, int width, int height) {
    }

    @Override
    public void onSetStreamVolume(float volume) {
        //this function used for parental control, so HDMI source don't need it.
        if ((mDeviceId >= DroidLogicTvUtils.DEVICE_ID_HDMI1 && mDeviceId <= DroidLogicTvUtils.DEVICE_ID_HDMI4)) {
            return;
        }
        if (DEBUG)
            Log.d(TAG, "onSetStreamVolume volume = " + volume);
        Message msg = mSessionHandler.obtainMessage(MSG_AUDIO_MUTE);
        if (0.0 == volume) {
            msg.arg1 = 0;
        } else {
            msg.arg1 = 1;
        }
        mSessionHandler.removeMessages(msg.what);
        mSessionHandler.sendMessageDelayed(msg, TVINPUT_BASE_DELAY_SEND_MSG);
    }

    @Override
    public void onAppPrivateCommand(String action, Bundle data) {
        if (DEBUG)
            Log.d(TAG, "onAppPrivateCommand, action = " + action);

        if (mSessionHandler == null)
            return;
        Message msg = mSessionHandler.obtainMessage(MSG_DO_PRI_CMD);
        mSessionHandler.removeMessages(msg.what);
        msg.setData(data);
        msg.obj = action;
        msg.sendToTarget();
    }

    @Override
    public void onSetCaptionEnabled(boolean enabled) {
        // TODO Auto-generated method stub
    }

    @Override
    public void onUnblockContent(TvContentRating unblockedRating) {
        if (DEBUG)
            Log.d(TAG, "onUnblockContent");

        doUnblockContent(unblockedRating);
    }

    public void initOverlayView(int resId) {
        LayoutInflater inflater = (LayoutInflater)
                mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mOverlayView = (DroidLogicOverlayView)inflater.inflate(resId, null);
        setOverlayViewEnabled(true);
    }

    private  BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(Intent.ACTION_SCREEN_OFF)) {
                if (DEBUG) Log.d(TAG, "Received ACTION_SCREEN_OFF");
                setOverlayViewEnabled(false);
            } else if (intent.getAction().equals(Intent.ACTION_SCREEN_ON)) {
                if (DEBUG) Log.d(TAG, "Received ACTION_SCREEN_ON");
                setOverlayViewEnabled(true);
            }
        }
    };

    @Override
    public View onCreateOverlayView() {
        return mOverlayView;
    }

    @Override
    public void onOverlayViewSizeChanged(int width, int height) {
        Log.d(TAG, "onOverlayViewSizeChanged: "+width+","+height);
    }

    @Override
    public void notifyVideoAvailable() {
        Log.d(TAG, "notifyVideoAvailable ");
        super.notifyVideoAvailable();
        if (mOverlayView != null) {
            mOverlayView.setImageVisibility(false);
            mOverlayView.setTextVisibility(false);
        }
    }

    @Override
    public void notifyVideoUnavailable(int reason) {
        Log.d(TAG, "notifyVideoUnavailable: "+reason);
        super.notifyVideoUnavailable(reason);
        Message msg = mSessionHandler.obtainMessage(MSG_IMAGETEXT_SET);
        mSessionHandler.removeMessages(msg.what);
        msg.sendToTarget();
    }

    @Override
     public boolean onSetSurface(Surface surface) {
        Log.d(TAG, "onSetSurface " + surface + this);
        Exception e = new Exception();
        Log.e(TAG, "setSurfaceInService " + e);

        if (surface == null) {
            mSessionHandler.removeCallbacksAndMessages(null);
            isSurfaceAlive = false;

            setOverlayViewEnabled(false);
            if (mOverlayView != null) {
                mOverlayView.releaseResource();
                mOverlayView = null;
            }
        } else {
            isSurfaceAlive = true;
        }

        return false;
     }

    @Override
    public void onRelease() {
        mSessionHandler.removeCallbacksAndMessages(null);
        if (mSessionHandler == null)
            return;
        Message msg = mSessionHandler.obtainMessage(MSG_DO_RELEASE);
        mSessionHandler.removeMessages(msg.what);
        msg.sendToTarget();
    }

    public void hideUI() {
        if (mOverlayView != null) {
            mOverlayView.setImageVisibility(false);
            mOverlayView.setTextVisibility(false);
            mOverlayView.setSubtitleVisibility(false);
        }
    }

    private void setAudiodMute(boolean mute) {
        Log.d(TAG, "setAudiodMute="+mute);
        if (mute) {
            mAudioManager.setParameters("parental_control_av_mute=true");
        } else {
            mAudioManager.setParameters("parental_control_av_mute=false");
        }
    }

    public void openTvAudio (int type){
        switch (type) {
            case DroidLogicTvUtils.SOURCE_TYPE_ATV:
                mAudioManager.setParameters("tuner_in=atv");
                break;
            case DroidLogicTvUtils.SOURCE_TYPE_DTV:
                mAudioManager.setParameters("tuner_in=dtv");
                break;
        }
    }


    @Override
    public boolean handleMessage(Message msg) {
        if (DEBUG)
            Log.d(TAG, "handleMessage, msg.what=" + msg.what + " isSurfaceAlive=" + isSurfaceAlive);

        if (!isSurfaceAlive) {
            if (msg.what != MSG_DO_RELEASE && msg.what != MSG_AUDIO_MUTE) {
                return false;
            }
        }

        switch (msg.what) {
            case MSG_REGISTER_BROADCAST:
                    IntentFilter intentFilter = new IntentFilter();
                    intentFilter.addAction(Intent.ACTION_SCREEN_OFF);
                    intentFilter.addAction(Intent.ACTION_SCREEN_ON);
                    mContext.registerReceiver(mBroadcastReceiver, intentFilter);
                break;
            case MSG_DO_PRI_CMD:
                doAppPrivateCmd((String)msg.obj, msg.getData());
                break;
            case MSG_SUBTITLE_SHOW:
                if (mOverlayView != null) {
                    mOverlayView.setSubtitleVisibility(true);
                }
                break;
            case MSG_SUBTITLE_HIDE:
                if (mOverlayView != null) {
                    mOverlayView.setSubtitleVisibility(false);
                }
                break;
            case MSG_DO_RELEASE:
                doRelease();
                break;
            case MSG_AUDIO_MUTE:
                long startTime = SystemClock.uptimeMillis();
                setAudiodMute(msg.arg1 == 0);
                if (DEBUG) Log.d(TAG, "setAudiodMute used " + (SystemClock.uptimeMillis() - startTime) + " ms");
                break;
            case MSG_IMAGETEXT_SET:
                if (mOverlayView != null) {
                    mOverlayView.setImageVisibility(true);
                    mOverlayView.setTextVisibility(true);
                }
                break;
        }
        return false;
    }

    @Override
    public void onSetMain(boolean isMain) {
        Log.d(TAG, "onSetMain, isMain: " + isMain +" mDeviceId: "+ mDeviceId +" mInputId: " + mInputId);
        TvInputInfo info = mTvInputManager.getTvInputInfo(mInputId);
        if (isMain && info != null)  {
            if (mDeviceId < DroidLogicTvUtils.DEVICE_ID_HDMI1 || mDeviceId > DroidLogicTvUtils.DEVICE_ID_HDMI4) {
                Log.d(TAG, "onSetMain, mDeviceId: " + mDeviceId + " not correct!");
            } else {
                mDroidLogicHdmiCecManager.connectHdmiCec(mDeviceId);
            }
        } else {
            if (info == null) {
                Log.d(TAG, "onSetMain, info is null");
            } else if (info.getHdmiDeviceInfo() == null) {
                Log.d(TAG, "onSetMain, info is: " + info + " but info.getHdmiDeviceInfo() is null");
            }
        }
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyUp: " + keyCode);
        boolean ret = true;

        if (mDroidLogicHdmiCecManager.hasHdmiCecDevice(mDeviceId)) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
                    mDroidLogicHdmiCecManager.sendKeyEvent((mKeyCodeMediaPlayPauseCount % 2 == 1 ? KeyEvent.KEYCODE_MEDIA_PAUSE : KeyEvent.KEYCODE_MEDIA_PLAY), false);
                    mKeyCodeMediaPlayPauseCount++;
                    break;
                case KeyEvent.KEYCODE_BACK:
                    Log.d(TAG, "KEYCODE_BACK shoud not send to live tv if cec device exits");
                    mDroidLogicHdmiCecManager.sendKeyEvent(keyCode, false);
                    break;
                default:
                    mDroidLogicHdmiCecManager.sendKeyEvent(keyCode, false);
                    break;
            }
        } else {
            Log.d(TAG, "cec device didn't exist");
            ret = false;
        }
        return ret;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyDown: " + keyCode);
        boolean ret = true;

        if (mDroidLogicHdmiCecManager.hasHdmiCecDevice(mDeviceId)) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
                    mDroidLogicHdmiCecManager.sendKeyEvent((mKeyCodeMediaPlayPauseCount % 2 == 1 ? KeyEvent.KEYCODE_MEDIA_PAUSE : KeyEvent.KEYCODE_MEDIA_PLAY), true);
                    break;
                case KeyEvent.KEYCODE_BACK:
                    Log.d(TAG, "KEYCODE_BACK shoud not send to live tv if cec device exits");
                    mDroidLogicHdmiCecManager.sendKeyEvent(keyCode, true);
                    break;
                default:
                    mDroidLogicHdmiCecManager.sendKeyEvent(keyCode, true);
                    break;
            }
        } else {
            Log.d(TAG, "cec device didn't exist");
            ret = false;
        }
        return ret;
    }
}
