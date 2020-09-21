/*
 * Copyright (C) 2016 The Android Open Source Project
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

import android.os.BatteryHealthEnum;
import android.os.BatteryPluggedStateEnum;
import android.os.BatteryStatusEnum;
import android.service.battery.BatteryServiceDumpProto;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

/** Test to check that the battery manager properly outputs its dump state. */
public class BatteryIncidentTest extends ProtoDumpTestCase {
    private static final String LEANBACK_FEATURE = "android.software.leanback";

    public void testBatteryServiceDump() throws Exception {
        if (hasBattery(getDevice())) {
            return;
        }

        final BatteryServiceDumpProto dump =
                getDump(BatteryServiceDumpProto.parser(), "dumpsys battery --proto");

        verifyBatteryServiceDumpProto(dump, PRIVACY_NONE);
    }

    static void verifyBatteryServiceDumpProto(BatteryServiceDumpProto dump, final int filterLevel) {
        if (!dump.getIsPresent()) {
            /* If the battery isn't present, no need to run this test. */
            return;
        }

        assertTrue(dump.getPlugged() != BatteryPluggedStateEnum.BATTERY_PLUGGED_WIRELESS);
        assertTrue(dump.getMaxChargingCurrent() >= 0);
        assertTrue(dump.getMaxChargingVoltage() >= 0);
        assertTrue(dump.getChargeCounter() >= 0);
        assertTrue(
                dump.getStatus() != BatteryStatusEnum.BATTERY_STATUS_INVALID);
        assertTrue(
                dump.getHealth() != BatteryHealthEnum.BATTERY_HEALTH_INVALID);
        int scale = dump.getScale();
        assertTrue(scale > 0);
        int level = dump.getLevel();
        assertTrue(level >= 0 && level <= scale);
        assertTrue(dump.getVoltage() > 0);
        assertTrue(dump.getTemperature() > 0);
    }

    static boolean hasBattery(ITestDevice device) throws DeviceNotAvailableException {
        /* Android TV reports that it has a battery, but it doesn't really. */
        return !isLeanback(device);
    }

    private static boolean isLeanback(ITestDevice device) throws DeviceNotAvailableException {
        final String commandOutput = device.executeShellCommand("pm list features");
        return commandOutput.contains(LEANBACK_FEATURE);
    }
}
