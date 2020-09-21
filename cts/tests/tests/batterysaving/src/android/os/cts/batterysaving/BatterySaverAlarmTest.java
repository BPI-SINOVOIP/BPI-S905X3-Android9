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
package android.os.cts.batterysaving;

import static android.os.cts.batterysaving.common.Values.APP_25_PACKAGE;
import static android.os.cts.batterysaving.common.Values.getRandomInt;

import static com.android.compatibility.common.util.AmUtils.runKill;
import static com.android.compatibility.common.util.AmUtils.runMakeUidIdle;
import static com.android.compatibility.common.util.BatteryUtils.enableBatterySaver;
import static com.android.compatibility.common.util.BatteryUtils.runDumpsysBatteryUnplug;
import static com.android.compatibility.common.util.SettingsUtils.putGlobalSetting;
import static com.android.compatibility.common.util.TestUtils.waitUntil;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.os.cts.batterysaving.common.BatterySavingCtsCommon.Payload;
import android.os.cts.batterysaving.common.BatterySavingCtsCommon.Payload.TestServiceRequest;
import android.os.cts.batterysaving.common.BatterySavingCtsCommon.Payload.TestServiceRequest.SetAlarmRequest;
import android.os.cts.batterysaving.common.BatterySavingCtsCommon.Payload.TestServiceRequest.StartServiceRequest;
import android.os.cts.batterysaving.common.Values;
import android.support.test.filters.LargeTest;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import com.android.compatibility.common.util.ThreadUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * CTS for battery saver alarm throttling
 *
 atest $ANDROID_BUILD_TOP/cts/tests/tests/batterysaving/src/android/os/cts/batterysaving/BatterySaverAlarmTest.java
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class BatterySaverAlarmTest extends BatterySavingTestBase {
    private static final String TAG = "BatterySaverAlarmTest";

    private static final long DEFAULT_WAIT = 1_000;
    private static final long POLL_INTERVAL = 200;

    // Tweaked alarm manager constants to facilitate testing
    private static final long MIN_REPEATING_INTERVAL = 5_000;
    private static final long ALLOW_WHILE_IDLE_SHORT_TIME = 10_000;
    private static final long ALLOW_WHILE_IDLE_LONG_TIME = 20_000;
    private static final long MIN_FUTURITY = 2_000;

    private void updateAlarmManagerConstants() throws IOException {
        putGlobalSetting("alarm_manager_constants",
                "min_interval=" + MIN_REPEATING_INTERVAL + ","
                + "min_futurity=" + MIN_FUTURITY + ","
                + "allow_while_idle_short_time=" + ALLOW_WHILE_IDLE_SHORT_TIME + ","
                + "allow_while_idle_long_time=" + ALLOW_WHILE_IDLE_LONG_TIME);
    }

    private void resetAlarmManagerConstants() throws IOException {
        putGlobalSetting("alarm_manager_constants", "null");
    }

    // Use a different broadcast action every time.
    private final String ACTION = "BATTERY_SAVER_ALARM_TEST_ALARM_ACTION_" + Values.getRandomInt();

    private final AtomicInteger mAlarmCount = new AtomicInteger();

    private final BroadcastReceiver mAlarmReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            mAlarmCount.incrementAndGet();
            Log.d(TAG, "Alarm received at " + SystemClock.elapsedRealtime());
        }
    };

    @Before
    public void setUp() throws IOException {
        updateAlarmManagerConstants();

        final IntentFilter filter = new IntentFilter(ACTION);
        getContext().registerReceiver(mAlarmReceiver, filter, null,
                new Handler(Looper.getMainLooper()));
    }

    @After
    public void tearDown() throws IOException {
        resetAlarmManagerConstants();
        getContext().unregisterReceiver(mAlarmReceiver);
    }

    private void scheduleAlarm(String targetPackage, int type, long triggerMillis)
            throws Exception {
        scheduleAlarm(targetPackage, type, triggerMillis, /*whileIdle=*/ true);
    }

    private void scheduleAlarm(String targetPackage, int type, long triggerMillis,
            boolean whileIdle) throws Exception {
        Log.d(TAG, "Setting an alarm at " + triggerMillis + " (in "
                + (triggerMillis - SystemClock.elapsedRealtime()) + "ms)");
        final SetAlarmRequest areq = SetAlarmRequest.newBuilder()
                .setIntentAction(ACTION)
                .setType(type)
                .setAllowWhileIdle(whileIdle)
                .setTriggerTime(triggerMillis)
                .build();
        final Payload response = mRpc.sendRequest(targetPackage,
                Payload.newBuilder().setTestServiceRequest(
                        TestServiceRequest.newBuilder().setSetAlarm(areq))
                        .build());
        assertTrue(response.hasTestServiceResponse()
                && response.getTestServiceResponse().getSetAlarmAck());
    }

    /**
     * Return a service in the target package.
     */
    private String startService(String targetPackage, boolean foreground)
            throws Exception {
        final String action = "start_service_" + getRandomInt() + "_fg=" + foreground;

        final Payload response = mRpc.sendRequest(targetPackage,
                Payload.newBuilder().setTestServiceRequest(
                        TestServiceRequest.newBuilder().setStartService(
                                StartServiceRequest.newBuilder()
                                        .setForeground(foreground)
                                        .setAction(action).build()
                        )).build());
        assertTrue(response.hasTestServiceResponse()
                && response.getTestServiceResponse().getStartServiceAck());
        return action;
    }

    private static void forcePackageIntoBg(String packageName) throws Exception {
        runMakeUidIdle(packageName);
        Thread.sleep(200);
        runKill(packageName, /*wait=*/ true);
        Thread.sleep(1000);
    }

    @LargeTest
    @Test
    public void testAllowWhileIdleThrottled() throws Exception {
        final String targetPackage = APP_25_PACKAGE;

        runDumpsysBatteryUnplug();

        enableBatterySaver(true);

        forcePackageIntoBg(targetPackage);

        // First alarm shouldn't be throttled.
        final long triggerElapsed1 = SystemClock.elapsedRealtime() + MIN_FUTURITY;
        scheduleAlarm(targetPackage, AlarmManager.ELAPSED_REALTIME_WAKEUP, triggerElapsed1);
        ThreadUtils.sleepUntilRealtime(triggerElapsed1 + DEFAULT_WAIT);
        assertEquals("Allow-while-idle alarm shouldn't be blocked in battery saver",
                1, mAlarmCount.get());

        // Second one should be throttled.
        mAlarmCount.set(0);

        final long triggerElapsed2 = triggerElapsed1 + ALLOW_WHILE_IDLE_SHORT_TIME;
        scheduleAlarm(targetPackage, AlarmManager.ELAPSED_REALTIME_WAKEUP, triggerElapsed2);
        ThreadUtils.sleepUntilRealtime(triggerElapsed2 + DEFAULT_WAIT);
        assertEquals("Follow up allow-while-idle alarm shouldn't go off before short time",
                0, mAlarmCount.get());

        final long triggerElapsed3 = triggerElapsed1 + ALLOW_WHILE_IDLE_LONG_TIME;
        ThreadUtils.sleepUntilRealtime(triggerElapsed3 + DEFAULT_WAIT);
        assertEquals("Follow-up allow-while-idle alarm should go off after long time",
                1, mAlarmCount.get());

        // Start an FG service, which should reset throttling.
        mAlarmCount.set(0);

        startService(targetPackage, true);

        final long triggerElapsed4 = triggerElapsed3 + ALLOW_WHILE_IDLE_SHORT_TIME;
        scheduleAlarm(targetPackage, AlarmManager.ELAPSED_REALTIME_WAKEUP, triggerElapsed4);
        ThreadUtils.sleepUntilRealtime(triggerElapsed4 + DEFAULT_WAIT);
        assertEquals("Allow-while-idle alarm shouldn't be throttled in battery saver"
                +" after FG service started",
                1, mAlarmCount.get());

        // Battery saver off. Always use the short time.
        enableBatterySaver(false);

        mAlarmCount.set(0);

        final long triggerElapsed5 = triggerElapsed4 + ALLOW_WHILE_IDLE_SHORT_TIME;
        scheduleAlarm(targetPackage, AlarmManager.ELAPSED_REALTIME_WAKEUP, triggerElapsed5);
        ThreadUtils.sleepUntilRealtime(triggerElapsed5 + DEFAULT_WAIT);
        assertEquals("Allow-while-idle alarm shouldn't be throttled in battery saver"
                        +" when BS is off",
                1, mAlarmCount.get());

        // One more time.
        mAlarmCount.set(0);

        final long triggerElapsed6 = triggerElapsed5 + ALLOW_WHILE_IDLE_SHORT_TIME;
        scheduleAlarm(targetPackage, AlarmManager.ELAPSED_REALTIME_WAKEUP, triggerElapsed6);
        ThreadUtils.sleepUntilRealtime(triggerElapsed6 + DEFAULT_WAIT);
        assertEquals("Allow-while-idle alarm shouldn't be throttled when BS is off",
                1, mAlarmCount.get());
    }

    @LargeTest
    @Test
    public void testAlarmsThrottled() throws Exception {
        final String targetPackage = APP_25_PACKAGE;

        runDumpsysBatteryUnplug();

        enableBatterySaver(true);

        forcePackageIntoBg(targetPackage);

        {
            // When battery saver is enabled, alarms should be blocked.
            final long triggerElapsed = SystemClock.elapsedRealtime() + MIN_FUTURITY;
            scheduleAlarm(targetPackage, AlarmManager.ELAPSED_REALTIME_WAKEUP, triggerElapsed,
                    /* whileIdle=*/ false);
            ThreadUtils.sleepUntilRealtime(triggerElapsed + DEFAULT_WAIT);
            assertEquals("Non-while-idle alarm should be blocked in battery saver",
                    0, mAlarmCount.get());
        }

        // Start an FG service -> should unblock the alarm.
        startService(targetPackage, true);

        waitUntil("Alarm should fire for an FG app",
                () -> mAlarmCount.get() == 1);

        // Try again.
        mAlarmCount.set(0);

        forcePackageIntoBg(targetPackage);

        // Try again.
        // When battery saver is enabled, alarms should be blocked.
        {
            final long triggerElapsed = SystemClock.elapsedRealtime() + MIN_FUTURITY;
            scheduleAlarm(targetPackage, AlarmManager.ELAPSED_REALTIME_WAKEUP, triggerElapsed,
                    /* whileIdle=*/ false);
            ThreadUtils.sleepUntilRealtime(triggerElapsed + DEFAULT_WAIT);
            assertEquals("Non-while-idle alarm should be blocked in battery saver",
                    0, mAlarmCount.get());
        }

        // This time, disable EBS -> should unblock the alarm.
        enableBatterySaver(false);
        waitUntil("Allow-while-idle alarm should be blocked in battery saver",
                () -> mAlarmCount.get() == 1);
    }
}
