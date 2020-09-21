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

package com.android.ims.internal;

import com.android.ims.IRcsPresenceListener;

import java.util.List;

/**
 * @hide
 */
interface IRcsPresence {
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
    int requestCapability(in List<String> contactsNumber,
            in IRcsPresenceListener listener);

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
    int requestAvailability(in String contactNumber, in IRcsPresenceListener listener);

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
    int requestAvailabilityNoThrottle(in String contactNumber, in IRcsPresenceListener listener);

    /**
     * Get the latest publish state.
     *
     * @see RcsPresence.PublishState
     */
    int getPublishState();
}
