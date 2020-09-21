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

package com.android.multiwindowplayground.activities;

import com.android.multiwindowplayground.R;

import android.os.Bundle;
import android.view.View;

/**
 * In free-form mode, this activity is to be launched within a defined bounds on screen.
 * This property is set as part of the Intent that starts this activity.
 *
 * @see com.android.multiwindowplayground.MainActivity#onStartLaunchBoundsActivity(View)
 */
public class LaunchBoundsActivity extends LoggingActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_logging);

        setBackgroundColor(R.color.lime);
        setDescription(R.string.activity_bounds_description);
    }

}
