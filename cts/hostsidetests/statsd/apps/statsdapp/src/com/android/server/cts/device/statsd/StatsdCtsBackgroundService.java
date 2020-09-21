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

package com.android.server.cts.device.statsd;

import com.android.server.cts.device.statsd.AtomTests;

import android.app.IntentService;
import android.content.Intent;
import android.util.Log;

/** An service (to be run as a background process) which performs one of a number of actions. */
public class StatsdCtsBackgroundService extends IntentService {
    private static final String TAG = StatsdCtsBackgroundService.class.getSimpleName();

    public static final String KEY_ACTION = "action";
    public static final String ACTION_BACKGROUND_SLEEP = "action.background_sleep";
    public static final String ACTION_END_IMMEDIATELY = "action.end_immediately";

    public static final int SLEEP_OF_ACTION_BACKGROUND_SLEEP = 2_000;

    public StatsdCtsBackgroundService() {
        super(StatsdCtsBackgroundService.class.getName());
    }

    @Override
    public void onHandleIntent(Intent intent) {
        String action = intent.getStringExtra(KEY_ACTION);
        Log.i(TAG, "Starting " + action + " from background service.");

        switch (action) {
            case ACTION_BACKGROUND_SLEEP:
                AtomTests.sleep(SLEEP_OF_ACTION_BACKGROUND_SLEEP);
                break;
            case ACTION_END_IMMEDIATELY:
                break;
            default:
                Log.e(TAG, "Intent had invalid action");
        }
    }
}
