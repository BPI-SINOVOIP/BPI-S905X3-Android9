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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import com.android.ims.internal.Logger;

import com.android.service.ims.RcsStackAdaptor;
import com.android.service.ims.TaskManager;

public class AlarmBroadcastReceiver extends BroadcastReceiver{
    private Logger logger = Logger.getLogger(this.getClass().getName());

    private static final String ACTION_RETRY_ALARM =
            RcsStackAdaptor.ACTION_RETRY_ALARM;
    private static final String ACTION_TASK_TIMEOUT_ALARM =
            PresenceCapabilityTask.ACTION_TASK_TIMEOUT_ALARM;
    private static final String ACTION_RETRY_PUBLISH_ALARM =
            PresencePublication.ACTION_RETRY_PUBLISH_ALARM;

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        logger.info("onReceive intent: " + action);

        // this could happen when there is a crash and the previous broadcasted intent
        // coming when the PresenceService is launching.
        RcsStackAdaptor rcsStackAdaptor = RcsStackAdaptor.getInstance(null);
        if(rcsStackAdaptor == null){
            logger.debug("rcsStackAdaptor=null");
            return;
        }

        if(ACTION_RETRY_ALARM.equals(action)) {
            int times = intent.getIntExtra("times", -1);
            rcsStackAdaptor.startInitThread(times);
        }else if(ACTION_TASK_TIMEOUT_ALARM.equals(action)){
            int taskId = intent.getIntExtra("taskId", -1);
            TaskManager.getDefault().onTimeout(taskId);
        } else if(ACTION_RETRY_PUBLISH_ALARM.equals(action)) {
            // default retry is for 888
            int sipCode = intent.getIntExtra("sipCode", 888);
            PresencePublication publication = PresencePublication.getPresencePublication();
            if(publication != null) {
                publication.retryPublish();
            }
        }
        else{
            logger.debug("not interest in intent=" + intent);
        }
    }
};

