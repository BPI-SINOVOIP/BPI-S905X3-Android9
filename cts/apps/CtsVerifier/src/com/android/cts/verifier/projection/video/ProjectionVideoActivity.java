/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.cts.verifier.projection.video;

import android.os.Bundle;
import android.util.Log;

import com.android.cts.verifier.R;
import com.android.cts.verifier.projection.ProjectionActivity;
import com.android.cts.verifier.projection.ProjectionPresentationType;

public class ProjectionVideoActivity extends ProjectionActivity {
    private static final String TAG = ProjectionVideoActivity.class.getSimpleName();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "onCreate");
        setContentViewAndInfoResources(R.layout.pa_main, R.string.pva_test, R.string.pva_info);
        mType = ProjectionPresentationType.VIDEO_PLAYBACK;
    }
}
