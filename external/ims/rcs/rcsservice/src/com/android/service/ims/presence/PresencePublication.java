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
import java.util.Arrays;

import android.content.Context;
import android.content.Intent;
import com.android.internal.telephony.Phone;
import android.provider.Settings;
import android.os.SystemProperties;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.telephony.TelephonyManager;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.TelephonyIntents;
import android.telecom.TelecomManager;
import android.content.IntentFilter;
import android.app.PendingIntent;
import android.app.AlarmManager;
import android.os.SystemClock;
import android.os.Message;
import android.os.Handler;


import com.android.ims.ImsManager;
import com.android.ims.ImsConnectionStateListener;
import com.android.ims.ImsServiceClass;
import com.android.ims.ImsException;
import android.telephony.SubscriptionManager;
import com.android.ims.ImsConfig;
import com.android.ims.ImsConfig.FeatureConstants;
import com.android.ims.ImsConfig.FeatureValueConstants;

import com.android.service.ims.RcsSettingUtils;
import com.android.ims.RcsPresenceInfo;
import com.android.ims.IRcsPresenceListener;
import com.android.ims.RcsManager.ResultCode;
import com.android.ims.RcsPresence.PublishState;

import com.android.ims.internal.Logger;
import com.android.service.ims.TaskManager;
import com.android.service.ims.Task;

import com.android.ims.internal.uce.presence.PresPublishTriggerType;
import com.android.ims.internal.uce.presence.PresSipResponse;
import com.android.ims.internal.uce.common.StatusCode;
import com.android.ims.internal.uce.presence.PresCmdStatus;

import com.android.service.ims.RcsStackAdaptor;

import com.android.service.ims.R;

public class PresencePublication extends PresenceBase {
    private Logger logger = Logger.getLogger(this.getClass().getName());

    private final Object mSyncObj = new Object();

    boolean mMovedToIWLAN = false;
    boolean mMovedToLTE = false;
    boolean mVoPSEnabled = false;

    boolean mIsVolteAvailable = false;
    boolean mIsVtAvailable = false;
    boolean mIsVoWifiAvailable = false;
    boolean mIsViWifiAvailable = false;

    // Queue for the pending PUBLISH request. Just need cache the last one.
    volatile PublishRequest mPendingRequest = null;
    volatile PublishRequest mPublishingRequest = null;
    volatile PublishRequest mPublishedRequest = null;

    /*
     * Message to notify for a publish
     */
    public static final int MESSAGE_RCS_PUBLISH_REQUEST = 1;

    /**
     *  Publisher Error base
     */
    public static final int PUBLISH_ERROR_CODE_START = ResultCode.SUBSCRIBER_ERROR_CODE_END;

    /**
     * All publish errors not covered by specific errors
     */
    public static final int PUBLISH_GENIRIC_FAILURE = PUBLISH_ERROR_CODE_START - 1;

    /**
     * Responding for 403 - not authorized
     */
    public static final int PUBLISH_NOT_AUTHORIZED_FOR_PRESENCE = PUBLISH_ERROR_CODE_START - 2;

    /**
     * Responding for 404 error code. The subscriber is not provisioned.
     * The Client should not send any EAB traffic after get this error.
     */
    public static final int PUBLISH_NOT_PROVISIONED = PUBLISH_ERROR_CODE_START - 3;

    /**
     * Responding for 200 - OK
     */
    public static final int PUBLISH_SUCESS = ResultCode.SUCCESS;

    private RcsStackAdaptor mRcsStackAdaptor = null;
    private PresenceSubscriber mSubscriber = null;
    static private PresencePublication sPresencePublication = null;
    private BroadcastReceiver mReceiver = null;

    private boolean mHasCachedTrigger = false;
    private boolean mGotTriggerFromStack = false;
    private boolean mDonotRetryUntilPowerCycle = false;
    private boolean mSimLoaded = false;
    private int mPreferredTtyMode = Phone.TTY_MODE_OFF;

    private boolean mImsRegistered = false;
    private boolean mVtEnabled = false;
    private boolean mDataEnabled = false;

    public class PublishType{
        public static final int PRES_PUBLISH_TRIGGER_DATA_CHANGED = 0;
        // the lower layer should send trigger when enable volte
        // the lower layer should unpublish when disable volte
        // so only handle VT here.
        public static final int PRES_PUBLISH_TRIGGER_VTCALL_CHANGED = 1;
        public static final int PRES_PUBLISH_TRIGGER_CACHED_TRIGGER = 2;
        public static final int PRES_PUBLISH_TRIGGER_TTY_ENABLE_STATUS = 3;
        public static final int PRES_PUBLISH_TRIGGER_RETRY = 4;
        public static final int PRES_PUBLISH_TRIGGER_FEATURE_AVAILABILITY_CHANGED = 5;
    };

    /**
     * @param rcsStackAdaptor
     * @param context
     */
    public PresencePublication(RcsStackAdaptor rcsStackAdaptor, Context context) {
        super();
        logger.debug("PresencePublication constrcuct");
        this.mRcsStackAdaptor = rcsStackAdaptor;
        this.mContext = context;

        mVtEnabled = getImsManager().isVtEnabledByUser();

        mDataEnabled = Settings.Global.getInt(mContext.getContentResolver(),
                    Settings.Global.MOBILE_DATA, 1) == 1;
        new Thread(() -> {
            RcsSettingUtils.setMobileDataEnabled(mContext, mDataEnabled);
        }).start();
        logger.debug("The current mobile data is: " + (mDataEnabled ? "enabled" : "disabled"));

        mPreferredTtyMode = Settings.Secure.getInt(
                mContext.getContentResolver(),
                Settings.Secure.PREFERRED_TTY_MODE,
                Phone.TTY_MODE_OFF);
        logger.debug("The current TTY mode is: " + mPreferredTtyMode);

        if(mRcsStackAdaptor != null){
            mRcsStackAdaptor.setPublishState(
                    SystemProperties.getInt("rcs.publish.status",
                    PublishState.PUBLISH_STATE_NOT_PUBLISHED));
        }

        mReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                logger.print("onReceive intent=" + intent);
                if(TelephonyIntents.ACTION_SIM_STATE_CHANGED.equalsIgnoreCase(intent.getAction())){
                    String stateExtra = intent.getStringExtra(
                            IccCardConstants.INTENT_KEY_ICC_STATE);
                    logger.print("ACTION_SIM_STATE_CHANGED INTENT_KEY_ICC_STATE=" + stateExtra);
                    final TelephonyManager teleMgr = (TelephonyManager) context.getSystemService(
                                        Context.TELEPHONY_SERVICE);
                    if(IccCardConstants.INTENT_VALUE_ICC_LOADED.equalsIgnoreCase(stateExtra)) {
                        new Thread(new Runnable() {
                            @Override
                            public void run() {
                                if(teleMgr == null){
                                    logger.error("teleMgr = null");
                                    return;
                                }

                                int count = 0;
                                // retry 2 minutes to get the information from ISIM and RUIM
                                for(int i=0; i<60; i++) {
                                     String[] myImpu = teleMgr.getIsimImpu();
                                     String myDomain = teleMgr.getIsimDomain();
                                     String line1Number = teleMgr.getLine1Number();
                                     if(line1Number != null && line1Number.length() != 0 ||
                                         myImpu != null && myImpu.length != 0 &&
                                         myDomain != null && myDomain.length() != 0){
                                         mSimLoaded = true;
                                         // treate hot SIM hot swap as power on.
                                         mDonotRetryUntilPowerCycle = false;
                                         if(mHasCachedTrigger) {
                                             invokePublish(PresencePublication.PublishType.
                                                     PRES_PUBLISH_TRIGGER_CACHED_TRIGGER);
                                         }
                                         break;
                                     }else{
                                         try{
                                             Thread.sleep(2000);//retry 2 seconds later
                                         }catch(InterruptedException e){
                                         }
                                     }
                                 }
                            }
                        }, "wait for ISIM and RUIM").start();
                    }else if (IccCardConstants.INTENT_VALUE_ICC_ABSENT.
                            equalsIgnoreCase(stateExtra)) {
                        // pulled out the SIM, set it as the same as power on status:
                        logger.print("Pulled out SIM, set to PUBLISH_STATE_NOT_PUBLISHED");

                        // only reset when the SIM gets absent.
                        reset();
                        mSimLoaded = false;
                        setPublishState(
                                PublishState.PUBLISH_STATE_NOT_PUBLISHED);
                    }
                }else if(Intent.ACTION_AIRPLANE_MODE_CHANGED.equalsIgnoreCase(intent.getAction())){
                    boolean airplaneMode = intent.getBooleanExtra("state", false);
                    if(airplaneMode){
                        logger.print("Airplane mode, set to PUBLISH_STATE_NOT_PUBLISHED");
                        reset();
                        setPublishState(
                                PublishState.PUBLISH_STATE_NOT_PUBLISHED);
                    }
                }else if(TelecomManager.ACTION_TTY_PREFERRED_MODE_CHANGED.equalsIgnoreCase(
                        intent.getAction())){
                    int newPreferredTtyMode = intent.getIntExtra(
                            TelecomManager.EXTRA_TTY_PREFERRED_MODE,
                            TelecomManager.TTY_MODE_OFF);
                    newPreferredTtyMode = telecomTtyModeToPhoneMode(newPreferredTtyMode);
                    logger.debug("Tty mode changed from " + mPreferredTtyMode
                            + " to " + newPreferredTtyMode);

                    boolean mIsTtyEnabled = isTtyEnabled(mPreferredTtyMode);
                    boolean isTtyEnabled = isTtyEnabled(newPreferredTtyMode);
                    mPreferredTtyMode = newPreferredTtyMode;
                    if (mIsTtyEnabled != isTtyEnabled) {
                        logger.print("ttyEnabled status changed from " + mIsTtyEnabled
                                + " to " + isTtyEnabled);
                        invokePublish(PresencePublication.PublishType.
                                PRES_PUBLISH_TRIGGER_TTY_ENABLE_STATUS);
                    }
                } else if(ImsConfig.ACTION_IMS_FEATURE_CHANGED.equalsIgnoreCase(
                        intent.getAction())){
                    int item = intent.getIntExtra(ImsConfig.EXTRA_CHANGED_ITEM, -1);
                    if ((ImsConfig.ConfigConstants.VLT_SETTING_ENABLED == item) ||
                            (ImsConfig.ConfigConstants.LVC_SETTING_ENABLED == item) ||
                            (ImsConfig.ConfigConstants.EAB_SETTING_ENABLED == item)) {
                        handleProvisionChanged();
                    }
                }
            }
        };

        IntentFilter statusFilter = new IntentFilter();
        statusFilter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        statusFilter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        statusFilter.addAction(TelecomManager.ACTION_TTY_PREFERRED_MODE_CHANGED);
        statusFilter.addAction(ImsConfig.ACTION_IMS_FEATURE_CHANGED);
        mContext.registerReceiver(mReceiver, statusFilter);

        sPresencePublication = this;
    }

    private boolean isIPVoiceSupported(boolean volteAvailable, boolean vtAvailable,
            boolean voWifiAvailable, boolean viWifiAvailable) {
        ImsManager imsManager = getImsManager();
        // volte and vowifi can be enabled separately
        if(!imsManager.isVolteEnabledByPlatform() && !imsManager.isWfcEnabledByPlatform()) {
            logger.print("Disabled by platform, voiceSupported=false");
            return false;
        }

        if(!imsManager.isVolteProvisionedOnDevice() &&
                !RcsSettingUtils.isVowifiProvisioned(mContext)) {
            logger.print("Wasn't provisioned, voiceSupported=false");
            return false;
        }

        if(!imsManager.isEnhanced4gLteModeSettingEnabledByUser() &&
                !imsManager.isWfcEnabledByUser()){
            logger.print("User didn't enable volte or wfc, voiceSupported=false");
            return false;
        }

        // for IWLAN. All WFC settings and provision should be fine if voWifiAvailable is true.
        if(isOnIWLAN()) {
            boolean voiceSupported=volteAvailable || voWifiAvailable;
            logger.print("on IWLAN, voiceSupported=" + voiceSupported);
            return voiceSupported;
        }

        // for none-IWLAN
        if(!isOnLTE()) {
            logger.print("isOnLTE=false, voiceSupported=false");
            return false;
        }

        if(!mVoPSEnabled) {
            logger.print("mVoPSEnabled=false, voiceSupported=false");
            return false;
        }

        logger.print("voiceSupported=true");
        return true;
    }

    private boolean isIPVideoSupported(boolean volteAvailable, boolean vtAvailable,
            boolean voWifiAvailable, boolean viWifiAvailable) {
        ImsManager imsManager = getImsManager();
        // if volte or vt was disabled then the viwifi will be disabled as well.
        if(!imsManager.isVolteEnabledByPlatform() ||
                !imsManager.isVtEnabledByPlatform()) {
            logger.print("Disabled by platform, videoSupported=false");
            return false;
        }

        if(!imsManager.isVolteProvisionedOnDevice() ||
                !RcsSettingUtils.isLvcProvisioned(mContext)) {
            logger.print("Not provisioned. videoSupported=false");
            return false;
        }

        if(!imsManager.isEnhanced4gLteModeSettingEnabledByUser() ||
                !mVtEnabled){
            logger.print("User disabled volte or vt, videoSupported=false");
            return false;
        }

        if(isTtyOn()){
            logger.print("isTtyOn=true, videoSupported=false");
            return false;
        }

        // for IWLAN, all WFC settings and provision should be fine if viWifiAvailable is true.
        if(isOnIWLAN()) {
            boolean videoSupported = vtAvailable || viWifiAvailable;
            logger.print("on IWLAN, videoSupported=" + videoSupported);
            return videoSupported;
        }

        if(!isDataEnabled()) {
            logger.print("isDataEnabled()=false, videoSupported=false");
            return false;
        }

        if(!isOnLTE()) {
            logger.print("isOnLTE=false, videoSupported=false");
            return false;
        }

        if(!mVoPSEnabled) {
            logger.print("mVoPSEnabled=false, videoSupported=false");
            return false;
        }

        return true;
    }

    public boolean isTtyOn() {
        logger.debug("isTtyOn settingsTtyMode=" + mPreferredTtyMode);
        return isTtyEnabled(mPreferredTtyMode);
    }

    public void onImsConnected() {
        mImsRegistered = true;
    }

    public void onImsDisconnected() {
        logger.debug("reset PUBLISH status for IMS had been disconnected");
        mImsRegistered = false;
        reset();
    }

    private void reset() {
        mIsVolteAvailable = false;
        mIsVtAvailable = false;
        mIsVoWifiAvailable = false;
        mIsViWifiAvailable = false;

        synchronized(mSyncObj) {
            mPendingRequest = null; //ignore the previous request
            mPublishingRequest = null;
            mPublishedRequest = null;
        }
    }

    public void handleImsServiceUp() {
        reset();
    }

    public void handleImsServiceDown() {
    }

    private void handleProvisionChanged() {
        if(RcsSettingUtils.isEabProvisioned(mContext)) {
            logger.debug("provisioned, set mDonotRetryUntilPowerCycle to false");
            mDonotRetryUntilPowerCycle = false;
            if(mHasCachedTrigger) {
                invokePublish(PresencePublication.PublishType.PRES_PUBLISH_TRIGGER_CACHED_TRIGGER);
            }
        }
    }

    static public PresencePublication getPresencePublication() {
        return sPresencePublication;
    }

    public void setSubscriber(PresenceSubscriber subscriber) {
        mSubscriber = subscriber;
    }

    public boolean isDataEnabled() {
        return  Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.MOBILE_DATA, 1) == 1;
    }

    public void onMobileDataChanged(boolean value){
        logger.print("onMobileDataChanged, mDataEnabled=" + mDataEnabled + " value=" + value);
        if(mDataEnabled != value) {
            mDataEnabled = value;
            RcsSettingUtils.setMobileDataEnabled(mContext, mDataEnabled);

            invokePublish(
                    PresencePublication.PublishType.PRES_PUBLISH_TRIGGER_DATA_CHANGED);
        }
    }

    public void onVtEnabled(boolean enabled) {
        logger.debug("onVtEnabled mVtEnabled=" + mVtEnabled + " enabled=" + enabled);

        if(mVtEnabled != enabled) {
            mVtEnabled = enabled;
            invokePublish(PresencePublication.PublishType.
                    PRES_PUBLISH_TRIGGER_VTCALL_CHANGED);
        }
    }

    /**
     * @return return true if it had been PUBLISHED.
     */
    public boolean isPublishedOrPublishing() {
        long publishThreshold = RcsSettingUtils.getPublishThrottle(mContext);

        boolean publishing = false;
        publishing = (mPublishingRequest != null) &&
                (System.currentTimeMillis() - mPublishingRequest.getTimestamp()
                <= publishThreshold);

        return (getPublishState() == PublishState.PUBLISH_STATE_200_OK || publishing);
    }

    /**
     * @return the Publish State
     */
    public int getPublishState() {
        if(mRcsStackAdaptor == null){
            return PublishState.PUBLISH_STATE_NOT_PUBLISHED;
        }

        return mRcsStackAdaptor.getPublishState();
    }

    /**
     * @param mPublishState the publishState to set
     */
    public void setPublishState(int publishState) {
        if(mRcsStackAdaptor != null){
            mRcsStackAdaptor.setPublishState(publishState);
        }
    }

    public boolean getHasCachedTrigger(){
        return mHasCachedTrigger;
    }

    // Tiggered by framework.
    public int invokePublish(int trigger){
        int ret;

        // if the value is true then it will call stack to send the PUBLISH
        // though the previous publish had the same capability
        // for example: for retry PUBLISH.
        boolean bForceToNetwork = true;
        switch(trigger)
        {
            case PublishType.PRES_PUBLISH_TRIGGER_DATA_CHANGED:
            {
                logger.print("PRES_PUBLISH_TRIGGER_DATA_CHANGED");
                bForceToNetwork = false;
                break;
            }
            case PublishType.PRES_PUBLISH_TRIGGER_VTCALL_CHANGED:
            {
                logger.print("PRES_PUBLISH_TRIGGER_VTCALL_CHANGED");
                // Didn't get featureCapabilityChanged sometimes.
                bForceToNetwork = true;
                break;
            }
            case PublishType.PRES_PUBLISH_TRIGGER_CACHED_TRIGGER:
            {
                logger.print("PRES_PUBLISH_TRIGGER_CACHED_TRIGGER");
                break;
            }
            case PublishType.PRES_PUBLISH_TRIGGER_TTY_ENABLE_STATUS:
            {
                logger.print("PRES_PUBLISH_TRIGGER_TTY_ENABLE_STATUS");
                bForceToNetwork = true;

                break;
            }
            case PublishType.PRES_PUBLISH_TRIGGER_FEATURE_AVAILABILITY_CHANGED:
            {
                bForceToNetwork = false;
                logger.print("PRES_PUBLISH_TRIGGER_FEATURE_AVAILABILITY_CHANGED");
                break;
            }
            case PublishType.PRES_PUBLISH_TRIGGER_RETRY:
            {
                logger.print("PRES_PUBLISH_TRIGGER_RETRY");
                break;
            }
            default:
            {
                logger.print("Unknown publish trigger from AP");
            }
        }

        if(mGotTriggerFromStack == false){
            // Waiting for RCS stack get ready.
            logger.print("Didn't get trigger from stack yet, discard framework trigger.");
            return ResultCode.ERROR_SERVICE_NOT_AVAILABLE;
        }

        if (mDonotRetryUntilPowerCycle) {
            logger.print("Don't publish until next power cycle");
            return ResultCode.SUCCESS;
        }

        if(!mSimLoaded){
            //need to read some information from SIM to publish
            logger.print("invokePublish cache the trigger since the SIM is not ready");
            mHasCachedTrigger = true;
            return ResultCode.ERROR_SERVICE_NOT_AVAILABLE;
        }

        //the provision status didn't be read from modem yet
        if(!RcsSettingUtils.isEabProvisioned(mContext)) {
            logger.print("invokePublish cache the trigger, not provision yet");
            mHasCachedTrigger = true;
            return ResultCode.ERROR_SERVICE_NOT_AVAILABLE;
        }

        PublishRequest publishRequest = new PublishRequest(
                bForceToNetwork, System.currentTimeMillis());

        requestPublication(publishRequest);

        return ResultCode.SUCCESS;
    }

    public int invokePublish(PresPublishTriggerType val) {
        int ret;

        mGotTriggerFromStack = true;

        // Always send the PUBLISH when we got the trigger from stack.
        // This can avoid any issue when switch the phone between IWLAN and LTE.
        boolean bForceToNetwork = true;
        switch (val.getPublishTrigeerType())
        {
            case PresPublishTriggerType.UCE_PRES_PUBLISH_TRIGGER_ETAG_EXPIRED:
            {
                logger.print("PUBLISH_TRIGGER_ETAG_EXPIRED");
                break;
            }
            case PresPublishTriggerType.UCE_PRES_PUBLISH_TRIGGER_MOVE_TO_LTE_VOPS_DISABLED:
            {
                logger.print("PUBLISH_TRIGGER_MOVE_TO_LTE_VOPS_DISABLED");
                mMovedToLTE = true;
                mVoPSEnabled = false;
                mMovedToIWLAN = false;

                // onImsConnected could came later than this trigger.
                mImsRegistered = true;
                break;
            }
            case PresPublishTriggerType.UCE_PRES_PUBLISH_TRIGGER_MOVE_TO_LTE_VOPS_ENABLED:
            {
                logger.print("PUBLISH_TRIGGER_MOVE_TO_LTE_VOPS_ENABLED");
                mMovedToLTE = true;
                mVoPSEnabled = true;
                mMovedToIWLAN = false;

                // onImsConnected could came later than this trigger.
                mImsRegistered = true;
                break;
            }
            case PresPublishTriggerType.UCE_PRES_PUBLISH_TRIGGER_MOVE_TO_IWLAN:
            {
                logger.print("QRCS_PRES_PUBLISH_TRIGGER_MOVE_TO_IWLAN");
                mMovedToLTE = false;
                mVoPSEnabled = false;
                mMovedToIWLAN = true;

                // onImsConnected could came later than this trigger.
                mImsRegistered = true;
                break;
            }
            case PresPublishTriggerType.UCE_PRES_PUBLISH_TRIGGER_MOVE_TO_EHRPD:
            {
                logger.print("PUBLISH_TRIGGER_MOVE_TO_EHRPD");
                mMovedToLTE = false;
                mVoPSEnabled = false;
                mMovedToIWLAN = false;

                // onImsConnected could came later than this trigger.
                mImsRegistered = true;
                break;
            }
            case PresPublishTriggerType.UCE_PRES_PUBLISH_TRIGGER_MOVE_TO_HSPAPLUS:
            {
                logger.print("PUBLISH_TRIGGER_MOVE_TO_HSPAPLUS");
                mMovedToLTE = false;
                mVoPSEnabled = false;
                mMovedToIWLAN = false;
                break;
            }
            case PresPublishTriggerType.UCE_PRES_PUBLISH_TRIGGER_MOVE_TO_2G:
            {
                logger.print("PUBLISH_TRIGGER_MOVE_TO_2G");
                mMovedToLTE = false;
                mVoPSEnabled = false;
                mMovedToIWLAN = false;
                break;
            }
            case PresPublishTriggerType.UCE_PRES_PUBLISH_TRIGGER_MOVE_TO_3G:
            {
                logger.print("PUBLISH_TRIGGER_MOVE_TO_3G");
                mMovedToLTE = false;
                mVoPSEnabled = false;
                mMovedToIWLAN = false;
                break;
            }
            default:
                logger.print("Unknow Publish Trigger Type");
        }

        if (mDonotRetryUntilPowerCycle) {
            logger.print("Don't publish until next power cycle");
            return ResultCode.SUCCESS;
        }

        if(!mSimLoaded){
            //need to read some information from SIM to publish
            logger.print("invokePublish cache the trigger since the SIM is not ready");
            mHasCachedTrigger = true;
            return ResultCode.ERROR_SERVICE_NOT_AVAILABLE;
        }

        //the provision status didn't be read from modem yet
        if(!RcsSettingUtils.isEabProvisioned(mContext)) {
            logger.print("invokePublish cache the trigger, not provision yet");
            mHasCachedTrigger = true;
            return ResultCode.ERROR_SERVICE_NOT_AVAILABLE;
        }

        PublishRequest publishRequest = new PublishRequest(
                bForceToNetwork, System.currentTimeMillis());

        requestPublication(publishRequest);

        return ResultCode.SUCCESS;
    }

    public class PublishRequest {
        private boolean mForceToNetwork = true;
        private long mCurrentTime = 0;
        private boolean mVolteCapable = false;
        private boolean mVtCapable = false;

        PublishRequest(boolean bForceToNetwork, long currentTime) {
            refreshPublishContent();
            mForceToNetwork = bForceToNetwork;
            mCurrentTime = currentTime;
        }

        public void refreshPublishContent() {
            mVolteCapable = isIPVoiceSupported(mIsVolteAvailable,
                mIsVtAvailable,
                mIsVoWifiAvailable,
                mIsViWifiAvailable);
            mVtCapable = isIPVideoSupported(mIsVolteAvailable,
                mIsVtAvailable,
                mIsVoWifiAvailable,
                mIsViWifiAvailable);
        }

        public boolean getForceToNetwork() {
            return mForceToNetwork;
        }

        public void setForceToNetwork(boolean bForceToNetwork) {
            mForceToNetwork = bForceToNetwork;
        }

        public long getTimestamp() {
            return mCurrentTime;
        }

        public void setTimestamp(long currentTime) {
            mCurrentTime = currentTime;
        }

        public void setVolteCapable(boolean capable) {
            mVolteCapable = capable;
        }

        public void setVtCapable(boolean capable) {
            mVtCapable = capable;
        }

        public boolean getVolteCapable() {
            return mVolteCapable;
        }

        public boolean getVtCapable() {
            return mVtCapable;
        }

        public boolean hasSamePublishContent(PublishRequest request) {
            if(request == null) {
                logger.error("request == null");
                return false;
            }

            return (mVolteCapable == request.getVolteCapable() &&
                    mVtCapable == request.getVtCapable());
        }

        public String toString(){
            return "mForceToNetwork=" + mForceToNetwork +
                    " mCurrentTime=" + mCurrentTime +
                    " mVolteCapable=" + mVolteCapable +
                    " mVtCapable=" + mVtCapable;
        }
    }

    private Handler mMsgHandler = new Handler() {
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
                case MESSAGE_RCS_PUBLISH_REQUEST:
                    logger.debug("handleMessage  msg=RCS_PUBLISH_REQUEST:");

                    PublishRequest publishRequest = (PublishRequest) msg.obj;
                    synchronized(mSyncObj) {
                        mPendingRequest = null;
                    }
                    doPublish(publishRequest);
                break;

                default:
                    logger.debug("handleMessage unknown msg=" + msg.what);
            }
        }
    };

    private void requestPublication(PublishRequest publishRequest) {
        // this is used to avoid the temp false feature change when switched between network.
        if(publishRequest == null) {
            logger.error("Invalid parameter publishRequest == null");
            return;
        }

        long requestThrottle = 2000;
        long currentTime = System.currentTimeMillis();
        synchronized(mSyncObj){
            // There is a PUBLISH will be done soon. Discard it
            if((mPendingRequest != null) && currentTime - mPendingRequest.getTimestamp()
                    <= requestThrottle) {
                logger.print("A publish is pending, update the pending request and discard this one");
                if(publishRequest.getForceToNetwork() && !mPendingRequest.getForceToNetwork()) {
                    mPendingRequest.setForceToNetwork(true);
                }
                mPendingRequest.setTimestamp(publishRequest.getTimestamp());
                return;
            }

            mPendingRequest = publishRequest;
        }

        Message publishMessage = mMsgHandler.obtainMessage(
                MESSAGE_RCS_PUBLISH_REQUEST, mPendingRequest);
        mMsgHandler.sendMessageDelayed(publishMessage, requestThrottle);
    }

    private void doPublish(PublishRequest publishRequest) {

        if(publishRequest == null) {
            logger.error("publishRequest == null");
            return;
        }

        if (mRcsStackAdaptor == null) {
            logger.error("mRcsStackAdaptor == null");
            return;
        }

        if(!mImsRegistered) {
            logger.debug("IMS wasn't registered");
            return;
        }

        // Since we are doing a publish, don't need the retry any more.
        if(mPendingRetry) {
            mPendingRetry = false;
            mAlarmManager.cancel(mRetryAlarmIntent);
        }

        // always publish the latest status.
        synchronized(mSyncObj) {
            publishRequest.refreshPublishContent();
        }

        logger.print("publishRequest=" + publishRequest);
        if(!publishRequest.getForceToNetwork() && isPublishedOrPublishing() &&
                (publishRequest.hasSamePublishContent(mPublishingRequest) ||
                        (mPublishingRequest == null) &&
                        publishRequest.hasSamePublishContent(mPublishedRequest)) &&
                (getPublishState() != PublishState.PUBLISH_STATE_NOT_PUBLISHED)) {
            logger.print("Don't need publish since the capability didn't change publishRequest " +
                    publishRequest + " getPublishState()=" + getPublishState());
            return;
        }

        // Don't publish too frequently. Checking the throttle timer.
        if(isPublishedOrPublishing()) {
            if(mPendingRetry) {
                logger.print("Pending a retry");
                return;
            }

            long publishThreshold = RcsSettingUtils.getPublishThrottle(mContext);
            long passed = publishThreshold;
            if(mPublishingRequest != null) {
                passed = System.currentTimeMillis() - mPublishingRequest.getTimestamp();
            } else if(mPublishedRequest != null) {
                passed = System.currentTimeMillis() - mPublishedRequest.getTimestamp();
            }
            passed = passed>=0?passed:publishThreshold;

            long left = publishThreshold - passed;
            left = left>120000?120000:left; // Don't throttle more then 2 mintues.
            if(left > 0) {
                // A publish is ongoing or published in 1 minute.
                // Since the new publish has new status. So schedule
                // the publish after the throttle timer.
                scheduleRetryPublish(left);
                return;
            }
        }

        // we need send PUBLISH once even the volte is off when power on the phone.
        // That will tell other phone that it has no volte/vt capability.
        if(!getImsManager().isEnhanced4gLteModeSettingEnabledByUser() &&
                getPublishState() != PublishState.PUBLISH_STATE_NOT_PUBLISHED) {
             // volte was not enabled.
             // or it is turnning off volte. lower layer should unpublish
             reset();
             return;
        }

        TelephonyManager teleMgr = (TelephonyManager) mContext.getSystemService(
                Context.TELEPHONY_SERVICE);
        if(teleMgr ==null) {
             logger.debug("teleMgr=null");
             return;
         }

        RcsPresenceInfo presenceInfo = new RcsPresenceInfo(teleMgr.getLine1Number(),
                RcsPresenceInfo.VolteStatus.VOLTE_UNKNOWN,
                publishRequest.getVolteCapable()?RcsPresenceInfo.ServiceState.ONLINE:
                        RcsPresenceInfo.ServiceState.OFFLINE, null, System.currentTimeMillis(),
                publishRequest.getVtCapable()?RcsPresenceInfo.ServiceState.ONLINE:
                        RcsPresenceInfo.ServiceState.OFFLINE, null,
                        System.currentTimeMillis());

        synchronized(mSyncObj) {
            mPublishingRequest = publishRequest;
            mPublishingRequest.setTimestamp(System.currentTimeMillis());
        }

        int ret = mRcsStackAdaptor.requestPublication(presenceInfo, null);
        if(ret == ResultCode.ERROR_SERVICE_NOT_AVAILABLE){
            mHasCachedTrigger = true;
        }else{
            //reset the cached trigger
            mHasCachedTrigger = false;
        }
    }

    public void handleCmdStatus(PresCmdStatus cmdStatus) {
        super.handleCmdStatus(cmdStatus);
    }

    private PendingIntent mRetryAlarmIntent = null;
    public static final String ACTION_RETRY_PUBLISH_ALARM =
            "com.android.service.ims.presence.retry.publish";
    private AlarmManager mAlarmManager = null;
    private BroadcastReceiver mRetryReceiver = null;
    boolean mCancelRetry = true;
    boolean mPendingRetry = false;

    private void scheduleRetryPublish(long timeSpan) {
        logger.print("timeSpan=" + timeSpan +
                " mPendingRetry=" + mPendingRetry +
                " mCancelRetry=" + mCancelRetry);

        // avoid duplicated retry.
        if(mPendingRetry) {
            logger.debug("There was a retry already");
            return;
        }
        mPendingRetry = true;
        mCancelRetry = false;

        Intent intent = new Intent(ACTION_RETRY_PUBLISH_ALARM);
        intent.setClass(mContext, AlarmBroadcastReceiver.class);
        mRetryAlarmIntent = PendingIntent.getBroadcast(mContext, 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT);

        if(mAlarmManager == null) {
            mAlarmManager = (AlarmManager) mContext.getSystemService(Context.ALARM_SERVICE);
        }

        mAlarmManager.setExact(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                SystemClock.elapsedRealtime() + timeSpan, mRetryAlarmIntent);
    }

    public void retryPublish() {
        logger.print("mCancelRetry=" + mCancelRetry);
        mPendingRetry = false;

        // Need some time to cancel it (1 minute for longest)
        // Don't do it if it was canceled already.
        if(mCancelRetry) {
            return;
        }

        invokePublish(PublishType.PRES_PUBLISH_TRIGGER_RETRY);
    }

    public void handleSipResponse(PresSipResponse pSipResponse) {
        logger.print( "Publish response code = " + pSipResponse.getSipResponseCode()
                +"Publish response reason phrase = " + pSipResponse.getReasonPhrase());

        synchronized(mSyncObj) {
            mPublishedRequest = mPublishingRequest;
            mPublishingRequest = null;
        }
        if(pSipResponse == null){
            logger.debug("handlePublishResponse pSipResponse = null");
            return;
        }
        int sipCode = pSipResponse.getSipResponseCode();

        if(isInConfigList(sipCode, pSipResponse.getReasonPhrase(),
                R.array.config_volte_provision_error_on_publish_response)) {
            logger.print("volte provision error. sipCode=" + sipCode + " phrase=" +
                    pSipResponse.getReasonPhrase());
            setPublishState(PublishState.PUBLISH_STATE_VOLTE_PROVISION_ERROR);
            mDonotRetryUntilPowerCycle = true;

            notifyDm();

            return;
        }

        if(isInConfigList(sipCode, pSipResponse.getReasonPhrase(),
                R.array.config_rcs_provision_error_on_publish_response)) {
            logger.print("rcs provision error.sipCode=" + sipCode + " phrase=" +
                    pSipResponse.getReasonPhrase());
            setPublishState(PublishState.PUBLISH_STATE_RCS_PROVISION_ERROR);
            mDonotRetryUntilPowerCycle = true;

            return;
        }

        switch (sipCode) {
            case 999:
                logger.debug("Publish ignored - No capability change");
                break;
            case 200:
                setPublishState(PublishState.PUBLISH_STATE_200_OK);
                if(mSubscriber != null) {
                    mSubscriber.retryToGetAvailability();
                }
                break;

            case 408:
                setPublishState(PublishState.PUBLISH_STATE_REQUEST_TIMEOUT);
                break;

            default: // Generic Failure
                if ((sipCode < 100) || (sipCode > 699)) {
                    logger.debug("Ignore internal response code, sipCode=" + sipCode);
                    // internal error
                    //  0- PERMANENT ERROR: UI should not retry immediately
                    //  888- TEMP ERROR:  UI Can retry immediately
                    //  999- NO CHANGE: No Publish needs to be sent
                    if(sipCode == 888) {
                        // retry per 2 minutes
                        scheduleRetryPublish(120000);
                    } else {
                        logger.debug("Ignore internal response code, sipCode=" + sipCode);
                    }
                } else {
                    logger.debug( "Generic Failure");
                    setPublishState(PublishState.PUBLISH_STATE_OTHER_ERROR);

                    if ((sipCode>=400) && (sipCode <= 699)) {
                        // 4xx/5xx/6xx, No retry, no impact on subsequent publish
                        logger.debug( "No Retry in OEM");
                    }
                }
                break;
        }

        // Suppose the request ID had been set when IQPresListener_CMDStatus
        Task task = TaskManager.getDefault().getTaskByRequestId(
                pSipResponse.getRequestId());
        if(task != null){
            task.mSipResponseCode = pSipResponse.getSipResponseCode();
            task.mSipReasonPhrase = pSipResponse.getReasonPhrase();
        }

        handleCallback(task, getPublishState(), false);
    }

    private static boolean isTtyEnabled(int mode) {
        return Phone.TTY_MODE_OFF != mode;
    }

    private static int telecomTtyModeToPhoneMode(int telecomMode) {
        switch (telecomMode) {
            case TelecomManager.TTY_MODE_FULL:
                return Phone.TTY_MODE_FULL;
            case TelecomManager.TTY_MODE_VCO:
                return Phone.TTY_MODE_VCO;
            case TelecomManager.TTY_MODE_HCO:
                return Phone.TTY_MODE_HCO;
            case TelecomManager.TTY_MODE_OFF:
            default:
                return Phone.TTY_MODE_OFF;
        }
    }

    public void finish() {
        if (mReceiver != null) {
            mContext.unregisterReceiver(mReceiver);
            mReceiver = null;
        }
    }

    protected void finalize() throws Throwable {
        finish();
    }

    public void onFeatureCapabilityChanged(final int serviceClass,
            final int[] enabledFeatures, final int[] disabledFeatures) {
        logger.debug("onFeatureCapabilityChanged serviceClass="+serviceClass
                +", enabledFeatures="+Arrays.toString(enabledFeatures)
                +", disabledFeatures="+Arrays.toString(disabledFeatures));

        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                onFeatureCapabilityChangedInternal(serviceClass,
                        enabledFeatures, disabledFeatures);
            }
        }, "onFeatureCapabilityChangedInternal thread");

        thread.start();
    }

    synchronized private void onFeatureCapabilityChangedInternal(int serviceClass,
            int[] enabledFeatures, int[] disabledFeatures) {
        if (serviceClass == ImsServiceClass.MMTEL) {
            boolean oldIsVolteAvailable = mIsVolteAvailable;
            boolean oldIsVtAvailable = mIsVtAvailable;
            boolean oldIsVoWifiAvailable = mIsVoWifiAvailable;
            boolean oldIsViWifiAvailable = mIsViWifiAvailable;

            if(enabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE] ==
                    ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE) {
                mIsVolteAvailable = true;
            } else if(enabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE] ==
                    ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN) {
                mIsVolteAvailable = false;
            } else {
                logger.print("invalid value for FEATURE_TYPE_VOICE_OVER_LTE");
            }

            if(enabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI] ==
                    ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI) {
                mIsVoWifiAvailable = true;
            } else if(enabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI] ==
                    ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN) {
                mIsVoWifiAvailable = false;
            } else {
                logger.print("invalid value for FEATURE_TYPE_VOICE_OVER_WIFI");
            }

            if (enabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE] ==
                    ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE) {
                mIsVtAvailable = true;
            } else if (enabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE] ==
                    ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN) {
                mIsVtAvailable = false;
            } else {
                logger.print("invalid value for FEATURE_TYPE_VIDEO_OVER_LTE");
            }

            if(enabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI] ==
                    ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI) {
                mIsViWifiAvailable = true;
            } else if(enabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI] ==
                    ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN) {
                mIsViWifiAvailable = false;
            } else {
                logger.print("invalid value for FEATURE_TYPE_VIDEO_OVER_WIFI");
            }

            logger.print("mIsVolteAvailable=" + mIsVolteAvailable +
                    " mIsVoWifiAvailable=" + mIsVoWifiAvailable +
                    " mIsVtAvailable=" + mIsVtAvailable +
                    " mIsViWifiAvailable=" + mIsViWifiAvailable +
                    " oldIsVolteAvailable=" + oldIsVolteAvailable +
                    " oldIsVoWifiAvailable=" + oldIsVoWifiAvailable +
                    " oldIsVtAvailable=" + oldIsVtAvailable +
                    " oldIsViWifiAvailable=" + oldIsViWifiAvailable);

            if(oldIsVolteAvailable != mIsVolteAvailable ||
                    oldIsVtAvailable != mIsVtAvailable ||
                    oldIsVoWifiAvailable != mIsVoWifiAvailable ||
                    oldIsViWifiAvailable != mIsViWifiAvailable) {
                if(mGotTriggerFromStack) {
                    if((Settings.Global.getInt(mContext.getContentResolver(),
                            Settings.Global.AIRPLANE_MODE_ON, 0) != 0) && !mIsVoWifiAvailable &&
                            !mIsViWifiAvailable) {
                        logger.print("Airplane mode was on and no vowifi and viwifi." +
                            " Don't need publish. Stack will unpublish");
                        return;
                    }

                    if(isOnIWLAN()) {
                        // will check duplicated PUBLISH in requestPublication by invokePublish
                        invokePublish(PresencePublication.PublishType.
                                PRES_PUBLISH_TRIGGER_FEATURE_AVAILABILITY_CHANGED);
                    }
                } else {
                    mHasCachedTrigger = true;
                }
            }
        }
    }

    private boolean isOnLTE() {
        TelephonyManager teleMgr = (TelephonyManager) mContext.getSystemService(
                Context.TELEPHONY_SERVICE);
            int networkType = teleMgr.getDataNetworkType();
            logger.debug("mMovedToLTE=" + mMovedToLTE + " networkType=" + networkType);

            // Had reported LTE by trigger and still have DATA.
            return mMovedToLTE && (networkType != TelephonyManager.NETWORK_TYPE_UNKNOWN);
    }

    private boolean isOnIWLAN() {
        TelephonyManager teleMgr = (TelephonyManager) mContext.getSystemService(
                Context.TELEPHONY_SERVICE);
            int networkType = teleMgr.getDataNetworkType();
            logger.debug("mMovedToIWLAN=" + mMovedToIWLAN + " networkType=" + networkType);

            // Had reported IWLAN by trigger and still have DATA.
            return mMovedToIWLAN && (networkType != TelephonyManager.NETWORK_TYPE_UNKNOWN);
    }


    private ImsManager getImsManager() {
        return ImsManager.getInstance(mContext, SubscriptionManager.getDefaultVoicePhoneId());
    }
}
