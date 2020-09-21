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

package android.alarmmanager.cts;

import static android.app.AlarmManager.ELAPSED_REALTIME_WAKEUP;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.alarmmanager.alarmtestapp.cts.TestAlarmReceiver;
import android.alarmmanager.alarmtestapp.cts.TestAlarmScheduler;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.uiautomator.UiDevice;
import android.util.Log;

import com.android.compatibility.common.util.AppStandbyUtils;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Tests that app standby imposes the appropriate restrictions on alarms
 */
@LargeTest
@RunWith(AndroidJUnit4.class)
public class AppStandbyTests {
    private static final String TAG = AppStandbyTests.class.getSimpleName();
    private static final String TEST_APP_PACKAGE = "android.alarmmanager.alarmtestapp.cts";
    private static final String TEST_APP_RECEIVER = TEST_APP_PACKAGE + ".TestAlarmScheduler";

    private static final long DEFAULT_WAIT = 4_000;
    private static final long POLL_INTERVAL = 200;

    // Tweaked alarm manager constants to facilitate testing
    private static final long ALLOW_WHILE_IDLE_SHORT_TIME = 15_000;
    private static final long MIN_FUTURITY = 2_000;
    private static final long[] APP_STANDBY_DELAYS = {0, 10_000, 20_000, 30_000, 600_000};
    private static final String[] APP_BUCKET_TAGS = {
            "active",
            "working_set",
            "frequent",
            "rare",
            "never"
    };
    private static final String[] APP_BUCKET_KEYS = {
            "standby_active_delay",
            "standby_working_delay",
            "standby_frequent_delay",
            "standby_rare_delay",
            "standby_never_delay",
    };

    // Save the state before running tests to restore it after we finish testing.
    private static boolean sOrigAppStandbyEnabled;

    private Context mContext;
    private ComponentName mAlarmScheduler;
    private UiDevice mUiDevice;
    private AtomicInteger mAlarmCount;
    private volatile long mLastAlarmTime;

    private final BroadcastReceiver mAlarmStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            mAlarmCount.getAndAdd(intent.getIntExtra(TestAlarmReceiver.EXTRA_ALARM_COUNT, 1));
            mLastAlarmTime = SystemClock.elapsedRealtime();
            Log.d(TAG, "No. of expirations: " + mAlarmCount
                    + " elapsed: " + SystemClock.elapsedRealtime());
        }
    };

    @BeforeClass
    public static void setUpTests() throws Exception {
        sOrigAppStandbyEnabled = AppStandbyUtils.isAppStandbyEnabledAtRuntime();
        if (!sOrigAppStandbyEnabled) {
            AppStandbyUtils.setAppStandbyEnabledAtRuntime(true);

            // Give system sometime to initialize itself.
            Thread.sleep(100);
        }
    }

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getTargetContext();
        mUiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mAlarmScheduler = new ComponentName(TEST_APP_PACKAGE, TEST_APP_RECEIVER);
        mAlarmCount = new AtomicInteger(0);
        updateAlarmManagerConstants();
        setBatteryCharging(false);
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(TestAlarmReceiver.ACTION_REPORT_ALARM_EXPIRED);
        mContext.registerReceiver(mAlarmStateReceiver, intentFilter);
        assumeTrue("App Standby not enabled on device", AppStandbyUtils.isAppStandbyEnabled());
        setAppStandbyBucket("active");
        scheduleAlarm(SystemClock.elapsedRealtime(), false, 0);
        Thread.sleep(MIN_FUTURITY);
        assertTrue("Alarm not sent when app in active", waitForAlarms(1));
    }

    private void scheduleAlarm(long triggerMillis, boolean allowWhileIdle, long interval) {
        final Intent setAlarmIntent = new Intent(TestAlarmScheduler.ACTION_SET_ALARM);
        setAlarmIntent.setComponent(mAlarmScheduler);
        setAlarmIntent.putExtra(TestAlarmScheduler.EXTRA_TYPE, ELAPSED_REALTIME_WAKEUP);
        setAlarmIntent.putExtra(TestAlarmScheduler.EXTRA_TRIGGER_TIME, triggerMillis);
        setAlarmIntent.putExtra(TestAlarmScheduler.EXTRA_REPEAT_INTERVAL, interval);
        setAlarmIntent.putExtra(TestAlarmScheduler.EXTRA_ALLOW_WHILE_IDLE, allowWhileIdle);
        setAlarmIntent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);
        mContext.sendBroadcast(setAlarmIntent);
    }

    private void testBucketDelay(int bucketIndex) throws Exception {
        setAppStandbyBucket(APP_BUCKET_TAGS[bucketIndex]);
        final long triggerTime = SystemClock.elapsedRealtime() + MIN_FUTURITY;
        final long minTriggerTime = mLastAlarmTime + APP_STANDBY_DELAYS[bucketIndex];
        scheduleAlarm(triggerTime, false, 0);
        Thread.sleep(MIN_FUTURITY);
        if (triggerTime + DEFAULT_WAIT < minTriggerTime) {
            assertFalse("Alarm went off before " + APP_BUCKET_TAGS[bucketIndex] + " delay",
                    waitForAlarms(1));
            Thread.sleep(minTriggerTime - SystemClock.elapsedRealtime());
        }
        assertTrue("Deferred alarm did not go off after " + APP_BUCKET_TAGS[bucketIndex] + " delay",
                waitForAlarms(1));
    }

    @Test
    public void testWorkingSetDelay() throws Exception {
        testBucketDelay(1);
    }

    @Test
    public void testFrequentDelay() throws Exception {
        testBucketDelay(2);
    }

    @Test
    public void testRareDelay() throws Exception {
        testBucketDelay(3);
    }

    @Test
    public void testBucketUpgradeToSmallerDelay() throws Exception {
        setAppStandbyBucket(APP_BUCKET_TAGS[2]);
        final long triggerTime = SystemClock.elapsedRealtime() + MIN_FUTURITY;
        final long workingSetExpectedTrigger = mLastAlarmTime + APP_STANDBY_DELAYS[1];
        scheduleAlarm(triggerTime, false, 0);
        Thread.sleep(workingSetExpectedTrigger - SystemClock.elapsedRealtime());
        assertFalse("The alarm went off before frequent delay", waitForAlarms(1));
        setAppStandbyBucket(APP_BUCKET_TAGS[1]);
        assertTrue("The alarm did not go off when app bucket upgraded to working_set",
                waitForAlarms(1));
    }


    /**
     * This is different to {@link #testBucketUpgradeToSmallerDelay()} in the sense that the bucket
     * upgrade shifts eligibility to a point earlier than when the alarm is scheduled for.
     * The alarm must then go off as soon as possible - at either the scheduled time or the bucket
     * change, whichever happened later.
     */
    @Test
    public void testBucketUpgradeToNoDelay() throws Exception {
        setAppStandbyBucket(APP_BUCKET_TAGS[3]);
        final long triggerTime1 = mLastAlarmTime + APP_STANDBY_DELAYS[2];
        scheduleAlarm(triggerTime1, false, 0);
        Thread.sleep(triggerTime1 - SystemClock.elapsedRealtime());
        assertFalse("The alarm went off after frequent delay when app in rare bucket",
                waitForAlarms(1));
        setAppStandbyBucket(APP_BUCKET_TAGS[1]);
        assertTrue("The alarm did not go off when app bucket upgraded to working_set",
                waitForAlarms(1));

        // Once more
        setAppStandbyBucket(APP_BUCKET_TAGS[3]);
        final long triggerTime2 = mLastAlarmTime + APP_STANDBY_DELAYS[2];
        scheduleAlarm(triggerTime2, false, 0);
        setAppStandbyBucket(APP_BUCKET_TAGS[0]);
        Thread.sleep(triggerTime2 - SystemClock.elapsedRealtime());
        assertTrue("The alarm did not go off as scheduled when the app was in active",
                waitForAlarms(1));
    }

    @Test
    public void testAllowWhileIdleAlarms() throws Exception {
        final long firstTrigger = SystemClock.elapsedRealtime() + MIN_FUTURITY;
        scheduleAlarm(firstTrigger, true, 0);
        Thread.sleep(MIN_FUTURITY);
        assertTrue("first allow_while_idle alarm did not go off as scheduled", waitForAlarms(1));
        scheduleAlarm(mLastAlarmTime + 9_000, true, 0);
        // First check for the case where allow_while_idle delay should supersede app standby
        setAppStandbyBucket(APP_BUCKET_TAGS[1]);
        Thread.sleep(APP_STANDBY_DELAYS[1]);
        assertFalse("allow_while_idle alarm went off before short time", waitForAlarms(1));
        long expectedTriggerTime = mLastAlarmTime + ALLOW_WHILE_IDLE_SHORT_TIME;
        Thread.sleep(expectedTriggerTime - SystemClock.elapsedRealtime());
        assertTrue("allow_while_idle alarm did not go off after short time", waitForAlarms(1));

        // Now the other case, app standby delay supersedes the allow_while_idle delay
        scheduleAlarm(mLastAlarmTime + 12_000, true, 0);
        setAppStandbyBucket(APP_BUCKET_TAGS[2]);
        Thread.sleep(ALLOW_WHILE_IDLE_SHORT_TIME);
        assertFalse("allow_while_idle alarm went off before " + APP_STANDBY_DELAYS[2]
                + "ms, when in bucket " + APP_BUCKET_TAGS[2], waitForAlarms(1));
        expectedTriggerTime = mLastAlarmTime + APP_STANDBY_DELAYS[2];
        Thread.sleep(expectedTriggerTime - SystemClock.elapsedRealtime());
        assertTrue("allow_while_idle alarm did not go off even after " + APP_STANDBY_DELAYS[2]
                + "ms, when in bucket " + APP_BUCKET_TAGS[2], waitForAlarms(1));
    }

    @Test
    public void testPowerWhitelistedAlarmNotBlocked() throws Exception {
        setAppStandbyBucket(APP_BUCKET_TAGS[3]);
        setPowerWhitelisted(true);
        final long triggerTime = SystemClock.elapsedRealtime() + MIN_FUTURITY;
        scheduleAlarm(triggerTime, false, 0);
        Thread.sleep(MIN_FUTURITY);
        assertTrue("Alarm did not go off for whitelisted app in rare bucket", waitForAlarms(1));
        setPowerWhitelisted(false);
    }

    @After
    public void tearDown() throws Exception {
        setPowerWhitelisted(false);
        setBatteryCharging(true);
        deleteAlarmManagerConstants();
        final Intent cancelAlarmsIntent = new Intent(TestAlarmScheduler.ACTION_CANCEL_ALL_ALARMS);
        cancelAlarmsIntent.setComponent(mAlarmScheduler);
        mContext.sendBroadcast(cancelAlarmsIntent);
        mContext.unregisterReceiver(mAlarmStateReceiver);
        // Broadcast unregister may race with the next register in setUp
        Thread.sleep(500);
    }

    @AfterClass
    public static void tearDownTests() throws Exception {
        if (!sOrigAppStandbyEnabled) {
            AppStandbyUtils.setAppStandbyEnabledAtRuntime(sOrigAppStandbyEnabled);
        }
    }

    private void updateAlarmManagerConstants() throws IOException {
        final StringBuffer cmd = new StringBuffer("settings put global alarm_manager_constants ");
        cmd.append("min_futurity="); cmd.append(MIN_FUTURITY);
        cmd.append(",allow_while_idle_short_time="); cmd.append(ALLOW_WHILE_IDLE_SHORT_TIME);
        for (int i = 0; i < APP_STANDBY_DELAYS.length; i++) {
            cmd.append(",");
            cmd.append(APP_BUCKET_KEYS[i]); cmd.append("="); cmd.append(APP_STANDBY_DELAYS[i]);
        }
        executeAndLog(cmd.toString());
    }

    private void setPowerWhitelisted(boolean whitelist) throws IOException {
        final StringBuffer cmd = new StringBuffer("cmd deviceidle whitelist ");
        cmd.append(whitelist ? "+" : "-");
        cmd.append(TEST_APP_PACKAGE);
        executeAndLog(cmd.toString());
    }

    private void deleteAlarmManagerConstants() throws IOException {
        executeAndLog("settings delete global alarm_manager_constants");
    }

    private void setAppStandbyBucket(String bucket) throws IOException {
        executeAndLog("am set-standby-bucket " + TEST_APP_PACKAGE + " " + bucket);
    }

    private void setBatteryCharging(final boolean charging) throws Exception {
        final BatteryManager bm = mContext.getSystemService(BatteryManager.class);
        final String cmd = "dumpsys battery " + (charging ? "reset" : "unplug");
        executeAndLog(cmd);
        if (!charging) {
            assertTrue("Battery could not be unplugged", waitUntil(() -> !bm.isCharging(), 5_000));
        }
    }

    private String executeAndLog(String cmd) throws IOException {
        final String output = mUiDevice.executeShellCommand(cmd).trim();
        Log.d(TAG, "command: [" + cmd + "], output: [" + output + "]");
        return output;
    }

    private boolean waitForAlarms(final int numAlarms) throws InterruptedException {
        final boolean success = waitUntil(() -> (mAlarmCount.get() == numAlarms), DEFAULT_WAIT);
        mAlarmCount.set(0);
        return success;
    }

    private boolean waitUntil(Condition condition, long timeout) throws InterruptedException {
        final long deadLine = SystemClock.uptimeMillis() + timeout;
        while (!condition.isMet() && SystemClock.uptimeMillis() < deadLine) {
            Thread.sleep(POLL_INTERVAL);
        }
        return condition.isMet();
    }

    @FunctionalInterface
    interface Condition {
        boolean isMet();
    }
}
