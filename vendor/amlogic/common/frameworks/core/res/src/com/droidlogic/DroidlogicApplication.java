/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC UsbCameraReceiver
 */

package com.droidlogic;

import android.app.Application;
import android.content.Intent;
import android.content.ContentProviderClient;
import android.media.tv.TvContract;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.text.TextUtils;
import android.util.Log;

import com.droidlogic.app.AudioSettingManager;

public class DroidlogicApplication extends Application {
    private static final String TAG = "DroidlogicApplication";
    private AudioSettingManager mAudioSettingManager;

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "onCreate");
        mAudioSettingManager = new AudioSettingManager(this);
        mHandler.sendEmptyMessage(MSG_CHECK_BOOTVIDEO_FINISHED);
    }

    private boolean isBootvideoStopped() {
        ContentProviderClient tvProvider = null;

        if (mAudioSettingManager.isTunerAudio()) {
            tvProvider = getContentResolver().acquireContentProviderClient(TvContract.AUTHORITY);
        }

        return (mAudioSettingManager.isTunerAudio() && tvProvider != null || !mAudioSettingManager.isTunerAudio()) &&
                (((SystemProperties.getInt("persist.vendor.media.bootvideo", 50)  > 100)
                        && TextUtils.equals(SystemProperties.get("service.bootvideo.exit", "1"), "0"))
                || ((SystemProperties.getInt("persist.vendor.media.bootvideo", 50)  <= 100)));
    }

    private static final int MSG_CHECK_BOOTVIDEO_FINISHED = 0;
    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_CHECK_BOOTVIDEO_FINISHED:
                    if (isBootvideoStopped()) {
                        Log.d(TAG, " bootvideo stopped, start initializing audio");
                        initAudio();
                    } else {
                        mHandler.sendEmptyMessageDelayed(MSG_CHECK_BOOTVIDEO_FINISHED, 10);
                    }
                    break;
                default:
                    break;
            }
        }
    };

    private void initAudio () {
        mAudioSettingManager.registerSurroundObserver();
        mAudioSettingManager.initSystemAudioSetting();
    }
}

