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

import java.util.List;
import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.Semaphore;
import android.content.ContentValues;
import android.text.TextUtils;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import com.android.internal.telephony.TelephonyIntents;
import android.os.HandlerThread;
import android.os.RemoteException;
import android.telephony.TelephonyManager;
import android.database.Cursor;

import java.lang.String;
import android.content.Context;
import android.util.Log;

import com.android.ims.internal.uce.presence.PresSipResponse;
import com.android.ims.internal.uce.common.StatusCode;
import com.android.ims.internal.uce.common.StatusCode;
import com.android.ims.internal.uce.presence.PresSubscriptionState;
import com.android.ims.internal.uce.presence.PresCmdStatus;
import com.android.ims.internal.uce.presence.PresResInfo;
import com.android.ims.internal.uce.presence.PresRlmiInfo;
import com.android.ims.internal.uce.presence.PresTupleInfo;

import com.android.ims.RcsPresenceInfo;
import com.android.ims.RcsPresence;
import com.android.ims.IRcsPresenceListener;
import com.android.ims.RcsManager.ResultCode;
import com.android.ims.RcsPresence.PublishState;

import com.android.ims.internal.Logger;
import com.android.ims.internal.ContactNumberUtils;
import com.android.service.ims.TaskManager;
import com.android.service.ims.Task;
import com.android.service.ims.RcsStackAdaptor;
import com.android.service.ims.RcsUtils;
import com.android.service.ims.RcsSettingUtils;

import com.android.service.ims.R;

public class PresenceSubscriber extends PresenceBase{
    /*
     * The logger
     */
    private Logger logger = Logger.getLogger(this.getClass().getName());

    private RcsStackAdaptor mRcsStackAdaptor = null;

    private static PresenceSubscriber sSubsriber = null;

    private String mAvailabilityRetryNumber = null;

    /*
     * Constructor
     */
    public PresenceSubscriber(RcsStackAdaptor rcsStackAdaptor, Context context){
        mRcsStackAdaptor = rcsStackAdaptor;
        mContext = context;
    }

    private String numberToUriString(String number){
        String formatedContact = number;
        if(!formatedContact.startsWith("sip:") && !formatedContact.startsWith("tel:")){
            String domain = TelephonyManager.getDefault().getIsimDomain();
            logger.debug("domain=" + domain);
            if(domain == null || domain.length() ==0){
                formatedContact = "tel:" + formatedContact;
            }else{
                formatedContact = "sip:" + formatedContact + "@" + domain;
            }
        }

        logger.print("numberToUriString formatedContact=" + formatedContact);
        return formatedContact;
    }

    private String numberToTelString(String number){
        String formatedContact = number;
        if(!formatedContact.startsWith("sip:") && !formatedContact.startsWith("tel:")){
            formatedContact = "tel:" + formatedContact;
        }

        logger.print("numberToTelString formatedContact=" + formatedContact);
        return formatedContact;
    }

    public int requestCapability(List<String> contactsNumber,
            IRcsPresenceListener listener){

        int ret = mRcsStackAdaptor.checkStackAndPublish();
        if(ret < ResultCode.SUCCESS){
            logger.error("requestCapability ret=" + ret);
            return ret;
        }

        if(contactsNumber == null || contactsNumber.size() ==0){
            ret = ResultCode.SUBSCRIBE_INVALID_PARAM;
            return ret;
        }

        logger.debug("check contact size ...");
        if(contactsNumber.size() > RcsSettingUtils.getMaxNumbersInRCL(mContext)){
            ret = ResultCode.SUBSCRIBE_TOO_LARGE;
            logger.error("requestCapability contctNumber size=" + contactsNumber.size());
            return ret;
        }

        String[] formatedNumbers = ContactNumberUtils.getDefault().format(contactsNumber);
        ret = ContactNumberUtils.getDefault().validate(formatedNumbers);
        if(ret != ContactNumberUtils.NUMBER_VALID){
            logger.error("requestCapability ret=" + ret);
            return ret;
        }

        String[] formatedContacts = new String[formatedNumbers.length];
        for(int i=0; i<formatedContacts.length; i++){
            formatedContacts[i] = numberToTelString(formatedNumbers[i]);
        }
        // In ms
        long timeout = RcsSettingUtils.getCapabPollListSubExp(mContext) * 1000;
        timeout += RcsSettingUtils.getSIPT1Timer(mContext);

        // The terminal notification may be received shortly after the time limit of
        // the subscription due to network delays or retransmissions.
        // Device shall wait for 3sec after the end of the subscription period in order to
        // accept such notifications without returning spurious errors (e.g. SIP 481)
        timeout += 3000;

        logger.print("add to task manager, formatedNumbers=" +
                RcsUtils.toContactString(formatedNumbers));
        int taskId = TaskManager.getDefault().addCapabilityTask(mContext, formatedNumbers,
                listener, timeout);
        logger.print("taskId=" + taskId);

        ret = mRcsStackAdaptor.requestCapability(formatedContacts, taskId);
        if(ret < ResultCode.SUCCESS){
            logger.error("requestCapability ret=" + ret + " remove taskId=" + taskId);
            TaskManager.getDefault().removeTask(taskId);
        }

        ret = taskId;

        return  ret;
    }

    public int requestAvailability(String contactNumber, IRcsPresenceListener listener,
            boolean forceToNetwork){

        String formatedContact = ContactNumberUtils.getDefault().format(contactNumber);
        int ret = ContactNumberUtils.getDefault().validate(formatedContact);
        if(ret != ContactNumberUtils.NUMBER_VALID){
            return ret;
        }

        if(!forceToNetwork){
            logger.debug("check if we can use the value in cache");
            int availabilityExpire = RcsSettingUtils.getAvailabilityCacheExpiration(mContext);
            availabilityExpire = availabilityExpire>0?availabilityExpire*1000:
                    60*1000; // by default is 60s
            logger.print("requestAvailability availabilityExpire=" + availabilityExpire);

            TaskManager.getDefault().clearTimeoutAvailabilityTask(availabilityExpire);

            Task task = TaskManager.getDefault().getAvailabilityTaskByContact(formatedContact);
            if(task != null && task instanceof PresenceAvailabilityTask) {
                PresenceAvailabilityTask availabilityTask = (PresenceAvailabilityTask)task;
                if(availabilityTask.getNotifyTimestamp() == 0) {
                    // The previous one didn't get response yet.
                    logger.print("requestAvailability: the request is pending in queue");
                    return ResultCode.SUBSCRIBE_ALREADY_IN_QUEUE;
                }else {
                    // not expire yet. Can use the previous value.
                    logger.print("requestAvailability: the prevous valuedoesn't be expired yet");
                    return ResultCode.SUBSCRIBE_TOO_FREQUENTLY;
                }
            }
        }

        boolean isFtSupported = false; // hard code to not support FT at present.
        boolean isChatSupported = false;  // hard code to not support chat at present.
        // Only poll/fetch capability/availability on LTE
        if(((TelephonyManager.getDefault().getNetworkType() != TelephonyManager.NETWORK_TYPE_LTE)
                && !isFtSupported && !isChatSupported)){
            logger.error("requestAvailability return ERROR_SERVICE_NOT_AVAILABLE" +
                    " for it is not LTE network");
            return ResultCode.ERROR_SERVICE_NOT_AVAILABLE;
        }

        ret = mRcsStackAdaptor.checkStackAndPublish();
        if(ret < ResultCode.SUCCESS){
            logger.error("requestAvailability=" + ret);
            return ret;
        }

        // user number format in TaskManager.
        int taskId = TaskManager.getDefault().addAvailabilityTask(formatedContact, listener);

        // Change it to URI format.
        formatedContact = numberToUriString(formatedContact);

        logger.print("addAvailabilityTask formatedContact=" + formatedContact);

        ret = mRcsStackAdaptor.requestAvailability(formatedContact, taskId);
        if(ret < ResultCode.SUCCESS){
            logger.error("requestAvailability ret=" + ret + " remove taskId=" + taskId);
            TaskManager.getDefault().removeTask(taskId);
        }

        ret = taskId;

        return  ret;
    }

    private int translateResponse403(PresSipResponse pSipResponse){
        String reasonPhrase = pSipResponse.getReasonPhrase();
        if(reasonPhrase == null){
            // No retry. The PS provisioning has not occurred correctly. UX Decision to show errror.
            return ResultCode.SUBSCRIBE_GENIRIC_FAILURE;
        }

        reasonPhrase = reasonPhrase.toLowerCase();
        if(reasonPhrase.contains("user not registered")){
            // Register to IMS then retry the single resource subscription if capability polling.
            // availability fetch: no retry. ignore the availability and allow LVC? (PLM decision)
            return ResultCode.SUBSCRIBE_NOT_REGISTERED;
        }

        if(reasonPhrase.contains("not authorized for presence")){
            // No retry.
            return ResultCode.SUBSCRIBE_NOT_AUTHORIZED_FOR_PRESENCE;
        }

        // unknown phrase: handle it as the same as no phrase
        return ResultCode.SUBSCRIBE_FORBIDDEN;
    }

    private int translateResponseCode(PresSipResponse pSipResponse){
        // pSipResponse should not be null.
        logger.debug("translateResponseCode getSipResponseCode=" +
                pSipResponse.getSipResponseCode());
        int ret = ResultCode.SUBSCRIBE_GENIRIC_FAILURE;

        int sipCode = pSipResponse.getSipResponseCode();
        if(sipCode < 100 || sipCode > 699){
            logger.debug("internal error code sipCode=" + sipCode);
            ret = ResultCode.SUBSCRIBE_TEMPORARY_ERROR; //it is internal issue. ignore it.
            return ret;
        }

        switch(sipCode){
            case 200:
                ret = ResultCode.SUCCESS;
                break;

            case 403:
                ret = translateResponse403(pSipResponse);
                break;

            case 404:
               // Target MDN is not provisioned for VoLTE or it is not  known as VzW IMS subscriber
               // Device shall not retry. Device shall remove the VoLTE status of the target MDN
               // and update UI
               ret = ResultCode.SUBSCRIBE_NOT_FOUND;
               break;

            case 408:
                // Request Timeout
                // Device shall retry with exponential back-off
                ret = ResultCode.SUBSCRIBE_TEMPORARY_ERROR;
                break;

            case 413:
                // Too Large.
                // Application need shrink the size of request contact list and resend the request
                ret = ResultCode.SUBSCRIBE_TOO_LARGE;
                break;

            case 423:
                // Interval Too Short. Requested expiry interval too short and server rejects it
                // Device shall re-attempt subscription after changing the expiration interval in
                // the Expires header field to be equal to or greater than the expiration interval
                // within the Min-Expires header field of the 423 response
                ret = ResultCode.SUBSCRIBE_TEMPORARY_ERROR;
                break;

            case 500:
                // 500 Server Internal Error
                // capability polling: exponential back-off retry (same rule as resource list)
                // availability fetch: no retry. ignore the availability and allow LVC
                // (PLM decision)
                ret = ResultCode.SUBSCRIBE_TEMPORARY_ERROR;
                break;

            case 503:
                // capability polling: exponential back-off retry (same rule as resource list)
                // availability fetch: no retry. ignore the availability and allow LVC?
                // (PLM decision)
                ret = ResultCode.SUBSCRIBE_TEMPORARY_ERROR;
                break;

                // capability polling: Device shall retry with exponential back-off
                // Availability Fetch: device shall ignore the error and shall not retry
            case 603:
                ret = ResultCode.SUBSCRIBE_TEMPORARY_ERROR;
                break;

            default:
                // Other 4xx/5xx/6xx
                // Device shall not retry
                ret = ResultCode.SUBSCRIBE_GENIRIC_FAILURE;
        }

        logger.debug("translateResponseCode ret=" + ret);
        return ret;
    }

    public void handleSipResponse(PresSipResponse pSipResponse){
        if(pSipResponse == null){
            logger.debug("handleSipResponse pSipResponse = null");
            return;
        }

        int sipCode = pSipResponse.getSipResponseCode();
        String phrase = pSipResponse.getReasonPhrase();
        if(isInConfigList(sipCode, phrase,
                R.array.config_volte_provision_error_on_subscribe_response)) {
            logger.print("volte provision sipCode=" + sipCode + " phrase=" + phrase);
            mRcsStackAdaptor.setPublishState(PublishState.PUBLISH_STATE_VOLTE_PROVISION_ERROR);

            notifyDm();
        } else if(isInConfigList(sipCode, phrase,
                R.array.config_rcs_provision_error_on_subscribe_response)) {
            logger.print("rcs provision sipCode=" + sipCode + " phrase=" + phrase);
            mRcsStackAdaptor.setPublishState(PublishState.PUBLISH_STATE_RCS_PROVISION_ERROR);
        }

        int errorCode = translateResponseCode(pSipResponse);
        logger.print("handleSipResponse errorCode=" + errorCode);

        if(errorCode == ResultCode.SUBSCRIBE_NOT_REGISTERED){
            logger.debug("setPublishState to unknown" +
                   " for subscribe error 403 not registered");
            mRcsStackAdaptor.setPublishState(
                   PublishState.PUBLISH_STATE_OTHER_ERROR);
        }

        if(errorCode == ResultCode.SUBSCRIBE_NOT_AUTHORIZED_FOR_PRESENCE) {
            logger.debug("ResultCode.SUBSCRIBE_NOT_AUTHORIZED_FOR_PRESENCE");
        }

        if(errorCode == ResultCode.SUBSCRIBE_FORBIDDEN){
            logger.debug("ResultCode.SUBSCRIBE_FORBIDDEN");
        }

        // Suppose the request ID had been set when IQPresListener_CMDStatus
        Task task = TaskManager.getDefault().getTaskByRequestId(
                pSipResponse.getRequestId());
        logger.debug("handleSipResponse task=" + task);
        if(task != null){
            task.mSipResponseCode = sipCode;
            task.mSipReasonPhrase = phrase;
            TaskManager.getDefault().putTask(task.mTaskId, task);
        }

        if(errorCode == ResultCode.SUBSCRIBE_NOT_REGISTERED &&
                task != null && task.mCmdId == TaskManager.TASK_TYPE_GET_AVAILABILITY) {
            String[] contacts = ((PresenceTask)task).mContacts;
            if(contacts != null && contacts.length>0){
                mAvailabilityRetryNumber = contacts[0];
            }
            logger.debug("retry to get availability for " + mAvailabilityRetryNumber);
        }

        // 404 error for single contact only as per requirement
        // need handle 404 for multiple contacts as per CV 3.24.
        if(errorCode == ResultCode.SUBSCRIBE_NOT_FOUND &&
                task != null && ((PresenceTask)task).mContacts != null) {
            String[] contacts = ((PresenceTask)task).mContacts;
            ArrayList<RcsPresenceInfo> presenceInfoList = new ArrayList<RcsPresenceInfo>();

            for(int i=0; i<contacts.length; i++){
                if(TextUtils.isEmpty(contacts[i])){
                    continue;
                }

                RcsPresenceInfo presenceInfo = new RcsPresenceInfo(contacts[i],
                        RcsPresenceInfo.VolteStatus.VOLTE_DISABLED,
                        RcsPresenceInfo.ServiceState.OFFLINE, null, System.currentTimeMillis(),
                        RcsPresenceInfo.ServiceState.OFFLINE, null, System.currentTimeMillis());
                presenceInfoList.add(presenceInfo);
            }

            // Notify presence information changed.
            logger.debug("notify presence changed for 404 error");
            Intent intent = new Intent(RcsPresence.ACTION_PRESENCE_CHANGED);
            intent.putParcelableArrayListExtra(RcsPresence.EXTRA_PRESENCE_INFO_LIST,
                    presenceInfoList);
            intent.putExtra("updateLastTimestamp", true);
            launchPersistService(intent);
        } else if(errorCode == ResultCode.SUBSCRIBE_GENIRIC_FAILURE) {
            updateAvailabilityToUnknown(task);
        }

        handleCallback(task, errorCode, false);
    }

    private void launchPersistService(Intent intent) {
        ComponentName component = new ComponentName("com.android.service.ims.presence",
                "com.android.service.ims.presence.PersistService");
        intent.setComponent(component);
        mContext.startService(intent);
    }

    public void retryToGetAvailability() {
        if(mAvailabilityRetryNumber == null){
            return;
        }
        requestAvailability(mAvailabilityRetryNumber, null, true);
        //retry one time only
        mAvailabilityRetryNumber = null;
    }

    public void updatePresence(String pPresentityUri, PresTupleInfo[] pTupleInfo){
        if(mContext == null){
            logger.error("updatePresence mContext == null");
            return;
        }

        RcsPresenceInfo rcsPresenceInfo = PresenceInfoParser.getPresenceInfoFromTuple(
                pPresentityUri, pTupleInfo);
        if(rcsPresenceInfo == null || TextUtils.isEmpty(
                rcsPresenceInfo.getContactNumber())){
            logger.error("rcsPresenceInfo is null or " +
                    "TextUtils.isEmpty(rcsPresenceInfo.getContactNumber()");
            return;
        }

        ArrayList<RcsPresenceInfo> rcsPresenceInfoList = new ArrayList<RcsPresenceInfo>();
        rcsPresenceInfoList.add(rcsPresenceInfo);

        // For single contact number we got 1 NOTIFY only. So regard it as terminated.
        TaskManager.getDefault().onTerminated(rcsPresenceInfo.getContactNumber());

        PresenceAvailabilityTask availabilityTask = TaskManager.getDefault().
                getAvailabilityTaskByContact(rcsPresenceInfo.getContactNumber());
        if(availabilityTask != null){
            availabilityTask.updateNotifyTimestamp();
        }

        // Notify presence information changed.
        Intent intent = new Intent(RcsPresence.ACTION_PRESENCE_CHANGED);
        intent.putParcelableArrayListExtra(RcsPresence.EXTRA_PRESENCE_INFO_LIST,
                rcsPresenceInfoList);
        intent.putExtra("updateLastTimestamp", true);
        launchPersistService(intent);
    }

    public void updatePresences(PresRlmiInfo pRlmiInfo, PresResInfo[] pRcsPresenceInfo) {
        if(mContext == null){
            logger.error("updatePresences: mContext == null");
            return;
        }

        RcsPresenceInfo[] rcsPresenceInfos = PresenceInfoParser.
                getPresenceInfosFromPresenceRes(pRlmiInfo, pRcsPresenceInfo);
        if(rcsPresenceInfos == null){
            logger.error("updatePresences: rcsPresenceInfos == null");
            return;
        }

        ArrayList<RcsPresenceInfo> rcsPresenceInfoList = new ArrayList<RcsPresenceInfo>();

        for (int i=0; i < rcsPresenceInfos.length; i++) {
            RcsPresenceInfo rcsPresenceInfo = rcsPresenceInfos[i];
            if(rcsPresenceInfo != null && !TextUtils.isEmpty(rcsPresenceInfo.getContactNumber())){
                logger.debug("rcsPresenceInfo=" + rcsPresenceInfo);

                rcsPresenceInfoList.add(rcsPresenceInfo);
            }
        }

        boolean isTerminated = false;
        if(pRlmiInfo.getPresSubscriptionState() != null){
            if(pRlmiInfo.getPresSubscriptionState().getPresSubscriptionStateValue() ==
                    PresSubscriptionState.UCE_PRES_SUBSCRIPTION_STATE_TERMINATED){
                isTerminated = true;
            }
        }

        if(isTerminated){
            TaskManager.getDefault().onTerminated(pRlmiInfo.getRequestId(),
                    pRlmiInfo.getSubscriptionTerminatedReason());
        }

        if (rcsPresenceInfoList.size() > 0) {
            // Notify presence changed
            Intent intent = new Intent(RcsPresence.ACTION_PRESENCE_CHANGED);
            intent.putParcelableArrayListExtra(RcsPresence.EXTRA_PRESENCE_INFO_LIST,
                    rcsPresenceInfoList);
            logger.debug("broadcast ACTION_PRESENCE_CHANGED, rcsPresenceInfo=" +
                    rcsPresenceInfoList);
            intent.putExtra("updateLastTimestamp", true);
            launchPersistService(intent);
        }
    }

    public void handleCmdStatus(PresCmdStatus pCmdStatus){
        if(pCmdStatus == null){
            logger.error("handleCallbackForCmdStatus pCmdStatus=null");
            return;
        }


        Task taskTmp = TaskManager.getDefault().getTask(pCmdStatus.getUserData());
        int resultCode = RcsUtils.statusCodeToResultCode(pCmdStatus.getStatus().getStatusCode());
        logger.print("handleCmdStatus resultCode=" + resultCode);
        PresenceTask task = null;
        if(taskTmp != null && (taskTmp instanceof PresenceTask)){
            task = (PresenceTask)taskTmp;
            task.mSipRequestId = pCmdStatus.getRequestId();
            task.mCmdStatus = resultCode;
            TaskManager.getDefault().putTask(task.mTaskId, task);

            // handle error as the same as temporary network error
            // set availability to false, keep old capability
            if(resultCode != ResultCode.SUCCESS && task.mContacts != null){
                updateAvailabilityToUnknown(task);
            }
        }

        handleCallback(task, resultCode, true);
    }

    private void updateAvailabilityToUnknown(Task inTask){
        //only used for serviceState is offline or unknown.
        if(mContext == null){
            logger.error("updateAvailabilityToUnknown mContext=null");
            return;
        }

        if(inTask == null){
            logger.error("updateAvailabilityToUnknown task=null");
            return;
        }

        if(!(inTask instanceof PresenceTask)){
            logger.error("updateAvailabilityToUnknown not PresencTask");
            return;
        }

        PresenceTask task = (PresenceTask)inTask;

        if(task.mContacts == null || task.mContacts.length ==0){
            logger.error("updateAvailabilityToUnknown no contacts");
            return;
        }

        ArrayList<RcsPresenceInfo> presenceInfoList = new ArrayList<RcsPresenceInfo>();
        for(int i = 0; i< task.mContacts.length; i++){
            if(task.mContacts[i] == null || task.mContacts[i].length() == 0){
                continue;
            }

            RcsPresenceInfo presenceInfo = new RcsPresenceInfo(
                PresenceInfoParser.getPhoneFromUri(task.mContacts[i]),
                        RcsPresenceInfo.VolteStatus.VOLTE_UNKNOWN,
                        RcsPresenceInfo.ServiceState.UNKNOWN, null, System.currentTimeMillis(),
                        RcsPresenceInfo.ServiceState.UNKNOWN, null, System.currentTimeMillis());
            presenceInfoList.add(presenceInfo);
        }

        if(presenceInfoList.size() > 0) {
             // Notify presence information changed.
             logger.debug("notify presence changed for cmd error");
             Intent intent = new Intent(RcsPresence.ACTION_PRESENCE_CHANGED);
             intent.putParcelableArrayListExtra(RcsPresence.EXTRA_PRESENCE_INFO_LIST,
                     presenceInfoList);

             // don't update timestamp so it can be subscribed soon.
             intent.putExtra("updateLastTimestamp", false);
             launchPersistService(intent);
        }
    }
}

