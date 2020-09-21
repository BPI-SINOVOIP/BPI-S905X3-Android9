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
 * limitations under the License.
 */

package com.android.cts.devicepolicy.printingapp;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.print.PageRange;
import android.print.PrintAttributes;
import android.print.PrintDocumentAdapter;
import android.print.PrintJob;
import android.print.PrintManager;

public class PrintActivity extends Activity {

    private static final String PRINT_JOB_NAME = "Test print job";
    private static final String EXTRA_ERROR_MESSAGE = "error_message";
    private static final int STATE_INIT = 0;
    private static final int STATE_STARTED = 1;
    private static final int STATE_FINISHED = 2;

    @Override
    public void onStart() {
        super.onStart();
        PrintManager printManager = getSystemService(PrintManager.class);
        final int[] state = new int[]{STATE_INIT};
        PrintJob printJob = printManager.print(PRINT_JOB_NAME, new PrintDocumentAdapter() {
            @Override
            public void onStart() {
                if (state[0] != STATE_INIT) {
                    fail("Unexpected call to onStart()");
                }
                state[0] = STATE_STARTED;
            }

            @Override
            public void onFinish() {
                if (state[0] != STATE_STARTED) {
                    fail("Unexpected call to onFinish()");
                }
                state[0] = STATE_FINISHED;
                // Use RESULT_FIRST_USER for success to avoid false positives.
                setResult(RESULT_FIRST_USER);
                finish();
            }

            @Override
            public void onLayout(PrintAttributes oldAttributes, PrintAttributes newAttributes,
                    CancellationSignal signal, PrintDocumentAdapter.LayoutResultCallback cb,
                    Bundle extras) {
                fail("onLayout() should never be called");
            }

            @Override
            public void onWrite(PageRange[] pages, ParcelFileDescriptor dest, CancellationSignal signal,
                    PrintDocumentAdapter.WriteResultCallback cb) {
                fail("onWrite() should never be called");
            }
        }, new PrintAttributes.Builder().build());
        if (printJob != null) {
            fail("print() should return null");
        }
    }

    private final void fail(String message) {
        setResult(RESULT_FIRST_USER + 1, new Intent().putExtra(EXTRA_ERROR_MESSAGE, message));
        finish();
    }
}
