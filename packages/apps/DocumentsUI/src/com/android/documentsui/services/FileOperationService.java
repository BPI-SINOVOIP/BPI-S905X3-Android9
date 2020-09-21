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

import static com.android.documentsui.base.SharedMinimal.DEBUG;

import android.annotation.IntDef;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.UserManager;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import com.android.documentsui.R;
import com.android.documentsui.base.Features;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicReference;

import javax.annotation.concurrent.GuardedBy;

public class FileOperationService extends Service implements Job.Listener {

    public static final String TAG = "FileOperationService";

    // Extra used for OperationDialogFragment, Notifications and picking copy destination.
    public static final String EXTRA_OPERATION_TYPE = "com.android.documentsui.OPERATION_TYPE";

    // Extras used for OperationDialogFragment...
    public static final String EXTRA_DIALOG_TYPE = "com.android.documentsui.DIALOG_TYPE";
    public static final String EXTRA_SRC_LIST = "com.android.documentsui.SRC_LIST";

    public static final String EXTRA_FAILED_URIS = "com.android.documentsui.FAILED_URIS";
    public static final String EXTRA_FAILED_DOCS = "com.android.documentsui.FAILED_DOCS";

    // Extras used to start or cancel a file operation...
    public static final String EXTRA_JOB_ID = "com.android.documentsui.JOB_ID";
    public static final String EXTRA_OPERATION = "com.android.documentsui.OPERATION";
    public static final String EXTRA_CANCEL = "com.android.documentsui.CANCEL";

    @IntDef({
            OPERATION_UNKNOWN,
            OPERATION_COPY,
            OPERATION_COMPRESS,
            OPERATION_EXTRACT,
            OPERATION_MOVE,
            OPERATION_DELETE
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface OpType {}
    public static final int OPERATION_UNKNOWN = -1;
    public static final int OPERATION_COPY = 1;
    public static final int OPERATION_EXTRACT = 2;
    public static final int OPERATION_COMPRESS = 3;
    public static final int OPERATION_MOVE = 4;
    public static final int OPERATION_DELETE = 5;

    @IntDef({
            MESSAGE_PROGRESS,
            MESSAGE_FINISH
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface MessageType {}
    public static final int MESSAGE_PROGRESS = 0;
    public static final int MESSAGE_FINISH = 1;

    // TODO: Move it to a shared file when more operations are implemented.
    public static final int FAILURE_COPY = 1;

    static final String NOTIFICATION_CHANNEL_ID = "channel_id";

    private static final int POOL_SIZE = 2;  // "pool size", not *max* "pool size".

    private static final int NOTIFICATION_ID_PROGRESS = 0;
    private static final int NOTIFICATION_ID_FAILURE = 1;
    private static final int NOTIFICATION_ID_WARNING = 2;

    // The executor and job factory are visible for testing and non-final
    // so we'll have a way to inject test doubles from the test. It's
    // a sub-optimal arrangement.
    @VisibleForTesting ExecutorService executor;

    // Use a separate thread pool to prioritize deletions.
    @VisibleForTesting ExecutorService deletionExecutor;

    // Use a handler to schedule monitor tasks.
    @VisibleForTesting Handler handler;

    // Use a foreground manager to change foreground state of this service.
    @VisibleForTesting ForegroundManager foregroundManager;

    // Use a notification manager to post and cancel notifications for jobs.
    @VisibleForTesting NotificationManager notificationManager;

    // Use a features to determine if notification channel is enabled.
    @VisibleForTesting Features features;

    @GuardedBy("mJobs")
    private final Map<String, JobRecord> mJobs = new HashMap<>();

    // The job whose notification is used to keep the service in foreground mode.
    private final AtomicReference<Job> mForegroundJob = new AtomicReference<>();

    private PowerManager mPowerManager;
    private PowerManager.WakeLock mWakeLock;  // the wake lock, if held.

    private int mLastServiceId;

    @Override
    public void onCreate() {
        // Allow tests to pre-set these with test doubles.
        if (executor == null) {
            executor = Executors.newFixedThreadPool(POOL_SIZE);
        }

        if (deletionExecutor == null) {
            deletionExecutor = Executors.newCachedThreadPool();
        }

        if (handler == null) {
            // Monitor tasks are small enough to schedule them on main thread.
            handler = new Handler();
        }

        if (foregroundManager == null) {
            foregroundManager = createForegroundManager(this);
        }

        if (notificationManager == null) {
            notificationManager = getSystemService(NotificationManager.class);
        }

        features = new Features.RuntimeFeatures(getResources(), UserManager.get(this));
        setUpNotificationChannel();

        if (DEBUG) Log.d(TAG, "Created.");
        mPowerManager = getSystemService(PowerManager.class);
    }

    private void setUpNotificationChannel() {
        if (features.isNotificationChannelEnabled()) {
            NotificationChannel channel = new NotificationChannel(
                    NOTIFICATION_CHANNEL_ID,
                    getString(R.string.app_label),
                    NotificationManager.IMPORTANCE_LOW);
            notificationManager.createNotificationChannel(channel);
        }
    }

    @Override
    public void onDestroy() {
        if (DEBUG) Log.d(TAG, "Shutting down executor.");

        List<Runnable> unfinishedCopies = executor.shutdownNow();
        List<Runnable> unfinishedDeletions = deletionExecutor.shutdownNow();
        List<Runnable> unfinished =
                new ArrayList<>(unfinishedCopies.size() + unfinishedDeletions.size());
        unfinished.addAll(unfinishedCopies);
        unfinished.addAll(unfinishedDeletions);
        if (!unfinished.isEmpty()) {
            Log.w(TAG, "Shutting down, but executor reports running jobs: " + unfinished);
        }

        executor = null;
        deletionExecutor = null;
        handler = null;

        if (DEBUG) Log.d(TAG, "Destroyed.");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int serviceId) {
        // TODO: Ensure we're not being called with retry or redeliver.
        // checkArgument(flags == 0);  // retry and redeliver are not supported.

        String jobId = intent.getStringExtra(EXTRA_JOB_ID);
        assert(jobId != null);

        if (DEBUG) Log.d(TAG, "onStartCommand: " + jobId + " with serviceId " + serviceId);

        if (intent.hasExtra(EXTRA_CANCEL)) {
            handleCancel(intent);
        } else {
            FileOperation operation = intent.getParcelableExtra(EXTRA_OPERATION);
            handleOperation(jobId, operation);
        }

        // Track the service supplied id so we can stop the service once we're out of work to do.
        mLastServiceId = serviceId;

        return START_NOT_STICKY;
    }

    private void handleOperation(String jobId, FileOperation operation) {
        synchronized (mJobs) {
            if (mWakeLock == null) {
                mWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);
            }

            if (mJobs.containsKey(jobId)) {
                Log.w(TAG, "Duplicate job id: " + jobId
                        + ". Ignoring job request for operation: " + operation + ".");
                return;
            }

            Job job = operation.createJob(this, this, jobId, features);

            if (job == null) {
                return;
            }

            assert (job != null);
            if (DEBUG) Log.d(TAG, "Scheduling job " + job.id + ".");
            Future<?> future = getExecutorService(operation.getOpType()).submit(job);
            mJobs.put(jobId, new JobRecord(job, future));

            // Acquire wake lock to keep CPU running until we finish all jobs. Acquire wake lock
            // after we create a job and put it in mJobs to avoid potential leaking of wake lock
            // in case where job creation fails.
            mWakeLock.acquire();
        }
    }

    /**
     * Cancels the operation corresponding to job id, identified in "EXTRA_JOB_ID".
     *
     * @param intent The cancellation intent.
     */
    private void handleCancel(Intent intent) {
        assert(intent.hasExtra(EXTRA_CANCEL));
        assert(intent.getStringExtra(EXTRA_JOB_ID) != null);

        String jobId = intent.getStringExtra(EXTRA_JOB_ID);

        if (DEBUG) Log.d(TAG, "handleCancel: " + jobId);

        synchronized (mJobs) {
            // Do nothing if the cancelled ID doesn't match the current job ID. This prevents racey
            // cancellation requests from affecting unrelated copy jobs.  However, if the current job ID
            // is null, the service most likely crashed and was revived by the incoming cancel intent.
            // In that case, always allow the cancellation to proceed.
            JobRecord record = mJobs.get(jobId);
            if (record != null) {
                record.job.cancel();
            }
        }

        // Dismiss the progress notification here rather than in the copy loop. This preserves
        // interactivity for the user in case the copy loop is stalled.
        // Try to cancel it even if we don't have a job id...in case there is some sad
        // orphan notification.
        notificationManager.cancel(jobId, NOTIFICATION_ID_PROGRESS);

        // TODO: Guarantee the job is being finalized
    }

    private ExecutorService getExecutorService(@OpType int operationType) {
        switch (operationType) {
            case OPERATION_COPY:
            case OPERATION_COMPRESS:
            case OPERATION_EXTRACT:
            case OPERATION_MOVE:
                return executor;
            case OPERATION_DELETE:
                return deletionExecutor;
            default:
                throw new UnsupportedOperationException();
        }
    }

    @GuardedBy("mJobs")
    private void deleteJob(Job job) {
        if (DEBUG) Log.d(TAG, "deleteJob: " + job.id);

        // Release wake lock before clearing jobs just in case we fail to clean them up.
        mWakeLock.release();
        if (!mWakeLock.isHeld()) {
            mWakeLock = null;
        }

        JobRecord record = mJobs.remove(job.id);
        assert(record != null);
        record.job.cleanup();

        // Delay the shutdown until we've cleaned up all notifications. shutdown() is now posted in
        // onFinished(Job job) to main thread.
    }

    /**
     * Most likely shuts down. Won't shut down if service has a pending
     * message. Thread pool is deal with in onDestroy.
     */
    private void shutdown() {
        if (DEBUG) Log.d(TAG, "Shutting down. Last serviceId was " + mLastServiceId);
        assert(mWakeLock == null);

        // Turns out, for us, stopSelfResult always returns false in tests,
        // so we can't guard executor shutdown. For this reason we move
        // executor shutdown to #onDestroy.
        boolean gonnaStop = stopSelfResult(mLastServiceId);
        if (DEBUG) Log.d(TAG, "Stopping service: " + gonnaStop);
        if (!gonnaStop) {
            Log.w(TAG, "Service should be stopping, but reports otherwise.");
        }
    }

    @VisibleForTesting
    boolean holdsWakeLock() {
        return mWakeLock != null && mWakeLock.isHeld();
    }

    @Override
    public void onStart(Job job) {
        if (DEBUG) Log.d(TAG, "onStart: " + job.id);

        Notification notification = job.getSetupNotification();
        // If there is no foreground job yet, set this job to foreground job.
        if (mForegroundJob.compareAndSet(null, job)) {
            if (DEBUG) Log.d(TAG, "Set foreground job to " + job.id);
            foregroundManager.startForeground(NOTIFICATION_ID_PROGRESS, notification);
        }

        // Show start up notification
        if (DEBUG) Log.d(TAG, "Posting notification for " + job.id);
        notificationManager.notify(
                job.id, NOTIFICATION_ID_PROGRESS, notification);

        // Set up related monitor
        JobMonitor monitor = new JobMonitor(job, notificationManager, handler, mJobs);
        monitor.start();
    }

    @Override
    public void onFinished(Job job) {
        assert(job.isFinished());
        if (DEBUG) Log.d(TAG, "onFinished: " + job.id);

        synchronized (mJobs) {
            // Delete the job from mJobs first to avoid this job being selected as the foreground
            // task again if we need to swap the foreground job.
            deleteJob(job);

            // Update foreground state before cleaning up notification. If the finishing job is the
            // foreground job, we would need to switch to another one or go to background before
            // we can clean up notifications.
            updateForegroundState(job);

            // Use the same thread of monitors to tackle notifications to avoid race conditions.
            // Otherwise we may fail to dismiss progress notification.
            handler.post(() -> cleanUpNotification(job));

            // Post the shutdown message to main thread after cleanUpNotification() to give it a
            // chance to run. Otherwise this process may be torn down by Android before we've
            // cleaned up the notifications of the last job.
            if (mJobs.isEmpty()) {
                handler.post(this::shutdown);
            }
        }
    }

    @GuardedBy("mJobs")
    private void updateForegroundState(Job job) {
        Job candidate = mJobs.isEmpty() ? null : mJobs.values().iterator().next().job;

        // If foreground job is retiring and there is still work to do, we need to set it to a new
        // job.
        if (mForegroundJob.compareAndSet(job, candidate)) {
            if (candidate == null) {
                if (DEBUG) Log.d(TAG, "Stop foreground");
                // Remove the notification here just in case we're torn down before we have the
                // chance to clean up notifications.
                foregroundManager.stopForeground(true);
            } else {
                if (DEBUG) Log.d(TAG, "Switch foreground job to " + candidate.id);

                Notification notification = (candidate.getState() == Job.STATE_STARTED)
                        ? candidate.getSetupNotification()
                        : candidate.getProgressNotification();
                foregroundManager.startForeground(NOTIFICATION_ID_PROGRESS, notification);
                notificationManager.notify(candidate.id, NOTIFICATION_ID_PROGRESS,
                        notification);
            }
        }
    }

    private void cleanUpNotification(Job job) {

        if (DEBUG) Log.d(TAG, "Canceling notification for " + job.id);
        // Dismiss the ongoing copy notification when the copy is done.
        notificationManager.cancel(job.id, NOTIFICATION_ID_PROGRESS);

        if (job.hasFailures()) {
            if (!job.failedUris.isEmpty()) {
                Log.e(TAG, "Job failed to resolve uris: " + job.failedUris + ".");
            }
            if (!job.failedDocs.isEmpty()) {
                Log.e(TAG, "Job failed to process docs: " + job.failedDocs + ".");
            }
            notificationManager.notify(
                    job.id, NOTIFICATION_ID_FAILURE, job.getFailureNotification());
        }

        if (job.hasWarnings()) {
            if (DEBUG) Log.d(TAG, "Job finished with warnings.");
            notificationManager.notify(
                    job.id, NOTIFICATION_ID_WARNING, job.getWarningNotification());
        }
    }

    private static final class JobRecord {
        private final Job job;
        private final Future<?> future;

        public JobRecord(Job job, Future<?> future) {
            this.job = job;
            this.future = future;
        }
    }

    /**
     * A class used to periodically polls state of a job.
     *
     * <p>It's possible that jobs hang because underlying document providers stop responding. We
     * still need to update notifications if jobs hang, so instead of jobs pushing their states,
     * we poll states of jobs.
     */
    private static final class JobMonitor implements Runnable {
        private static final long PROGRESS_INTERVAL_MILLIS = 500L;

        private final Job mJob;
        private final NotificationManager mNotificationManager;
        private final Handler mHandler;
        private final Object mJobsLock;

        private JobMonitor(Job job, NotificationManager notificationManager, Handler handler,
                Object jobsLock) {
            mJob = job;
            mNotificationManager = notificationManager;
            mHandler = handler;
            mJobsLock = jobsLock;
        }

        private void start() {
            mHandler.post(this);
        }

        @Override
        public void run() {
            synchronized (mJobsLock) {
                if (mJob.isFinished()) {
                    // Finish notification is already shown. Progress notification is removed.
                    // Just finish itself.
                    return;
                }

                // Only job in set up state has progress bar
                if (mJob.getState() == Job.STATE_SET_UP) {
                    mNotificationManager.notify(
                            mJob.id, NOTIFICATION_ID_PROGRESS, mJob.getProgressNotification());
                }

                mHandler.postDelayed(this, PROGRESS_INTERVAL_MILLIS);
            }
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;  // Boilerplate. See super#onBind
    }

    private static ForegroundManager createForegroundManager(final Service service) {
        return new ForegroundManager() {
            @Override
            public void startForeground(int id, Notification notification) {
                service.startForeground(id, notification);
            }

            @Override
            public void stopForeground(boolean removeNotification) {
                service.stopForeground(removeNotification);
            }
        };
    }

    @VisibleForTesting
    interface ForegroundManager {
        void startForeground(int id, Notification notification);
        void stopForeground(boolean removeNotification);
    }
}
