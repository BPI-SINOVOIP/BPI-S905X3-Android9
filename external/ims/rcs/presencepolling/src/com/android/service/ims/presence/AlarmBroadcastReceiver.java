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
import com.android.service.ims.presence.Contacts;
import com.android.service.ims.presence.SharedPrefUtil;

public class AlarmBroadcastReceiver extends BroadcastReceiver {
    private Logger logger = Logger.getLogger(this.getClass().getName());

    private static final String ACTION_PERIODICAL_DISCOVERY_ALARM =
            CapabilityPolling.ACTION_PERIODICAL_DISCOVERY_ALARM;
    private static final String ACTION_POLLING_RETRY_ALARM =
            PollingTask.ACTION_POLLING_RETRY_ALARM;
    private static final String ACTION_EAB_NEW_CONTACT_INSERTED =
            Contacts.ACTION_NEW_CONTACT_INSERTED;

    @Override
    public void onReceive(Context context, Intent intent) {
        logger.info("onReceive(), intent: " + intent +
                ", context: " + context);

        String action = intent.getAction();
        if (ACTION_PERIODICAL_DISCOVERY_ALARM.equals(action)) {
            CapabilityPolling capabilityPolling = CapabilityPolling.getInstance(null);
            if (capabilityPolling != null) {
                int pollingType = intent.getIntExtra("pollingType",
                        CapabilityPolling.ACTION_POLLING_NORMAL);
                capabilityPolling.enqueueDiscovery(pollingType);
            }
        } else if (ACTION_POLLING_RETRY_ALARM.equals(action)) {
            PollingsQueue queue = PollingsQueue.getInstance(null);
            if (queue != null) {
                long id = intent.getLongExtra("pollingTaskId", -1);
                queue.retry(id);
            }
        } else if (ACTION_EAB_NEW_CONTACT_INSERTED.equals(action)) {
            CapabilityPolling capabilityPolling = CapabilityPolling.getInstance(null);
            if (capabilityPolling != null) {
                String number = intent.getStringExtra(Contacts.NEW_PHONE_NUMBER);
                capabilityPolling.enqueueNewContact(number);
            }
        } else {
            logger.debug("No interest in this intent: " + action);
        }
    }
};

