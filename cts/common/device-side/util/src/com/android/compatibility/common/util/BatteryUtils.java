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
package com.android.compatibility.common.util;

import static com.android.compatibility.common.util.SettingsUtils.putGlobalSetting;
import static com.android.compatibility.common.util.TestUtils.waitUntil;

import android.os.BatteryManager;
import android.os.PowerManager;
import android.provider.Settings.Global;
import android.support.test.InstrumentationRegistry;
import android.util.Log;

public class BatteryUtils {
    private static final String TAG = "CtsBatteryUtils";

    private BatteryUtils() {
    }

    public static BatteryManager getBatteryManager() {
        return InstrumentationRegistry.getContext().getSystemService(BatteryManager.class);
    }

    public static PowerManager getPowerManager() {
        return InstrumentationRegistry.getContext().getSystemService(PowerManager.class);
    }

    /** Make the target device think it's off charger. */
    public static void runDumpsysBatteryUnplug() throws Exception {
        SystemUtil.runShellCommandForNoOutput("dumpsys battery unplug");

        Log.d(TAG, "Battery UNPLUGGED");
    }

    /** Reset {@link #runDumpsysBatteryUnplug}.  */
    public static void runDumpsysBatteryReset() throws Exception {
        SystemUtil.runShellCommandForNoOutput(("dumpsys battery reset"));

        Log.d(TAG, "Battery RESET");
    }

    /**
     * Enable / disable battery saver. Note {@link #runDumpsysBatteryUnplug} must have been
     * executed before enabling BS.
     */
    public static void enableBatterySaver(boolean enabled) throws Exception {
        if (enabled) {
            SystemUtil.runShellCommandForNoOutput("cmd power set-mode 1");
            putGlobalSetting(Global.LOW_POWER_MODE, "1");
            waitUntil("Battery saver still off", () -> getPowerManager().isPowerSaveMode());
            waitUntil("Location mode still " + getPowerManager().getLocationPowerSaveMode(),
                    () -> (PowerManager.LOCATION_MODE_NO_CHANGE
                            != getPowerManager().getLocationPowerSaveMode()));

            Thread.sleep(500);
            waitUntil("Force all apps standby still off",
                    () -> SystemUtil.runShellCommand("dumpsys alarm")
                            .contains(" Force all apps standby: true\n"));

        } else {
            SystemUtil.runShellCommandForNoOutput("cmd power set-mode 0");
            putGlobalSetting(Global.LOW_POWER_MODE_STICKY, "0");
            waitUntil("Battery saver still on", () -> !getPowerManager().isPowerSaveMode());
            waitUntil("Location mode still " + getPowerManager().getLocationPowerSaveMode(),
                    () -> (PowerManager.LOCATION_MODE_NO_CHANGE
                            == getPowerManager().getLocationPowerSaveMode()));

            Thread.sleep(500);
            waitUntil("Force all apps standby still on",
                    () -> SystemUtil.runShellCommand("dumpsys alarm")
                            .contains(" Force all apps standby: false\n"));
        }

        AmUtils.waitForBroadcastIdle();
        Log.d(TAG, "Battery saver turned " + (enabled ? "ON" : "OFF"));
    }

    /**
     * Turn on/off screen.
     */
    public static void turnOnScreen(boolean on) throws Exception {
        if (on) {
            SystemUtil.runShellCommandForNoOutput("input keyevent KEYCODE_WAKEUP");
            waitUntil("Device still not interactive", () -> getPowerManager().isInteractive());

        } else {
            SystemUtil.runShellCommandForNoOutput("input keyevent KEYCODE_SLEEP");
            waitUntil("Device still interactive", () -> !getPowerManager().isInteractive());
        }
        AmUtils.waitForBroadcastIdle();
        Log.d(TAG, "Screen turned " + (on ? "ON" : "OFF"));
    }
}
