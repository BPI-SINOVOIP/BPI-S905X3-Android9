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

import java.util.Set;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import android.os.RemoteException;

import com.android.ims.internal.uce.presence.PresCmdStatus;

import com.android.ims.internal.Logger;
import com.android.ims.RcsManager.ResultCode;
import com.android.ims.RcsPresenceInfo;
import com.android.ims.IRcsPresenceListener;

import com.android.service.ims.Task;
import com.android.service.ims.RcsUtils;

/**
 * PresenceTask
 */
public class PresenceTask extends Task{
    /*
     * The logger
     */
    private Logger logger = Logger.getLogger(this.getClass().getName());

    // filled before send the request to stack
    public String[] mContacts;

    public PresenceTask(int taskId, int cmdId, IRcsPresenceListener listener, String[] contacts){
        super(taskId, cmdId, listener);

        mContacts = contacts;
        mListener = listener;
    }

    public String toString(){
        return "PresenceTask: mTaskId=" + mTaskId +
                " mCmdId=" + mCmdId +
                " mContacts=" + RcsUtils.toContactString(mContacts) +
                " mCmdStatus=" + mCmdStatus +
                " mSipRequestId=" + mSipRequestId +
                " mSipResponseCode=" + mSipResponseCode +
                " mSipReasonPhrase=" + mSipReasonPhrase;
    }
};

