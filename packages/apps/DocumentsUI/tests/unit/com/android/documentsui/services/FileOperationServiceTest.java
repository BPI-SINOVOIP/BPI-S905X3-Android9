/*
 * Copyright (C) 2015 The Android Open Source Project
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

import static com.android.documentsui.services.FileOperationService.OPERATION_COPY;
import static com.android.documentsui.services.FileOperationService.OPERATION_DELETE;
import static com.android.documentsui.services.FileOperations.createBaseIntent;
import static com.android.documentsui.services.FileOperations.createJobId;
import static com.google.android.collect.Lists.newArrayList;
import static org.junit.Assert.fail;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.test.ServiceTestCase;

import com.android.documentsui.R;
import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.base.DocumentStack;
import com.android.documentsui.base.Features;
import com.android.documentsui.clipping.UrisSupplier;
import com.android.documentsui.services.FileOperationService.OpType;
import com.android.documentsui.testing.DocsProviders;
import com.android.documentsui.testing.TestFeatures;
import com.android.documentsui.testing.TestHandler;
import com.android.documentsui.testing.TestScheduledExecutorService;

import java.util.ArrayList;
import java.util.List;

@MediumTest
public class FileOperationServiceTest extends ServiceTestCase<FileOperationService> {

    private static final Uri SRC_PARENT =
            Uri.parse("content://com.android.documentsui.testing/parent");
    private static final DocumentInfo ALPHA_DOC = createDoc("alpha");
    private static final DocumentInfo BETA_DOC = createDoc("alpha");
    private static final DocumentInfo GAMMA_DOC = createDoc("gamma");
    private static final DocumentInfo DELTA_DOC = createDoc("delta");

    private final List<TestJob> mCopyJobs = new ArrayList<>();
    private final List<TestJob> mDeleteJobs = new ArrayList<>();

    private FileOperationService mService;
    private TestScheduledExecutorService mExecutor;
    private TestScheduledExecutorService mDeletionExecutor;
    private TestHandler mHandler;
    private TestForegroundManager mForegroundManager;
    private TestNotificationManager mTestNotificationManager;

    public FileOperationServiceTest() {
        super(FileOperationService.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        setupService();  // must be called first for our test setup to work correctly.

        mExecutor = new TestScheduledExecutorService();
        mDeletionExecutor = new TestScheduledExecutorService();
        mHandler = new TestHandler();
        mForegroundManager = new TestForegroundManager();
        mTestNotificationManager = new TestNotificationManager(mForegroundManager);
        TestFeatures features = new TestFeatures();
        features.notificationChannel = InstrumentationRegistry.getTargetContext()
                .getResources().getBoolean(R.bool.feature_notification_channel);

        mCopyJobs.clear();
        mDeleteJobs.clear();

        // Install test doubles.
        mService = getService();

        assertNull(mService.executor);
        mService.executor = mExecutor;

        assertNull(mService.deletionExecutor);
        mService.deletionExecutor = mDeletionExecutor;

        assertNull(mService.handler);
        mService.handler = mHandler;

        assertNull(mService.foregroundManager);
        mService.foregroundManager = mForegroundManager;

        assertNull(mService.notificationManager);
        mService.notificationManager = mTestNotificationManager.createNotificationManager();

        assertNull(mService.features);
        mService.features = features;
    }

    @Override
    protected void tearDown() {
        // Release all possibly held wake lock here
        mExecutor.runAll();
        mDeletionExecutor.runAll();

        // There are lots of progress notifications generated in this test case.
        // Dismiss all of them here.
        mHandler.dispatchAllMessages();
    }

    public void testRunsCopyJobs() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));
        startService(createCopyIntent(newArrayList(GAMMA_DOC), DELTA_DOC));

        mExecutor.runAll();
        assertAllCopyJobsStarted();
    }

    public void testRunsCopyJobs_AfterExceptionInJobCreation() throws Exception {
        try {
            startService(createCopyIntent(new ArrayList<>(), BETA_DOC));
            fail("Should have throw exception.");
        } catch(IllegalArgumentException expected) {
            // We're sending a naughty empty list that should result in an IllegalArgumentException.
        }
        startService(createCopyIntent(newArrayList(GAMMA_DOC), DELTA_DOC));

        assertJobsCreated(1);

        mExecutor.runAll();
        assertAllCopyJobsStarted();
    }

    public void testRunsCopyJobs_AfterFailure() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));
        startService(createCopyIntent(newArrayList(GAMMA_DOC), DELTA_DOC));

        mCopyJobs.get(0).fail(ALPHA_DOC);

        mExecutor.runAll();
        assertAllCopyJobsStarted();
    }

    public void testRunsCopyJobs_notRunsDeleteJobs() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));
        startService(createDeleteIntent(newArrayList(GAMMA_DOC)));

        mExecutor.runAll();
        assertNoDeleteJobsStarted();
    }

    public void testRunsDeleteJobs() throws Exception {
        startService(createDeleteIntent(newArrayList(ALPHA_DOC)));

        mDeletionExecutor.runAll();
        assertAllDeleteJobsStarted();
    }

    public void testRunsDeleteJobs_NotRunsCopyJobs() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));
        startService(createDeleteIntent(newArrayList(GAMMA_DOC)));

        mDeletionExecutor.runAll();
        assertNoCopyJobsStarted();
    }

    public void testUpdatesNotification() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));
        mExecutor.runAll();

        // Assert monitoring continues until job is done
        assertTrue(mHandler.hasScheduledMessage());
        // Two notifications -- one for setup; one for progress
        assertEquals(2, mCopyJobs.get(0).getNumOfNotifications());
    }

    public void testStopsUpdatingNotificationAfterFinished() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));
        mExecutor.runAll();

        mHandler.dispatchNextMessage();
        // Assert monitoring stops once job is completed.
        assertFalse(mHandler.hasScheduledMessage());

        // Assert no more notification is generated after finish.
        assertEquals(2, mCopyJobs.get(0).getNumOfNotifications());

    }

    public void testHoldsWakeLockWhileWorking() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));

        assertTrue(mService.holdsWakeLock());
    }

    public void testReleasesWakeLock_AfterSuccess() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));

        assertTrue(mService.holdsWakeLock());
        mExecutor.runAll();
        assertFalse(mService.holdsWakeLock());
    }

    public void testReleasesWakeLock_AfterFailure() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));

        assertTrue(mService.holdsWakeLock());
        mExecutor.runAll();
        assertFalse(mService.holdsWakeLock());
    }

    public void testShutdownStopsExecutor_AfterSuccess() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));

        mExecutor.assertAlive();
        mDeletionExecutor.assertAlive();

        mExecutor.runAll();
        shutdownService();

        assertExecutorsShutdown();
    }

    public void testShutdownStopsExecutor_AfterMixedFailures() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));
        startService(createCopyIntent(newArrayList(GAMMA_DOC), DELTA_DOC));

        mCopyJobs.get(0).fail(ALPHA_DOC);

        mExecutor.runAll();
        shutdownService();

        assertExecutorsShutdown();
    }

    public void testShutdownStopsExecutor_AfterTotalFailure() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));
        startService(createCopyIntent(newArrayList(GAMMA_DOC), DELTA_DOC));

        mCopyJobs.get(0).fail(ALPHA_DOC);
        mCopyJobs.get(1).fail(GAMMA_DOC);

        mExecutor.runAll();
        shutdownService();

        assertExecutorsShutdown();
    }

    public void testRunsInForeground_MultipleJobs() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));
        startService(createCopyIntent(newArrayList(GAMMA_DOC), DELTA_DOC));

        mExecutor.run(0);
        mForegroundManager.assertInForeground();

        mHandler.dispatchAllMessages();
        mForegroundManager.assertInForeground();
    }

    public void testFinishesInBackground_MultipleJobs() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));
        startService(createCopyIntent(newArrayList(GAMMA_DOC), DELTA_DOC));

        mExecutor.run(0);
        mForegroundManager.assertInForeground();

        mHandler.dispatchAllMessages();
        mForegroundManager.assertInForeground();

        mExecutor.run(0);
        mHandler.dispatchAllMessages();
        mForegroundManager.assertInBackground();
    }

    public void testAllNotificationsDismissedAfterShutdown() throws Exception {
        startService(createCopyIntent(newArrayList(ALPHA_DOC), BETA_DOC));
        startService(createCopyIntent(newArrayList(GAMMA_DOC), DELTA_DOC));

        mExecutor.runAll();

        mHandler.dispatchAllMessages();
        mTestNotificationManager.assertNumberOfNotifications(0);
    }

    private Intent createCopyIntent(ArrayList<DocumentInfo> files, DocumentInfo dest)
            throws Exception {
        DocumentStack stack = new DocumentStack();
        stack.push(dest);

        List<Uri> uris = new ArrayList<>(files.size());
        for (DocumentInfo file: files) {
            uris.add(file.derivedUri);
        }

        UrisSupplier urisSupplier = DocsProviders.createDocsProvider(uris);
        TestFileOperation operation = new TestFileOperation(OPERATION_COPY, urisSupplier, stack);

        return createBaseIntent(getContext(), createJobId(), operation);
    }

    private Intent createDeleteIntent(ArrayList<DocumentInfo> files) {
        DocumentStack stack = new DocumentStack();

        List<Uri> uris = new ArrayList<>(files.size());
        for (DocumentInfo file: files) {
            uris.add(file.derivedUri);
        }

        UrisSupplier urisSupplier = DocsProviders.createDocsProvider(uris);
        TestFileOperation operation = new TestFileOperation(OPERATION_DELETE, urisSupplier, stack);

        return createBaseIntent(getContext(), createJobId(), operation);
    }

    private static DocumentInfo createDoc(String name) {
        // Doesn't need to be valid content Uri, just some urly looking thing.
        Uri uri = new Uri.Builder()
                .scheme("content")
                .authority("com.android.documentsui.testing")
                .path(name)
                .build();

        return createDoc(uri);
    }

    void assertAllCopyJobsStarted() {
        for (TestJob job : mCopyJobs) {
            job.assertStarted();
        }
    }

    void assertAllDeleteJobsStarted() {
        for (TestJob job : mDeleteJobs) {
            job.assertStarted();
        }
    }

    void assertNoCopyJobsStarted() {
        for (TestJob job : mCopyJobs) {
            job.assertNotStarted();
        }
    }

    void assertNoDeleteJobsStarted() {
        for (TestJob job : mDeleteJobs) {
            job.assertNotStarted();
        }
    }

    void assertJobsCreated(int expected) {
        assertEquals(expected, mCopyJobs.size() + mDeleteJobs.size());
    }
    private static DocumentInfo createDoc(Uri destination) {
        DocumentInfo destDoc = new DocumentInfo();
        destDoc.derivedUri = destination;
        return destDoc;
    }

    private void assertExecutorsShutdown() {
        mExecutor.assertShutdown();
        mDeletionExecutor.assertShutdown();
    }

    private final class TestFileOperation extends FileOperation {

        private final Runnable mJobRunnable = () -> {
            // The following statement is executed concurrently to Job.start() in real situation.
            // Call it in TestJob.start() to mimic this behavior.
            mHandler.dispatchNextMessage();
        };
        private final @OpType int mOpType;
        private final UrisSupplier mSrcs;
        private final DocumentStack mDestination;

        private TestFileOperation(
                @OpType int opType, UrisSupplier srcs, DocumentStack destination) {
            super(opType, srcs, destination);
            mOpType = opType;
            mSrcs = srcs;
            mDestination = destination;
        }

        @Override
        public Job createJob(Context service, Job.Listener listener, String id, Features features) {
            TestJob job = new TestJob(
                    service, listener, id, mOpType, mDestination, mSrcs, mJobRunnable, features);

            if (mOpType == OPERATION_COPY) {
                mCopyJobs.add(job);
            }

            if (mOpType == OPERATION_DELETE) {
                mDeleteJobs.add(job);
            }

            return job;
        }

        /**
         * CREATOR is required for Parcelables, but we never pass this class via parcel.
         */
        public Parcelable.Creator<TestFileOperation> CREATOR =
                new Parcelable.Creator<TestFileOperation>() {

            @Override
            public TestFileOperation createFromParcel(Parcel source) {
                throw new UnsupportedOperationException("Can't create from a parcel.");
            }

            @Override
            public TestFileOperation[] newArray(int size) {
                throw new UnsupportedOperationException("Can't create a new array.");
            }
        };
    }
}
