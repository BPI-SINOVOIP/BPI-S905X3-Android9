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

package com.android.server.telecom.testapps;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class TestConnectionServiceReceiver extends BroadcastReceiver {
    private static final String TAG = "TestConnectionServiceBR";

    // Switches the PhoneAccount to the second test SIM account.
    public static final String INTENT_SWITCH_PHONE_ACCOUNT =
            "android.server.telecom.testapps.ACTION_SWITCH_PHONE_ACCOUNT";

    // Creates a PhoneAccount that is incorrect and should create a SecurityException
    public static final String INTENT_SWITCH_PHONE_ACCOUNT_WRONG =
            "android.server.telecom.testapps.ACTION_SWITCH_PHONE_ACCOUNT_WRONG";

    /**
     * Handles broadcasts directed at the {@link TestInCallServiceImpl}.
     *
     * @param context The Context in which the receiver is running.
     * @param intent  The Intent being received.
     */
    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Log.v(TAG, "onReceive: " + action);

        TestConnectionService cs = TestConnectionService.getInstance();
        if (cs == null) {
            Log.i(TAG, "null TestConnectionService");
            return;
        }

        if (INTENT_SWITCH_PHONE_ACCOUNT.equals(action)) {
            cs.switchPhoneAccount();
        } else if (INTENT_SWITCH_PHONE_ACCOUNT_WRONG.equals(action)) {
            cs.switchPhoneAccountWrong();
        }
    }
}
