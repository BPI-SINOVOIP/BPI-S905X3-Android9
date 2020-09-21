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

package com.android.cts.deviceandprofileowner;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Wrapper class used to call the activity in the non-test APK and wait for its result.
 */
public class PrintActivity extends Activity {

    private static final String PRINTING_PACKAGE = "com.android.cts.devicepolicy.printingapp";
    private static final String PRINT_ACTIVITY = PRINTING_PACKAGE + ".PrintActivity";
    private static final String EXTRA_ERROR_MESSAGE = "error_message";

    private final CountDownLatch mLatch = new CountDownLatch(1);
    private String mErrorMessage;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        startActivityForResult(
                new Intent().setComponent(new ComponentName(PRINTING_PACKAGE, PRINT_ACTIVITY)), 0);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        // Use RESULT_FIRST_USER for success to avoid false positives.
        if (resultCode != RESULT_FIRST_USER) {
            if (data != null) {
                mErrorMessage = data.getStringExtra(EXTRA_ERROR_MESSAGE);
            }
            if (mErrorMessage == null) {
                mErrorMessage = "Unknown error, resultCode: " + resultCode;
            }
        }
        mLatch.countDown();
    }

    public String getErrorMessage() throws InterruptedException {
        final boolean called = mLatch.await(4, TimeUnit.SECONDS);
        if (!called) {
            throw new IllegalStateException("PrintActivity didn't finish in time");
        }
        finish();
        return mErrorMessage;
    }
}
