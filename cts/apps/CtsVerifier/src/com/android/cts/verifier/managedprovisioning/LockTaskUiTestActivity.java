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

package com.android.cts.verifier.managedprovisioning;

import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_GLOBAL_ACTIONS;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_HOME;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_KEYGUARD;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_NONE;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_NOTIFICATIONS;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_OVERVIEW;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_SYSTEM_INFO;

import static com.android.cts.verifier.managedprovisioning.Utils.createInteractiveTestItem;

import android.app.ActivityManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.DataSetObserver;
import android.os.AsyncTask;
import android.os.Bundle;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import android.util.Log;
import android.widget.Button;
import android.widget.Toast;

import com.android.cts.verifier.ArrayTestListAdapter;
import com.android.cts.verifier.IntentDrivenTestActivity.ButtonInfo;
import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;
import com.android.cts.verifier.TestListAdapter.TestListItem;
import com.android.cts.verifier.TestResult;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Tests for {@link DevicePolicyManager#setLockTaskFeatures(ComponentName, int)}.
 */
public class LockTaskUiTestActivity extends PassFailButtons.TestListActivity {

    private static final String TAG = LockTaskUiTestActivity.class.getSimpleName();

    public static final String EXTRA_TEST_ID =
            "com.android.cts.verifier.managedprovisioning.extra.TEST_ID";

    /** Broadcast action sent by {@link DeviceAdminTestReceiver} when LockTask starts. */
    static final String ACTION_LOCK_TASK_STARTED =
            "com.android.cts.verifier.managedprovisioning.action.LOCK_TASK_STARTED";
    /** Broadcast action sent by {@link DeviceAdminTestReceiver} when LockTask stops. */
    static final String ACTION_LOCK_TASK_STOPPED =
            "com.android.cts.verifier.managedprovisioning.action.LOCK_TASK_STOPPED";

    private static final ComponentName ADMIN_RECEIVER =
            DeviceAdminTestReceiver.getReceiverComponentName();
    private static final String TEST_PACKAGE_NAME = "com.android.cts.verifier";
    private static final String ACTION_STOP_LOCK_TASK =
            "com.android.cts.verifier.managedprovisioning.action.STOP_LOCK_TASK";

    private static final String TEST_ID_DEFAULT = "lock-task-ui-default";
    private static final String TEST_ID_SYSTEM_INFO = "lock-task-ui-system-info";
    private static final String TEST_ID_NOTIFICATIONS = "lock-task-ui-notifications";
    private static final String TEST_ID_HOME = "lock-task-ui-home";
    private static final String TEST_ID_RECENTS = "lock-task-ui-recents";
    private static final String TEST_ID_GLOBAL_ACTIONS = "lock-task-ui-global-actions";
    private static final String TEST_ID_KEYGUARD = "lock-task-ui-keyguard";
    private static final String TEST_ID_STOP_LOCK_TASK = "lock-task-ui-stop-lock-task";

    private DevicePolicyManager mDpm;
    private ActivityManager mAm;
    private NotificationManager mNotifyMgr;

    private LockTaskStateChangedReceiver mStateChangedReceiver;
    private CountDownLatch mLockTaskStartedLatch;
    private CountDownLatch mLockTaskStoppedLatch;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.device_owner_lock_task_ui);
        setPassFailButtonClickListeners();

        mDpm = getSystemService(DevicePolicyManager.class);
        mAm = getSystemService(ActivityManager.class);
        mNotifyMgr = getSystemService(NotificationManager.class);

        final ArrayTestListAdapter adapter = new ArrayTestListAdapter(this);
        addTestsToAdapter(adapter);
        adapter.registerDataSetObserver(new DataSetObserver() {
            @Override
            public void onChanged() {
                updatePassButton();
            }
        });
        setTestListAdapter(adapter);

        Button startLockTaskButton = findViewById(R.id.start_lock_task_button);
        startLockTaskButton.setOnClickListener((view) -> startLockTaskMode());

        if (ACTION_STOP_LOCK_TASK.equals(getIntent().getAction())) {
            // This means we're started by the "stop LockTask mode" test activity (the last one in
            // the list) in order to stop LockTask.
            stopLockTaskMode();
        }
    }

    private void addTestsToAdapter(final ArrayTestListAdapter adapter) {
        adapter.add(createSetLockTaskFeaturesTest(
                TEST_ID_DEFAULT,
                LOCK_TASK_FEATURE_NONE,
                R.string.device_owner_lock_task_ui_default_test,
                R.string.device_owner_lock_task_ui_default_test_info));

        adapter.add(createSetLockTaskFeaturesTest(
                TEST_ID_SYSTEM_INFO,
                LOCK_TASK_FEATURE_SYSTEM_INFO,
                R.string.device_owner_lock_task_ui_system_info_test,
                R.string.device_owner_lock_task_ui_system_info_test_info));

        adapter.add(createSetLockTaskFeaturesTest(
                TEST_ID_NOTIFICATIONS,
                LOCK_TASK_FEATURE_HOME | LOCK_TASK_FEATURE_NOTIFICATIONS,
                R.string.device_owner_lock_task_ui_notifications_test,
                R.string.device_owner_lock_task_ui_notifications_test_info));

        adapter.add(createSetLockTaskFeaturesTest(
                TEST_ID_HOME,
                LOCK_TASK_FEATURE_HOME,
                R.string.device_owner_lock_task_ui_home_test,
                R.string.device_owner_lock_task_ui_home_test_info));

        adapter.add(createSetLockTaskFeaturesTest(
                TEST_ID_RECENTS,
                LOCK_TASK_FEATURE_HOME | LOCK_TASK_FEATURE_OVERVIEW,
                R.string.device_owner_lock_task_ui_recents_test,
                R.string.device_owner_lock_task_ui_recents_test_info));

        adapter.add(createSetLockTaskFeaturesTest(
                TEST_ID_GLOBAL_ACTIONS,
                LOCK_TASK_FEATURE_GLOBAL_ACTIONS,
                R.string.device_owner_lock_task_ui_global_actions_test,
                R.string.device_owner_lock_task_ui_global_actions_test_info));

        adapter.add(createSetLockTaskFeaturesTest(
                TEST_ID_KEYGUARD,
                LOCK_TASK_FEATURE_KEYGUARD,
                R.string.device_owner_lock_task_ui_keyguard_test,
                R.string.device_owner_lock_task_ui_keyguard_test_info));

        final Intent stopLockTaskIntent = new Intent(this, LockTaskUiTestActivity.class);
        stopLockTaskIntent.setAction(ACTION_STOP_LOCK_TASK);
        adapter.add(createInteractiveTestItem(this,
                TEST_ID_STOP_LOCK_TASK,
                R.string.device_owner_lock_task_ui_stop_lock_task_test,
                R.string.device_owner_lock_task_ui_stop_lock_task_test_info,
                new ButtonInfo(
                        R.string.device_owner_lock_task_ui_stop_lock_task_test,
                        stopLockTaskIntent
                )));
    }

    /** Receives LockTask start/stop callbacks forwarded by {@link DeviceAdminTestReceiver}. */
    private final class LockTaskStateChangedReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            switch (action) {
                case ACTION_LOCK_TASK_STARTED:
                    if (mLockTaskStartedLatch != null) {
                        mLockTaskStartedLatch.countDown();
                    }
                    break;
                case ACTION_LOCK_TASK_STOPPED:
                    if (mLockTaskStoppedLatch != null) {
                        mLockTaskStoppedLatch.countDown();
                    }
                    break;
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        mStateChangedReceiver = new LockTaskStateChangedReceiver();
        final IntentFilter filter = new IntentFilter();
        filter.addAction(ACTION_LOCK_TASK_STARTED);
        filter.addAction(ACTION_LOCK_TASK_STOPPED);
        LocalBroadcastManager.getInstance(this).registerReceiver(mStateChangedReceiver, filter);
    }

    @Override
    protected void onPause() {
        if (mStateChangedReceiver != null) {
            LocalBroadcastManager.getInstance(this).unregisterReceiver(mStateChangedReceiver);
            mStateChangedReceiver = null;
        }
        super.onPause();
    }

    /**
     * Starts LockTask mode and waits for callback from {@link DeviceAdminTestReceiver} to confirm
     * LockTask has started successfully. If the callback isn't received, the entire test will be
     * marked as failed.
     *
     * @see LockTaskStateChangedReceiver
     */
    private void startLockTaskMode() {
        if (mAm.getLockTaskModeState() == ActivityManager.LOCK_TASK_MODE_LOCKED) {
            return;
        }

        mLockTaskStartedLatch = new CountDownLatch(1);
        try {
            mDpm.setLockTaskPackages(ADMIN_RECEIVER, new String[] {TEST_PACKAGE_NAME});
            mDpm.setLockTaskFeatures(ADMIN_RECEIVER, LOCK_TASK_FEATURE_NONE);
            startLockTask();

            new CheckLockTaskStateTask() {
                @Override
                protected void onPostExecute(Boolean success) {
                    if (success) {
                        issueTestNotification();
                    } else {
                        notifyFailure(getTestId(), "Failed to start LockTask mode");
                    }
                }
            }.execute(mLockTaskStartedLatch);
        } catch (SecurityException e) {
            Log.e(TAG, e.getMessage(), e);
            Toast.makeText(this, "Failed to run test. Did you set up device owner correctly?",
                    Toast.LENGTH_SHORT).show();
        }
    }

    /**
     * Stops LockTask mode and waits for callback from {@link DeviceAdminTestReceiver} to confirm
     * LockTask has stopped successfully. If the callback isn't received, the "Stop LockTask mode"
     * test case will be marked as failed.
     *
     * Note that we {@link #finish()} this activity here, since it's started by the "Stop LockTask
     * mode" test activity, and shouldn't be exposed to the tester once its job is done.
     *
     * @see LockTaskStateChangedReceiver
     */
    private void stopLockTaskMode() {
        if (mAm.getLockTaskModeState() == ActivityManager.LOCK_TASK_MODE_NONE) {
            finish();
            return;
        }

        mLockTaskStoppedLatch = new CountDownLatch(1);
        try {
            stopLockTask();

            new CheckLockTaskStateTask() {
                @Override
                protected void onPostExecute(Boolean success) {
                    if (!success) {
                        notifyFailure(TEST_ID_STOP_LOCK_TASK, "Failed to stop LockTask mode");
                    }
                    cancelTestNotification();
                    mDpm.setLockTaskFeatures(ADMIN_RECEIVER, LOCK_TASK_FEATURE_NONE);
                    mDpm.setLockTaskPackages(ADMIN_RECEIVER, new String[] {});
                    LockTaskUiTestActivity.this.finish();
                }
            }.execute(mLockTaskStoppedLatch);
        } catch (SecurityException e) {
            Log.e(TAG, e.getMessage(), e);
            Toast.makeText(this, "Failed to finish test. Did you set up device owner correctly?",
                    Toast.LENGTH_SHORT).show();
        }
    }

    private abstract class CheckLockTaskStateTask extends AsyncTask<CountDownLatch, Void, Boolean> {
        @Override
        protected Boolean doInBackground(CountDownLatch... latches) {
            if (latches.length > 0 && latches[0] != null) {
                try {
                    return latches[0].await(1, TimeUnit.SECONDS);
                } catch (InterruptedException e) {
                    // Fall through
                }
            }
            return false;
        }

        @Override
        protected abstract void onPostExecute(Boolean success);
    }

    private void notifyFailure(String testId, String message) {
        Log.e(TAG, message);
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
        TestResult.setFailedResult(this, testId, message);
    }

    private void issueTestNotification() {
        String channelId = getTestId();
        if (mNotifyMgr.getNotificationChannel(channelId) == null) {
            NotificationChannel channel = new NotificationChannel(
                    channelId, getTestId(), NotificationManager.IMPORTANCE_HIGH);
            mNotifyMgr.createNotificationChannel(channel);
        }

        Notification note = new Notification.Builder(this, channelId)
                .setContentTitle(getString(R.string.device_owner_lock_task_ui_test))
                .setSmallIcon(android.R.drawable.sym_def_app_icon)
                .setOngoing(true)
                .build();

        mNotifyMgr.notify(0, note);
    }

    private void cancelTestNotification() {
        mNotifyMgr.cancelAll();
    }

    private TestListItem createSetLockTaskFeaturesTest(String testId, int featureFlags,
            int titleResId, int detailResId) {
        final Intent commandIntent = new Intent(CommandReceiverActivity.ACTION_EXECUTE_COMMAND);
        commandIntent.putExtra(CommandReceiverActivity.EXTRA_COMMAND,
                CommandReceiverActivity.COMMAND_SET_LOCK_TASK_FEATURES);
        commandIntent.putExtra(CommandReceiverActivity.EXTRA_VALUE, featureFlags);

        return createInteractiveTestItem(this, testId, titleResId, detailResId,
                new ButtonInfo(titleResId, commandIntent));
    }

    @Override
    public String getTestId() {
        return getIntent().getStringExtra(EXTRA_TEST_ID);
    }
}
