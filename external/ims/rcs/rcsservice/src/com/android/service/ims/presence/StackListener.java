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

import java.util.ArrayList;
import java.util.List;

import android.content.ServiceConnection;
import android.annotation.SuppressLint;
import android.content.Intent;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;
import android.os.Parcel;

import com.android.ims.internal.uce.presence.IPresenceListener;
import com.android.ims.internal.uce.presence.PresCmdId;
import com.android.ims.internal.uce.presence.PresCmdStatus;
import com.android.ims.internal.uce.presence.PresPublishTriggerType;
import com.android.ims.internal.uce.presence.PresResInfo;
import com.android.ims.internal.uce.presence.PresRlmiInfo;
import com.android.ims.internal.uce.presence.PresSipResponse;
import com.android.ims.internal.uce.presence.PresTupleInfo;
import com.android.ims.internal.uce.common.StatusCode;
import com.android.ims.internal.uce.common.StatusCode;

import com.android.ims.RcsManager;
import com.android.ims.RcsManager.ResultCode;
import com.android.ims.RcsPresence;
import com.android.ims.RcsPresenceInfo;
import com.android.ims.IRcsPresenceListener;

import com.android.ims.internal.Logger;
import com.android.service.ims.TaskManager;
import com.android.service.ims.Task;
import com.android.service.ims.RcsStackAdaptor;
import com.android.service.ims.LauncherUtils;

public class StackListener extends Handler{
    /*
     * The logger
     */
    private Logger logger = Logger.getLogger(this.getClass().getName());

    Context mContext;
    private PresencePublication mPresencePublication = null;
    private PresenceSubscriber mPresenceSubscriber = null;

    // RCS stack notify the AP to publish the presence.
    private static final short PRESENCE_IMS_UNSOL_PUBLISH_TRIGGER = 1;
    // PUBLISH CMD status changed
    private static final short PRESENCE_IMS_UNSOL_PUBLISH_CMDSTATUS = 2;
    // Received the SIP response for publish
    private static final short PRESENCE_IMS_UNSOL_PUBLISH_SIPRESPONSE = 3;
    // Received the presence for single contact
    private static final short PRESENCE_IMS_UNSOL_NOTIFY_UPDATE = 4;
    // Received the presence for contacts.
    private static final short PRESENCE_IMS_UNSOL_NOTIFY_LIST_UPDATE = 5;
    // Received the CMD status for capability/availability request
    private static final short PRESENCE_IMS_UNSOL_NOTIFY_UPDATE_CMDSTATUS = 6;
    // Received the SIP response for capability/availability request
    private static final short PRESENCE_IMS_UNSOL_NOTIFY_UPDATE_SIPRESPONSE = 7;

    private final Object mSyncObj = new Object();

    public StackListener(Context context, Looper looper) {
        super(looper);
        mContext = context;
    }

    public void setPresencePublication(PresencePublication presencePublication){
        mPresencePublication = presencePublication;
    }

    public void setPresenceSubscriber(PresenceSubscriber presenceSubscriber){
        mPresenceSubscriber = presenceSubscriber;
    }

    @Override
    public void handleMessage(Message msg) {
        super.handleMessage(msg);

        logger.debug( "Thread=" + Thread.currentThread().getName() + " received "
                + msg);
        if(msg == null){
            logger.error("msg=null");
            return;
        }

        switch (msg.what) {
            // RCS stack notify the AP to publish the presence.
            case PRESENCE_IMS_UNSOL_PUBLISH_TRIGGER:
            {
                PresPublishTriggerType val = (PresPublishTriggerType) msg.obj;
                if(mPresencePublication == null || val == null){
                    logger.error("mPresencePublication=" + mPresencePublication + " val=" + val);
                    return;
                }

                mPresencePublication.invokePublish(val);
                break;
            }

            // RCS stack tell AP that the CMD status changed.
            case PRESENCE_IMS_UNSOL_PUBLISH_CMDSTATUS:
            {
                PresCmdStatus pCmdStatus = (PresCmdStatus) msg.obj;
                if(mPresencePublication == null || pCmdStatus == null){
                    logger.error("mPresencePublication=" + mPresencePublication +
                            " pCmdStatus=" + pCmdStatus);
                    return;
                }

                mPresencePublication.handleCmdStatus(pCmdStatus);
            }
                break;

            // RCS stack tells AP that the CMD status changed.
            case PRESENCE_IMS_UNSOL_NOTIFY_UPDATE_CMDSTATUS:
            {
                PresCmdStatus pCmdStatus = (PresCmdStatus) msg.obj;
                if(mPresenceSubscriber == null || pCmdStatus == null){
                    logger.error("mPresenceSubcriber=" + mPresenceSubscriber +
                            " pCmdStatus=" + pCmdStatus);
                    return;
                }

                mPresenceSubscriber.handleCmdStatus(pCmdStatus);
                break;
            }

            // RCS stack tells AP that the SIP response has been received.
            case PRESENCE_IMS_UNSOL_PUBLISH_SIPRESPONSE:
            {
                PresSipResponse pSipResponse =  (PresSipResponse) msg.obj;
                if(mPresencePublication == null || pSipResponse == null){
                    logger.error("mPresencePublication=" + mPresencePublication +
                            "pSipResponse=" +pSipResponse);
                    return;
                }

                mPresencePublication.handleSipResponse(pSipResponse);
                break;
            }

            // RCS stack tells AP that the SIP response has been received.
            case PRESENCE_IMS_UNSOL_NOTIFY_UPDATE_SIPRESPONSE:
            {
                PresSipResponse pSipResponse =  (PresSipResponse) msg.obj;
                if(mPresenceSubscriber == null || pSipResponse == null){
                    logger.error("mPresenceSubscriber=" + mPresenceSubscriber +
                            " pSipResponse=" + pSipResponse);
                    return;
                }

                mPresenceSubscriber.handleSipResponse(pSipResponse);
                break;
            }

            // RCS stack tells AP that the presence data has been received.
            case PRESENCE_IMS_UNSOL_NOTIFY_UPDATE:
            {
                NotifyData notifyData = (NotifyData) msg.obj;
                if(mPresenceSubscriber == null || notifyData == null){
                    logger.error("mPresenceSubscriber=" + mPresenceSubscriber +
                            " notifyData=" + notifyData);
                    return;
                }

                mPresenceSubscriber.updatePresence(notifyData.getUri(),
                        notifyData.getTupleInfo());
                break;
            }

            case PRESENCE_IMS_UNSOL_NOTIFY_LIST_UPDATE:
            {
                NotifyListData notifyListData = (NotifyListData) msg.obj;
                logger.debug("Received PRESENCE_IMS_UNSOL_NOTIFY_LIST_UPDATE");
                if(mPresenceSubscriber==null || notifyListData == null){
                    logger.error("mPresenceSubscriber=" + mPresenceSubscriber +
                            " notifyListData=" + notifyListData);
                    return;
                }

                mPresenceSubscriber.updatePresences(notifyListData.getRlmiInfo(),
                        notifyListData.getResInfo());
                break;
            }

            default:
                logger.debug("Unknown mesg " + msg.what + " recieved.");
        }
    }

    public class NotifyData{
        private String mUri;
        private PresTupleInfo[] mTupleInfo;

        NotifyData(){
            mUri = null;
            mTupleInfo = null;
        }

        NotifyData(String uri, PresTupleInfo[] pTupleInfo){
            mUri = uri;
            mTupleInfo = pTupleInfo;
        }

        public String getUri(){
            return mUri;
        }

        public PresTupleInfo[] getTupleInfo(){
            return mTupleInfo;
        }
    }

    public class NotifyListData{
        private PresRlmiInfo mRlmiInfo;
        private PresResInfo[] mResInfo;

        NotifyListData(){
            mRlmiInfo = null;
            mResInfo = null;
        }

        NotifyListData(PresRlmiInfo pRlmiInfo, PresResInfo[] pResInfo){
            mRlmiInfo = pRlmiInfo;
            mResInfo = pResInfo;
        }

        public PresRlmiInfo getRlmiInfo(){
            return mRlmiInfo;
        }

        public PresResInfo[] getResInfo(){
            return mResInfo;
        }
    }

    public IPresenceListener mPresenceListener = new IPresenceListener.Stub() {
        public boolean onTransact(int code, Parcel data, Parcel reply, int flags)
               throws RemoteException{
            try{
                return super.onTransact(code, data, reply, flags);
            } catch (RemoteException e) {
                Log.w("ListenerHandler", "Unexpected remote exception", e);
                e.printStackTrace();
                throw e;
           }
        }

        public void getVersionCb(String pVersion) {
            logger.debug("pVersion=" + pVersion);
        }

        public void sipResponseReceived(PresSipResponse pSipResponse) throws RemoteException {
            synchronized(mSyncObj){
                if(pSipResponse == null){
                    logger.error("ISipResponseReceived pSipResponse=null");
                    return;
                }

                logger.debug("pSipResponse.getCmdId() "+
                        pSipResponse.getCmdId().getCmdId());
                logger.debug("getReasonPhrase() "+pSipResponse.getReasonPhrase());
                logger.debug("getsRequestID() "+pSipResponse.getRequestId());
                logger.debug("getsSipResponseCode() "+pSipResponse.getSipResponseCode());

                switch (pSipResponse.getCmdId().getCmdId()) {
                    case PresCmdId.UCE_PRES_CMD_PUBLISHMYCAP:
                    {
                        Message updateMesgSipPub = StackListener.this.obtainMessage(
                                PRESENCE_IMS_UNSOL_PUBLISH_SIPRESPONSE,
                                pSipResponse);
                        StackListener.this.sendMessage(updateMesgSipPub);
                        break;
                    }

                    case PresCmdId.UCE_PRES_CMD_GETCONTACTCAP:
                    case PresCmdId.UCE_PRES_CMD_GETCONTACTLISTCAP:
                    {
                        Message updateMesgSipPub = StackListener.this.obtainMessage(
                                PRESENCE_IMS_UNSOL_NOTIFY_UPDATE_SIPRESPONSE,
                                pSipResponse);
                        StackListener.this.sendMessage(updateMesgSipPub);

                        break;
                    }

                    case PresCmdId.UCE_PRES_CMD_SETNEWFEATURETAG:
                    {
                        logger.debug("UCE_PRES_CMD_SETNEWFEATURETAG, doesn't care it");
                        break;
                    }

                    default:
                        logger.debug("CMD ID for unknown value=" +
                                pSipResponse.getCmdId().getCmdId());
                }
            }
        }

        public void serviceUnAvailable(StatusCode statusCode) throws RemoteException {
            synchronized(mSyncObj){
                if(statusCode == null){
                    logger.error("statusCode=null");
                }else{
                    logger.debug("IServiceUnAvailable statusCode " +
                        statusCode.getStatusCode());
                }
                logger.debug("QPresListener_ServiceUnAvailable");

                RcsStackAdaptor rcsStackAdaptor = RcsStackAdaptor.getInstance(null);
                if (rcsStackAdaptor != null) {
                    rcsStackAdaptor.setImsEnableState(false);
                }

                Intent intent = new Intent(RcsManager.ACTION_RCS_SERVICE_UNAVAILABLE);
                mContext.sendBroadcast(intent,
                        "com.android.ims.rcs.permission.STATUS_CHANGED");
            }
        }

        public void serviceAvailable(StatusCode statusCode) throws RemoteException {
            synchronized(mSyncObj){
                if(statusCode == null){
                    logger.error("statusCode=null");
                }else{
                    logger.debug("IServiceAvailable statusCode " +
                        statusCode.getStatusCode());
                }

                logger.debug("QPresListener_ServiceAvailable");

                RcsStackAdaptor rcsStackAdaptor = RcsStackAdaptor.getInstance(null);
                if (rcsStackAdaptor != null) {
                    rcsStackAdaptor.setImsEnableState(true);
                }

                // Handle the cached trigger which got from stack
                if(mPresencePublication != null && mPresencePublication.getHasCachedTrigger()){
                    logger.debug("publish for cached trigger");

                    mPresencePublication.invokePublish(
                            PresencePublication.PublishType.PRES_PUBLISH_TRIGGER_CACHED_TRIGGER);
                }

                Intent intent = new Intent(RcsManager.ACTION_RCS_SERVICE_AVAILABLE);
                mContext.sendBroadcast(intent,
                        "com.android.ims.rcs.permission.STATUS_CHANGED");
            }
        }

        public void publishTriggering(PresPublishTriggerType publishTrigger)
                throws RemoteException {
            if(publishTrigger == null){
                logger.error("publishTrigger=null");
            }else{
                logger.debug("getPublishTrigeerType() "+
                         publishTrigger.getPublishTrigeerType());
            }
            logger.debug("ListenerHandler : PublishTriggering");

            Message publishTrigerMsg = StackListener.this.obtainMessage(
                    PRESENCE_IMS_UNSOL_PUBLISH_TRIGGER, publishTrigger);
            StackListener.this.sendMessage(publishTrigerMsg);
        }

        public void listCapInfoReceived(PresRlmiInfo pRlmiInfo, PresResInfo[] pResInfo)
                throws RemoteException {
            if(pRlmiInfo == null || pResInfo == null){
                logger.error("pRlmiInfo=" + pRlmiInfo + " pResInfo=" + pResInfo);
            }else{
                logger.debug("pRlmiInfo.getListName "+pRlmiInfo.getListName());
                logger.debug("pRlmiInfo.isFullState "+pRlmiInfo.isFullState());
                logger.debug("pRlmiInfo.getUri "+pRlmiInfo.getUri());
                logger.debug("pRlmiInfo.getVersion "+pRlmiInfo.getVersion());
                logger.debug("pRlmiInfo.getSubscriptionTerminatedReason " +
                        pRlmiInfo.getSubscriptionTerminatedReason());
                logger.debug("pRlmiInfo.getPresSubscriptionState " +
                        pRlmiInfo.getPresSubscriptionState());
                logger.debug("pRlmiInfo.getRequestID=" + pRlmiInfo.getRequestId());
                for(int i=0; i < pResInfo.length; i++ ){
                    if(pResInfo[i] == null){
                        logger.debug("ignoring, pResInfo[" + i + "]=null");
                        continue;
                    }

                    logger.debug(".getDisplayName() "+pResInfo[i].getDisplayName());
                    logger.debug("getResUri() "+pResInfo[i].getResUri());
                    if(pResInfo[i].getInstanceInfo() != null){
                        logger.debug("getInstanceInfo().getPresentityUri() "+
                                 pResInfo[i].getInstanceInfo().getPresentityUri());
                        logger.debug("getInstanceInfo().getResId() "+
                                 pResInfo[i].getInstanceInfo().getResId());
                        logger.debug("getInstanceInfo().getsReason() "+
                                 pResInfo[i].getInstanceInfo().getReason());
                        logger.debug("getInstanceInfo().getResInstanceState() "+
                                 pResInfo[i].getInstanceInfo().getResInstanceState());
                        if(pResInfo[i].getInstanceInfo().getTupleInfo() == null){
                           logger.debug("pResInfo[" + i +"].getInstanceInfo().getTupleInfo()=null");
                            continue;
                        }

                        logger.debug("getTupleInfo().length "+
                                 pResInfo[i].getInstanceInfo().getTupleInfo().length);
                        if(pResInfo[i].getInstanceInfo().getTupleInfo() != null){
                            for(int j = 0; j < pResInfo[i].getInstanceInfo().getTupleInfo().length;
                                j++)
                            {
                                if(pResInfo[i].getInstanceInfo().getTupleInfo() == null){
                                    logger.debug("ignoring, pResInfo[" + i +
                                            "].getInstanceInfo().getTupleInfo()[" +j + "]");
                                    continue;
                                }

                                logger.debug("getFeatureTag "+
                                        pResInfo[i].getInstanceInfo().getTupleInfo()[j].
                                        getFeatureTag());
                                logger.debug("getsContactUri "+
                                        pResInfo[i].getInstanceInfo().getTupleInfo()[j].
                                        getContactUri());
                                logger.debug("getsTimestamp "+
                                        pResInfo[i].getInstanceInfo().getTupleInfo()[j].
                                        getTimestamp());
                            }
                        }
                    }
                }
            }

            Message notifyListReceivedMsg = StackListener.this.obtainMessage(
                    PRESENCE_IMS_UNSOL_NOTIFY_LIST_UPDATE,
                    new NotifyListData(pRlmiInfo, pResInfo));
            logger.debug("Send PRESENCE_IMS_UNSOL_NOTIFY_LIST_UPDATE");

            StackListener.this.sendMessage(notifyListReceivedMsg);
        }

        public void capInfoReceived(String presentityURI, PresTupleInfo[] pTupleInfo)
                throws RemoteException {
            logger.debug("ListenerHandler : CapInfoReceived");
            if(presentityURI == null || presentityURI == null){
                logger.error("presentityURI=null or presentityURI=null");
                return;
            }

            logger.debug("ListenerHandler : CapInfoReceived : presentityURI "+ presentityURI);

            Message notifyReceivedMsg = StackListener.this.obtainMessage(
                    PRESENCE_IMS_UNSOL_NOTIFY_UPDATE,
                    new NotifyData(presentityURI, pTupleInfo));
            StackListener.this.sendMessage(notifyReceivedMsg);
        }

        public void cmdStatus(PresCmdStatus pCmdStatus) throws RemoteException {
            synchronized(mSyncObj){
                if(pCmdStatus == null || pCmdStatus.getCmdId() == null){
                     logger.debug( "ICMDStatus error, pCmdStatus="+ pCmdStatus);
                    return;
                 }

                logger.debug("ListenerHandler : CMDStatus");
                logger.debug("ListenerHandler : CMDStatus : pCmdStatus.getRequestID() "+
                        pCmdStatus.getRequestId());
                logger.debug("ListenerHandler : CMDStatus : pCmdStatus.getUserData() "+
                        pCmdStatus.getUserData());
                logger.debug("ListenerHandler : CMDStatus : pCmdStatus.getCmdId() "+
                        pCmdStatus.getCmdId().getCmdId());
                if(pCmdStatus.getStatus() != null){
                    logger.debug("ListenerHandler : CMDStatus : pCmdStatus.getStatus() "+
                           pCmdStatus.getStatus().getStatusCode());
                }

                switch (pCmdStatus.getCmdId().getCmdId()) {
                case PresCmdId.UCE_PRES_CMD_PUBLISHMYCAP:
                    Message publishCmdMsg = StackListener.this.obtainMessage(
                            PRESENCE_IMS_UNSOL_PUBLISH_CMDSTATUS,
                            pCmdStatus);
                    StackListener.this.sendMessage(publishCmdMsg);
                    break;

                case PresCmdId.UCE_PRES_CMD_GETCONTACTCAP:
                case PresCmdId.UCE_PRES_CMD_GETCONTACTLISTCAP:
                    Message notifyUpdateCmdMsg = StackListener.this.obtainMessage(
                            PRESENCE_IMS_UNSOL_NOTIFY_UPDATE_CMDSTATUS,
                            pCmdStatus);
                    StackListener.this.sendMessage(notifyUpdateCmdMsg);
                    break;

                case PresCmdId.UCE_PRES_CMD_SETNEWFEATURETAG:
                    logger.debug("UCE_PRES_CMD_SETNEWFEATURETAG: app does not care it");
                    break;

                default:
                    logger.debug("CMD ID for unknown value=" +
                            pCmdStatus.getCmdId().getCmdId());
                }
            }
        }

        public void unpublishMessageSent() {
            logger.debug("unpublishMessageSent()");
        }
    };
}

