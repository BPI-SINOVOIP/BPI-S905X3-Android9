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

package android.server.am.displaysize;

import static android.server.am.displaysize.Components.SmallestWidthActivity
        .EXTRA_LAUNCH_ANOTHER_ACTIVITY;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;

public class SmallestWidthActivity extends Activity {

    @Override
    protected void onNewIntent(final Intent intent) {
        super.onNewIntent(intent);

        if (intent.hasExtra(EXTRA_LAUNCH_ANOTHER_ACTIVITY)) {
            startActivity(new Intent().setComponent(ComponentName.unflattenFromString(
                    intent.getStringExtra(EXTRA_LAUNCH_ANOTHER_ACTIVITY))));
        }
    }
}
