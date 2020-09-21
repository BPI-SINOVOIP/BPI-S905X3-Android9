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
package com.android.tv;

import android.content.Context;
import android.util.Log;

/** Initializes TvApplication. */
public interface Starter {

    /**
     * Initializes TvApplication.
     *
     * <p>Note: it should be called at the beginning of any Service.onCreate, Activity.onCreate, or
     * BroadcastReceiver.onCreate.
     */
    static void start(Context context) {
        // TODO(b/63064354) TvApplication should not have to know if it is "the main process"
        if (context.getApplicationContext() instanceof Starter) {
            Starter starter = (Starter) context.getApplicationContext();
            starter.start();
        } else {
            // Application context can be MockTvApplication.
            Log.w("Start", "It is not a context of TvApplication");
        }
    }

    void start();
}
