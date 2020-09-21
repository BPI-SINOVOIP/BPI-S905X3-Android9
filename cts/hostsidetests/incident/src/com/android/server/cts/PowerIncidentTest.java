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

import android.app.ProcessStateEnum;
import android.content.IntentProto;
import android.os.BatteryPluggedStateEnum;
import android.os.LooperProto;
import android.os.PowerManagerInternalProto;
import android.os.PowerManagerProto;
import com.android.server.power.PowerManagerServiceDumpProto;
import com.android.server.power.PowerServiceSettingsAndConfigurationDumpProto;
import com.android.server.power.WakeLockProto;

/** Test to check that the power manager properly outputs its dump state. */
public class PowerIncidentTest extends ProtoDumpTestCase {
    private static final int SYSTEM_UID = 1000;

    public void testPowerServiceDump() throws Exception {
        final PowerManagerServiceDumpProto dump =
                getDump(PowerManagerServiceDumpProto.parser(), "dumpsys power --proto");

        verifyPowerManagerServiceDumpProto(dump, PRIVACY_NONE);
    }

    static void verifyPowerManagerServiceDumpProto(PowerManagerServiceDumpProto dump, int filterLevel) {
        assertTrue(dump.getBatteryLevel() >= 0);
        assertTrue(dump.getBatteryLevel() <= 100);

        assertTrue(
                PowerManagerInternalProto.Wakefulness.getDescriptor()
                        .getValues()
                        .contains(dump.getWakefulness().getValueDescriptor()));
        assertTrue(
                BatteryPluggedStateEnum.getDescriptor()
                        .getValues()
                        .contains(dump.getPlugType().getValueDescriptor()));
        assertTrue(
                IntentProto.DockState.getDescriptor()
                        .getValues()
                        .contains(dump.getDockState().getValueDescriptor()));

        final PowerServiceSettingsAndConfigurationDumpProto settingsAndConfiguration =
                dump.getSettingsAndConfiguration();
        assertTrue(settingsAndConfiguration.getMinimumScreenOffTimeoutConfigMs() > 0);
        assertTrue(settingsAndConfiguration.getMaximumScreenDimDurationConfigMs() >= 0);
        assertTrue(settingsAndConfiguration.getMaximumScreenDimRatioConfig() > 0);
        assertTrue(settingsAndConfiguration.getScreenOffTimeoutSettingMs() > 0);
        // Default value is -1.
        assertTrue(settingsAndConfiguration.getSleepTimeoutSettingMs() >= -1);
        assertTrue(settingsAndConfiguration.getMaximumScreenOffTimeoutFromDeviceAdminMs() > 0);
        // -1 is used to disable, so is valid.
        assertTrue(settingsAndConfiguration.getUserActivityTimeoutOverrideFromWindowManagerMs() >= -1);
        final PowerServiceSettingsAndConfigurationDumpProto.ScreenBrightnessSettingLimitsProto
                brightnessLimits = settingsAndConfiguration.getScreenBrightnessSettingLimits();
        int settingMax = brightnessLimits.getSettingMaximum();
        int settingMin = brightnessLimits.getSettingMinimum();
        assertTrue(settingMin >= 0);
        assertTrue(settingMax > 0);
        assertTrue("Brightness limit max setting (" + settingMax + ") is less than min setting (" + settingMin + ")",
                settingMax >= settingMin);
        assertTrue(brightnessLimits.getSettingDefault() > 0);

        final PowerManagerServiceDumpProto.UidStateProto uid = dump.getUidStates(0);
        assertEquals(uid.getUid(), SYSTEM_UID);
        assertEquals(uid.getUidString(), Integer.toString(SYSTEM_UID));
        assertTrue(uid.getIsActive());

        for (PowerManagerServiceDumpProto.UidStateProto us : dump.getUidStatesList()) {
            assertTrue(0 <= us.getUid());
            assertTrue(0 <= us.getNumWakeLocks());
            assertTrue(ProcessStateEnum.getDescriptor()
                    .getValues()
                    .contains(us.getProcessState().getValueDescriptor()));
        }

        final LooperProto looper = dump.getLooper();
        assertNotNull(looper.getThreadName());
        assertTrue(looper.getThreadId() > 0);

        assertTrue(dump.getSuspendBlockersCount() > 0);

        // Check that times/durations are not incorrectly negative.
        assertTrue(dump.getNotifyLongScheduledMs() >= 0);
        assertTrue(dump.getNotifyLongDispatchedMs() >= 0);
        assertTrue(dump.getNotifyLongNextCheckMs() >= 0);
        assertTrue(dump.getLastWakeTimeMs() >= 0);
        assertTrue(dump.getLastSleepTimeMs() >= 0);
        assertTrue(dump.getLastUserActivityTimeMs() >= 0);
        assertTrue(dump.getLastUserActivityTimeNoChangeLightsMs() >= 0);
        assertTrue(dump.getLastInteractivePowerHintTimeMs() >= 0);
        assertTrue(dump.getLastScreenBrightnessBoostTimeMs() >= 0);
        // -1 is a valid value.
        assertTrue(dump.getSleepTimeoutMs() >= -1);
        assertTrue(dump.getScreenOffTimeoutMs() >= 0);
        assertTrue(dump.getScreenDimDurationMs() >= 0);

        for (WakeLockProto wl : dump.getWakeLocksList()) {
            assertTrue(0 <= wl.getAcqMs());
            assertTrue(0 <= wl.getUid());
            assertTrue(0 <= wl.getPid());
        }
    }
}
