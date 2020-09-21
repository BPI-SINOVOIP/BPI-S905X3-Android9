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
package com.android.server.cts;

import com.android.tradefed.log.LogUtil;

import java.util.Random;

/**
 * Test for "dumpsys batterystats -c
 *
 * Validates reporting of battery stats based on different events
 */
public class BatteryStatsValidationTest extends ProtoDumpTestCase {
    private static final String TAG = "BatteryStatsValidationTest";

    private static final String DEVICE_SIDE_TEST_APK = "CtsBatteryStatsApp.apk";
    private static final String DEVICE_SIDE_TEST_PACKAGE
            = "com.android.server.cts.device.batterystats";
    private static final String DEVICE_SIDE_BG_SERVICE_COMPONENT
            = "com.android.server.cts.device.batterystats/.BatteryStatsBackgroundService";
    private static final String DEVICE_SIDE_FG_ACTIVITY_COMPONENT
            = "com.android.server.cts.device.batterystats/.BatteryStatsForegroundActivity";
    private static final String DEVICE_SIDE_JOB_COMPONENT
            = "com.android.server.cts.device.batterystats/.SimpleJobService";
    private static final String DEVICE_SIDE_SYNC_COMPONENT
            = "com.android.server.cts.device.batterystats.provider/"
            + "com.android.server.cts.device.batterystats";

    // These constants are those in PackageManager.
    public static final String FEATURE_BLUETOOTH_LE = "android.hardware.bluetooth_le";
    public static final String FEATURE_LEANBACK_ONLY = "android.software.leanback_only";
    public static final String FEATURE_LOCATION_GPS = "android.hardware.location.gps";

    private static final int STATE_TIME_TOP_INDEX = 4;
    private static final int STATE_TIME_FOREGROUND_SERVICE_INDEX = 5;
    private static final int STATE_TIME_FOREGROUND_INDEX = 6;
    private static final int STATE_TIME_BACKGROUND_INDEX = 7;
    private static final int STATE_TIME_CACHED_INDEX = 10;

    private static final long TIME_SPENT_IN_TOP = 2000;
    private static final long TIME_SPENT_IN_FOREGROUND = 2000;
    private static final long TIME_SPENT_IN_BACKGROUND = 2000;
    private static final long TIME_SPENT_IN_CACHED = 2000;
    private static final long SCREEN_STATE_CHANGE_TIMEOUT = 4000;
    private static final long SCREEN_STATE_POLLING_INTERVAL = 500;

    // Constants from BatteryStatsBgVsFgActions.java (not directly accessible here).
    public static final String KEY_ACTION = "action";
    public static final String ACTION_BLE_SCAN_OPTIMIZED = "action.ble_scan_optimized";
    public static final String ACTION_BLE_SCAN_UNOPTIMIZED = "action.ble_scan_unoptimized";
    public static final String ACTION_GPS = "action.gps";
    public static final String ACTION_JOB_SCHEDULE = "action.jobs";
    public static final String ACTION_SYNC = "action.sync";
    public static final String ACTION_SLEEP_WHILE_BACKGROUND = "action.sleep_background";
    public static final String ACTION_SLEEP_WHILE_TOP = "action.sleep_top";
    public static final String ACTION_SHOW_APPLICATION_OVERLAY = "action.show_application_overlay";

    public static final String KEY_REQUEST_CODE = "request_code";
    public static final String BG_VS_FG_TAG = "BatteryStatsBgVsFgActions";

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        // Uninstall to clear the history in case it's still on the device.
        getDevice().uninstallPackage(DEVICE_SIDE_TEST_PACKAGE);
    }

    @Override
    protected void tearDown() throws Exception {
        getDevice().uninstallPackage(DEVICE_SIDE_TEST_PACKAGE);

        batteryOffScreenOn();
        super.tearDown();
    }

    protected void screenOff() throws Exception {
        getDevice().executeShellCommand("dumpsys batterystats enable pretend-screen-off");
    }

    /**
     * This will turn the screen on for real, not just disabling pretend-screen-off
     */
    protected void turnScreenOnForReal() throws Exception {
        getDevice().executeShellCommand("input keyevent KEYCODE_WAKEUP");
        getDevice().executeShellCommand("wm dismiss-keyguard");
    }

    /**
     * This will send the screen to sleep
     */
    protected void turnScreenOffForReal() throws Exception {
        getDevice().executeShellCommand("input keyevent KEYCODE_SLEEP");
    }

    protected void batteryOnScreenOn() throws Exception {
        getDevice().executeShellCommand("dumpsys battery unplug");
        getDevice().executeShellCommand("dumpsys batterystats disable pretend-screen-off");
    }

    protected void batteryOnScreenOff() throws Exception {
        getDevice().executeShellCommand("dumpsys battery unplug");
        getDevice().executeShellCommand("dumpsys batterystats enable pretend-screen-off");
    }

    protected void batteryOffScreenOn() throws Exception {
        getDevice().executeShellCommand("dumpsys battery reset");
        getDevice().executeShellCommand("dumpsys batterystats disable pretend-screen-off");
    }

    private void forceStop() throws Exception {
        getDevice().executeShellCommand("am force-stop " + DEVICE_SIDE_TEST_PACKAGE);
    }

    private void resetBatteryStats() throws Exception {
        getDevice().executeShellCommand("dumpsys batterystats --reset");
    }

    public void testAlarms() throws Exception {
        batteryOnScreenOff();

        installPackage(DEVICE_SIDE_TEST_APK, /* grantPermissions= */ true);

        runDeviceTests(DEVICE_SIDE_TEST_PACKAGE, ".BatteryStatsAlarmTest", "testAlarms");

        assertValueRange("wua", "*walarm*:com.android.server.cts.device.batterystats.ALARM",
                5, 3, 3);

        batteryOffScreenOn();
    }

    public void testWakeLockDuration() throws Exception {
        batteryOnScreenOff();

        installPackage(DEVICE_SIDE_TEST_APK, /* grantPermissions= */ true);

        runDeviceTests(DEVICE_SIDE_TEST_PACKAGE, ".BatteryStatsWakeLockTests",
                "testHoldShortWakeLock");

        runDeviceTests(DEVICE_SIDE_TEST_PACKAGE, ".BatteryStatsWakeLockTests",
                "testHoldLongWakeLock");

        assertValueRange("wl", "BSShortWakeLock", 15, (long) (500 * 0.9), 500 * 2); // partial max duration
        assertValueRange("wl", "BSLongWakeLock", 15, (long) (3000 * 0.9), 3000 * 2);  // partial max duration

        batteryOffScreenOn();
    }

    private void startSimpleActivity() throws Exception {
        getDevice().executeShellCommand(
                "am start -n com.android.server.cts.device.batterystats/.SimpleActivity");
    }

    public void testServiceForegroundDuration() throws Exception {
        batteryOnScreenOff();
        installPackage(DEVICE_SIDE_TEST_APK, true);

        startSimpleActivity();
        assertValueRange("st", "", STATE_TIME_FOREGROUND_SERVICE_INDEX, 0,
                0); // No foreground service time before test
        runDeviceTests(DEVICE_SIDE_TEST_PACKAGE, ".BatteryStatsProcessStateTests",
                "testForegroundService");
        assertValueRange("st", "", STATE_TIME_FOREGROUND_SERVICE_INDEX, (long) (2000 * 0.8), 4000);
        batteryOffScreenOn();
    }

    public void testUidForegroundDuration() throws Exception {
        batteryOnScreenOff();
        installPackage(DEVICE_SIDE_TEST_APK, true);
        // No foreground time before test
        assertValueRange("st", "", STATE_TIME_FOREGROUND_INDEX, 0, 0);
        turnScreenOnForReal();
        assertScreenOn();
        executeForeground(ACTION_SHOW_APPLICATION_OVERLAY, 2000);
        Thread.sleep(TIME_SPENT_IN_FOREGROUND); // should be in foreground for about this long
        assertApproximateTimeInState(STATE_TIME_FOREGROUND_INDEX, TIME_SPENT_IN_FOREGROUND);
        batteryOffScreenOn();
    }

    public void testUidBackgroundDuration() throws Exception {
        batteryOnScreenOff();
        installPackage(DEVICE_SIDE_TEST_APK, true);
        // No background time before test
        assertValueRange("st", "", STATE_TIME_BACKGROUND_INDEX, 0, 0);
        executeBackground(ACTION_SLEEP_WHILE_BACKGROUND, 4000);
        assertApproximateTimeInState(STATE_TIME_BACKGROUND_INDEX, TIME_SPENT_IN_BACKGROUND);
        batteryOffScreenOn();
    }

    public void testTopDuration() throws Exception {
        batteryOnScreenOff();
        installPackage(DEVICE_SIDE_TEST_APK, true);
        // No top time before test
        assertValueRange("st", "", STATE_TIME_TOP_INDEX, 0, 0);
        turnScreenOnForReal();
        assertScreenOn();
        executeForeground(ACTION_SLEEP_WHILE_TOP, 4000);
        assertApproximateTimeInState(STATE_TIME_TOP_INDEX, TIME_SPENT_IN_TOP);
        batteryOffScreenOn();
    }

    public void testCachedDuration() throws Exception {
        batteryOnScreenOff();
        installPackage(DEVICE_SIDE_TEST_APK, true);
        // No cached time before test
        assertValueRange("st", "", STATE_TIME_CACHED_INDEX, 0, 0);
        startSimpleActivity();
        Thread.sleep(TIME_SPENT_IN_CACHED); // process should be in cached state for about this long
        assertApproximateTimeInState(STATE_TIME_CACHED_INDEX, TIME_SPENT_IN_CACHED);
        batteryOffScreenOn();
    }

    private void assertScreenOff() throws Exception {
        final long deadLine = System.currentTimeMillis() + SCREEN_STATE_CHANGE_TIMEOUT;
        boolean screenAwake = true;
        do {
            final String dumpsysPower = getDevice().executeShellCommand("dumpsys power").trim();
            for (String line : dumpsysPower.split("\n")) {
                if (line.contains("Display Power")) {
                    screenAwake = line.trim().endsWith("ON");
                    break;
                }
            }
            Thread.sleep(SCREEN_STATE_POLLING_INTERVAL);
        } while (screenAwake && System.currentTimeMillis() < deadLine);
        assertFalse("Screen could not be turned off", screenAwake);
    }

    private void assertScreenOn() throws Exception {
        // this also checks that the keyguard is dismissed
        final long deadLine = System.currentTimeMillis() + SCREEN_STATE_CHANGE_TIMEOUT;
        boolean screenAwake;
        do {
            final String dumpsysWindowPolicy =
                    getDevice().executeShellCommand("dumpsys window policy").trim();
            boolean keyguardStateLines = false;
            screenAwake = true;
            for (String line : dumpsysWindowPolicy.split("\n")) {
                if (line.contains("KeyguardServiceDelegate")) {
                    keyguardStateLines = true;
                } else if (keyguardStateLines && line.contains("showing=")) {
                    screenAwake &= line.trim().endsWith("false");
                } else if (keyguardStateLines && line.contains("screenState=")) {
                    screenAwake &= line.trim().endsWith("SCREEN_STATE_ON");
                }
            }
            Thread.sleep(SCREEN_STATE_POLLING_INTERVAL);
        } while (!screenAwake && System.currentTimeMillis() < deadLine);
        assertTrue("Screen could not be turned on", screenAwake);
    }

    public void testBleScans() throws Exception {
        if (isTV() || !hasFeature(FEATURE_BLUETOOTH_LE, true)) {
            return;
        }

        batteryOnScreenOff();
        installPackage(DEVICE_SIDE_TEST_APK, true);
        turnScreenOnForReal();
        assertScreenOn();

        // Background test.
        executeBackground(ACTION_BLE_SCAN_UNOPTIMIZED, 40_000);
        assertValueRange("blem", "", 5, 1, 1); // ble_scan_count
        assertValueRange("blem", "", 6, 1, 1); // ble_scan_count_bg

        // Foreground test.
        executeForeground(ACTION_BLE_SCAN_UNOPTIMIZED, 40_000);
        assertValueRange("blem", "", 5, 2, 2); // ble_scan_count
        assertValueRange("blem", "", 6, 1, 1); // ble_scan_count_bg

        batteryOffScreenOn();
    }


    public void testUnoptimizedBleScans() throws Exception {
        if (isTV() || !hasFeature(FEATURE_BLUETOOTH_LE, true)) {
            return;
        }
        batteryOnScreenOff();
        installPackage(DEVICE_SIDE_TEST_APK, true);
        turnScreenOnForReal();
        assertScreenOn();
        // Ble scan time in BatteryStatsBgVsFgActions is 2 seconds, but be lenient.
        final int minTime = 1500; // min single scan time in ms
        final int maxTime = 3000; // max single scan time in ms

        // Optimized - Background.
        executeBackground(ACTION_BLE_SCAN_OPTIMIZED, 40_000);
        assertValueRange("blem", "", 7, 1*minTime, 1*maxTime); // actualTime
        assertValueRange("blem", "", 8, 1*minTime, 1*maxTime); // actualTimeBg
        assertValueRange("blem", "", 11, 0, 0); // unoptimizedScanTotalTime
        assertValueRange("blem", "", 12, 0, 0); // unoptimizedScanTotalTimeBg
        assertValueRange("blem", "", 13, 0, 0); // unoptimizedScanMaxTime
        assertValueRange("blem", "", 14, 0, 0); // unoptimizedScanMaxTimeBg

        // Optimized - Foreground.
        executeForeground(ACTION_BLE_SCAN_OPTIMIZED, 40_000);
        assertValueRange("blem", "", 7, 2*minTime, 2*maxTime); // actualTime
        assertValueRange("blem", "", 8, 1*minTime, 1*maxTime); // actualTimeBg
        assertValueRange("blem", "", 11, 0, 0); // unoptimizedScanTotalTime
        assertValueRange("blem", "", 12, 0, 0); // unoptimizedScanTotalTimeBg
        assertValueRange("blem", "", 13, 0, 0); // unoptimizedScanMaxTime
        assertValueRange("blem", "", 14, 0, 0); // unoptimizedScanMaxTimeBg

        // Unoptimized - Background.
        executeBackground(ACTION_BLE_SCAN_UNOPTIMIZED, 40_000);
        assertValueRange("blem", "", 7, 3*minTime, 3*maxTime); // actualTime
        assertValueRange("blem", "", 8, 2*minTime, 2*maxTime); // actualTimeBg
        assertValueRange("blem", "", 11, 1*minTime, 1*maxTime); // unoptimizedScanTotalTime
        assertValueRange("blem", "", 12, 1*minTime, 1*maxTime); // unoptimizedScanTotalTimeBg
        assertValueRange("blem", "", 13, 1*minTime, 1*maxTime); // unoptimizedScanMaxTime
        assertValueRange("blem", "", 14, 1*minTime, 1*maxTime); // unoptimizedScanMaxTimeBg

        // Unoptimized - Foreground.
        executeForeground(ACTION_BLE_SCAN_UNOPTIMIZED, 40_000);
        assertValueRange("blem", "", 7, 4*minTime, 4*maxTime); // actualTime
        assertValueRange("blem", "", 8, 2*minTime, 2*maxTime); // actualTimeBg
        assertValueRange("blem", "", 11, 2*minTime, 2*maxTime); // unoptimizedScanTotalTime
        assertValueRange("blem", "", 12, 1*minTime, 1*maxTime); // unoptimizedScanTotalTimeBg
        assertValueRange("blem", "", 13, 1*minTime, 1*maxTime); // unoptimizedScanMaxTime
        assertValueRange("blem", "", 14, 1*minTime, 1*maxTime); // unoptimizedScanMaxTimeBg

        batteryOffScreenOn();
    }

    public void testGpsUpdates() throws Exception {
        if (isTV() || !hasFeature(FEATURE_LOCATION_GPS, true)) {
            return;
        }

        final String gpsSensorNumber = "-10000";

        batteryOnScreenOff();
        installPackage(DEVICE_SIDE_TEST_APK, true);
        // Whitelist this app against background location request throttling
        getDevice().executeShellCommand(String.format(
                "settings put global location_background_throttle_package_whitelist %s",
                DEVICE_SIDE_TEST_PACKAGE));

        // Background test.
        executeBackground(ACTION_GPS, 60_000);
        assertValueRange("sr", gpsSensorNumber, 6, 1, 1); // count
        assertValueRange("sr", gpsSensorNumber, 7, 1, 1); // background_count

        // Foreground test.
        executeForeground(ACTION_GPS, 60_000);
        assertValueRange("sr", gpsSensorNumber, 6, 2, 2); // count
        assertValueRange("sr", gpsSensorNumber, 7, 1, 1); // background_count

        batteryOffScreenOn();
    }

    public void testJobBgVsFg() throws Exception {
        if (isTV()) {
            return;
        }
        batteryOnScreenOff();
        installPackage(DEVICE_SIDE_TEST_APK, true);
        turnScreenOnForReal();
        assertScreenOn();
        allowImmediateSyncs();

        // Background test.
        executeBackground(ACTION_JOB_SCHEDULE, 60_000);
        assertValueRange("jb", "", 6, 1, 1); // count
        assertValueRange("jb", "", 8, 1, 1); // background_count

        // Foreground test.
        executeForeground(ACTION_JOB_SCHEDULE, 60_000);
        assertValueRange("jb", "", 6, 2, 2); // count
        assertValueRange("jb", "", 8, 1, 1); // background_count

        batteryOffScreenOn();
    }

    public void testSyncBgVsFg() throws Exception {
        if (isTV()) {
            return;
        }
        batteryOnScreenOff();
        installPackage(DEVICE_SIDE_TEST_APK, true);
        turnScreenOnForReal();
        assertScreenOn();
        allowImmediateSyncs();

        // Background test.
        executeBackground(ACTION_SYNC, 60_000);
        // Allow one or two syncs in this time frame (not just one) due to unpredictable syncs.
        assertValueRange("sy", DEVICE_SIDE_SYNC_COMPONENT, 6, 1, 2); // count
        assertValueRange("sy", DEVICE_SIDE_SYNC_COMPONENT, 8, 1, 2); // background_count

        // Foreground test.
        executeForeground(ACTION_SYNC, 60_000);
        assertValueRange("sy", DEVICE_SIDE_SYNC_COMPONENT, 6, 2, 4); // count
        assertValueRange("sy", DEVICE_SIDE_SYNC_COMPONENT, 8, 1, 2); // background_count

        batteryOffScreenOn();
    }

    /**
     * Tests whether the on-battery realtime and total realtime values
     * are properly updated in battery stats.
     */
    public void testRealtime() throws Exception {
        batteryOnScreenOff();
        long startingValueRealtime = getLongValue(0, "bt", "", 7);
        long startingValueBatteryRealtime = getLongValue(0, "bt", "", 5);
        // After going on battery
        Thread.sleep(2000);
        batteryOffScreenOn();
        // After going off battery
        Thread.sleep(2000);

        long currentValueRealtime = getLongValue(0, "bt", "", 7);
        long currentValueBatteryRealtime = getLongValue(0, "bt", "", 5);

        // Total realtime increase should be 4000ms at least
        assertTrue(currentValueRealtime >= startingValueRealtime + 4000);
        // But not too much more
        assertTrue(currentValueRealtime < startingValueRealtime + 6000);
        // Battery on realtime should be more than 2000 but less than 4000
        assertTrue(currentValueBatteryRealtime >= startingValueBatteryRealtime + 2000);
        assertTrue(currentValueBatteryRealtime < startingValueBatteryRealtime + 4000);
    }

    /**
     * Tests the total duration reported for jobs run on the job scheduler.
     */
    public void testJobDuration() throws Exception {
        batteryOnScreenOff();

        installPackage(DEVICE_SIDE_TEST_APK, true);
        allowImmediateSyncs();

        runDeviceTests(DEVICE_SIDE_TEST_PACKAGE, ".BatteryStatsJobDurationTests",
                "testJobDuration");

        // Should be approximately 15000 ms (3 x 5000 ms). Use 0.8x and 2x as the lower and upper
        // bounds to account for possible errors due to thread scheduling and cpu load.
        assertValueRange("jb", DEVICE_SIDE_JOB_COMPONENT, 5, (long) (15000 * 0.8), 15000 * 2);
        batteryOffScreenOn();
    }

    /**
     * Tests the total duration and # of syncs reported for sync activities.
     */
    public void testSyncs() throws Exception {
        batteryOnScreenOff();

        installPackage(DEVICE_SIDE_TEST_APK, true);
        allowImmediateSyncs();

        runDeviceTests(DEVICE_SIDE_TEST_PACKAGE, ".BatteryStatsSyncTest", "testRunSyncs");

        // First, check the count, which should be 10.
        // (It could be 11, if the initial sync actually happened before getting cancelled.)
        assertValueRange("sy", DEVICE_SIDE_SYNC_COMPONENT, 6, 10L, 11L);

        // Should be approximately, but at least 10 seconds. Use 2x as the upper
        // bounds to account for possible errors due to thread scheduling and cpu load.
        assertValueRange("sy", DEVICE_SIDE_SYNC_COMPONENT, 5, 10000, 10000 * 2);
    }

    private int getUid() throws Exception {
        String uidLine = getDevice().executeShellCommand("cmd package list packages -U "
                + DEVICE_SIDE_TEST_PACKAGE);
        String[] uidLineParts = uidLine.split(":");
        // 3rd entry is package uid
        assertTrue(uidLineParts.length > 2);
        int uid = Integer.parseInt(uidLineParts[2].trim());
        assertTrue(uid > 10000);
        return uid;
    }

    private void assertApproximateTimeInState(int index, long duration) throws Exception {
        assertValueRange("st", "", index, (long) (0.8 * duration), 2 * duration);
    }

    /**
     * Verifies that the recorded time for the specified tag and name in the test package
     * is within the specified range.
     */
    private void assertValueRange(String tag, String optionalAfterTag,
            int index, long min, long max) throws Exception {
        int uid = getUid();
        long value = getLongValue(uid, tag, optionalAfterTag, index);
        assertTrue("Value " + value + " is less than min " + min, value >= min);
        assertTrue("Value " + value + " is greater than max " + max, value <= max);
    }

    /**
     * Returns a particular long value from a line matched by uid, tag and the optionalAfterTag.
     */
    private long getLongValue(int uid, String tag, String optionalAfterTag, int index)
            throws Exception {
        String dumpsys = getDevice().executeShellCommand("dumpsys batterystats --checkin");
        String[] lines = dumpsys.split("\n");
        long value = 0;
        if (optionalAfterTag == null) {
            optionalAfterTag = "";
        }
        for (int i = lines.length - 1; i >= 0; i--) {
            String line = lines[i];
            if (line.contains(uid + ",l," + tag + "," + optionalAfterTag)
                    || (!optionalAfterTag.equals("") &&
                        line.contains(uid + ",l," + tag + ",\"" + optionalAfterTag))) {
                String[] wlParts = line.split(",");
                value = Long.parseLong(wlParts[index]);
            }
        }
        return value;
    }

    /**
     * Runs a (background) service to perform the given action, and waits for
     * the device to report that the action has finished (via a logcat message) before returning.
     * @param actionValue one of the constants in BatteryStatsBgVsFgActions indicating the desired
     *                    action to perform.
     * @param maxTimeMs max time to wait (in ms) for action to report that it has completed.
     * @return A string, representing a random integer, assigned to this particular request for the
     *                     device to perform the given action. This value can be used to receive
     *                     communications via logcat from the device about this action.
     */
    private String executeBackground(String actionValue, int maxTimeMs) throws Exception {
        String requestCode = executeBackground(actionValue);
        String searchString = getCompletedActionString(actionValue, requestCode);
        checkLogcatForText(BG_VS_FG_TAG, searchString, maxTimeMs);
        return requestCode;
    }

    /**
     * Runs a (background) service to perform the given action.
     * @param actionValue one of the constants in BatteryStatsBgVsFgActions indicating the desired
     *                    action to perform.
     * @return A string, representing a random integer, assigned to this particular request for the
      *                     device to perform the given action. This value can be used to receive
      *                     communications via logcat from the device about this action.
     */
    private String executeBackground(String actionValue) throws Exception {
        allowBackgroundServices();
        String requestCode = Integer.toString(new Random().nextInt());
        getDevice().executeShellCommand(String.format(
                "am startservice -n '%s' -e %s %s -e %s %s",
                DEVICE_SIDE_BG_SERVICE_COMPONENT,
                KEY_ACTION, actionValue,
                KEY_REQUEST_CODE, requestCode));
        return requestCode;
    }

    /** Required to successfully start a background service from adb in O. */
    private void allowBackgroundServices() throws Exception {
        getDevice().executeShellCommand(String.format(
                "cmd deviceidle tempwhitelist %s", DEVICE_SIDE_TEST_PACKAGE));
    }

    /** Make the test-app standby-active so it can run syncs and jobs immediately. */
    protected void allowImmediateSyncs() throws Exception {
        getDevice().executeShellCommand("am set-standby-bucket "
                + DEVICE_SIDE_TEST_PACKAGE + " active");
    }

    /**
     * Runs an activity (in the foreground) to perform the given action, and waits
     * for the device to report that the action has finished (via a logcat message) before returning.
     * @param actionValue one of the constants in BatteryStatsBgVsFgActions indicating the desired
     *                    action to perform.
     * @param maxTimeMs max time to wait (in ms) for action to report that it has completed.
     * @return A string, representing a random integer, assigned to this particular request for the
     *                     device to perform the given action. This value can be used to receive
     *                     communications via logcat from the device about this action.
     */
    private String executeForeground(String actionValue, int maxTimeMs) throws Exception {
        String requestCode = executeForeground(actionValue);
        String searchString = getCompletedActionString(actionValue, requestCode);
        checkLogcatForText(BG_VS_FG_TAG, searchString, maxTimeMs);
        return requestCode;
    }

    /**
     * Runs an activity (in the foreground) to perform the given action.
     * @param actionValue one of the constants in BatteryStatsBgVsFgActions indicating the desired
     *                    action to perform.
     * @return A string, representing a random integer, assigned to this particular request for the
     *                     device to perform the given action. This value can be used to receive
     *                     communications via logcat from the device about this action.
     */
    private String executeForeground(String actionValue) throws Exception {
        String requestCode = Integer.toString(new Random().nextInt());
        getDevice().executeShellCommand(String.format(
                "am start -n '%s' -e %s %s -e %s %s",
                DEVICE_SIDE_FG_ACTIVITY_COMPONENT,
                KEY_ACTION, actionValue,
                KEY_REQUEST_CODE, requestCode));
        return requestCode;
    }

    /**
     * The string that will be printed in the logcat when the action completes. This needs to be
     * identical to {@link com.android.server.cts.device.batterystats.BatteryStatsBgVsFgActions#tellHostActionFinished}.
     */
    private String getCompletedActionString(String actionValue, String requestCode) {
        return String.format("Completed performing %s for request %s", actionValue, requestCode);
    }

    /** Determine if device is just a TV and is not expected to have proper batterystats. */
    private boolean isTV() throws Exception {
        return hasFeature(FEATURE_LEANBACK_ONLY, false);
    }

    /**
     * Determines if the device has the given feature.
     * Prints a warning if its value differs from requiredAnswer.
     */
    private boolean hasFeature(String featureName, boolean requiredAnswer) throws Exception {
        final String features = getDevice().executeShellCommand("pm list features");
        boolean hasIt = features.contains(featureName);
        if (hasIt != requiredAnswer) {
            LogUtil.CLog.w("Device does " + (requiredAnswer ? "not " : "") + "have feature "
                    + featureName);
        }
        return hasIt;
    }
}
