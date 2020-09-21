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
package com.android.cts.managedprofile;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

/**
 * Activity that lives in the managed profile.
 */
public class ManagedProfileActivity extends Activity {
    private static final String TAG = ManagedProfileActivity.class.getName();

    public static final String ACTION =
            "com.android.cts.managedprofile.ACTION_TEST_MANAGED_ACTIVITY";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "Managed profile activity started!");
        setResult(RESULT_OK);
        finish();
    }
}