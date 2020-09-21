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
 * limitations under the License
 */

package android.jobscheduler.cts.jobtestapp;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

/**
 * Just a dummy activity to keep the test app process in the foreground state when desired.
 */
public class TestActivity extends Activity {
    private static final String TAG = TestActivity.class.getSimpleName();
    private static final String PACKAGE_NAME = "android.jobscheduler.cts.jobtestapp";
    private static final long DEFAULT_WAIT_DURATION = 30_000;

    static final int FINISH_ACTIVITY_MSG = 1;
    public static final String ACTION_FINISH_ACTIVITY = PACKAGE_NAME + ".action.FINISH_ACTIVITY";

    Handler mFinishHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case FINISH_ACTIVITY_MSG:
                    Log.d(TAG, "Finishing test activity: " + TestActivity.class.getCanonicalName());
                    unregisterReceiver(mFinishReceiver);
                    finish();
            }
        }
    };

    final BroadcastReceiver mFinishReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            mFinishHandler.removeMessages(FINISH_ACTIVITY_MSG);
            mFinishHandler.sendEmptyMessage(FINISH_ACTIVITY_MSG);
        }
    };

    @Override
    public void onCreate(Bundle savedInstance) {
        Log.d(TAG, "Started test activity: " + TestActivity.class.getCanonicalName());
        super.onCreate(savedInstance);
        // automatically finish after 30 seconds.
        mFinishHandler.sendEmptyMessageDelayed(FINISH_ACTIVITY_MSG, DEFAULT_WAIT_DURATION);
        registerReceiver(mFinishReceiver, new IntentFilter(ACTION_FINISH_ACTIVITY));
    }
}
