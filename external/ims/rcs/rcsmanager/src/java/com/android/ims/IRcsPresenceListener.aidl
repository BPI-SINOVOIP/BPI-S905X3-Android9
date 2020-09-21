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

/**
 * RcsPresenceListener is used by requestCapability and requestAvailability to return the
 * status. The reqId which returned by requestCapability and requestAvailability can be
 * used to match the result with request when share the implementation of IRcsPresenceListener.
 *
 * There are following normal cases:
 *   1> The presence service got terminated notify after submitted the SUBSCRIBE request.
 *       onSuccess() ---> onFinish()
 *   2> The presence service submitted the request to network and got "200 OK" for SIP response.
 *   but it didn't get "terminated notify" in expries + T1 timer.
 *       onSuccess() ---> onTimeout()
 *   3> The presence service got error from SIP response for SUBSCRIBE request.
 *       onError()
 *
 * @hide
 */
interface IRcsPresenceListener {
    /**
     * It is called when it gets "200 OK" from network for SIP request or
     * it gets "404 xxxx" for SIP request of single contact number (means not capable).
     *
     * @param reqId the request ID which returned by requestCapability or
     * requestAvailability
     *
     * @see RcsPresence#requestCapability
     * @see RcsPresence#requestAvailability
     */
    void onSuccess(in int reqId);

    /**
     * It is called when it gets local error or gets SIP error from network.
     *
     * @param reqId the request ID which returned by requestCapability or
     * requestAvailability
     * @param resultCode the result code which defined in RcsManager.ResultCode.
     *
     * @see RcsPresence#requestCapability
     * @see RcsPresence#requestAvailability
     * @see RcsManager.ResultCode
     */
    void onError(in int reqId, in int resultCode);

    /**
     * It is called when it gets the "terminated notify" from network.
     * The presence service will not receive notify for the request any more after it got
     * "terminated notify" from network. So onFinish means the requestCapability or
     * requestAvailability had been finished completely.
     *
     * @param reqId the request ID which returned by requestCapability or
     * requestAvailability
     *
     * @see RcsPresence#requestCapability
     * @see RcsPresence#requestAvailability
     */
    void onFinish(in int reqId);

    /**
     * It is called when it is timeout to wait for the "terminated notify" from network.
     *
     * @param reqId the request ID which returned by requestCapability or
     * requestAvailability
     *
     * @see RcsPresence#requestCapability
     * @see RcsPresence#requestAvailability
     */
    void onTimeout(in int reqId);
}
