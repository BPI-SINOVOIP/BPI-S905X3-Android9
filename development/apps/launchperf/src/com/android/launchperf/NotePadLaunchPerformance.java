/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.android.launchperf;

import android.app.Activity;
import android.test.LaunchPerformanceBase;
import android.os.Bundle;

import java.util.Map;

/**
 * Instrumentation class for Notepad launch performance testing.
 */
public class NotePadLaunchPerformance extends LaunchPerformanceBase {
 
    public static final String LOG_TAG = "NotePadLaunchPerformance";

    public NotePadLaunchPerformance() {
        super();
    }

    @Override
    public void onCreate(Bundle arguments) {
        super.onCreate(arguments);

        mIntent.setClassName(getTargetContext(), "com.android.notepad.NotesList");
        start();
    }

    /**
     * Calls LaunchApp and finish.
     */
    @Override
    public void onStart() {
        super.onStart();
        LaunchApp();
        finish(Activity.RESULT_OK, mResults);
    }
}
