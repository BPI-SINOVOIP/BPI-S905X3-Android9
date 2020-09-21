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

import static android.app.WindowConfiguration.WINDOWING_MODE_PINNED;
import static android.content.Intent.FLAG_ACTIVITY_CLEAR_TASK;
import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;

import android.app.Activity;
import android.app.ActivityOptions;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Rect;

public class AlwaysFocusablePipActivity extends Activity {

    static void launchAlwaysFocusablePipActivity(Activity caller, boolean newTask) {
        final Intent intent = new Intent(caller, AlwaysFocusablePipActivity.class);

        intent.setFlags(FLAG_ACTIVITY_CLEAR_TASK);
        if (newTask) {
            intent.addFlags(FLAG_ACTIVITY_NEW_TASK);
        }

        final ActivityOptions options = ActivityOptions.makeBasic();
        options.setLaunchBounds(new Rect(0, 0, 500, 500));
        options.setLaunchWindowingMode(WINDOWING_MODE_PINNED);
        caller.startActivity(intent, options.toBundle());
    }
}
