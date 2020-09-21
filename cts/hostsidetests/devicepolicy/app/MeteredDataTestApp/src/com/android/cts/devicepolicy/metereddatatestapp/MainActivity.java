/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.cts.devicepolicy.metereddatatestapp;

import android.app.Activity;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

public class MainActivity extends Activity {
    private static final String TAG = MainActivity.class.getSimpleName();

    private static final String EXTRA_MESSENGER = "messenger";
    private static final int MSG_NOTIFY_NETWORK_STATE = 1;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        Log.i(TAG, "onCreate called");
        notifyNetworkStateCallback();
        finish();
    }

    private void notifyNetworkStateCallback() {
        final Messenger callbackMessenger = new Messenger(getIntent().getExtras().getBinder(
                EXTRA_MESSENGER));
        if (callbackMessenger == null) {
            return;
        }
        try {
            final NetworkInfo networkInfo = getActiveNetworkInfo();
            Log.e(TAG, "getActiveNetworkNetworkInfo() is null");
            callbackMessenger.send(Message.obtain(null,
                    MSG_NOTIFY_NETWORK_STATE, networkInfo));
        } catch (RemoteException e) {
            Log.e(TAG, "Exception while sending the network state", e);
        }
    }

    private NetworkInfo getActiveNetworkInfo() {
        final ConnectivityManager cm = (ConnectivityManager) getSystemService(
                Context.CONNECTIVITY_SERVICE);
        return cm.getActiveNetworkInfo();
    }
}
