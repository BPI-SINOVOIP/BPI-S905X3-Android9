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

package android.print.test;

import android.app.Activity;
import android.app.KeyguardManager;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

public class PrintDocumentActivity extends Activity {
    private static final String LOG_TAG = "PrintDocumentActivity";
    int mTestId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mTestId = getIntent().getIntExtra(BasePrintTest.TEST_ID, -1);
        Log.d(LOG_TAG, "onCreate() " + this + " for " + mTestId);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
                | WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);

        KeyguardManager km = getSystemService(KeyguardManager.class);
        if (km != null) {
            km.requestDismissKeyguard(this, null);
        }

        BasePrintTest.onActivityCreateCalled(mTestId, this);

        if (savedInstanceState != null) {
            Log.d(LOG_TAG,
                    "We cannot deal with lifecycle. Hence finishing " + this + " for " + mTestId);
            finish();
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        Log.d(LOG_TAG, "onConfigurationChanged(" + newConfig + ")");
    }

    @Override
    protected void onDestroy() {
        Log.d(LOG_TAG, "onDestroy() " + this);
        BasePrintTest.onActivityDestroyCalled(mTestId);
        super.onDestroy();
    }
}
