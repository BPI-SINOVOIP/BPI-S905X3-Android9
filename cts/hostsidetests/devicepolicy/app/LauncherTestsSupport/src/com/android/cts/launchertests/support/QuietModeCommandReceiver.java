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
package com.android.cts.launchertests.support;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.UserHandle;
import android.os.UserManager;
import android.util.Log;

/**
 * Runs {@link UserManager#requestQuietModeEnabled(boolean, UserHandle)} APIs by receiving
 * broadcast and returns the result back to the broadcast sender.
 */
public class QuietModeCommandReceiver extends BroadcastReceiver {
    private static final String TAG = "QuietModeReceiver";
    private static final String ACTION_TOGGLE_QUIET_MODE = "toggle_quiet_mode";
    private static final String EXTRA_QUIET_MODE = "quiet_mode";
    private static final String RESULT_SECURITY_EXCEPTION = "security-exception";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (!ACTION_TOGGLE_QUIET_MODE.equals(intent.getAction())) {
            return;
        }
        final UserManager userManager = context.getSystemService(UserManager.class);
        final boolean enableQuietMode = intent.getBooleanExtra(EXTRA_QUIET_MODE, false);
        final UserHandle targetUser = intent.getParcelableExtra(Intent.EXTRA_USER);
        String result;
        try {
            final boolean setQuietModeResult =
                    userManager.requestQuietModeEnabled(enableQuietMode, targetUser);
            result = Boolean.toString(setQuietModeResult);
            Log.i(TAG, "trySetQuietModeEnabled returns " + setQuietModeResult);
        } catch (SecurityException ex) {
            Log.i(TAG, "trySetQuietModeEnabled throws security exception", ex);
            result = RESULT_SECURITY_EXCEPTION;
        }
        setResultData(result);
    }
}
