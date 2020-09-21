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

import java.util.Set;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import android.os.RemoteException;

import com.android.ims.internal.Logger;
import com.android.ims.IRcsPresenceListener;

/**
 * Task
 */
public class Task{
    /*
     * The logger
     */
    private Logger logger = Logger.getLogger(this.getClass().getName());

    // filled before send the reques
    public int mTaskId;

    // filled before send the request
    // map to QRCS_PRES_CMD_ID
    // QRCS_PRES_CMD_PUBLISHMYCAP
    // QRCS_PRES_CMD_GETCONTACTCAP, we use it for capability
    // QRCS_PRES_CMD_GETCONTACTLISTCAP, we use it for availability
    // QRCS_PRES_CMD_SETNEWFEATURETAG
    public int mCmdId;

    // filled after IQPresListener_CMDStatus
    // QRCS_STATUSCODE, possible values are:
    // QRCS_SUCCESS
    // QRCS_FAILURE
    // QRCS_SUCCESS_ASYC_UPDATE
    // QRCS_INVALID_SERVICE_HANDLE
    // QRCS_INVALID_LISTENER_HANDLE
    // QRCS_INVALID_PARAM
    // QRCS_FETCH_ERROR
    // QRCS_REQUEST_TIMEOUT
    // QRCS_INSUFFICIENT_MEMORY
    // QRCS_LOST_NET
    // QRCS_NOT_SUPPORTED
    // QRCS_NOT_FOUND
    // Note: have converted it to ResultCode.
    public int mCmdStatus;

    //filled after IQPresListener_CMDStatus
    public int mSipRequestId;

    // filled after IQPresListener_SipResponseReceived
    public int mSipResponseCode;

    // filled after IQPresListener_SipResponseReceived
    public String mSipReasonPhrase;

    // filled before send the request
    public IRcsPresenceListener mListener;

    public Task(int taskId, int cmdId, IRcsPresenceListener listener){
        mTaskId = taskId;
        mCmdId = cmdId;
        mListener = listener;
    }

    public String toString(){
        return "Task: mTaskId=" + mTaskId +
                " mCmdId=" + mCmdId +
                " mCmdStatus=" + mCmdStatus +
                " mSipRequestId=" + mSipRequestId +
                " mSipResponseCode=" + mSipResponseCode +
                " mSipReasonPhrase=" + mSipReasonPhrase;
    }
};

