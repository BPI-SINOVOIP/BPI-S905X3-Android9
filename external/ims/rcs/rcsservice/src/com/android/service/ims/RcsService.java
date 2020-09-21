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

package com.android.service.ims;

import java.util.List;

import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.os.RemoteException;
import android.content.Context;
import android.app.Service;
import android.os.ServiceManager;
import android.os.Handler;
import android.database.ContentObserver;
import android.content.BroadcastReceiver;
import android.provider.Settings;
import android.net.ConnectivityManager;
import com.android.ims.ImsManager;
import com.android.ims.ImsConnectionStateListener;
import com.android.ims.ImsException;
import android.telephony.SubscriptionManager;
import android.telephony.ims.ImsReasonInfo;

import com.android.ims.RcsManager.ResultCode;
import com.android.ims.internal.IRcsService;
import com.android.ims.IRcsPresenceListener;
import com.android.ims.internal.IRcsPresence;

import com.android.ims.internal.Logger;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.TelephonyIntents;

import com.android.service.ims.presence.PresencePublication;
import com.android.service.ims.presence.PresenceSubscriber;

public class RcsService extends Service{
    /**
     * The logger
     */
    private Logger logger = Logger.getLogger(this.getClass().getName());

    private RcsStackAdaptor mRcsStackAdaptor = null;
    private PresencePublication mPublication = null;
    private PresenceSubscriber mSubscriber = null;

    private BroadcastReceiver mReceiver = null;

    @Override
    public void onCreate() {
        super.onCreate();

        logger.debug("RcsService onCreate");

        mRcsStackAdaptor = RcsStackAdaptor.getInstance(this);

        mPublication = new PresencePublication(mRcsStackAdaptor, this);
        mRcsStackAdaptor.getListener().setPresencePublication(mPublication);

        mSubscriber = new PresenceSubscriber(mRcsStackAdaptor, this);
        mRcsStackAdaptor.getListener().setPresenceSubscriber(mSubscriber);
        mPublication.setSubscriber(mSubscriber);

        ConnectivityManager cm = ConnectivityManager.from(this);
        if (cm != null) {
            boolean enabled = Settings.Global.getInt(getContentResolver(),
                    Settings.Global.MOBILE_DATA, 1) == 1;
            logger.debug("Mobile data enabled status: " + (enabled ? "ON" : "OFF"));

            onMobileDataEnabled(enabled);
        }

        // TODO: support MSIM
        ServiceManager.addService("rcs", mBinder);

        mObserver = new MobileDataContentObserver();
        getContentResolver().registerContentObserver(
                Settings.Global.getUriFor(Settings.Global.MOBILE_DATA),
                false, mObserver);

        mSiminfoSettingObserver = new SimInfoContentObserver();
        getContentResolver().registerContentObserver(
                SubscriptionManager.CONTENT_URI, false, mSiminfoSettingObserver);

        mReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                logger.print("onReceive intent=" + intent);
                if(ImsManager.ACTION_IMS_SERVICE_UP.equalsIgnoreCase(
                        intent.getAction())){
                    handleImsServiceUp();
                } else if(ImsManager.ACTION_IMS_SERVICE_DOWN.equalsIgnoreCase(
                        intent.getAction())){
                    handleImsServiceDown();
                } else if(TelephonyIntents.ACTION_SIM_STATE_CHANGED.equalsIgnoreCase(
                        intent.getAction())) {
                    String stateExtra = intent.getStringExtra(
                            IccCardConstants.INTENT_KEY_ICC_STATE);
                    handleSimStateChanged(stateExtra);
                }
            }
        };

        IntentFilter statusFilter = new IntentFilter();
        statusFilter.addAction(ImsManager.ACTION_IMS_SERVICE_UP);
        statusFilter.addAction(ImsManager.ACTION_IMS_SERVICE_DOWN);
        statusFilter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        registerReceiver(mReceiver, statusFilter);
    }

    public void handleImsServiceUp() {
        if(mPublication != null) {
            mPublication.handleImsServiceUp();
        }

        registerImsConnectionStateListener();
    }

    public void handleImsServiceDown() {
        if(mPublication != null) {
            mPublication.handleImsServiceDown();
        }
    }

    public void handleSimStateChanged(String state) {

        if(IccCardConstants.INTENT_VALUE_ICC_LOADED.equalsIgnoreCase(state)) {
            // ImsManager depends on a loaded SIM to get the default Voice Registration.
            registerImsConnectionStateListener();
        }
    }


    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        logger.debug("RcsService onStartCommand");

        return super.onStartCommand(intent, flags, startId);
    }

    /**
      * Cleans up when the service is destroyed
      */
    @Override
    public void onDestroy() {
        getContentResolver().unregisterContentObserver(mObserver);
        getContentResolver().unregisterContentObserver(mSiminfoSettingObserver);
        if (mReceiver != null) {
            unregisterReceiver(mReceiver);
            mReceiver = null;
        }

        mRcsStackAdaptor.finish();
        mPublication.finish();
        mPublication = null;
        mSubscriber = null;

        logger.debug("RcsService onDestroy");
        super.onDestroy();
    }

    public PresencePublication getPublication() {
        return mPublication;
    }

    public PresenceSubscriber getPresenceSubscriber(){
        return mSubscriber;
    }

    IRcsPresence.Stub mIRcsPresenceImpl = new IRcsPresence.Stub(){
        /**
         * Asyncrhonously request the latest capability for a given contact list.
         * The result will be saved to DB directly if the contactNumber can be found in DB.
         * And then send intent com.android.ims.presence.CAPABILITY_STATE_CHANGED to notify it.
         * @param contactsNumber the contact list which will request capability.
         *                       Currently only support phone number.
         * @param listener the listener to get the response.
         * @return the resultCode which is defined by ResultCode.
         * @note framework uses only.
         * @hide
         */
        public int requestCapability(List<String> contactsNumber,
            IRcsPresenceListener listener){
            logger.debug("calling requestCapability");
            if(mSubscriber == null){
                logger.debug("requestCapability, mPresenceSubscriber == null");
                return ResultCode.ERROR_SERVICE_NOT_AVAILABLE;
            }

            return mSubscriber.requestCapability(contactsNumber, listener);
         }

        /**
         * Asyncrhonously request the latest presence for a given contact.
         * The result will be saved to DB directly if it can be found in DB. And then send intent
         * com.android.ims.presence.AVAILABILITY_STATE_CHANGED to notify it.
         * @param contactNumber the contact which will request available.
         *                       Currently only support phone number.
         * @param listener the listener to get the response.
         * @return the resultCode which is defined by ResultCode.
         * @note framework uses only.
         * @hide
         */
        public int requestAvailability(String contactNumber, IRcsPresenceListener listener){
            if(mSubscriber == null){
                logger.error("requestAvailability, mPresenceSubscriber is null");
                return ResultCode.ERROR_SERVICE_NOT_AVAILABLE;
            }

            // check availability cache (in RAM).
            return mSubscriber.requestAvailability(contactNumber, listener, false);
        }

        /**
         * Same as requestAvailability. but requestAvailability will consider throttle to avoid too
         * fast call. Which means it will not send the request to network in next 60s for the same
         * request.
         * The error code SUBSCRIBE_TOO_FREQUENTLY will be returned under the case.
         * But for this funcation it will always send the request to network.
         *
         * @see IRcsPresenceListener
         * @see RcsManager.ResultCode
         * @see ResultCode.SUBSCRIBE_TOO_FREQUENTLY
         */
        public int requestAvailabilityNoThrottle(String contactNumber,
                IRcsPresenceListener listener) {
            if(mSubscriber == null){
                logger.error("requestAvailabilityNoThrottle, mPresenceSubscriber is null");
                return ResultCode.ERROR_SERVICE_NOT_AVAILABLE;
            }

            // check availability cache (in RAM).
            return mSubscriber.requestAvailability(contactNumber, listener, true);
        }

        public int getPublishState() throws RemoteException {
            return mPublication.getPublishState();
        }
    };

    @Override
    public IBinder onBind(Intent arg0) {
        return mBinder;
    }

    /**
     * Receives notifications when Mobile data is enabled or disabled.
     */
    private class MobileDataContentObserver extends ContentObserver {
        public MobileDataContentObserver() {
            super(new Handler());
        }

        @Override
        public void onChange(final boolean selfChange) {
            boolean enabled = Settings.Global.getInt(getContentResolver(),
                    Settings.Global.MOBILE_DATA, 1) == 1;
            logger.debug("Mobile data enabled status: " + (enabled ? "ON" : "OFF"));
            onMobileDataEnabled(enabled);
        }
    }

    /** Observer to get notified when Mobile data enabled status changes */
    private MobileDataContentObserver mObserver;

    private void onMobileDataEnabled(final boolean enabled) {
        logger.debug("Enter onMobileDataEnabled: " + enabled);
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                try{
                    if(mPublication != null){
                        mPublication.onMobileDataChanged(enabled);
                        return;
                    }
                }catch(Exception e){
                    logger.error("Exception onMobileDataEnabled:", e);
                }
            }
        }, "onMobileDataEnabled thread");

        thread.start();
    }


    private SimInfoContentObserver mSiminfoSettingObserver;

    /**
     * Receives notifications when the TelephonyProvider is changed.
     */
    private class SimInfoContentObserver extends ContentObserver {
        public SimInfoContentObserver() {
            super(new Handler());
        }

        @Override
        public void onChange(final boolean selfChange) {
            ImsManager imsManager = ImsManager.getInstance(RcsService.this,
                    SubscriptionManager.getDefaultVoicePhoneId());
            if (imsManager == null) {
                return;
            }
            boolean enabled = imsManager.isVtEnabledByUser();
             logger.debug("vt enabled status: " + (enabled ? "ON" : "OFF"));

            onVtEnabled(enabled);
        }
    }

    private void onVtEnabled(boolean enabled) {
        if(mPublication != null){
            mPublication.onVtEnabled(enabled);
        }
    }

    private final IRcsService.Stub mBinder = new IRcsService.Stub() {
        /**
         * return true if the rcs service is ready for use.
         */
        public boolean isRcsServiceAvailable(){
            logger.debug("calling isRcsServiceAvailable");
            if(mRcsStackAdaptor == null){
                return false;
            }

            return mRcsStackAdaptor.isImsEnableState();
        }

        /**
         * Gets the presence interface.
         *
         * @see IRcsPresence
         */
        public IRcsPresence getRcsPresenceInterface(){
            return mIRcsPresenceImpl;
        }
    };

    void registerImsConnectionStateListener() {
        try {
            ImsManager imsManager = ImsManager.getInstance(this,
                    SubscriptionManager.getDefaultVoicePhoneId());
            if (imsManager != null) {
                imsManager.addRegistrationListener(mImsConnectionStateListener);
            }
        } catch (ImsException e) {
            logger.error("addRegistrationListener exception=", e);
        }
    }

    private ImsConnectionStateListener mImsConnectionStateListener =
        new ImsConnectionStateListener() {
            @Override
            public void onImsConnected(int imsRadioTech) {
                logger.debug("onImsConnected imsRadioTech=" + imsRadioTech);
                if(mRcsStackAdaptor != null) {
                    mRcsStackAdaptor.checkSubService();
                }

                if(mPublication != null) {
                    mPublication.onImsConnected();
                }
            }

            @Override
            public void onImsDisconnected(ImsReasonInfo imsReasonInfo) {
                logger.debug("onImsDisconnected");
                if(mPublication != null) {
                    mPublication.onImsDisconnected();
                }
            }

            @Override
            public void onFeatureCapabilityChanged(final int serviceClass,
                    final int[] enabledFeatures, final int[] disabledFeatures) {
                logger.debug("onFeatureCapabilityChanged");
                if(mPublication != null) {
                    mPublication.onFeatureCapabilityChanged(serviceClass, enabledFeatures, disabledFeatures);
                }
            }
        };
}

