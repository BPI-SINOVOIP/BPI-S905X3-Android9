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

import static android.server.am.Components.KeyguardDismissLoggerCallback.ENTRY_ON_DISMISS_CANCELLED;
import static android.server.am.Components.KeyguardDismissLoggerCallback.ENTRY_ON_DISMISS_ERROR;
import static android.server.am.Components.KeyguardDismissLoggerCallback.ENTRY_ON_DISMISS_SUCCEEDED;
import static android.server.am.Components.KeyguardDismissLoggerCallback.KEYGUARD_DISMISS_LOG_TAG;

import android.app.KeyguardManager;
import android.app.KeyguardManager.KeyguardDismissCallback;
import android.content.Context;
import android.util.Log;

class KeyguardDismissLoggerCallback extends KeyguardDismissCallback {

    private final Context mContext;

    KeyguardDismissLoggerCallback(Context context) {
        mContext = context;
    }

    @Override
    public void onDismissError() {
        Log.i(KEYGUARD_DISMISS_LOG_TAG, ENTRY_ON_DISMISS_ERROR);
    }

    @Override
    public void onDismissSucceeded() {
        if (mContext.getSystemService(KeyguardManager.class).isDeviceLocked()) {
            // Device is still locked? What a fail. Don't print "onDismissSucceeded" such that the
            // log fails.
            Log.i(KEYGUARD_DISMISS_LOG_TAG,
                    "dismiss succeeded was called but device is still locked.");
        } else {
            Log.i(KEYGUARD_DISMISS_LOG_TAG, ENTRY_ON_DISMISS_SUCCEEDED);
        }
    }

    @Override
    public void onDismissCancelled() {
        Log.i(KEYGUARD_DISMISS_LOG_TAG, ENTRY_ON_DISMISS_CANCELLED);
    }
}
