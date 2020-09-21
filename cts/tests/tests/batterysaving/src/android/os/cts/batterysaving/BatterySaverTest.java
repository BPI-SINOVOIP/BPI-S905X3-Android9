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

import static com.android.compatibility.common.util.BatteryUtils.enableBatterySaver;
import static com.android.compatibility.common.util.BatteryUtils.runDumpsysBatteryUnplug;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import android.os.PowerManager;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests related to battery saver:
 *
 * atest $ANDROID_BUILD_TOP/cts/tests/tests/batterysaving/src/android/os/cts/batterysaving/BatterySaverTest.java
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class BatterySaverTest extends BatterySavingTestBase {
    /**
     * Enable battery saver and make sure the relevant components get notifed.
     * @throws Exception
     */
    @Test
    public void testActivateBatterySaver() throws Exception {
        assertFalse(getPowerManager().isPowerSaveMode());
        assertEquals(PowerManager.LOCATION_MODE_NO_CHANGE,
                getPowerManager().getLocationPowerSaveMode());

        // Unplug the charger.
        runDumpsysBatteryUnplug();

        // Activate battery saver.
        enableBatterySaver(true);

        // Make sure the job scheduler and the alarm manager are informed.
        waitUntilAlarmForceAppStandby(true);
        waitUntilJobForceAppStandby(true);
        waitUntilForceBackgroundCheck(true);

        // Deactivate.
        // To avoid too much churn, let's sleep a little bit before deactivating.
        Thread.sleep(1000);

        enableBatterySaver(false);

        // Make sure the job scheduler and the alarm manager are informed.
        waitUntilAlarmForceAppStandby(false);
        waitUntilJobForceAppStandby(false);
        waitUntilForceBackgroundCheck(false);
    }
}
