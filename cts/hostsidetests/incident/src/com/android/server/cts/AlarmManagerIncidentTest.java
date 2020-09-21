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

import com.android.server.AlarmClockMetadataProto;
import com.android.server.AlarmManagerServiceDumpProto;
import com.android.server.AlarmProto;
import com.android.server.BatchProto;
import com.android.server.BroadcastStatsProto;
import com.android.server.ConstantsProto;
import com.android.server.FilterStatsProto;
import com.android.server.ForceAppStandbyTrackerProto;
import com.android.server.ForceAppStandbyTrackerProto.RunAnyInBackgroundRestrictedPackages;
import com.android.server.IdleDispatchEntryProto;
import com.android.server.InFlightProto;
import com.android.server.WakeupEventProto;
import java.util.List;

/**
 * Test to check that the alarm manager service properly outputs its dump state.
 */
public class AlarmManagerIncidentTest extends ProtoDumpTestCase {
    public void testAlarmManagerServiceDump() throws Exception {
        final AlarmManagerServiceDumpProto dump =
                getDump(AlarmManagerServiceDumpProto.parser(), "dumpsys alarm --proto");

        verifyAlarmManagerServiceDumpProto(dump, PRIVACY_NONE);
    }

    static void verifyAlarmManagerServiceDumpProto(AlarmManagerServiceDumpProto dump, final int filterLevel) throws Exception {
        // Times should be positive.
        assertTrue(0 < dump.getCurrentTime());
        assertTrue(0 < dump.getElapsedRealtime());
        // Can be 0 if the time hasn't been changed yet.
        assertTrue(0 <= dump.getLastTimeChangeClockTime());
        assertTrue(0 <= dump.getLastTimeChangeRealtime());

        // ConstantsProto
        ConstantsProto settings = dump.getSettings();
        assertTrue(0 < settings.getMinFuturityDurationMs());
        assertTrue(0 < settings.getMinIntervalDurationMs());
        assertTrue(0 < settings.getListenerTimeoutDurationMs());
        assertTrue(0 < settings.getAllowWhileIdleShortDurationMs());
        assertTrue(0 < settings.getAllowWhileIdleLongDurationMs());
        assertTrue(0 < settings.getAllowWhileIdleWhitelistDurationMs());

        // ForceAppStandbyTrackerProto
        ForceAppStandbyTrackerProto forceAppStandbyTracker = dump.getForceAppStandbyTracker();
        for (int uid : forceAppStandbyTracker.getForegroundUidsList()) {
            // 0 is technically a valid UID.
            assertTrue(0 <= uid);
        }
        for (int aid : forceAppStandbyTracker.getPowerSaveWhitelistAppIdsList()) {
            assertTrue(0 <= aid);
        }
        for (int aid : forceAppStandbyTracker.getTempPowerSaveWhitelistAppIdsList()) {
            assertTrue(0 <= aid);
        }
        for (RunAnyInBackgroundRestrictedPackages r : forceAppStandbyTracker.getRunAnyInBackgroundRestrictedPackagesList()) {
            assertTrue(0 <= r.getUid());
        }

        if (!dump.getIsInteractive()) {
            // These are only valid if is_interactive is false.
            assertTrue(0 < dump.getTimeSinceNonInteractiveMs());
            assertTrue(0 < dump.getMaxWakeupDelayMs());
            assertTrue(0 < dump.getTimeSinceLastDispatchMs());
            // time_until_next_non_wakeup_delivery_ms could be negative if the delivery time is in the past.
        }

        assertTrue(0 < dump.getTimeUntilNextWakeupMs());
        assertTrue(0 < dump.getTimeSinceLastWakeupMs());
        assertTrue(0 < dump.getTimeSinceLastWakeupSetMs());
        assertTrue(0 <= dump.getTimeChangeEventCount());

        for (int aid : dump.getDeviceIdleUserWhitelistAppIdsList()) {
            assertTrue(0 <= aid);
        }

        // AlarmClockMetadataProto
        for (AlarmClockMetadataProto ac : dump.getNextAlarmClockMetadataList()) {
            assertTrue(0 <= ac.getUser());
            assertTrue(0 < ac.getTriggerTimeMs());
        }

        for (BatchProto b : dump.getPendingAlarmBatchesList()) {
            final long start = b.getStartRealtime();
            final long end = b.getEndRealtime();
            assertTrue("Batch start time (" + start+ ") is negative", 0 <= start);
            assertTrue("Batch end time (" + end + ") is negative", 0 <= end);
            assertTrue("Batch start time (" + start + ") is after its end time (" + end + ")",
                start <= end);
            testAlarmProtoList(b.getAlarmsList(), filterLevel);
        }

        testAlarmProtoList(dump.getPendingUserBlockedBackgroundAlarmsList(), filterLevel);

        testAlarmProto(dump.getPendingIdleUntil(), filterLevel);

        testAlarmProtoList(dump.getPendingWhileIdleAlarmsList(), filterLevel);

        testAlarmProto(dump.getNextWakeFromIdle(), filterLevel);

        testAlarmProtoList(dump.getPastDueNonWakeupAlarmsList(), filterLevel);

        assertTrue(0 <= dump.getDelayedAlarmCount());
        assertTrue(0 <= dump.getTotalDelayTimeMs());
        assertTrue(0 <= dump.getMaxDelayDurationMs());
        assertTrue(0 <= dump.getMaxNonInteractiveDurationMs());

        assertTrue(0 <= dump.getBroadcastRefCount());
        assertTrue(0 <= dump.getPendingIntentSendCount());
        assertTrue(0 <= dump.getPendingIntentFinishCount());
        assertTrue(0 <= dump.getListenerSendCount());
        assertTrue(0 <= dump.getListenerFinishCount());

        for (InFlightProto f : dump.getOutstandingDeliveriesList())  {
            assertTrue(0 <= f.getUid());
            assertTrue(0 < f.getWhenElapsedMs());
            testBroadcastStatsProto(f.getBroadcastStats());
            testFilterStatsProto(f.getFilterStats(), filterLevel);
            if (filterLevel == PRIVACY_AUTO) {
                assertTrue(f.getTag().isEmpty());
            }
        }

        for (AlarmManagerServiceDumpProto.LastAllowWhileIdleDispatch l : dump.getLastAllowWhileIdleDispatchTimesList()) {
            assertTrue(0 <= l.getUid());
            assertTrue(0 < l.getTimeMs());
        }

        for (AlarmManagerServiceDumpProto.TopAlarm ta : dump.getTopAlarmsList()) {
            assertTrue(0 <= ta.getUid());
            testFilterStatsProto(ta.getFilter(), filterLevel);
        }

        for (AlarmManagerServiceDumpProto.AlarmStat as : dump.getAlarmStatsList()) {
            testBroadcastStatsProto(as.getBroadcast());
            for (FilterStatsProto f : as.getFiltersList()) {
                testFilterStatsProto(f, filterLevel);
            }
        }

        for (IdleDispatchEntryProto id : dump.getAllowWhileIdleDispatchesList()) {
            assertTrue(0 <= id.getUid());
            assertTrue(0 <= id.getEntryCreationRealtime());
            assertTrue(0 <= id.getArgRealtime());
            if (filterLevel == PRIVACY_AUTO) {
                assertTrue(id.getTag().isEmpty());
            }
        }

        for (WakeupEventProto we : dump.getRecentWakeupHistoryList()) {
            assertTrue(0 <= we.getUid());
            assertTrue(0 <= we.getWhen());
        }
    }

    private static void testAlarmProtoList(List<AlarmProto> alarms, final int filterLevel) throws Exception {
        for (AlarmProto a : alarms) {
            testAlarmProto(a, filterLevel);
        }
    }

    private static void testAlarmProto(AlarmProto alarm, final int filterLevel) throws Exception {
        assertNotNull(alarm);

        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(alarm.getTag().isEmpty());
            assertTrue(alarm.getListener().isEmpty());
        }
        // alarm.time_until_when_elapsed_ms can be negative if 'when' is in the past.
        assertTrue(0 <= alarm.getWindowLengthMs());
        assertTrue(0 <= alarm.getRepeatIntervalMs());
        assertTrue(0 <= alarm.getCount());
    }

    private static void testBroadcastStatsProto(BroadcastStatsProto broadcast) throws Exception {
        assertNotNull(broadcast);

        assertTrue(0 <= broadcast.getUid());
        assertTrue(0 <= broadcast.getTotalFlightDurationMs());
        assertTrue(0 <= broadcast.getCount());
        assertTrue(0 <= broadcast.getWakeupCount());
        assertTrue(0 <= broadcast.getStartTimeRealtime());
        // Nesting should be non-negative.
        assertTrue(0 <= broadcast.getNesting());
    }

    private static void testFilterStatsProto(FilterStatsProto filter, final int filterLevel) throws Exception {
        assertNotNull(filter);

        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(filter.getTag().isEmpty());
        }
        assertTrue(0 <= filter.getLastFlightTimeRealtime());
        assertTrue(0 <= filter.getTotalFlightDurationMs());
        assertTrue(0 <= filter.getCount());
        assertTrue(0 <= filter.getWakeupCount());
        assertTrue(0 <= filter.getStartTimeRealtime());
        // Nesting should be non-negative.
        assertTrue(0 <= filter.getNesting());
    }
}

