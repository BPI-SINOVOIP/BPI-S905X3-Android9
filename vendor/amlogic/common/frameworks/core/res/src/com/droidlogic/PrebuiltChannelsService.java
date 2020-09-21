/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic;

import android.app.Service;
import android.os.*;
import android.util.Log;
import com.droidlogic.app.tv.PrebuiltChannelsManager;

import android.content.Intent;


public class PrebuiltChannelsService extends Service {
    private static final String TAG = "PrebuiltChannelsService";
    private static final boolean DEBUG = true;

    private static final int DEFAULT_INT_VALUE = 0xffffffff;

    public PrebuiltChannelsService() {
        super();
        Log.d(TAG, "PrebuiltChannelsService constructed!");
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "------onCreate");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "------onStartCommand");
        cmdDispatch(intent);
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "------onDestroy");
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "------onBind");
        return null;
    }

    private void cmdDispatch(Intent intent){
        int cmd = intent.getIntExtra("cmd", DEFAULT_INT_VALUE);
        Log.d(TAG, "dispatch cmd:0x" + Integer.toHexString(cmd) );
        if (DEFAULT_INT_VALUE == cmd) {
            return;
        }
        switch (cmd) {
            // save channel info of db to config file
            case PrebuiltChannelsManager.DISPATCH_CMD_BACKUP_CHANNEL_INFO_TO_FILE: {
                PrebuiltChannelsManager preSetChannelProcess = new PrebuiltChannelsManager(this);
                preSetChannelProcess.dbChannelInfoWriteToConfigFile();
                break;
            }
            // save channel info of config file to db
            case PrebuiltChannelsManager.DISPATCH_CMD_CFG_CHANNEL_INFO_TO_DB: {
                PrebuiltChannelsManager preSetChannelProcess = new PrebuiltChannelsManager(this);
                preSetChannelProcess.configFileReadToDbChannelInfo();
                break;
            }
            default:
                break;
        }
    }
}
