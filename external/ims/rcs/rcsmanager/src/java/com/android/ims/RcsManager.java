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

import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.os.IBinder.DeathRecipient;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.telephony.Rlog;

import com.android.ims.internal.IRcsService;
import com.android.ims.internal.IRcsPresence;

import java.util.HashMap;

/**
 * Provides APIs for Rcs services, currently it supports presence only.
 * This class is the starting point for any RCS actions.
 * You can acquire an instance of it with {@link #getInstance getInstance()}.
 *
 * @hide
 */
public class RcsManager {
    /**
     * For accessing the RCS related service.
     * Internal use only.
     *
     * @hide
     */
    private static final String RCS_SERVICE = "rcs";

    /**
     * Part of the ACTION_RCS_SERVICE_AVAILABLE and ACTION_RCS_SERVICE_UNAVAILABLE intents.
     * A long value; the subId corresponding to the RCS service. For MSIM implementation.
     *
     * @see #ACTION_RCS_SERVICE_AVAILABLE
     * @see #ACTION_RCS_SERVICE_UNAVAILABLE
     */
    public static final String EXTRA_SUBID = "android:subid";

    /**
     * Action to broadcast when RcsService is available.
     *
     * @see #EXTRA_SUBID
     */
    public static final String ACTION_RCS_SERVICE_AVAILABLE =
            "com.android.ims.ACTION_RCS_SERVICE_AVAILABLE";

    /**
     * Action to broadcast when RcsService is unavailable (such as ims is not registered).
     *
     * @see #EXTRA_SUBID
     */
    public static final String ACTION_RCS_SERVICE_UNAVAILABLE =
            "com.android.ims.ACTION_RCS_SERVICE_UNAVAILABLE";

    /**
     * Action to broadcast when RcsService is died.
     * The caller can listen to the intent to clean the pending request.
     *
     * It takes the extra parameter subid as well since it depends on OEM implementation for
     * RcsService. It will not send broadcast for ACTION_RCS_SERVICE_UNAVAILABLE under the case.
     *
     * @see #EXTRA_SUBID
     */
    public static final String ACTION_RCS_SERVICE_DIED =
            "com.android.ims.ACTION_RCS_SERVICE_DIED";

    public static class ResultCode {
        /**
         * The code is used when the request is success.
         */
        public static final int SUCCESS =0;

        /**
         * Return this code if the service doesn't be enabled on the phone.
         * As per the requirement the feature can be enabled/disabled by DM.
         */
        public static final int ERROR_SERVICE_NOT_ENABLED = -1;

        /**
         * Return this code if the service didn't publish yet.
         */
        public static final int ERROR_SERVICE_NOT_PUBLISHED = -2;

        /**
         * The service is not available, for example it is 1x only
         */
        public static final int ERROR_SERVICE_NOT_AVAILABLE = -3;

        /**
         *  SUBSCRIBE Error base
         */
        public static final int SUBSCRIBER_ERROR_CODE_START = ERROR_SERVICE_NOT_AVAILABLE;

        /**
         * Temporary error and need retry later.
         * such as:
         * 503 Service Unavailable
         * Device shall retry with exponential back-off
         *
         * 408 Request Timeout
         * Device shall retry with exponential back-off
         *
         * 423 Interval Too Short. Requested expiry interval too short and server rejects it
         * Device shall re-attempt subscription after changing the expiration interval in
         * the Expires header field to be equal to or greater than the expiration interval
         * within the Min-Expires header field of the 423 response
         */
        public static final int SUBSCRIBE_TEMPORARY_ERROR = SUBSCRIBER_ERROR_CODE_START - 1;

        /**
         * receives 403 (reason="User Not Registered").
         * Re-Register to IMS then retry the single resource subscription if capability polling.
         * availability fetch: no retry.
         */
         public static final int SUBSCRIBE_NOT_REGISTERED = SUBSCRIBER_ERROR_CODE_START - 2;

        /**
         * Responding for 403 - not authorized (Requestor)
         * No retry.
         */
        public static final int SUBSCRIBE_NOT_AUTHORIZED_FOR_PRESENCE =
                SUBSCRIBER_ERROR_CODE_START - 3;

        /**
         * Responding for "403 Forbidden" or "403"
         * Handle it as same as 404 Not found.
         * No retry.
         */
        public static final int SUBSCRIBE_FORBIDDEN = SUBSCRIBER_ERROR_CODE_START - 4;

        /**
         * Responding for 404 (target number)
         * No retry.
         */
        public static final int SUBSCRIBE_NOT_FOUND = SUBSCRIBER_ERROR_CODE_START - 5;

        /**
         *  Responding for 413 - Too Large. Top app need shrink the size
         *  of request contact list and resend the request
         */
        public static final int SUBSCRIBE_TOO_LARGE = SUBSCRIBER_ERROR_CODE_START - 6;

        /**
         * All subscribe errors not covered by specific errors
         * Other 4xx/5xx/6xx
         *
         * Device shall not retry
         */
        public static final int SUBSCRIBE_GENIRIC_FAILURE = SUBSCRIBER_ERROR_CODE_START - 7;

        /**
         * Invalid parameter - The caller should check the parameter.
         */
        public static final int SUBSCRIBE_INVALID_PARAM = SUBSCRIBER_ERROR_CODE_START - 8;

        /**
         * Fetch error - The RCS statck failed to fetch the presence information.
         */
        public static final int SUBSCRIBE_FETCH_ERROR = SUBSCRIBER_ERROR_CODE_START - 9;

        /**
         * Request timeout - The RCS statck returns timeout error.
         */
        public static final int SUBSCRIBE_REQUEST_TIMEOUT = SUBSCRIBER_ERROR_CODE_START - 10;

        /**
         * Insufficient memory - The RCS statck returns the insufficient memory error.
         */
        public static final int SUBSCRIBE_INSUFFICIENT_MEMORY = SUBSCRIBER_ERROR_CODE_START - 11;

        /**
         * Lost network error - The RCS statck returns the lost network error.
         */
        public static final int SUBSCRIBE_LOST_NETWORK = SUBSCRIBER_ERROR_CODE_START - 12;

        /**
         * Not supported error - The RCS statck returns the not supported error.
         */
        public static final int SUBSCRIBE_NOT_SUPPORTED = SUBSCRIBER_ERROR_CODE_START - 13;

        /**
         * Generic error - RCS Presence stack returns generic error
         */
        public static final int SUBSCRIBE_GENERIC = SUBSCRIBER_ERROR_CODE_START - 14;

        /**
         * There is a request for the same number in queue.
         */
        public static final int SUBSCRIBE_ALREADY_IN_QUEUE = SUBSCRIBER_ERROR_CODE_START - 16;

        /**
         * Request too frequently.
         */
        public static final int SUBSCRIBE_TOO_FREQUENTLY = SUBSCRIBER_ERROR_CODE_START - 17;

        /**
         *  The last Subscriber error code
         */
        public static final int SUBSCRIBER_ERROR_CODE_END = SUBSCRIBER_ERROR_CODE_START - 17;
    };

    private static final String TAG = "RcsManager";
    private static final boolean DBG = true;

    private static HashMap<Integer, RcsManager> sRcsManagerInstances =
            new HashMap<Integer, RcsManager>();

    private Context mContext;
    private int mSubId;
    private IRcsService mRcsService = null;
    private RcsServiceDeathRecipient mDeathRecipient = new RcsServiceDeathRecipient();

    // Interface for presence
    // TODO: Could add other RCS service such RcsChat, RcsFt later.
    private RcsPresence  mRcsPresence = null;

    /**
     * Gets a manager instance.
     *
     * @param context application context for creating the manager object
     * @param subId the subscription ID for the RCS Service
     * @return the manager instance corresponding to the subId
     */
    public static RcsManager getInstance(Context context, int subId) {
        synchronized (sRcsManagerInstances) {
            if (sRcsManagerInstances.containsKey(subId)){
                return sRcsManagerInstances.get(subId);
            }

            RcsManager mgr = new RcsManager(context, subId);
            sRcsManagerInstances.put(subId, mgr);

            return mgr;
        }
    }

    private RcsManager(Context context, int subId) {
        mContext = context;
        mSubId = subId;
        createRcsService(true);
    }

    /**
     * return true if the rcs service is ready for use.
     */
    public boolean isRcsServiceAvailable() {
        if (DBG) Rlog.d(TAG, "call isRcsServiceAvailable ...");

        boolean ret = false;

        try {
            checkAndThrowExceptionIfServiceUnavailable();

            ret = mRcsService.isRcsServiceAvailable();
        }  catch (RemoteException e) {
            // return false under the case.
            Rlog.e(TAG, "isRcsServiceAvailable RemoteException", e);
        }catch (RcsException e){
            // return false under the case.
            Rlog.e(TAG, "isRcsServiceAvailable RcsException", e);
        }

        if (DBG) Rlog.d(TAG, "isRcsServiceAvailable ret =" + ret);
        return ret;
    }

    /**
     * Gets the presence interface
     *
     * @return the RcsPresence instance.
     * @throws  if getting the RcsPresence interface results in an error.
     */
    public RcsPresence getRcsPresenceInterface() throws RcsException {

        if (mRcsPresence == null) {
            checkAndThrowExceptionIfServiceUnavailable();

            try {
                IRcsPresence rcsPresence = mRcsService.getRcsPresenceInterface();
                if (rcsPresence == null) {
                    throw new RcsException("getRcsPresenceInterface()",
                            ResultCode.ERROR_SERVICE_NOT_AVAILABLE);
                }
                mRcsPresence = new RcsPresence(rcsPresence);
            } catch (RemoteException e) {
                throw new RcsException("getRcsPresenceInterface()", e,
                        ResultCode.ERROR_SERVICE_NOT_AVAILABLE);
            }
        }
        if (DBG) Rlog.d(TAG, "getRcsPresenceInterface(), mRcsPresence= " + mRcsPresence);
        return mRcsPresence;
    }

    /**
     * Binds the RCS service only if the service is not created.
     */
    private void checkAndThrowExceptionIfServiceUnavailable()
            throws  RcsException {
        if (mRcsService == null) {
            createRcsService(true);

            if (mRcsService == null) {
                throw new RcsException("Service is unavailable",
                        ResultCode.ERROR_SERVICE_NOT_AVAILABLE);
            }
        }
    }

    private static String getRcsServiceName(int subId) {
        // use the same mechanism as IMS_SERVICE?
        return RCS_SERVICE;
    }

    /**
     * Binds the RCS service.
     */
    private void createRcsService(boolean checkService) {
        if (checkService) {
            IBinder binder = ServiceManager.checkService(getRcsServiceName(mSubId));

            if (binder == null) {
                return;
            }
        }

        IBinder b = ServiceManager.getService(getRcsServiceName(mSubId));

        if (b != null) {
            try {
                b.linkToDeath(mDeathRecipient, 0);
            } catch (RemoteException e) {
            }
        }

        mRcsService = IRcsService.Stub.asInterface(b);
    }

    /**
     * Death recipient class for monitoring RCS service.
     */
    private class RcsServiceDeathRecipient implements IBinder.DeathRecipient {
        @Override
        public void binderDied() {
            mRcsService = null;
            mRcsPresence = null;

            if (mContext != null) {
                Intent intent = new Intent(ACTION_RCS_SERVICE_DIED);
                intent.putExtra(EXTRA_SUBID, mSubId);
                mContext.sendBroadcast(new Intent(intent));
            }
        }
    }
}
