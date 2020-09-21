/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC OneTouchPlayService
 */

package com.droidlogic;

import android.app.Service;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.os.Process;
import android.util.Log;


import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiPlaybackClient;
import android.hardware.hdmi.HdmiPlaybackClient.OneTouchPlayCallback;
import android.content.ContentResolver;
import android.provider.Settings.Global;
import android.os.Handler;
import com.droidlogic.app.HdmiCecManager;
import android.os.Message;

public class OneTouchPlayService extends Service {
    private static final String TAG = "OneTouchPlayService";

    private Context mContext;
    private HdmiControlManager mControl = null;
    private HdmiPlaybackClient mPlayback = null;
     private final Handler mHandler = new Handler();
    private final ScreenBroadcastReceiver
            mScreenBroadcastReceiver = new ScreenBroadcastReceiver();

    private class ScreenBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG,"onReceive:"+intent.getAction());
            switch (intent.getAction()) {
                case Intent.ACTION_SCREEN_OFF:
                    break;
                case Intent.ACTION_SCREEN_ON:
                    if (mPlayback != null) {
                        mHandler.postDelayed(mDelayedRun, 1000);
                    }
                    break;
            }
        }
    }

    private final Runnable mDelayedRun = new Runnable() {
        @Override
        public void run() {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    if (isOneTouchPlayOn() && mPlayback != null) {
                        Log.d(TAG,"send oneTouchPlay");
                        /* to wake up tv in case of last oneTouchPlayAction timeout and not finish when wake up playback and try to start a new action */
                        //SendCecMessage(ADDR_TV, buildCecMsg(MESSAGE_TEXT_VIEW_ON, new byte[0]));
                        mPlayback.oneTouchPlay(mOneTouchPlayCallback);
                    }
                }
            }).start();
        }
    };

    private boolean isOneTouchPlayOn() {
        ContentResolver cr = mContext.getContentResolver();
        return Global.getInt(cr, HdmiCecManager.HDMI_CONTROL_ONE_TOUCH_PLAY_ENABLED, 1) == 1;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = this;
        Log.d(TAG,"onCreate");
        mControl = (HdmiControlManager) mContext.getSystemService(Context.HDMI_CONTROL_SERVICE);
        if (mControl != null) {
            mPlayback = mControl.getPlaybackClient();
        }


            IntentFilter filter = new IntentFilter();
            filter.addAction(Intent.ACTION_SCREEN_OFF);
            filter.addAction(Intent.ACTION_SCREEN_ON);
            //filter.addAction(Intent.ACTION_SHUTDOWN);
            mContext.registerReceiver(mScreenBroadcastReceiver, filter);
    }

    private final OneTouchPlayCallback mOneTouchPlayCallback = new OneTouchPlayCallback () {
        @Override
        public void onComplete(int result) {
            Log.d(TAG, "oneTouchPlay:" + result);
        }
    };

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}

