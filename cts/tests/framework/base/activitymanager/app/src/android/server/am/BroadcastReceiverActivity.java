/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.server.am;

import static android.server.am.Components.BroadcastReceiverActivity.ACTION_TRIGGER_BROADCAST;
import static android.server.am.Components.BroadcastReceiverActivity.EXTRA_BROADCAST_ORIENTATION;
import static android.server.am.Components.BroadcastReceiverActivity.EXTRA_DISMISS_KEYGUARD;
import static android.server.am.Components.BroadcastReceiverActivity.EXTRA_DISMISS_KEYGUARD_METHOD;
import static android.server.am.Components.BroadcastReceiverActivity.EXTRA_FINISH_BROADCAST;
import static android.server.am.Components.BroadcastReceiverActivity.EXTRA_MOVE_BROADCAST_TO_BACK;
import static android.view.WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD;

import android.app.Activity;
import android.app.KeyguardManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;

/**
 * Activity that registers broadcast receiver .
 */
public class BroadcastReceiverActivity extends Activity {
    private static final String TAG = BroadcastReceiverActivity.class.getSimpleName();

    private TestBroadcastReceiver mBroadcastReceiver = new TestBroadcastReceiver();

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        IntentFilter broadcastFilter = new IntentFilter(ACTION_TRIGGER_BROADCAST);

        registerReceiver(mBroadcastReceiver, broadcastFilter);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        unregisterReceiver(mBroadcastReceiver);
    }

    public class TestBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            final Bundle extras = intent.getExtras();
            Log.i(TAG, "onReceive: extras=" + extras);

            if (extras == null) {
                return;
            }
            if (extras.getBoolean(EXTRA_FINISH_BROADCAST)) {
                finish();
            }
            if (extras.getBoolean(EXTRA_MOVE_BROADCAST_TO_BACK)) {
                moveTaskToBack(true);
            }
            if (extras.containsKey(EXTRA_BROADCAST_ORIENTATION)) {
                setRequestedOrientation(extras.getInt(EXTRA_BROADCAST_ORIENTATION));
            }
            if (extras.getBoolean(EXTRA_DISMISS_KEYGUARD)) {
                getWindow().addFlags(FLAG_DISMISS_KEYGUARD);
            }
            if (extras.getBoolean(EXTRA_DISMISS_KEYGUARD_METHOD)) {
                getSystemService(KeyguardManager.class).requestDismissKeyguard(
                        BroadcastReceiverActivity.this, new KeyguardDismissLoggerCallback(context));
            }

            ActivityLauncher.launchActivityFromExtras(BroadcastReceiverActivity.this, extras);
        }
    }
}
