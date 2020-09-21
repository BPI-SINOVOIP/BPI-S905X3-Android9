/*
 * Copyright (C) 2015 The Android Open Source Project
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

package android.voiceinteraction.cts;

import android.app.Activity;
import android.content.Intent;
import android.content.ComponentName;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;

import android.voiceinteraction.common.Utils;

public class TestStartActivity extends Activity {
    static final String TAG = "TestStartActivity";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, " in onCreate");
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.i(TAG, " in onResume");
        Intent intent = new Intent();
        intent.setAction("android.intent.action.START_TEST");
        intent.setComponent(new ComponentName("android.voiceinteraction.service",
                "android.voiceinteraction.service.VoiceInteractionMain"));
        startActivity(intent);
    }

    @Override protected void onPause() {
        Log.i(TAG, " in onPause");
        super.onPause();
    }

    @Override protected void onStart() {
        super.onStart();
        Log.i(TAG, " in onStart");
    }

    @Override protected void onRestart() {
        super.onRestart();
        Log.i(TAG, " in onRestart");
    }

    @Override protected void onStop() {
        Log.i(TAG, " in onStop");
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        Log.i(TAG, " in onDestroy");
        super.onDestroy();
    }
}
