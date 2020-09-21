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

package com.android.documentsui.services;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import android.net.Uri;
import android.support.annotation.Nullable;

import com.android.documentsui.base.DocumentInfo;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class TestJobListener implements Job.Listener {

    private final CountDownLatch latch = new CountDownLatch(1);
    private final List<Job> progress = new ArrayList<>();
    @Nullable private Job started;
    @Nullable private Job finished;

    @Override
    public void onStart(Job job) {
        started = job;
    }

    @Override
    public void onFinished(Job job) {
        this.finished = job;
        latch.countDown();
    }

    public void assertStarted() {
        if (started == null) {
            fail("Job didn't start. onStart never called.");
        }
    }

    public void assertFinished() {
        if (finished == null) {
            fail("Job didn't finish. onFinish never called.");
        }
    }

    public void assertFailed() {
        if (finished == null || !finished.hasFailures()) {
            fail("Job didn't fail. onFailed never called.");
        }
    }

    public void assertFilesFailed(List<String> names) {
        if (finished == null || !finished.hasFailures()) {
            fail("Can't test failed documetns. Job didn't fail.");
        }

        assertEquals(finished.failedDocs.size(), names.size());
        for (String name : names) {
            assertFileFailed(name);
        }
    }

    public void assertFileFailed(String name) {
        if (finished == null || !finished.hasFailures()) {
            fail("Can't test failed documetns. Job didn't fail.");
        }

        for (DocumentInfo failed : finished.failedDocs) {
            if (name.equals(failed.displayName)) {
                return;
            }
        }
        fail("Couldn't find failed file: " + name);
    }

    public void assertUrisFailed(List<Uri> uris) {
        if (finished == null || !finished.hasFailures()) {
            fail("Can't test failed documetns. Job didn't fail.");
        }

        assertEquals(finished.failedDocs.size(), uris.size());
        for (Uri uri : uris) {
            assertUriFailed(uri);
        }
    }

    public void assertUriFailed(Uri uri) {
        if (finished == null || !finished.hasFailures()) {
            fail("Can't test failed documetns. Job didn't fail.");
        }

        for (Uri failed : finished.failedUris) {
            if (uri.equals(failed)) {
                return;
            }
        }
        fail("Couldn't find failed uri: " + uri);
    }

    public void assertFailureCount(int expected) {
        if (finished == null) {
            fail("No job to test.");
        }

        assertEquals(expected, finished.failureCount);
    }

    public void assertCanceled() {
        if (finished == null) {
            fail("Can't determine if job was canceled. Job didn't finish.");
        }
        if (!finished.isCanceled()) {
            fail("Job wasn't canceled. Job#isCanceled returned false.");
        }
    }

    public void assertMadeProgress() {
        if (progress.isEmpty()) {
            fail("Job made no progress. onProgress never called.");
        }
    }

    public void waitForFinished() throws InterruptedException {
        latch.await(500, TimeUnit.MILLISECONDS);
    }
}
