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

import java.lang.String;
import android.telephony.TelephonyManager;
import android.content.Context;

import com.android.ims.internal.uce.common.StatusCode;

import com.android.ims.RcsManager.ResultCode;

import com.android.ims.internal.Logger;

public class RcsUtils{
    /*
     * The logger
     */
    static private Logger logger = Logger.getLogger("RcsUtils");

    public RcsUtils() {
    }

    static public int statusCodeToResultCode(int sipStatusCode){
        if(sipStatusCode == StatusCode.UCE_SUCCESS ||
            sipStatusCode == StatusCode.UCE_SUCCESS_ASYC_UPDATE){
            return ResultCode.SUCCESS;
        }

        if(sipStatusCode == StatusCode.UCE_INVALID_PARAM) {
            return ResultCode.SUBSCRIBE_INVALID_PARAM;
        }

        if(sipStatusCode == StatusCode.UCE_FETCH_ERROR) {
            return ResultCode.SUBSCRIBE_FETCH_ERROR;
        }

        if(sipStatusCode == StatusCode.UCE_REQUEST_TIMEOUT) {
            return ResultCode.SUBSCRIBE_REQUEST_TIMEOUT;
        }

        if(sipStatusCode == StatusCode.UCE_INSUFFICIENT_MEMORY) {
            return ResultCode.SUBSCRIBE_INSUFFICIENT_MEMORY;
        }

        if(sipStatusCode == StatusCode.UCE_LOST_NET) {
            return ResultCode.SUBSCRIBE_LOST_NETWORK;
        }

        if(sipStatusCode == StatusCode.UCE_NOT_SUPPORTED){
            return ResultCode.SUBSCRIBE_NOT_SUPPORTED;
        }

        if(sipStatusCode == StatusCode.UCE_NOT_FOUND){
            return ResultCode.SUBSCRIBE_NOT_FOUND;
        }

        if(sipStatusCode == StatusCode.UCE_FAILURE ||
                sipStatusCode == StatusCode.UCE_INVALID_SERVICE_HANDLE ||
                sipStatusCode == StatusCode.UCE_INVALID_LISTENER_HANDLE){
            return ResultCode.SUBSCRIBE_GENERIC;
        }

        return ResultCode.SUBSCRIBE_GENERIC;
    }

    static public String toContactString(String[] contacts) {
        if(contacts == null) {
            return null;
        }

         String result = "";
        for(int i=0; i<contacts.length; i++) {
            result += contacts[i];
            if(i != contacts.length -1) {
                result += ";";
            }
        }

        return result;
    }
}

