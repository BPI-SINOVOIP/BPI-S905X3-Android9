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
package com.android.devicehealth.tests;

import android.content.Context;
import android.os.DropBoxManager;
import android.support.test.InstrumentationRegistry;
import android.util.Log;

import org.junit.Assert;
import org.junit.Before;

abstract class HealthCheckBase {

    private static final int MAX_DROPBOX_READ = 4096; // read up to 4K from a dropbox entry
    private static final String INCLUDE_KNOWN_FAILURES = "include_known_failures";
    private static final String LOG_TAG = HealthCheckBase.class.getSimpleName();
    private Context mContext;
    private KnownFailures mKnownFailures = new KnownFailures();
    /** whether known failures should be reported anyways, useful for bug investigation */
    private boolean mIncludeKnownFailures;


    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getInstrumentation().getContext();
        mIncludeKnownFailures = InstrumentationRegistry.getArguments().getBoolean(
                INCLUDE_KNOWN_FAILURES, false);

    }

    /**
     * Check dropbox service for a particular label and assert if found
     */
    protected void checkCrash(String label) {
        DropBoxManager dropbox = (DropBoxManager) mContext
                .getSystemService(Context.DROPBOX_SERVICE);
        Assert.assertNotNull("Unable access the DropBoxManager service", dropbox);

        long timestamp = 0;
        DropBoxManager.Entry entry;
        int crashCount = 0;
        StringBuilder errorDetails = new StringBuilder("Error details:\n");
        while (null != (entry = dropbox.getNextEntry(label, timestamp))) {
            String dropboxSnippet;
            try {
                dropboxSnippet = entry.getText(MAX_DROPBOX_READ);
            } finally {
                entry.close();
            }
            KnownFailureItem k = mKnownFailures.findMatchedKnownFailure(label, dropboxSnippet);
            if (k != null && !mIncludeKnownFailures) {
                Log.i(LOG_TAG, String.format(
                        "Ignored a known failure, type: %s, pattern: %s, bug: %s",
                        label, k.failurePattern, k.bugNumber));
            } else {
                crashCount++;
                errorDetails.append(label);
                errorDetails.append(": ");
                errorDetails.append(dropboxSnippet.substring(0, 70));
                errorDetails.append("    ...\n");
            }
            timestamp = entry.getTimeMillis();
        }
        Assert.assertEquals(errorDetails.toString(), 0, crashCount);
    }
}
