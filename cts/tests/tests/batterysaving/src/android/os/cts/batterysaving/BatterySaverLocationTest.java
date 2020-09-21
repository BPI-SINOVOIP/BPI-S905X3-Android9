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

import static android.provider.Settings.Secure.LOCATION_MODE_OFF;
import static android.provider.Settings.Secure.LOCATION_PROVIDERS_ALLOWED;

import static com.android.compatibility.common.util.BatteryUtils.enableBatterySaver;
import static com.android.compatibility.common.util.BatteryUtils.runDumpsysBatteryUnplug;
import static com.android.compatibility.common.util.BatteryUtils.turnOnScreen;
import static com.android.compatibility.common.util.SettingsUtils.putSecureSetting;
import static com.android.compatibility.common.util.TestUtils.waitUntil;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.location.LocationManager;
import android.os.PowerManager;
import android.provider.Settings;
import android.provider.Settings.Global;
import android.provider.Settings.Secure;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import com.android.compatibility.common.util.CallbackAsserter;
import com.android.compatibility.common.util.RequiredFeatureRule;
import com.android.compatibility.common.util.TestUtils.RunnableWithThrow;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests related to battery saver:
 *
 atest $ANDROID_BUILD_TOP/cts/tests/tests/batterysaving/src/android/os/cts/batterysaving/BatterySaverLocationTest.java
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class BatterySaverLocationTest extends BatterySavingTestBase {
    private static final String TAG = "BatterySaverLocationTest";

    @Rule
    public final RequiredFeatureRule mRequireLocationRule =
            new RequiredFeatureRule(PackageManager.FEATURE_LOCATION);

    @Rule
    public final RequiredFeatureRule mRequireLocationGpsRule =
            new RequiredFeatureRule(PackageManager.FEATURE_LOCATION_GPS);

    /**
     * Test for the {@link PowerManager#LOCATION_MODE_ALL_DISABLED_WHEN_SCREEN_OFF} mode.
     */
    @Test
    public void testLocationAllDisabled() throws Exception {
        assertTrue("Screen is off", getPowerManager().isInteractive());

        assertFalse(getPowerManager().isPowerSaveMode());
        assertEquals(PowerManager.LOCATION_MODE_NO_CHANGE,
                getPowerManager().getLocationPowerSaveMode());

        assertEquals(0, getLocationGlobalKillSwitch());

        // Make sure GPS is enabled.
        putSecureSetting(LOCATION_PROVIDERS_ALLOWED, "+gps");
        assertNotEquals(LOCATION_MODE_OFF, getLocationMode());
        assertTrue(getLocationManager().isLocationEnabled());

        // Unplug the charger and activate battery saver.
        runDumpsysBatteryUnplug();
        enableBatterySaver(true);

        // Skip if the location mode is not what's expected.
        final int mode = getPowerManager().getLocationPowerSaveMode();
        if (mode != PowerManager.LOCATION_MODE_ALL_DISABLED_WHEN_SCREEN_OFF) {
            Log.i(TAG, "Unexpected location power save mode (" + mode + "), skipping.");
            return;
        }

        // Make sure screen is on.
        assertTrue(getPowerManager().isInteractive());

        // Make sure the kill switch is still off.
        assertEquals(0, getLocationGlobalKillSwitch());

        // Make sure location is still enabled.
        assertNotEquals(LOCATION_MODE_OFF, getLocationMode());
        assertTrue(getLocationManager().isLocationEnabled());

        // Turn screen off.
        runWithExpectingLocationCallback(() -> {
            turnOnScreen(false);
            waitUntil("Kill switch still off", () -> getLocationGlobalKillSwitch() == 1);
            assertEquals(LOCATION_MODE_OFF, getLocationMode());
            assertFalse(getLocationManager().isLocationEnabled());
        });

        // On again.
        runWithExpectingLocationCallback(() -> {
            turnOnScreen(true);
            waitUntil("Kill switch still off", () -> getLocationGlobalKillSwitch() == 0);
            assertNotEquals(LOCATION_MODE_OFF, getLocationMode());
            assertTrue(getLocationManager().isLocationEnabled());
        });

        // Off again.
        runWithExpectingLocationCallback(() -> {
            turnOnScreen(false);
            waitUntil("Kill switch still off", () -> getLocationGlobalKillSwitch() == 1);
            assertEquals(LOCATION_MODE_OFF, getLocationMode());
            assertFalse(getLocationManager().isLocationEnabled());
        });

        // Disable battery saver and make sure the kill swtich is off.
        runWithExpectingLocationCallback(() -> {
            enableBatterySaver(false);
            waitUntil("Kill switch still on", () -> getLocationGlobalKillSwitch() == 0);
            assertNotEquals(LOCATION_MODE_OFF, getLocationMode());
            assertTrue(getLocationManager().isLocationEnabled());
        });
    }

    private int getLocationGlobalKillSwitch() {
        return Global.getInt(getContext().getContentResolver(),
                Global.LOCATION_GLOBAL_KILL_SWITCH, 0);
    }

    private int getLocationMode() {
        return Secure.getInt(getContext().getContentResolver(), Secure.LOCATION_MODE, 0);
    }

    private void runWithExpectingLocationCallback(RunnableWithThrow r) throws Exception {
        CallbackAsserter locationModeBroadcastAsserter = CallbackAsserter.forBroadcast(
                new IntentFilter(LocationManager.MODE_CHANGED_ACTION));
        CallbackAsserter locationModeObserverAsserter = CallbackAsserter.forContentUri(
                Settings.Secure.getUriFor(Settings.Secure.LOCATION_PROVIDERS_ALLOWED));

        r.run();

        locationModeBroadcastAsserter.assertCalled("Broadcast not received",
                DEFAULT_TIMEOUT_SECONDS);
        locationModeObserverAsserter.assertCalled("Observer not notified",
                DEFAULT_TIMEOUT_SECONDS);
    }
}
