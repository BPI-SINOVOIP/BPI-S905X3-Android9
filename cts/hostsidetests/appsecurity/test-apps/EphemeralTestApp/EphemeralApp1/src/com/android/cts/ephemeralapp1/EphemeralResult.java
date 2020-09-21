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

package com.android.cts.ephemeralapp1;

import android.app.Activity;
import android.app.SearchManager;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.ResolveInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.os.Bundle;
import android.provider.SearchRecentSuggestions;
import android.util.Log;

import com.android.cts.util.TestResult;

import java.util.List;

public class EphemeralResult extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final Intent intent  = getIntent();
        if (Intent.ACTION_VIEW.equals(intent.getAction())
                && intent.hasExtra(TestResult.EXTRA_TEST_RESULT)) {
            ((TestResult) intent.getParcelableExtra(TestResult.EXTRA_TEST_RESULT)).broadcast(this);
        }
        finish();
    }
}
