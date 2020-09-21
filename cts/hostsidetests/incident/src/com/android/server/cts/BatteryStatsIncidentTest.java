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

import android.os.BatteryStatsProto;
import android.os.ControllerActivityProto;
import android.os.SystemProto;
import android.os.TimerProto;
import android.os.UidProto;
import android.service.batterystats.BatteryStatsServiceDumpProto;
import android.telephony.NetworkTypeEnum;

/**
 * Test to BatteryStats proto dump.
 */
public class BatteryStatsIncidentTest extends ProtoDumpTestCase {

    @Override
    protected void tearDown() throws Exception {
        batteryOffScreenOn();
        super.tearDown();
    }

    protected void batteryOnScreenOff() throws Exception {
        getDevice().executeShellCommand("dumpsys battery unplug");
        getDevice().executeShellCommand("dumpsys batterystats enable pretend-screen-off");
    }

    protected void batteryOffScreenOn() throws Exception {
        getDevice().executeShellCommand("dumpsys battery reset");
        getDevice().executeShellCommand("dumpsys batterystats disable pretend-screen-off");
    }

    /**
     * Tests that batterystats is dumped to proto with sane values.
     */
    public void testBatteryStatsServiceDump() throws Exception {
        batteryOnScreenOff();
        Thread.sleep(5000); // Allow some time for battery data to accumulate.

        final BatteryStatsServiceDumpProto dump = getDump(BatteryStatsServiceDumpProto.parser(),
                "dumpsys batterystats --proto");

        verifyBatteryStatsServiceDumpProto(dump, PRIVACY_NONE);

        batteryOffScreenOn();
    }

    static void verifyBatteryStatsServiceDumpProto(BatteryStatsServiceDumpProto dump, final int filterLevel) throws Exception {
        final BatteryStatsProto bs = dump.getBatterystats();
        assertNotNull(bs);

        // Proto dumps were finalized when the batterystats report version was ~29 and the parcel
        // version was ~172.
        assertTrue(29 <= bs.getReportVersion());
        assertTrue(172 <= bs.getParcelVersion());
        assertNotNull(bs.getStartPlatformVersion());
        assertFalse(bs.getStartPlatformVersion().isEmpty());
        assertNotNull(bs.getEndPlatformVersion());
        assertFalse(bs.getEndPlatformVersion().isEmpty());

        for (UidProto u : bs.getUidsList()) {
            testUidProto(u, filterLevel);
        }

        testSystemProto(bs.getSystem());
    }

    private static void testControllerActivityProto(ControllerActivityProto ca) throws Exception {
        assertNotNull(ca);

        assertTrue(0 <= ca.getIdleDurationMs());
        assertTrue(0 <= ca.getRxDurationMs());
        assertTrue(0 <= ca.getPowerMah());
        for (ControllerActivityProto.TxLevel tx : ca.getTxList()) {
            assertTrue(0 <= tx.getDurationMs());
        }
    }

    private static void testBatteryLevelStep(SystemProto.BatteryLevelStep bls) throws Exception {
        assertNotNull(bls);

        assertTrue(0 < bls.getDurationMs());
        assertTrue(0 <= bls.getLevel());
        assertTrue(100 >= bls.getLevel());

        assertTrue(SystemProto.BatteryLevelStep.DisplayState.getDescriptor().getValues()
                .contains(bls.getDisplayState().getValueDescriptor()));
        assertTrue(SystemProto.BatteryLevelStep.PowerSaveMode.getDescriptor().getValues()
                .contains(bls.getPowerSaveMode().getValueDescriptor()));
        assertTrue(SystemProto.BatteryLevelStep.IdleMode.getDescriptor().getValues()
                .contains(bls.getIdleMode().getValueDescriptor()));
    }

    private static void testSystemProto(SystemProto s) throws Exception {
        final long epsilon = 500; // Allow ~500 ms of error when comparing times.
        assertNotNull(s);

        SystemProto.Battery b = s.getBattery();
        assertTrue(0 < b.getStartClockTimeMs());
        assertTrue(0 <= b.getStartCount());
        long totalRealtimeMs = b.getTotalRealtimeMs();
        long totalUptimeMs = b.getTotalUptimeMs();
        assertTrue(0 <= totalUptimeMs);
        assertTrue(totalUptimeMs <= totalRealtimeMs + epsilon);
        long batteryRealtimeMs = b.getBatteryRealtimeMs();
        long batteryUptimeMs = b.getBatteryUptimeMs();
        assertTrue(0 <= batteryUptimeMs);
        assertTrue(batteryUptimeMs <= batteryRealtimeMs + epsilon);
        assertTrue("Battery realtime (" + batteryRealtimeMs + ") is greater than total realtime (" + totalRealtimeMs + ")",
            batteryRealtimeMs <= totalRealtimeMs + epsilon);
        assertTrue(batteryUptimeMs <= totalUptimeMs + epsilon);
        long screenOffRealtimeMs = b.getScreenOffRealtimeMs();
        long screenOffUptimeMs = b.getScreenOffUptimeMs();
        assertTrue(0 <= screenOffUptimeMs);
        assertTrue(screenOffUptimeMs <= screenOffRealtimeMs + epsilon);
        assertTrue(screenOffRealtimeMs <= totalRealtimeMs + epsilon);
        assertTrue(screenOffUptimeMs <= totalUptimeMs + epsilon);
        long screenDozeDurationMs = b.getScreenDozeDurationMs();
        assertTrue(0 <= screenDozeDurationMs);
        assertTrue(screenDozeDurationMs <= screenOffRealtimeMs + epsilon);
        assertTrue(0 < b.getEstimatedBatteryCapacityMah());
        long minLearnedCapacityUah = b.getMinLearnedBatteryCapacityUah();
        long maxLearnedCapacityUah = b.getMaxLearnedBatteryCapacityUah();
        assertTrue(0 <= minLearnedCapacityUah);
        assertTrue(minLearnedCapacityUah <= maxLearnedCapacityUah);

        SystemProto.BatteryDischarge bd = s.getBatteryDischarge();
        int lowerBound = bd.getLowerBoundSinceCharge();
        int upperBound = bd.getUpperBoundSinceCharge();
        assertTrue(0 <= lowerBound);
        assertTrue(lowerBound <= upperBound);
        assertTrue(0 <= bd.getScreenOnSinceCharge());
        int screenOff = bd.getScreenOffSinceCharge();
        int screenDoze = bd.getScreenDozeSinceCharge();
        assertTrue(0 <= screenDoze);
        assertTrue(screenDoze <= screenOff);
        long totalMah = bd.getTotalMah();
        long totalMahScreenOff = bd.getTotalMahScreenOff();
        long totalMahScreenDoze = bd.getTotalMahScreenDoze();
        long totalMahLightDoze = bd.getTotalMahLightDoze();
        long totalMahDeepDoze = bd.getTotalMahDeepDoze();
        assertTrue(0 <= totalMahScreenDoze);
        assertTrue(0 <= totalMahLightDoze);
        assertTrue(0 <= totalMahDeepDoze);
        assertTrue(totalMahScreenDoze <= totalMahScreenOff);
        assertTrue(totalMahLightDoze <= totalMahScreenOff);
        assertTrue(totalMahDeepDoze <= totalMahScreenOff);
        assertTrue(totalMahScreenOff <= totalMah);

        assertTrue(-1 <= s.getChargeTimeRemainingMs());
        assertTrue(-1 <= s.getDischargeTimeRemainingMs());

        for (SystemProto.BatteryLevelStep bls : s.getChargeStepList()) {
            testBatteryLevelStep(bls);
        }
        for (SystemProto.BatteryLevelStep bls : s.getDischargeStepList()) {
            testBatteryLevelStep(bls);
        }

        for (SystemProto.DataConnection dc : s.getDataConnectionList()) {
            // If isNone is not set, then the name will be a valid network type.
            if (!dc.getIsNone()) {
                assertTrue(NetworkTypeEnum.getDescriptor().getValues()
                        .contains(dc.getName().getValueDescriptor()));
            }
            testTimerProto(dc.getTotal());
        }

        testControllerActivityProto(s.getGlobalBluetoothController());
        testControllerActivityProto(s.getGlobalModemController());
        testControllerActivityProto(s.getGlobalWifiController());

        SystemProto.GlobalNetwork gn = s.getGlobalNetwork();
        assertTrue(0 <= gn.getMobileBytesRx());
        assertTrue(0 <= gn.getMobileBytesTx());
        assertTrue(0 <= gn.getWifiBytesRx());
        assertTrue(0 <= gn.getWifiBytesTx());
        assertTrue(0 <= gn.getMobilePacketsRx());
        assertTrue(0 <= gn.getMobilePacketsTx());
        assertTrue(0 <= gn.getWifiPacketsRx());
        assertTrue(0 <= gn.getWifiPacketsTx());
        assertTrue(0 <= gn.getBtBytesRx());
        assertTrue(0 <= gn.getBtBytesTx());

        SystemProto.GlobalWifi gw = s.getGlobalWifi();
        assertTrue(0 <= gw.getOnDurationMs());
        assertTrue(0 <= gw.getRunningDurationMs());

        for (SystemProto.KernelWakelock kw : s.getKernelWakelockList()) {
            testTimerProto(kw.getTotal());
        }

        SystemProto.Misc m = s.getMisc();
        assertTrue(0 <= m.getScreenOnDurationMs());
        assertTrue(0 <= m.getPhoneOnDurationMs());
        assertTrue(0 <= m.getFullWakelockTotalDurationMs());
        assertTrue(0 <= m.getPartialWakelockTotalDurationMs());
        assertTrue(0 <= m.getMobileRadioActiveDurationMs());
        assertTrue(0 <= m.getMobileRadioActiveAdjustedTimeMs());
        assertTrue(0 <= m.getMobileRadioActiveCount());
        assertTrue(0 <= m.getMobileRadioActiveUnknownDurationMs());
        assertTrue(0 <= m.getInteractiveDurationMs());
        assertTrue(0 <= m.getBatterySaverModeEnabledDurationMs());
        assertTrue(0 <= m.getNumConnectivityChanges());
        assertTrue(0 <= m.getDeepDozeEnabledDurationMs());
        assertTrue(0 <= m.getDeepDozeCount());
        assertTrue(0 <= m.getDeepDozeIdlingDurationMs());
        assertTrue(0 <= m.getDeepDozeIdlingCount());
        assertTrue(0 <= m.getLongestDeepDozeDurationMs());
        assertTrue(0 <= m.getLightDozeEnabledDurationMs());
        assertTrue(0 <= m.getLightDozeCount());
        assertTrue(0 <= m.getLightDozeIdlingDurationMs());
        assertTrue(0 <= m.getLightDozeIdlingCount());
        assertTrue(0 <= m.getLongestLightDozeDurationMs());

        for (SystemProto.PhoneSignalStrength pss : s.getPhoneSignalStrengthList()) {
            testTimerProto(pss.getTotal());
        }

        for (SystemProto.PowerUseItem pui : s.getPowerUseItemList()) {
            assertTrue(SystemProto.PowerUseItem.Sipper.getDescriptor().getValues()
                    .contains(pui.getName().getValueDescriptor()));
            assertTrue(0 <= pui.getUid());
            assertTrue(0 <= pui.getComputedPowerMah());
            assertTrue(0 <= pui.getScreenPowerMah());
            assertTrue(0 <= pui.getProportionalSmearMah());
        }

        SystemProto.PowerUseSummary pus = s.getPowerUseSummary();
        assertTrue(0 < pus.getBatteryCapacityMah());
        assertTrue(0 <= pus.getComputedPowerMah());
        double minDrained = pus.getMinDrainedPowerMah();
        double maxDrained = pus.getMaxDrainedPowerMah();
        assertTrue(0 <= minDrained);
        assertTrue(minDrained <= maxDrained);

        for (SystemProto.ResourcePowerManager rpm : s.getResourcePowerManagerList()) {
            assertNotNull(rpm.getName());
            assertFalse(rpm.getName().isEmpty());
            testTimerProto(rpm.getTotal());
            testTimerProto(rpm.getScreenOff());
        }

        for (SystemProto.ScreenBrightness sb : s.getScreenBrightnessList()) {
            testTimerProto(sb.getTotal());
        }

        testTimerProto(s.getSignalScanning());

        for (SystemProto.WakeupReason wr : s.getWakeupReasonList()) {
            testTimerProto(wr.getTotal());
        }

        SystemProto.WifiMulticastWakelockTotal wmwl = s.getWifiMulticastWakelockTotal();
        assertTrue(0 <= wmwl.getDurationMs());
        assertTrue(0 <= wmwl.getCount());

        for (SystemProto.WifiSignalStrength wss : s.getWifiSignalStrengthList()) {
            testTimerProto(wss.getTotal());
        }

        for (SystemProto.WifiState ws : s.getWifiStateList()) {
            assertTrue(SystemProto.WifiState.Name.getDescriptor().getValues()
                    .contains(ws.getName().getValueDescriptor()));
            testTimerProto(ws.getTotal());
        }

        for (SystemProto.WifiSupplicantState wss : s.getWifiSupplicantStateList()) {
            assertTrue(SystemProto.WifiSupplicantState.Name.getDescriptor().getValues()
                    .contains(wss.getName().getValueDescriptor()));
            testTimerProto(wss.getTotal());
        }
    }

    private static void testTimerProto(TimerProto t) throws Exception {
        assertNotNull(t);

        long duration = t.getDurationMs();
        long curDuration = t.getCurrentDurationMs();
        long maxDuration = t.getMaxDurationMs();
        long totalDuration = t.getTotalDurationMs();
        assertTrue(0 <= duration);
        assertTrue(0 <= t.getCount());
        // Not all TimerProtos will have max duration, current duration, or total duration
        // populated, so must tread carefully. Regardless, they should never be negative.
        assertTrue(0 <= curDuration);
        assertTrue(0 <= maxDuration);
        assertTrue(0 <= totalDuration);
        if (maxDuration > 0) {
            assertTrue(curDuration <= maxDuration);
        }
        if (totalDuration > 0) {
            assertTrue(maxDuration <= totalDuration);
            assertTrue("Duration " + duration + " is greater than totalDuration " + totalDuration,
                    duration <= totalDuration);
        }
    }

    private static void testByFrequency(UidProto.Cpu.ByFrequency bf) throws Exception {
        assertNotNull(bf);

        assertTrue(1 <= bf.getFrequencyIndex());
        long total = bf.getTotalDurationMs();
        long screenOff = bf.getScreenOffDurationMs();
        assertTrue(0 <= screenOff);
        assertTrue(screenOff <= total);
    }

    private static void testUidProto(UidProto u, final int filterLevel) throws Exception {
        assertNotNull(u);

        assertTrue(0 <= u.getUid());

        for (UidProto.Package p : u.getPackagesList()) {
            assertNotNull(p.getName());
            assertFalse(p.getName().isEmpty());

            for (UidProto.Package.Service s : p.getServicesList()) {
                assertNotNull(s.getName());
                assertFalse(s.getName().isEmpty());
                assertTrue(0 <= s.getStartDurationMs());
                assertTrue(0 <= s.getStartCount());
                assertTrue(0 <= s.getLaunchCount());
            }
        }

        testControllerActivityProto(u.getBluetoothController());
        testControllerActivityProto(u.getModemController());
        testControllerActivityProto(u.getWifiController());

        UidProto.BluetoothMisc bm = u.getBluetoothMisc();
        testTimerProto(bm.getApportionedBleScan());
        testTimerProto(bm.getBackgroundBleScan());
        testTimerProto(bm.getUnoptimizedBleScan());
        testTimerProto(bm.getBackgroundUnoptimizedBleScan());
        assertTrue(0 <= bm.getBleScanResultCount());
        assertTrue(0 <= bm.getBackgroundBleScanResultCount());

        UidProto.Cpu c = u.getCpu();
        assertTrue(0 <= c.getUserDurationMs());
        assertTrue(0 <= c.getSystemDurationMs());
        for (UidProto.Cpu.ByFrequency bf : c.getByFrequencyList()) {
            testByFrequency(bf);
        }
        for (UidProto.Cpu.ByProcessState bps : c.getByProcessStateList()) {
            assertTrue(UidProto.Cpu.ProcessState.getDescriptor().getValues()
                    .contains(bps.getProcessState().getValueDescriptor()));
            for (UidProto.Cpu.ByFrequency bf : bps.getByFrequencyList()) {
                testByFrequency(bf);
            }
        }

        testTimerProto(u.getAudio());
        testTimerProto(u.getCamera());
        testTimerProto(u.getFlashlight());
        testTimerProto(u.getForegroundActivity());
        testTimerProto(u.getForegroundService());
        testTimerProto(u.getVibrator());
        testTimerProto(u.getVideo());

        for (UidProto.Job j : u.getJobsList()) {
            assertNotNull(j.getName());
            assertFalse(j.getName().isEmpty());

            testTimerProto(j.getTotal());
            testTimerProto(j.getBackground());
        }

        for (UidProto.JobCompletion jc : u.getJobCompletionList()) {
            assertNotNull(jc.getName());
            assertFalse(jc.getName().isEmpty());

            for (UidProto.JobCompletion.ReasonCount rc : jc.getReasonCountList()) {
                assertTrue(0 <= rc.getCount());
            }
        }

        UidProto.Network n = u.getNetwork();
        assertTrue(0 <= n.getMobileBytesRx());
        assertTrue(0 <= n.getMobileBytesTx());
        assertTrue(0 <= n.getWifiBytesRx());
        assertTrue(0 <= n.getWifiBytesTx());
        assertTrue(0 <= n.getBtBytesRx());
        assertTrue(0 <= n.getBtBytesTx());
        assertTrue(0 <= n.getMobilePacketsRx());
        assertTrue(0 <= n.getMobilePacketsTx());
        assertTrue(0 <= n.getWifiPacketsRx());
        assertTrue(0 <= n.getWifiPacketsTx());
        assertTrue(0 <= n.getMobileActiveDurationMs());
        assertTrue(0 <= n.getMobileActiveCount());
        assertTrue(0 <= n.getMobileWakeupCount());
        assertTrue(0 <= n.getWifiWakeupCount());
        assertTrue(0 <= n.getMobileBytesBgRx());
        assertTrue(0 <= n.getMobileBytesBgTx());
        assertTrue(0 <= n.getWifiBytesBgRx());
        assertTrue(0 <= n.getWifiBytesBgTx());
        assertTrue(0 <= n.getMobilePacketsBgRx());
        assertTrue(0 <= n.getMobilePacketsBgTx());
        assertTrue(0 <= n.getWifiPacketsBgRx());
        assertTrue(0 <= n.getWifiPacketsBgTx());

        UidProto.PowerUseItem pui = u.getPowerUseItem();
        assertTrue(0 <= pui.getComputedPowerMah());
        assertTrue(0 <= pui.getScreenPowerMah());
        assertTrue(0 <= pui.getProportionalSmearMah());

        for (UidProto.Process p : u.getProcessList()) {
            assertNotNull(p.getName());
            assertFalse(p.getName().isEmpty());
            assertTrue(0 <= p.getUserDurationMs());
            assertTrue("Process system duration is negative: " + p.getSystemDurationMs(),
                    0 <= p.getSystemDurationMs());
            assertTrue(0 <= p.getForegroundDurationMs());
            assertTrue(0 <= p.getStartCount());
            assertTrue(0 <= p.getAnrCount());
            assertTrue(0 <= p.getCrashCount());
        }

        for (UidProto.StateTime st : u.getStatesList()) {
            assertTrue(UidProto.StateTime.State.getDescriptor().getValues()
                    .contains(st.getState().getValueDescriptor()));
            assertTrue(0 <= st.getDurationMs());
        }

        for (UidProto.Sensor s : u.getSensorsList()) {
            testTimerProto(s.getApportioned());
            testTimerProto(s.getBackground());
        }

        for (UidProto.Sync s : u.getSyncsList()) {
            assertFalse(s.getName().isEmpty());

            testTimerProto(s.getTotal());
            testTimerProto(s.getBackground());
        }

        for (UidProto.UserActivity ua : u.getUserActivityList()) {
            assertTrue(0 <= ua.getCount());
        }

        UidProto.AggregatedWakelock aw = u.getAggregatedWakelock();
        long awPartial = aw.getPartialDurationMs();
        long awBgPartial = aw.getBackgroundPartialDurationMs();
        assertTrue(0 <= awBgPartial);
        assertTrue(awBgPartial <= awPartial);

        for (UidProto.Wakelock w : u.getWakelocksList()) {
            // Unfortunately, apps can legitimately pass an empty string as the wakelock name, so we
            // can't guarantee that wakelock names will be non-empty.
            testTimerProto(w.getFull());
            testTimerProto(w.getPartial());
            testTimerProto(w.getBackgroundPartial());
            testTimerProto(w.getWindow());
        }

        for (UidProto.WakeupAlarm wa : u.getWakeupAlarmList()) {
            assertTrue(0 <= wa.getCount());
        }

        UidProto.Wifi w = u.getWifi();
        assertTrue(0 <= w.getFullWifiLockDurationMs());
        assertTrue(0 <= w.getRunningDurationMs());
        testTimerProto(w.getApportionedScan());
        testTimerProto(w.getBackgroundScan());

        testTimerProto(u.getWifiMulticastWakelock());
    }
}
