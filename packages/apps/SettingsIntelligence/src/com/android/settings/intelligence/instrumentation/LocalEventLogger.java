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

package com.android.settings.intelligence.instrumentation;

import android.util.Log;

import com.android.settings.intelligence.nano.SettingsIntelligenceLogProto;

public class LocalEventLogger implements EventLogger {

    private static final boolean SHOULD_LOG = false;
    private static final String TAG = "SettingsIntLogLocal";

    @Override
    public void log(SettingsIntelligenceLogProto.SettingsIntelligenceEvent event) {
        if (SHOULD_LOG) {
            Log.i(TAG, event.toString());
        }
    }
}
