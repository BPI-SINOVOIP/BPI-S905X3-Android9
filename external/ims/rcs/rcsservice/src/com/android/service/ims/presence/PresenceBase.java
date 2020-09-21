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

import java.lang.String;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.os.RemoteException;
import android.content.Intent;
import com.android.internal.telephony.TelephonyIntents;
import android.content.ComponentName;

import com.android.ims.internal.uce.common.StatusCode;
import com.android.ims.internal.uce.common.StatusCode;
import com.android.ims.internal.uce.presence.PresCmdStatus;
import com.android.ims.internal.uce.presence.PresResInfo;
import com.android.ims.internal.uce.presence.PresSipResponse;

import com.android.ims.RcsManager.ResultCode;
import com.android.ims.RcsPresence.PublishState;
import com.android.ims.RcsPresenceInfo;

import com.android.ims.internal.Logger;
import com.android.service.ims.Task;
import com.android.service.ims.RcsUtils;
import com.android.service.ims.TaskManager;

public abstract class PresenceBase{
    /*
     * The logger
     */
    static private Logger logger = Logger.getLogger("PresenceBase");

    protected Context mContext = null;

    public PresenceBase() {
    }

    protected void handleCallback(Task task, int resultCode, boolean forCmdStatus){
        if(task == null){
            logger.debug("task == null");
            return;
        }

        if(task.mListener != null){
            try{
                if(resultCode >= ResultCode.SUCCESS){
                    if(!forCmdStatus){
                        task.mListener.onSuccess(task.mTaskId);
                    }
                }else{
                    task.mListener.onError(task.mTaskId, resultCode);
                }
            }catch(RemoteException e){
                logger.debug("Failed to send the status to client.");
            }
        }

        // remove task when error
        // remove task when SIP response success.
        // For list capability polling we will waiting for the terminated notify or timeout.
        if(resultCode != ResultCode.SUCCESS){
            if(task instanceof PresencePublishTask){
                PresencePublishTask publishTask = (PresencePublishTask) task;
                logger.debug("handleCallback for publishTask=" + publishTask);
                if(resultCode == PublishState.PUBLISH_STATE_VOLTE_PROVISION_ERROR) {
                    // retry 3 times for "403 Not Authorized for Presence".
                    if(publishTask.getRetryCount() >= 3) {
                        //remove capability after try 3 times by PresencePolling
                        logger.debug("handleCallback remove task=" + task);
                        TaskManager.getDefault().removeTask(task.mTaskId);
                    } else {
                        // Continue retry
                        publishTask.setRetryCount(publishTask.getRetryCount() + 1);
                    }
                } else {
                    logger.debug("handleCallback remove task=" + task);
                    TaskManager.getDefault().removeTask(task.mTaskId);
                }
            } else {
                logger.debug("handleCallback remove task=" + task);
                TaskManager.getDefault().removeTask(task.mTaskId);
            }
        }else{
            if(forCmdStatus || !forCmdStatus && (task instanceof PresenceCapabilityTask)){
                logger.debug("handleCallback remove task later");

                //waiting for Notify from network
                if(!forCmdStatus){
                    ((PresenceCapabilityTask)task).setWaitingForNotify(true);
                }
            }else{
                if(!forCmdStatus && (task instanceof PresenceAvailabilityTask) &&
                        (resultCode == ResultCode.SUCCESS)){
                    // Availiablity, cache for 60s, remove it later.
                    logger.debug("handleCallback PresenceAvailabilityTask cache for 60s task="
                            + task);
                    return;
                }

                logger.debug("handleCallback remove task=" + task);
                TaskManager.getDefault().removeTask(task.mTaskId);
            }
        }
    }

    public void handleCmdStatus(PresCmdStatus pCmdStatus){
        if(pCmdStatus == null){
            logger.error("handleCallbackForCmdStatus pCmdStatus=null");
            return;
        }

        Task task = TaskManager.getDefault().getTask(pCmdStatus.getUserData());
        int resultCode = RcsUtils.statusCodeToResultCode(pCmdStatus.getStatus().getStatusCode());
        if(task != null){
            task.mSipRequestId = pCmdStatus.getRequestId();
            task.mCmdStatus = resultCode;
            TaskManager.getDefault().putTask(task.mTaskId, task);
        }

        handleCallback(task, resultCode, true);
    }

    protected void notifyDm() {
        logger.debug("notifyDm");
        Intent intent = new Intent(
                TelephonyIntents.ACTION_FORBIDDEN_NO_SERVICE_AUTHORIZATION);
        intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING);

        mContext.sendBroadcast(intent);
    }

    protected boolean isInConfigList(int errorNo, String phrase, int configId) {
        String inErrorString = ("" + errorNo).trim();

        String[] errorArray = mContext.getResources().getStringArray(configId);
        logger.debug("errorArray length=" + errorArray.length + " errorArray=" + errorArray);
        for (String errorStr : errorArray) {
            if (errorStr != null && errorStr.startsWith(inErrorString)) {
                String errorPhrase = errorStr.substring(inErrorString.length());
                if(errorPhrase == null || errorPhrase.isEmpty()) {
                    return true;
                }

                if(phrase == null || phrase.isEmpty()) {
                    return false;
                }

                return phrase.toLowerCase().contains(errorPhrase.toLowerCase());
            }
        }
        return false;
    }

    abstract public void handleSipResponse(PresSipResponse pSipResponse);
}

