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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.alarmmanager.alarmtestapp.cts.TestAlarmScheduler;
import android.alarmmanager.alarmtestapp.cts.TestAlarmReceiver;
import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.uiautomator.UiDevice;
import android.util.Log;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;

/**
 * Tests that apps put in forced app standby by the user do not get to run alarms while in the
 * background
 */
@LargeTest
@RunWith(AndroidJUnit4.class)
public class BackgroundRestrictedAlarmsTest {
    private static final String TAG = BackgroundRestrictedAlarmsTest.class.getSimpleName();
    private static final String TEST_APP_PACKAGE = "android.alarmmanager.alarmtestapp.cts";
    private static final String TEST_APP_RECEIVER = TEST_APP_PACKAGE + ".TestAlarmScheduler";
    private static final String APP_OP_RUN_ANY_IN_BACKGROUND = "RUN_ANY_IN_BACKGROUND";
    private static final String APP_OP_MODE_ALLOWED = "allow";
    private static final String APP_OP_MODE_IGNORED = "ignore";

    private static final long DEFAULT_WAIT = 1_000;
    private static final long POLL_INTERVAL = 200;
    private static final long MIN_REPEATING_INTERVAL = 10_000;

    private Object mLock = new Object();
    private Context mContext;
    private ComponentName mAlarmScheduler;
    private UiDevice mUiDevice;
    private int mAlarmCount;

    private final BroadcastReceiver mAlarmStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "Received action " + intent.getAction()
                    + " elapsed: " + SystemClock.elapsedRealtime());
            synchronized (mLock) {
                mAlarmCount = intent.getIntExtra(TestAlarmReceiver.EXTRA_ALARM_COUNT, 1);
            }
        }
    };

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getTargetContext();
        mUiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mAlarmScheduler = new ComponentName(TEST_APP_PACKAGE, TEST_APP_RECEIVER);
        mAlarmCount = 0;
        updateAlarmManagerConstants();
        setAppOpsMode(APP_OP_MODE_IGNORED);
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(TestAlarmReceiver.ACTION_REPORT_ALARM_EXPIRED);
        mContext.registerReceiver(mAlarmStateReceiver, intentFilter);
        setAppStandbyBucket("active");
    }

    private void scheduleAlarm(int type, long triggerMillis, long interval) {
        final Intent setAlarmIntent = new Intent(TestAlarmScheduler.ACTION_SET_ALARM);
        setAlarmIntent.setComponent(mAlarmScheduler);
        setAlarmIntent.putExtra(TestAlarmScheduler.EXTRA_TYPE, type);
        setAlarmIntent.putExtra(TestAlarmScheduler.EXTRA_TRIGGER_TIME, triggerMillis);
        setAlarmIntent.putExtra(TestAlarmScheduler.EXTRA_REPEAT_INTERVAL, interval);
        mContext.sendBroadcast(setAlarmIntent);
    }

    private void scheduleAlarmClock(long triggerRTC) {
        AlarmManager.AlarmClockInfo alarmInfo = new AlarmManager.AlarmClockInfo(triggerRTC, null);

        final Intent setAlarmClockIntent = new Intent(TestAlarmScheduler.ACTION_SET_ALARM_CLOCK);
        setAlarmClockIntent.setComponent(mAlarmScheduler);
        setAlarmClockIntent.putExtra(TestAlarmScheduler.EXTRA_ALARM_CLOCK_INFO, alarmInfo);
        mContext.sendBroadcast(setAlarmClockIntent);
    }

    private static int getMinExpectedExpirations(long now, long start, long interval) {
        if (now - start <= 1000) {
            return 0;
        }
        return 1 + (int)((now - start - 1000)/interval);
    }

    @Test
    public void testRepeatingAlarmBlocked() throws Exception {
        final long interval = MIN_REPEATING_INTERVAL;
        final long triggerElapsed = SystemClock.elapsedRealtime() + interval;
        scheduleAlarm(AlarmManager.ELAPSED_REALTIME_WAKEUP, triggerElapsed, interval);
        Thread.sleep(DEFAULT_WAIT);
        Thread.sleep(2 * interval);
        assertFalse("Alarm got triggered even under restrictions", waitForAlarms(1, DEFAULT_WAIT));
        Thread.sleep(interval);
        setAppOpsMode(APP_OP_MODE_ALLOWED);
        // The alarm is due to go off about 3 times by now. Adding some tolerance just in case
        // an expiration is due right about now.
        final int minCount = getMinExpectedExpirations(SystemClock.elapsedRealtime(),
                triggerElapsed, interval);
        assertTrue("Alarm should have expired at least " + minCount
                + " times when restrictions were lifted", waitForAlarms(minCount, DEFAULT_WAIT));
    }

    @Test
    public void testAlarmClockNotBlocked() throws Exception {
        final long nowRTC = System.currentTimeMillis();
        final long waitInterval = 10_000;
        final long triggerRTC = nowRTC + waitInterval;
        scheduleAlarmClock(triggerRTC);
        Thread.sleep(waitInterval);
        assertTrue("AlarmClock did not go off as scheduled when under restrictions",
                waitForAlarms(1, DEFAULT_WAIT));
    }

    @After
    public void tearDown() throws Exception {
        deleteAlarmManagerConstants();
        setAppOpsMode(APP_OP_MODE_ALLOWED);
        // Cancel any leftover alarms
        final Intent cancelAlarmsIntent = new Intent(TestAlarmScheduler.ACTION_CANCEL_ALL_ALARMS);
        cancelAlarmsIntent.setComponent(mAlarmScheduler);
        mContext.sendBroadcast(cancelAlarmsIntent);
        mContext.unregisterReceiver(mAlarmStateReceiver);
        // Broadcast unregister may race with the next register in setUp
        Thread.sleep(DEFAULT_WAIT);
    }

    private void updateAlarmManagerConstants() throws IOException {
        String cmd = "settings put global alarm_manager_constants min_interval="
                + MIN_REPEATING_INTERVAL;
        mUiDevice.executeShellCommand(cmd);
    }

    private void deleteAlarmManagerConstants() throws IOException {
        mUiDevice.executeShellCommand("settings delete global alarm_manager_constants");
    }

    private void setAppStandbyBucket(String bucket) throws IOException {
        mUiDevice.executeShellCommand("am set-standby-bucket " + TEST_APP_PACKAGE + " " + bucket);
    }

    private void setAppOpsMode(String mode) throws IOException {
        StringBuilder commandBuilder = new StringBuilder("appops set ")
                .append(TEST_APP_PACKAGE)
                .append(" ")
                .append(APP_OP_RUN_ANY_IN_BACKGROUND)
                .append(" ")
                .append(mode);
        mUiDevice.executeShellCommand(commandBuilder.toString());
    }

    private boolean waitForAlarms(int expectedAlarms, long timeout) throws InterruptedException {
        final long deadLine = SystemClock.uptimeMillis() + timeout;
        int alarmCount;
        do {
            Thread.sleep(POLL_INTERVAL);
            synchronized (mLock) {
                alarmCount = mAlarmCount;
            }
        } while (alarmCount < expectedAlarms && SystemClock.uptimeMillis() < deadLine);
        return alarmCount >= expectedAlarms;
    }
}
