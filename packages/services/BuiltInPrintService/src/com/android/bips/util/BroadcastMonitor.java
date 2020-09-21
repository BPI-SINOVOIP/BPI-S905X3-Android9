/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.bips.util;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

/**
 * A stoppable receiver for broadcast messages
 */
public class BroadcastMonitor extends BroadcastReceiver {
    BroadcastReceiver mReceiver;
    Context mContext;

    /**
     * Deliver specified broadcast intents to the receiver until stopped.
     *
     * @param receiver Listener to receive all broadcast messages of interest
     * @param actions  Broadcast intent action names to listen for
     */
    public BroadcastMonitor(Context context, BroadcastReceiver receiver, String... actions) {
        IntentFilter filter = new IntentFilter();
        for (String action : actions) {
            filter.addAction(action);
        }
        mContext = context;
        mReceiver = receiver;
        mContext.registerReceiver(this, filter);
    }

    /** Stop monitoring for broadcast intents */
    public void close() {
        if (mReceiver != null) {
            mReceiver = null;
            mContext.unregisterReceiver(this);
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent.getAction() != null && mReceiver != null) {
            mReceiver.onReceive(context, intent);
        }
    }
}
