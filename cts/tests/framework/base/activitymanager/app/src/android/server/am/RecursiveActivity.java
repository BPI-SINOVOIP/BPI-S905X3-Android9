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
 * limitations under the License
 */

package android.server.am;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class RecursiveActivity extends AbstractLifecycleLogActivity {

    private static final int MAX_RECURSION_DEPTH = 20;
    private static int sRecursionDepth;

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        Log.i(getTag(), "Instance number: " + sRecursionDepth);
        if (sRecursionDepth++ < MAX_RECURSION_DEPTH) {
            Log.i(getTag(), "Launching new instance recursively.");
            final Intent intent = new Intent(this, RecursiveActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
            startActivity(intent);
        } else {
            Log.i(getTag(), "Reached end of recursive launches. Launching TestActivity.");
            final Intent intent = new Intent(this, TestActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
            startActivity(intent);
        }
    }
}
