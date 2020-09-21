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

package com.android.ims;

import java.util.List;
import java.util.ArrayList;
import android.content.Intent;
import android.os.RemoteException;
import android.util.Log;

import com.android.ims.internal.IRcsPresence;
import com.android.ims.RcsManager.ResultCode;

/**
 *
 * @hide
 */
public class RcsPresence {
    static final String TAG = "RcsPresence";
    private boolean DBG = true;
    private IRcsPresence mIRcsPresence = null;

    /**
     * Key to retrieve the RcsPresenceInfo list from intent ACTION_PRESENCE_CHANGED
     * The RcsPresenceInfo list can be got by the following function call:
     * ArrayList<RcsPresenceInfo> rcsPresenceInfoList = intent.getParcelableArrayListExtra(
     *           RcsPresence.EXTRA_PRESENCE_INFO_LIST);
     *
     * @see RcsPresenceInfo
     */
    public static final String EXTRA_PRESENCE_INFO_LIST = "presence_info_list";

    /**
     * Key to retrieve the subscription ID. This is for muliti-SIM implementation.
     */
    public static final String EXTRA_SUBID = "android:subid";

    /**
     * The intent will be broadcasted when the presence changed.
     * It happens on the following cases:
     *   1. When the phone gets a NOTIFY from network.
     *   2. When the phone gets some SUBSCRIBE error from network for some case (such as 404).
     * It takes two extra parameters:
     *   1. EXTRA_PRESENCE_INFO_LIST
     *     need to get it by the following statement:
     *     ArrayList<RcsPresenceInfo> rcsPresenceInfoList = intent.getParcelableArrayListExtra(
     *             RcsPresence.EXTRA_PRESENCE_INFO_LIST);
     *   2. EXTRA_SUBID
     *
     * @see RcsPresenceInfO
     * @see EXTRA_PRESENCE_INFO_LIST
     * @see EXTRA_SUBID
     */
    public static final String ACTION_PRESENCE_CHANGED = "com.android.ims.ACTION_PRESENCE_CHANGED";

    /**
     * Key to retrieve "publish_state" for
     * intent ACTION_PUBLISH_STATE_CHANGED.
     *
     * @See PublishState.
     * @see ACTION_PUBLISH_STATE_CHANGED
     */
    public static final String EXTRA_PUBLISH_STATE = "publish_state";

    /**
     * The intent will be broadcasted when latest publish status changed
     * It takes two extra parameters:
     * 1. EXTRA_PUBLISH_STATE
     * 2. EXTRA_SUBID
     *
     * @see #EXTRA_PUBLISH_STATE
     * @see #EXTRA_SUBID
     * @see PublishState
     */
    public static final String ACTION_PUBLISH_STATE_CHANGED =
            "com.android.ims.ACTION_PUBLISH_STATUS_CHANGED";

    /**
     *  The last publish state
     */
    public static class PublishState {
        /**
         * The phone is PUBLISH_STATE_200_OK when
         * the response of the last publish is "200 OK"
         */
        public static final int PUBLISH_STATE_200_OK = 0;

        /**
         * The phone didn't publish after power on.
         * the phone didn't get any publish response yet.
         */
        public static final int PUBLISH_STATE_NOT_PUBLISHED = 1;

        /**
         * The phone is PUBLISH_STATE_VOLTE_PROVISION_ERROR when the response is one of items
          * in config_volte_provision_error_on_publish_response for PUBLISH or
          * in config_volte_provision_error_on_subscribe_response for SUBSCRIBE.
         */
        public static final int PUBLISH_STATE_VOLTE_PROVISION_ERROR = 2;

        /**
         * The phone is PUBLISH_STATE_RCS_PROVISION_ERROR when the response is one of items
          * in config_rcs_provision_error_on_publish_response for PUBLISH or
          * in config_rcs_provision_error_on_subscribe_response for SUBSCRIBE.
         */
        public static final int PUBLISH_STATE_RCS_PROVISION_ERROR = 3;

        /**
         * The phone is PUBLISH_STATE_REQUEST_TIMEOUT when
         * The response of the last publish is "408 Request Timeout".
         */
        public static final int PUBLISH_STATE_REQUEST_TIMEOUT = 4;

        /**
         * The phone is PUBLISH_STATE_OTHER_ERROR when
         * the response of the last publish is other temp error. Such as
         * 503 Service Unavailable
         * Device shall retry with exponential back-off
         *
         * 423 Interval Too Short. Requested expiry interval too short and server rejects it
         * Device shall re-attempt subscription after changing the expiration interval in
         * the Expires header field to be equal to or greater than the expiration interval
         * within the Min-Expires header field of the 423 response
         *
         * ...
         */
        public static final int PUBLISH_STATE_OTHER_ERROR = 5;
    };

    /**
     * Constructor.
     * @param presenceService the IRcsPresence which get by RcsManager.getRcsPresenceInterface.
     *
     * @see RcsManager#getRcsPresenceInterface
     */
    public RcsPresence(IRcsPresence presenceService) {
        if (DBG) Log.d(TAG, "IRcsPresence creates");

        mIRcsPresence = presenceService;
    }

    /**
     * Send the request to the server to get the capability.
     *   1. If the presence service sent the request to network successfully
     * then it will return the request ID (>0). It will not wait for the response from
     * network. The response from network will be returned by callback onSuccess() or onError().
     *   2. If the presence service failed to send the request to network then it will return error
     *  code which is defined by RcsManager.ResultCode (<0).
     *   3. If the network returns "200 OK" for a request then the listener.onSuccess() will be
     * called by presence service.
     *   4. If the network resturns "404" for a single target number then it means the target
     * number is not VoLte capable, so the listener.onSuccess() will be called and intent
     * ACTION_PRESENCE_CHANGED will be broadcasted by presence service.
     *   5. If the network returns other error then the listener.onError() will be called by
     * presence service.
     *   6. If the network returns "200 OK" then we can expect the presence service receives notify
     * from network. If the presence service receives notify then it will broadcast the
     * intent ACTION_PRESENCE_CHANGED. If the notify state is "terminated" then the
     * listener.onFinish() will be called by presence service as well.
     *   7. If the presence service doesn't get response after "Subscribe Expiration + T1" then the
     * listener.onTimeout() will be called by presence service.
     *
     * @param contactsNumber the contact number list which will be requested.
     * @param listener the IRcsPresenceListener which will return the status and response.
     *
     * @return the request ID if it is >0. Or it is RcsManager.ResultCode for error.
     *
     * @see IRcsPresenceListener
     * @see RcsManager.ResultCode
     */
    public int requestCapability(List<String> contactsNumber,
            IRcsPresenceListener listener) throws RcsException {
        if (DBG) Log.d(TAG, "call requestCapability, contactsNumber=" + contactsNumber);
        int ret = ResultCode.ERROR_SERVICE_NOT_AVAILABLE;

        try {
            ret = mIRcsPresence.requestCapability(contactsNumber, listener);
        }  catch (RemoteException e) {
            throw new RcsException("requestCapability", e,
                    ResultCode.ERROR_SERVICE_NOT_AVAILABLE);
        }

        if (DBG) Log.d(TAG, "requestCapability ret =" + ret);
        return ret;
    }

    /**
     * Send the request to the server to get the availability.
     *   1. If the presence service sent the request to network successfully then it will return
     * the request ID (>0).
     *   2. If the presence serive failed to send the request to network then it will return error
     * code which is defined by RcsManager.ResultCode (<0).
     *   3. If the network returns "200 OK" for a request then the listener.onSuccess() will be
     * called by presence service.
     *   4. If the network resturns "404" then it means the target number is not VoLte capable,
     * so the listener.onSuccess() will be called and intent ACTION_PRESENCE_CHANGED will be
     * broadcasted by presence service.
     *   5. If the network returns other error code then the listener.onError() will be called by
     * presence service.
     *   6. If the network returns "200 OK" then we can expect the presence service receives notify
     * from network. If the presence service receives notify then it will broadcast the intent
     * ACTION_PRESENCE_CHANGED. If the notify state is "terminated" then the listener.onFinish()
     * will be called by presence service as well.
     *   7. If the presence service doesn't get response after "Subscribe Expiration + T1" then it
     * will call listener.onTimeout().
     *
     * @param contactNumber the contact which will request the availability.
     *     Only support phone number at present.
     * @param listener the IRcsPresenceListener to get the response.
     *
     * @return the request ID if it is >0. Or it is RcsManager.ResultCode for error.
     *
     * @see IRcsPresenceListener
     * @see RcsManager.ResultCode
     * @see RcsPresence.ACTION_PRESENCE_CHANGED
     */
    public int requestAvailability(String contactNumber, IRcsPresenceListener listener)
             throws RcsException {
        if (DBG) Log.d(TAG, "call requestAvailability, contactNumber=" + contactNumber);
        int ret = ResultCode.ERROR_SERVICE_NOT_AVAILABLE;

        try {
            ret = mIRcsPresence.requestAvailability(contactNumber, listener);
        }  catch (RemoteException e) {
            throw new RcsException("requestAvailability", e,
                    ResultCode.ERROR_SERVICE_NOT_AVAILABLE);
        }

        if (DBG) Log.d(TAG, "requestAvailability ret =" + ret);
        return ret;
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
     * @see RcsPresence.ACTION_PRESENCE_CHANGED
     * @see ResultCode.SUBSCRIBE_TOO_FREQUENTLY
     */
    public int requestAvailabilityNoThrottle(String contactNumber, IRcsPresenceListener listener)
             throws RcsException {
        if (DBG) Log.d(TAG, "call requestAvailabilityNoThrottle, contactNumber=" + contactNumber);
        int ret = ResultCode.ERROR_SERVICE_NOT_AVAILABLE;

        try {
            ret = mIRcsPresence.requestAvailabilityNoThrottle(contactNumber, listener);
        }  catch (RemoteException e) {
            throw new RcsException("requestAvailabilityNoThrottle", e,
                    ResultCode.ERROR_SERVICE_NOT_AVAILABLE);
        }

        if (DBG) Log.d(TAG, "requestAvailabilityNoThrottle ret =" + ret);
        return ret;
    }

    /**
     * Get the latest publish state.
     *
     * @see PublishState
     */
    public int getPublishState() throws RcsException {
        int ret = PublishState.PUBLISH_STATE_NOT_PUBLISHED;
        try {
            ret = mIRcsPresence.getPublishState();
        }  catch (RemoteException e) {
            throw new RcsException("getPublishState", e,
                    ResultCode.ERROR_SERVICE_NOT_AVAILABLE);
        }

        if (DBG) Log.d(TAG, "getPublishState ret =" + ret);
        return ret;
    }
}

