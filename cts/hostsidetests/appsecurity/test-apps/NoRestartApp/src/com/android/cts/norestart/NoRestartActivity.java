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
 * limitations under the License.
 */

package com.android.cts.norestart;

import android.content.Intent;
import android.os.RemoteCallback;
import com.android.cts.norestart.R;

import android.app.Activity;
import android.os.Bundle;

public class NoRestartActivity extends Activity {
    private static int sCreateCount;
    private int mNewIntentCount;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.no_restart_activity);
        sCreateCount++;
        final RemoteCallback callback = getIntent().getParcelableExtra("RESPONSE");
        sendResponse(callback);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        mNewIntentCount++;
        final RemoteCallback callback = intent.getParcelableExtra("RESPONSE");
        sendResponse(callback);
    }

    private void sendResponse(RemoteCallback callback) {
        final Bundle payload = new Bundle();
        payload.putInt("CREATE_COUNT", sCreateCount);
        payload.putInt("NEW_INTENT_COUNT", mNewIntentCount);
        callback.sendResult(payload);
    }
}
