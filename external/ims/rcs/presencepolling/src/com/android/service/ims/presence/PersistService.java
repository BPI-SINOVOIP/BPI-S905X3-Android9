/*
 * Copyright (c) 2015, Motorola Mobility LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     - Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     - Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     - Neither the name of Motorola Mobility nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY LLC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

package com.android.service.ims.presence;

import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;
import android.app.Service;
import android.os.SystemProperties;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.HandlerThread;
import android.os.Process;
import java.util.ArrayList;
import android.content.ContentValues;
import android.text.TextUtils;

import com.android.ims.internal.Logger;
import com.android.ims.RcsManager.ResultCode;
import com.android.ims.RcsPresence;
import com.android.ims.RcsPresenceInfo;

/**
 * This service essentially plays the role of a "worker thread", allowing us to store
 * presence information.
 */
public class PersistService extends Service {
    private Logger logger = Logger.getLogger(this.getClass().getName());

    private static final int MESSAGE_PRESENCE_CHANGED = 1;
    private static final int MESSAGE_PUBLISH_STATE_CHANGED = 2;

    private int mVltProvisionErrorCount = 0;
    private Looper mServiceLooper = null;
    private ServiceHandler mServiceHandler = null;
    private EABContactManager mEABContactManager = null;

    @Override
    public void onCreate() {
        mEABContactManager = new EABContactManager(getContentResolver(), getPackageName());

        // separate thread because the service normally runs in the process's
        // main thread, which we don't want to block.
        HandlerThread thread = new HandlerThread("PersistServiceThread",
                Process.THREAD_PRIORITY_BACKGROUND);
        thread.start();

        mServiceLooper = thread.getLooper();
        mServiceHandler = new ServiceHandler(mServiceLooper);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if(intent == null) {
            return Service.START_NOT_STICKY;
        }

        if(RcsPresence.ACTION_PRESENCE_CHANGED.equals(intent.getAction())) {
            Message msg = mServiceHandler.obtainMessage(MESSAGE_PRESENCE_CHANGED);
            msg.arg1 = startId;
            msg.obj = intent;
            mServiceHandler.sendMessage(msg);
            return Service.START_NOT_STICKY;
        } else if(RcsPresence.ACTION_PUBLISH_STATE_CHANGED.equals(intent.getAction())) {
            Message msg = mServiceHandler.obtainMessage(MESSAGE_PUBLISH_STATE_CHANGED);
            msg.arg1 = startId;
            msg.obj = intent;
            mServiceHandler.sendMessage(msg);
            return Service.START_NOT_STICKY;
        } else {
            logger.debug("unknown intent=" + intent);
        }

        return Service.START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        mServiceLooper.quit();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private final class ServiceHandler extends Handler {
        public ServiceHandler(Looper looper) {
            super(looper);
        }

        /**
         * Handler to save the presence information.
         */
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            logger.print( "Thread=" + Thread.currentThread().getName() + " received "
                    + msg);
            if(msg == null){
                logger.error("msg=null");
                return;
            }

            int serviceId = msg.arg1;
            Intent intent = (Intent)msg.obj;
            logger.print("handleMessage serviceId: " + serviceId + " intent: " + intent);
            switch (msg.what) {
                case MESSAGE_PRESENCE_CHANGED:
                    if (intent != null) {
                        if (RcsPresence.ACTION_PRESENCE_CHANGED.equals(intent.getAction())) {
                            handlePresence(intent);
                        } else {
                            logger.debug("I don't care about intent: " + intent);
                        }
                        intent.setComponent(null);
                        sendBroadcast(intent);
                    }
                break;

                case MESSAGE_PUBLISH_STATE_CHANGED:
                    if (intent != null) {
                        if (RcsPresence.ACTION_PUBLISH_STATE_CHANGED.equals(intent.getAction())) {
                            int publishState = intent.getIntExtra(RcsPresence.EXTRA_PUBLISH_STATE,
                                    RcsPresence.PublishState.PUBLISH_STATE_200_OK);
                            handlePublishState(publishState);
                        } else {
                            logger.debug("I don't care about intent: " + intent);
                        }
                    }
                break;

                default:
                    logger.debug("unknown message:" + msg);
            }
        }
    }

    private void handlePublishState(int publishState) {
        if(publishState == RcsPresence.PublishState.PUBLISH_STATE_VOLTE_PROVISION_ERROR) {
            mVltProvisionErrorCount += 1;
            if(mVltProvisionErrorCount >= 3){
                if(mEABContactManager != null) {
                    mEABContactManager.updateAllCapabilityToUnknown();
                    logger.print("updateAllCapabilityToUnknown for publish 403 error");
                    Intent intent = new Intent(RcsPresence.ACTION_PRESENCE_CHANGED);
                    sendBroadcast(intent);
                } else {
                    logger.error("mEABContactManager is null");
                }
            }
        } else if(publishState == RcsPresence.PublishState.PUBLISH_STATE_RCS_PROVISION_ERROR){
            mVltProvisionErrorCount = 0;
            if(mEABContactManager != null) {
                mEABContactManager.updateAllVtCapabilityToUnknown();
                logger.print("updateAllVtCapabilityToUnknown for publish 404 error");
                Intent intent = new Intent(RcsPresence.ACTION_PRESENCE_CHANGED);
                sendBroadcast(intent);
            } else {
                logger.error("mEABContactManager is null");
            }
        } else {
            mVltProvisionErrorCount = 0;
        }
    }

    private void handlePresence(Intent intent) {
        if(intent == null) {
            return;
        }

        // save the presence information.
        ArrayList<RcsPresenceInfo> rcsPresenceInfoList = intent.getParcelableArrayListExtra(
                RcsPresence.EXTRA_PRESENCE_INFO_LIST);
        boolean updateLastTimestamp = intent.getBooleanExtra("updateLastTimestamp", true);
        logger.print("updateLastTimestamp=" + updateLastTimestamp +
                " RcsPresenceInfoList=" + rcsPresenceInfoList);
        for(int i=0; i< rcsPresenceInfoList.size(); i++){
            RcsPresenceInfo rcsPresenceInfoTmp = rcsPresenceInfoList.get(i);
            if((rcsPresenceInfoTmp != null) && !TextUtils.isEmpty(
                    rcsPresenceInfoTmp.getContactNumber())){
                mEABContactManager.update(rcsPresenceInfoTmp, updateLastTimestamp);
            }
        }
    }
}


