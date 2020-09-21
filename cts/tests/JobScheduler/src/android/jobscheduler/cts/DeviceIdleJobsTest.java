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
 * limitations under the License
 */

package android.jobscheduler.cts;

import static android.jobscheduler.cts.jobtestapp.TestJobService.ACTION_JOB_STARTED;
import static android.jobscheduler.cts.jobtestapp.TestJobService.ACTION_JOB_STOPPED;
import static android.jobscheduler.cts.jobtestapp.TestJobService.JOB_PARAMS_EXTRA_KEY;
import static android.os.PowerManager.ACTION_DEVICE_IDLE_MODE_CHANGED;
import static android.os.PowerManager.ACTION_LIGHT_DEVICE_IDLE_MODE_CHANGED;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.app.job.JobParameters;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.jobscheduler.cts.jobtestapp.TestActivity;
import android.jobscheduler.cts.jobtestapp.TestJobSchedulerReceiver;
import android.os.PowerManager;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.uiautomator.UiDevice;
import android.util.Log;

import com.android.compatibility.common.util.AppStandbyUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests that temp whitelisted apps can run jobs if all the other constraints are met
 */
@RunWith(AndroidJUnit4.class)
@LargeTest
public class DeviceIdleJobsTest {
    private static final String TAG = DeviceIdleJobsTest.class.getSimpleName();
    private static final String TEST_APP_PACKAGE = "android.jobscheduler.cts.jobtestapp";
    private static final String TEST_APP_RECEIVER = TEST_APP_PACKAGE + ".TestJobSchedulerReceiver";
    private static final String TEST_APP_ACTIVITY = TEST_APP_PACKAGE + ".TestActivity";
    private static final long BACKGROUND_JOBS_EXPECTED_DELAY = 3_000;
    private static final long POLL_INTERVAL = 500;
    private static final long DEFAULT_WAIT_TIMEOUT = 1000;
    private static final long SHELL_TIMEOUT = 3_000;

    enum Bucket {
        ACTIVE,
        WORKING_SET,
        FREQUENT,
        RARE,
        NEVER
    }

    private Context mContext;
    private UiDevice mUiDevice;
    private PowerManager mPowerManager;
    private long mTempWhitelistExpiryElapsed;
    private int mTestJobId;
    private int mTestPackageUid;
    private boolean mDeviceInDoze;
    private boolean mDeviceIdleEnabled;
    private boolean mAppStandbyEnabled;

    /* accesses must be synchronized on itself */
    private final TestJobStatus mTestJobStatus = new TestJobStatus();
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "Received action " + intent.getAction());
            switch (intent.getAction()) {
                case ACTION_JOB_STARTED:
                case ACTION_JOB_STOPPED:
                    final JobParameters params = intent.getParcelableExtra(JOB_PARAMS_EXTRA_KEY);
                    Log.d(TAG, "JobId: " + params.getJobId());
                    synchronized (mTestJobStatus) {
                        mTestJobStatus.running = ACTION_JOB_STARTED.equals(intent.getAction());
                        mTestJobStatus.jobId = params.getJobId();
                    }
                    break;
                case ACTION_DEVICE_IDLE_MODE_CHANGED:
                case ACTION_LIGHT_DEVICE_IDLE_MODE_CHANGED:
                    synchronized (DeviceIdleJobsTest.this) {
                        mDeviceInDoze = mPowerManager.isDeviceIdleMode();
                        Log.d(TAG, "mDeviceInDoze: " + mDeviceInDoze);
                    }
                    break;
            }
        }
    };

    private static boolean isDeviceIdleEnabled(UiDevice uiDevice) throws Exception {
        final String output = uiDevice.executeShellCommand("cmd deviceidle enabled deep").trim();
        return Integer.parseInt(output) != 0;
    }

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getTargetContext();
        mUiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mPowerManager = mContext.getSystemService(PowerManager.class);
        mDeviceInDoze = mPowerManager.isDeviceIdleMode();
        mTestPackageUid = mContext.getPackageManager().getPackageUid(TEST_APP_PACKAGE, 0);
        mTestJobId = (int) (SystemClock.uptimeMillis() / 1000);
        mTestJobStatus.reset();
        mTempWhitelistExpiryElapsed = -1;
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(ACTION_JOB_STARTED);
        intentFilter.addAction(ACTION_JOB_STOPPED);
        intentFilter.addAction(ACTION_DEVICE_IDLE_MODE_CHANGED);
        intentFilter.addAction(ACTION_LIGHT_DEVICE_IDLE_MODE_CHANGED);
        mContext.registerReceiver(mReceiver, intentFilter);
        assertFalse("Test package already in temp whitelist", isTestAppTempWhitelisted());
        makeTestPackageIdle();
        mDeviceIdleEnabled = isDeviceIdleEnabled(mUiDevice);
        mAppStandbyEnabled = AppStandbyUtils.isAppStandbyEnabled();
        if (mAppStandbyEnabled) {
            setTestPackageStandbyBucket(Bucket.ACTIVE);
        } else {
            Log.w(TAG, "App standby not enabled on test device");
        }
    }

    @Test
    public void testAllowWhileIdleJobInTempwhitelist() throws Exception {
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);

        toggleDeviceIdleState(true);
        Thread.sleep(DEFAULT_WAIT_TIMEOUT);
        sendScheduleJobBroadcast(true);
        assertFalse("Job started without being tempwhitelisted", awaitJobStart(5_000));
        tempWhitelistTestApp(5_000);
        assertTrue("Job with allow_while_idle flag did not start when the app was tempwhitelisted",
                awaitJobStart(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testForegroundJobsStartImmediately() throws Exception {
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);

        sendScheduleJobBroadcast(false);
        assertTrue("Job did not start after scheduling", awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        toggleDeviceIdleState(true);
        assertTrue("Job did not stop on entering doze", awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        Thread.sleep(TestJobSchedulerReceiver.JOB_INITIAL_BACKOFF);
        startAndKeepTestActivity();
        toggleDeviceIdleState(false);
        assertTrue("Job for foreground app did not start immediately when device exited doze",
                awaitJobStart(3_000));
    }

    @Test
    public void testBackgroundJobsDelayed() throws Exception {
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);

        sendScheduleJobBroadcast(false);
        assertTrue("Job did not start after scheduling", awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        toggleDeviceIdleState(true);
        assertTrue("Job did not stop on entering doze", awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        Thread.sleep(TestJobSchedulerReceiver.JOB_INITIAL_BACKOFF);
        toggleDeviceIdleState(false);
        assertFalse("Job for background app started immediately when device exited doze",
                awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        Thread.sleep(BACKGROUND_JOBS_EXPECTED_DELAY - DEFAULT_WAIT_TIMEOUT);
        assertTrue("Job for background app did not start after the expected delay of "
                + BACKGROUND_JOBS_EXPECTED_DELAY + "ms", awaitJobStart(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testJobsInNeverApp() throws Exception {
        assumeTrue("app standby not enabled", mAppStandbyEnabled);

        enterFakeUnpluggedState();
        setTestPackageStandbyBucket(Bucket.NEVER);
        Thread.sleep(DEFAULT_WAIT_TIMEOUT);
        sendScheduleJobBroadcast(false);
        assertFalse("New job started in NEVER standby", awaitJobStart(3_000));
        resetFakeUnpluggedState();
    }

    @Test
    public void testUidActiveBypassesStandby() throws Exception {
        enterFakeUnpluggedState();
        setTestPackageStandbyBucket(Bucket.NEVER);
        tempWhitelistTestApp(6_000);
        Thread.sleep(DEFAULT_WAIT_TIMEOUT);
        sendScheduleJobBroadcast(false);
        assertTrue("New job in uid-active app failed to start in NEVER standby",
                awaitJobStart(4_000));
        resetFakeUnpluggedState();
    }

    @After
    public void tearDown() throws Exception {
        if (mDeviceIdleEnabled) {
            toggleDeviceIdleState(false);
        }
        final Intent cancelJobsIntent = new Intent(TestJobSchedulerReceiver.ACTION_CANCEL_JOBS);
        cancelJobsIntent.setComponent(new ComponentName(TEST_APP_PACKAGE, TEST_APP_RECEIVER));
        cancelJobsIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.sendBroadcast(cancelJobsIntent);
        mContext.sendBroadcast(new Intent(TestActivity.ACTION_FINISH_ACTIVITY));
        mContext.unregisterReceiver(mReceiver);
        Thread.sleep(500); // To avoid any race between unregister and the next register in setUp
        waitUntilTestAppNotInTempWhitelist();
    }

    private boolean isTestAppTempWhitelisted() throws Exception {
        final String output = mUiDevice.executeShellCommand("cmd deviceidle tempwhitelist").trim();
        for (String line : output.split("\n")) {
            if (line.contains("UID="+mTestPackageUid)) {
                return true;
            }
        }
        return false;
    }

    private void startAndKeepTestActivity() {
        final Intent testActivity = new Intent();
        testActivity.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        testActivity.setComponent(new ComponentName(TEST_APP_PACKAGE, TEST_APP_ACTIVITY));
        mContext.startActivity(testActivity);
    }

    private void sendScheduleJobBroadcast(boolean allowWhileIdle) throws Exception {
        final Intent scheduleJobIntent = new Intent(TestJobSchedulerReceiver.ACTION_SCHEDULE_JOB);
        scheduleJobIntent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);
        scheduleJobIntent.putExtra(TestJobSchedulerReceiver.EXTRA_JOB_ID_KEY, mTestJobId);
        scheduleJobIntent.putExtra(TestJobSchedulerReceiver.EXTRA_ALLOW_IN_IDLE, allowWhileIdle);
        scheduleJobIntent.setComponent(new ComponentName(TEST_APP_PACKAGE, TEST_APP_RECEIVER));
        mContext.sendBroadcast(scheduleJobIntent);
    }

    private void toggleDeviceIdleState(final boolean idle) throws Exception {
        mUiDevice.executeShellCommand("cmd deviceidle " + (idle ? "force-idle" : "unforce"));
        assertTrue("Could not change device idle state to " + idle,
                waitUntilTrue(SHELL_TIMEOUT, () -> {
                    synchronized (DeviceIdleJobsTest.this) {
                        return mDeviceInDoze == idle;
                    }
                }));
    }

    private void tempWhitelistTestApp(long duration) throws Exception {
        mUiDevice.executeShellCommand("cmd deviceidle tempwhitelist -d " + duration
                + " " + TEST_APP_PACKAGE);
        mTempWhitelistExpiryElapsed = SystemClock.elapsedRealtime() + duration;
    }

    private void makeTestPackageIdle() throws Exception {
        mUiDevice.executeShellCommand("am make-uid-idle --user current " + TEST_APP_PACKAGE);
    }

    private void setTestPackageStandbyBucket(Bucket bucket) throws Exception {
        final String bucketName;
        switch (bucket) {
            case ACTIVE: bucketName = "active"; break;
            case WORKING_SET: bucketName = "working"; break;
            case FREQUENT: bucketName = "frequent"; break;
            case RARE: bucketName = "rare"; break;
            case NEVER: bucketName = "never"; break;
            default:
                throw new IllegalArgumentException("Requested unknown bucket " + bucket);
        }
        mUiDevice.executeShellCommand("am set-standby-bucket " + TEST_APP_PACKAGE
                + " " + bucketName);
    }

    private void enterFakeUnpluggedState() throws Exception {
        mUiDevice.executeShellCommand("dumpsys battery unplug");
    }

    private void resetFakeUnpluggedState() throws Exception  {
        mUiDevice.executeShellCommand("dumpsys battery reset");
    }

    private boolean waitUntilTestAppNotInTempWhitelist() throws Exception {
        long now;
        boolean interrupted = false;
        while ((now = SystemClock.elapsedRealtime()) < mTempWhitelistExpiryElapsed) {
            try {
                Thread.sleep(mTempWhitelistExpiryElapsed - now);
            } catch (InterruptedException iexc) {
                interrupted = true;
            }
        }
        if (interrupted) {
            Thread.currentThread().interrupt();
        }
        return waitUntilTrue(SHELL_TIMEOUT, () -> !isTestAppTempWhitelisted());
    }

    private boolean awaitJobStart(long maxWait) throws Exception {
        return waitUntilTrue(maxWait, () -> {
            synchronized (mTestJobStatus) {
                return (mTestJobStatus.jobId == mTestJobId) && mTestJobStatus.running;
            }
        });
    }

    private boolean awaitJobStop(long maxWait) throws Exception {
        return waitUntilTrue(maxWait, () -> {
            synchronized (mTestJobStatus) {
                return (mTestJobStatus.jobId == mTestJobId) && !mTestJobStatus.running;
            }
        });
    }

    private boolean waitUntilTrue(long maxWait, Condition condition) throws Exception {
        final long deadLine = SystemClock.uptimeMillis() + maxWait;
        do {
            Thread.sleep(POLL_INTERVAL);
        } while (!condition.isTrue() && SystemClock.uptimeMillis() < deadLine);
        return condition.isTrue();
    }

    private static final class TestJobStatus {
        int jobId;
        boolean running;
        private void reset() {
            running = false;
        }
    }

    private interface Condition {
        boolean isTrue() throws Exception;
    }
}
