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
 * limitations under the License
 */

package android.server.am;

import static android.server.am.Components.MoveTaskToBackActivity.EXTRA_FINISH_POINT;
import static android.server.am.Components.MoveTaskToBackActivity.FINISH_POINT_ON_PAUSE;
import static android.server.am.Components.MoveTaskToBackActivity.FINISH_POINT_ON_STOP;

import android.content.Intent;
import android.os.Bundle;

/**
 * Activity that finishes itself using "moveTaskToBack".
 */
public class MoveTaskToBackActivity extends AbstractLifecycleLogActivity {

    private String mFinishPoint;

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        final Intent intent = getIntent();
        mFinishPoint = intent.getExtras().getString(EXTRA_FINISH_POINT);
    }

    @Override
    protected void onPause() {
        super.onPause();

        if (FINISH_POINT_ON_PAUSE.equals(mFinishPoint)) {
            moveTaskToBack(true /* nonRoot */);
        }
    }

    @Override
    protected void onStop() {
        super.onStop();

        if (FINISH_POINT_ON_STOP.equals(mFinishPoint)) {
            moveTaskToBack(true /* nonRoot */);
        }
    }
}
