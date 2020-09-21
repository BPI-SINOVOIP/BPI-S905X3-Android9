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
package android.cts.statsd.validation;

import static org.junit.Assert.assertTrue;

import android.cts.statsd.atom.DeviceAtomTestCase;
import android.platform.test.annotations.RestrictedBuildTest;
import android.os.BatteryStatsProto;
import android.os.UidProto;
import android.os.UidProto.Wakelock;
import android.os.BatteryPluggedStateEnum;
import android.os.WakeLockLevelEnum;
import android.view.DisplayStateEnum;


import com.android.internal.os.StatsdConfigProto.AtomMatcher;
import com.android.internal.os.StatsdConfigProto.FieldMatcher;
import com.android.internal.os.StatsdConfigProto.FieldValueMatcher;
import com.android.internal.os.StatsdConfigProto.DurationMetric;
import com.android.internal.os.StatsdConfigProto.LogicalOperation;
import com.android.internal.os.StatsdConfigProto.Position;
import com.android.internal.os.StatsdConfigProto.Predicate;
import com.android.internal.os.StatsdConfigProto.SimpleAtomMatcher;
import com.android.internal.os.StatsdConfigProto.SimplePredicate;
import com.android.internal.os.StatsdConfigProto.StatsdConfig;
import com.android.internal.os.StatsdConfigProto.TimeUnit;
import com.android.os.AtomsProto.Atom;
import com.android.os.AtomsProto.PluggedStateChanged;
import com.android.os.AtomsProto.ScreenStateChanged;
import com.android.os.AtomsProto.WakelockStateChanged;
import com.android.os.StatsLog.EventMetricData;
import com.android.os.StatsLog.DimensionsValue;
import com.android.os.StatsLog.DurationBucketInfo;
import com.android.os.StatsLog.DurationMetricData;
import com.android.os.StatsLog.StatsLogReport;
import com.android.tradefed.log.LogUtil.CLog;



import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Side-by-side comparison between statsd and batterystats.
 */
public class ValidationTests extends DeviceAtomTestCase {

    private static final String TAG = "Statsd.ValidationTests";
    private static final boolean ENABLE_LOAD_TEST = false;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
    }

    @Override
    protected void tearDown() throws Exception {
        resetBatteryStatus(); // Undo any unplugDevice().
        turnScreenOn(); // Reset screen to on state
        super.tearDown();
    }

    public void testPartialWakelock() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        resetBatteryStats();
        unplugDevice();
        turnScreenOff();

        final int atomTag = Atom.WAKELOCK_STATE_CHANGED_FIELD_NUMBER;
        Set<Integer> wakelockOn = new HashSet<>(Arrays.asList(
                WakelockStateChanged.State.ACQUIRE_VALUE,
                WakelockStateChanged.State.CHANGE_ACQUIRE_VALUE));
        Set<Integer> wakelockOff = new HashSet<>(Arrays.asList(
                WakelockStateChanged.State.RELEASE_VALUE,
                WakelockStateChanged.State.CHANGE_RELEASE_VALUE));

        final String EXPECTED_TAG = "StatsdPartialWakelock";
        final WakeLockLevelEnum EXPECTED_LEVEL = WakeLockLevelEnum.PARTIAL_WAKE_LOCK;

        // Add state sets to the list in order.
        List<Set<Integer>> stateSet = Arrays.asList(wakelockOn, wakelockOff);

        createAndUploadConfig(atomTag, true);  // True: uses attribution.
        runDeviceTests(DEVICE_SIDE_TEST_PACKAGE, ".AtomTests", "testWakelockState");

        // Sorted list of events in order in which they occurred.
        List<EventMetricData> data = getEventMetricDataList();

        BatteryStatsProto batterystatsProto = getBatteryStatsProto();

        //=================== verify that statsd is correct ===============//
        // Assert that the events happened in the expected order.
        assertStatesOccurred(stateSet, data, WAIT_TIME_SHORT,
                atom -> atom.getWakelockStateChanged().getState().getNumber());

        for (EventMetricData event : data) {
            String tag = event.getAtom().getWakelockStateChanged().getTag();
            WakeLockLevelEnum type = event.getAtom().getWakelockStateChanged().getLevel();
            assertTrue("Expected tag: " + EXPECTED_TAG + ", but got tag: " + tag,
                    tag.equals(EXPECTED_TAG));
            assertTrue("Expected wakelock level: " + EXPECTED_LEVEL + ", but got level: " + type,
                    type == EXPECTED_LEVEL);
        }

        //=================== verify that batterystats is correct ===============//
        int uid = getUid();
        android.os.TimerProto wl =
                getBatteryStatsPartialWakelock(batterystatsProto, uid, EXPECTED_TAG);

        assertNotNull(wl);
        assertTrue(wl.getDurationMs() > 0);
        assertTrue(wl.getCount() == 1);
        assertTrue(wl.getMaxDurationMs() >= 500);
        assertTrue(wl.getMaxDurationMs() < 700);
        assertTrue(wl.getTotalDurationMs() >= 500);
        assertTrue(wl.getTotalDurationMs() < 700);
    }

    @RestrictedBuildTest
    public void testPartialWakelockDuration() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        turnScreenOn(); // To ensure that the ScreenOff later gets logged.
        uploadWakelockDurationBatteryStatsConfig(TimeUnit.CTS);
        Thread.sleep(WAIT_TIME_SHORT);
        resetBatteryStats();
        unplugDevice();
        turnScreenOff();

        runDeviceTests(DEVICE_SIDE_TEST_PACKAGE, ".AtomTests", "testWakelockState");
        Thread.sleep(WAIT_TIME_LONG); // Make sure the one second bucket has ended.


        final String EXPECTED_TAG = "StatsdPartialWakelock";
        final long EXPECTED_TAG_HASH = Long.parseUnsignedLong("15814523794762874414");
        final int EXPECTED_UID = getUid();
        final int MIN_DURATION = 350;
        final int MAX_DURATION = 700;

        BatteryStatsProto batterystatsProto = getBatteryStatsProto();
        HashMap<Integer, HashMap<Long, Long>> statsdWakelockData = getStatsdWakelockData();

        // Get the batterystats wakelock time and make sure it's reasonable.
        android.os.TimerProto bsWakelock =
                getBatteryStatsPartialWakelock(batterystatsProto, EXPECTED_UID, EXPECTED_TAG);
        assertNotNull("Could not find any partial wakelocks with uid " + EXPECTED_UID +
                " and tag " + EXPECTED_TAG + " in BatteryStats", bsWakelock);
        long bsDurationMs = bsWakelock.getTotalDurationMs();
        assertTrue("Wakelock in batterystats with uid " + EXPECTED_UID + " and tag "
                + EXPECTED_TAG + "was too short. Expected " + MIN_DURATION +
                ", received " + bsDurationMs, bsDurationMs >= MIN_DURATION);
        assertTrue("Wakelock in batterystats with uid " + EXPECTED_UID + " and tag "
                + EXPECTED_TAG + "was too long. Expected " + MAX_DURATION +
                ", received " + bsDurationMs, bsDurationMs <= MAX_DURATION);

        // Get the statsd wakelock time and make sure it's reasonable.
        assertTrue("Could not find any wakelocks with uid " + EXPECTED_UID + " in statsd",
                statsdWakelockData.containsKey(EXPECTED_UID));
        assertTrue("Did not find any wakelocks with tag " + EXPECTED_TAG + " in statsd",
                statsdWakelockData.get(EXPECTED_UID).containsKey(EXPECTED_TAG_HASH));
        long statsdDurationMs = statsdWakelockData.get(EXPECTED_UID)
                .get(EXPECTED_TAG_HASH) / 1_000_000;
        assertTrue("Wakelock in statsd with uid " + EXPECTED_UID + " and tag " + EXPECTED_TAG +
                    "was too short. Expected " + MIN_DURATION + ", received " + statsdDurationMs,
                statsdDurationMs >= MIN_DURATION);
        assertTrue("Wakelock in statsd with uid " + EXPECTED_UID + " and tag " + EXPECTED_TAG +
                    "was too long. Expected " + MAX_DURATION + ", received " + statsdDurationMs,
                statsdDurationMs <= MAX_DURATION);

        // Compare batterystats with statsd.
        long difference = Math.abs(statsdDurationMs - bsDurationMs);
        assertTrue("For uid=" + EXPECTED_UID + " tag=" + EXPECTED_TAG + " had " +
                        "BatteryStats=" + bsDurationMs + "ms but statsd=" + statsdDurationMs + "ms",
                difference <= Math.max(bsDurationMs / 10, 10L));
    }

    public void testPartialWakelockLoad() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        if (!ENABLE_LOAD_TEST) return;
        turnScreenOn(); // To ensure that the ScreenOff later gets logged.
        uploadWakelockDurationBatteryStatsConfig(TimeUnit.CTS);
        Thread.sleep(WAIT_TIME_SHORT);
        resetBatteryStats();
        unplugDevice();
        turnScreenOff();

        runDeviceTests(DEVICE_SIDE_TEST_PACKAGE, ".AtomTests", "testWakelockLoad");
        // Give time for stuck wakelocks to increase duration.
        Thread.sleep(10_000);


        final String EXPECTED_TAG = "StatsdPartialWakelock";
        final int EXPECTED_UID = getUid();
        final int NUM_THREADS = 16;
        final int NUM_COUNT_PER_THREAD = 1000;
        final int MAX_DURATION_MS = 15_000;
        final int MIN_DURATION_MS = 1_000;


        BatteryStatsProto batterystatsProto = getBatteryStatsProto();
        HashMap<Integer, HashMap<Long, Long>> statsdWakelockData = getStatsdWakelockData();

        // TODO: this fails because we only have the hashes of the wakelock tags in statsd.
        // If we want to run this test, we need to fix this.

        // Verify batterystats output is reasonable.
        // boolean foundUid = false;
        // for (UidProto uidProto : batterystatsProto.getUidsList()) {
        //     if (uidProto.getUid() == EXPECTED_UID) {
        //         foundUid = true;
        //         CLog.d("Battery stats has the following wakelocks: \n" +
        //                 uidProto.getWakelocksList());
        //         assertTrue("UidProto has size "  + uidProto.getWakelocksList().size() +
        //                 " wakelocks in it. Expected " + NUM_THREADS + " wakelocks.",
        //                 uidProto.getWakelocksList().size() == NUM_THREADS);
        //
        //         for (Wakelock wl : uidProto.getWakelocksList()) {
        //             String tag = wl.getName();
        //             assertTrue("Wakelock tag in batterystats " + tag + " does not contain "
        //                     + "expected tag " + EXPECTED_TAG, tag.contains(EXPECTED_TAG));
        //             assertTrue("Wakelock in batterystats with tag " + tag + " does not have any "
        //                             + "partial wakelock data.", wl.hasPartial());
        //             assertTrue("Wakelock in batterystats with tag " + tag + " tag has count " +
        //                     wl.getPartial().getCount() + " Expected " + NUM_COUNT_PER_THREAD,
        //                     wl.getPartial().getCount() == NUM_COUNT_PER_THREAD);
        //             long bsDurationMs = wl.getPartial().getTotalDurationMs();
        //             assertTrue("Wakelock in batterystats with uid " + EXPECTED_UID + " and tag "
        //                     + EXPECTED_TAG + "was too short. Expected " + MIN_DURATION_MS +
        //                     ", received " + bsDurationMs, bsDurationMs >= MIN_DURATION_MS);
        //             assertTrue("Wakelock in batterystats with uid " + EXPECTED_UID + " and tag "
        //                     + EXPECTED_TAG + "was too long. Expected " + MAX_DURATION_MS +
        //                     ", received " + bsDurationMs, bsDurationMs <= MAX_DURATION_MS);
        //
        //             // Validate statsd.
        //             long statsdDurationNs = statsdWakelockData.get(EXPECTED_UID).get(tag);
        //             long statsdDurationMs = statsdDurationNs / 1_000_000;
        //             long difference = Math.abs(statsdDurationMs - bsDurationMs);
        //             assertTrue("Unusually large difference in wakelock duration for tag: " + tag +
        //                         ". Statsd had duration " + statsdDurationMs +
        //                         " and batterystats had duration " + bsDurationMs,
        //                         difference <= bsDurationMs / 10);
        //
        //         }
        //     }
        // }
        // assertTrue("Did not find uid " + EXPECTED_UID + " in batterystats.", foundUid);
        //
        // // Assert that the wakelock appears in statsd and is correct.
        // assertTrue("Could not find any wakelocks with uid " + EXPECTED_UID + " in statsd",
        //         statsdWakelockData.containsKey(EXPECTED_UID));
        // HashMap<String, Long> expectedWakelocks = statsdWakelockData.get(EXPECTED_UID);
        // assertEquals("Expected " + NUM_THREADS + " wakelocks in statsd with UID " + EXPECTED_UID +
        //         ". Received " + expectedWakelocks.size(), expectedWakelocks.size(), NUM_THREADS);
    }


    // Helper functions
    // TODO: Refactor these into some utils class.

    public HashMap<Integer, HashMap<Long, Long>> getStatsdWakelockData() throws Exception {
        StatsLogReport report = getStatsLogReport();
        CLog.d("Received the following stats log report: \n" + report.toString());

        // Stores total duration of each wakelock across buckets.
        HashMap<Integer, HashMap<Long, Long>> statsdWakelockData = new HashMap<>();

        for (DurationMetricData data : report.getDurationMetrics().getDataList()) {
            // Gets tag and uid.
            List<DimensionsValue> dims = data.getDimensionLeafValuesInWhatList();
            assertTrue("Expected 2 dimensions, received " + dims.size(), dims.size() == 2);
            boolean hasTag = false;
            long tag = 0;
            int uid = -1;
            long duration = 0;
            for (DimensionsValue dim : dims) {
                if (dim.hasValueInt()) {
                    uid = dim.getValueInt();
                } else if (dim.hasValueStrHash()) {
                    hasTag = true;
                    tag = dim.getValueStrHash();
                }
            }
            assertTrue("Did not receive a tag for the wakelock", hasTag);
            assertTrue("Did not receive a uid for the wakelock", uid != -1);

            // Gets duration.
            for (DurationBucketInfo bucketInfo : data.getBucketInfoList()) {
                duration += bucketInfo.getDurationNanos();
            }

            // Store the info.
            if (statsdWakelockData.containsKey(uid)) {
                HashMap<Long, Long> tagToDuration = statsdWakelockData.get(uid);
                tagToDuration.put(tag, duration);
            } else {
                HashMap<Long, Long> tagToDuration = new HashMap<>();
                tagToDuration.put(tag, duration);
                statsdWakelockData.put(uid, tagToDuration);
            }
        }
        CLog.d("follow: statsdwakelockdata is: " + statsdWakelockData);
        return statsdWakelockData;
    }

    private android.os.TimerProto getBatteryStatsPartialWakelock(BatteryStatsProto proto,
            long uid, String tag) {
        if (proto.getUidsList().size() < 1) {
            CLog.w("Batterystats proto contains no uids");
            return null;
        }
        boolean hadUid = false;
        for (UidProto uidProto : proto.getUidsList()) {
            if (uidProto.getUid() == uid) {
                hadUid = true;
                for (Wakelock wl : uidProto.getWakelocksList()) {
                    if (tag.equals(wl.getName())) {
                        if (wl.hasPartial()) {
                            return wl.getPartial();
                        }
                        CLog.w("Batterystats had wakelock for uid (" + uid + ") "
                                + "with tag (" + tag + ") "
                                + "but it didn't have a partial wakelock");
                    }
                }
                CLog.w("Batterystats didn't have a partial wakelock for uid " + uid
                        + " with tag " + tag);
            }
        }
        if (!hadUid) CLog.w("Batterystats didn't have uid " + uid);
        return null;
    }

    public void uploadWakelockDurationBatteryStatsConfig(TimeUnit bucketsize) throws Exception {
        final int atomTag = Atom.WAKELOCK_STATE_CHANGED_FIELD_NUMBER;
        String metricName = "DURATION_PARTIAL_WAKELOCK_PER_TAG_UID_WHILE_SCREEN_OFF_ON_BATTERY";
        int metricId = metricName.hashCode();

        String partialWakelockIsOnName = "PARTIAL_WAKELOCK_IS_ON";
        int partialWakelockIsOnId = partialWakelockIsOnName.hashCode();

        String partialWakelockOnName = "PARTIAL_WAKELOCK_ON";
        int partialWakelockOnId = partialWakelockOnName.hashCode();
        String partialWakelockOffName = "PARTIAL_WAKELOCK_OFF";
        int partialWakelockOffId = partialWakelockOffName.hashCode();

        String partialWakelockAcquireName = "PARTIAL_WAKELOCK_ACQUIRE";
        int partialWakelockAcquireId = partialWakelockAcquireName.hashCode();
        String partialWakelockChangeAcquireName = "PARTIAL_WAKELOCK_CHANGE_ACQUIRE";
        int partialWakelockChangeAcquireId = partialWakelockChangeAcquireName.hashCode();

        String partialWakelockReleaseName = "PARTIAL_WAKELOCK_RELEASE";
        int partialWakelockReleaseId = partialWakelockReleaseName.hashCode();
        String partialWakelockChangeReleaseName = "PARTIAL_WAKELOCK_CHANGE_RELEASE";
        int partialWakelockChangeReleaseId = partialWakelockChangeReleaseName.hashCode();


        String screenOffBatteryOnName = "SCREEN_IS_OFF_ON_BATTERY";
        int screenOffBatteryOnId = screenOffBatteryOnName.hashCode();

        String screenStateUnknownName = "SCREEN_STATE_UNKNOWN";
        int screenStateUnknownId = screenStateUnknownName.hashCode();
        String screenStateOffName = "SCREEN_STATE_OFF";
        int screenStateOffId = screenStateOffName.hashCode();
        String screenStateOnName = "SCREEN_STATE_ON";
        int screenStateOnId = screenStateOnName.hashCode();
        String screenStateDozeName = "SCREEN_STATE_DOZE";
        int screenStateDozeId = screenStateDozeName.hashCode();
        String screenStateDozeSuspendName = "SCREEN_STATE_DOZE_SUSPEND";
        int screenStateDozeSuspendId = screenStateDozeSuspendName.hashCode();
        String screenStateVrName = "SCREEN_STATE_VR";
        int screenStateVrId = screenStateVrName.hashCode();
        String screenStateOnSuspendName = "SCREEN_STATE_ON_SUSPEND";
        int screenStateOnSuspendId = screenStateOnSuspendName.hashCode();

        String screenTurnedOnName = "SCREEN_TURNED_ON";
        int screenTurnedOnId = screenTurnedOnName.hashCode();
        String screenTurnedOffName = "SCREEN_TURNED_OFF";
        int screenTurnedOffId = screenTurnedOffName.hashCode();

        String screenIsOffName = "SCREEN_IS_OFF";
        int screenIsOffId = screenIsOffName.hashCode();

        String pluggedStateBatteryPluggedNoneName = "PLUGGED_STATE_BATTERY_PLUGGED_NONE";
        int pluggedStateBatteryPluggedNoneId = pluggedStateBatteryPluggedNoneName.hashCode();
        String pluggedStateBatteryPluggedAcName = "PLUGGED_STATE_BATTERY_PLUGGED_AC";
        int pluggedStateBatteryPluggedAcId = pluggedStateBatteryPluggedAcName.hashCode();
        String pluggedStateBatteryPluggedUsbName = "PLUGGED_STATE_BATTERY_PLUGGED_USB";
        int pluggedStateBatteryPluggedUsbId = pluggedStateBatteryPluggedUsbName.hashCode();
        String pluggedStateBatteryPluggedWlName = "PLUGGED_STATE_BATTERY_PLUGGED_WIRELESS";
        int pluggedStateBatteryPluggedWirelessId = pluggedStateBatteryPluggedWlName.hashCode();

        String pluggedStateBatteryPluggedName = "PLUGGED_STATE_BATTERY_PLUGGED";
        int pluggedStateBatteryPluggedId = pluggedStateBatteryPluggedName.hashCode();

        String deviceIsUnpluggedName = "DEVICE_IS_UNPLUGGED";
        int deviceIsUnpluggedId = deviceIsUnpluggedName.hashCode();



        FieldMatcher.Builder dimensions = FieldMatcher.newBuilder()
            .setField(atomTag)
            .addChild(FieldMatcher.newBuilder()
                .setField(WakelockStateChanged.TAG_FIELD_NUMBER))
            .addChild(FieldMatcher.newBuilder()
                .setField(1)
                .setPosition(Position.FIRST)
                .addChild(FieldMatcher.newBuilder()
                    .setField(1)));

        AtomMatcher.Builder wakelockAcquire = AtomMatcher.newBuilder()
            .setId(partialWakelockAcquireId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(atomTag)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(WakelockStateChanged.LEVEL_FIELD_NUMBER)
                    .setEqInt(WakeLockLevelEnum.PARTIAL_WAKE_LOCK_VALUE))
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(WakelockStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(WakelockStateChanged.State.ACQUIRE_VALUE)));

        AtomMatcher.Builder wakelockChangeAcquire = AtomMatcher.newBuilder()
            .setId(partialWakelockChangeAcquireId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(atomTag)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(WakelockStateChanged.LEVEL_FIELD_NUMBER)
                    .setEqInt(WakeLockLevelEnum.PARTIAL_WAKE_LOCK_VALUE))
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(WakelockStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(WakelockStateChanged.State.CHANGE_ACQUIRE_VALUE)));

        AtomMatcher.Builder wakelockRelease = AtomMatcher.newBuilder()
            .setId(partialWakelockReleaseId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(atomTag)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(WakelockStateChanged.LEVEL_FIELD_NUMBER)
                    .setEqInt(WakeLockLevelEnum.PARTIAL_WAKE_LOCK_VALUE))
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(WakelockStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(WakelockStateChanged.State.RELEASE_VALUE)));

        AtomMatcher.Builder wakelockChangeRelease = AtomMatcher.newBuilder()
            .setId(partialWakelockChangeReleaseId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(atomTag)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(WakelockStateChanged.LEVEL_FIELD_NUMBER)
                    .setEqInt(WakeLockLevelEnum.PARTIAL_WAKE_LOCK_VALUE))
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(WakelockStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(WakelockStateChanged.State.CHANGE_RELEASE_VALUE)));

        AtomMatcher.Builder wakelockOn = AtomMatcher.newBuilder()
            .setId(partialWakelockOnId)
            .setCombination(AtomMatcher.Combination.newBuilder()
                .setOperation(LogicalOperation.OR)
                .addMatcher(partialWakelockAcquireId)
                .addMatcher(partialWakelockChangeAcquireId));

        AtomMatcher.Builder wakelockOff = AtomMatcher.newBuilder()
            .setId(partialWakelockOffId)
            .setCombination(AtomMatcher.Combination.newBuilder()
                .setOperation(LogicalOperation.OR)
                .addMatcher(partialWakelockReleaseId)
                .addMatcher(partialWakelockChangeReleaseId));


        Predicate.Builder wakelockPredicate = Predicate.newBuilder()
            .setId(partialWakelockIsOnId)
            .setSimplePredicate(SimplePredicate.newBuilder()
                .setStart(partialWakelockOnId)
                .setStop(partialWakelockOffId)
                .setCountNesting(true)
                .setDimensions(dimensions));

        AtomMatcher.Builder pluggedStateBatteryPluggedNone = AtomMatcher.newBuilder()
            .setId(pluggedStateBatteryPluggedNoneId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(Atom.PLUGGED_STATE_CHANGED_FIELD_NUMBER)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(PluggedStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(BatteryPluggedStateEnum.BATTERY_PLUGGED_NONE_VALUE)));

        AtomMatcher.Builder pluggedStateBatteryPluggedAc = AtomMatcher.newBuilder()
            .setId(pluggedStateBatteryPluggedAcId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(Atom.PLUGGED_STATE_CHANGED_FIELD_NUMBER)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(PluggedStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(BatteryPluggedStateEnum.BATTERY_PLUGGED_AC_VALUE)));

        AtomMatcher.Builder pluggedStateBatteryPluggedUsb = AtomMatcher.newBuilder()
            .setId(pluggedStateBatteryPluggedUsbId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(Atom.PLUGGED_STATE_CHANGED_FIELD_NUMBER)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(PluggedStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(BatteryPluggedStateEnum.BATTERY_PLUGGED_USB_VALUE)));

        AtomMatcher.Builder pluggedStateBatteryPluggedWireless = AtomMatcher.newBuilder()
            .setId(pluggedStateBatteryPluggedWirelessId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(Atom.PLUGGED_STATE_CHANGED_FIELD_NUMBER)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(PluggedStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(BatteryPluggedStateEnum.BATTERY_PLUGGED_WIRELESS_VALUE)));

        AtomMatcher.Builder pluggedStateBatteryPlugged = AtomMatcher.newBuilder()
            .setId(pluggedStateBatteryPluggedId)
            .setCombination(AtomMatcher.Combination.newBuilder()
                .setOperation(LogicalOperation.OR)
                .addMatcher(pluggedStateBatteryPluggedAcId)
                .addMatcher(pluggedStateBatteryPluggedUsbId)
                .addMatcher(pluggedStateBatteryPluggedWirelessId));

        Predicate.Builder deviceIsUnplugged = Predicate.newBuilder()
            .setId(deviceIsUnpluggedId)
            .setSimplePredicate(SimplePredicate.newBuilder()
                .setStart(pluggedStateBatteryPluggedNoneId)
                .setStop(pluggedStateBatteryPluggedId)
                .setCountNesting(false));

        AtomMatcher.Builder screenStateUnknown = AtomMatcher.newBuilder()
            .setId(screenStateUnknownId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(Atom.SCREEN_STATE_CHANGED_FIELD_NUMBER)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(ScreenStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(DisplayStateEnum.DISPLAY_STATE_UNKNOWN_VALUE)));

        AtomMatcher.Builder screenStateOff = AtomMatcher.newBuilder()
            .setId(screenStateOffId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(Atom.SCREEN_STATE_CHANGED_FIELD_NUMBER)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(ScreenStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(DisplayStateEnum.DISPLAY_STATE_OFF_VALUE)));

        AtomMatcher.Builder screenStateOn = AtomMatcher.newBuilder()
            .setId(screenStateOnId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(Atom.SCREEN_STATE_CHANGED_FIELD_NUMBER)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(ScreenStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(DisplayStateEnum.DISPLAY_STATE_ON_VALUE)));

        AtomMatcher.Builder screenStateDoze = AtomMatcher.newBuilder()
            .setId(screenStateDozeId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(Atom.SCREEN_STATE_CHANGED_FIELD_NUMBER)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(ScreenStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(DisplayStateEnum.DISPLAY_STATE_DOZE_VALUE)));

        AtomMatcher.Builder screenStateDozeSuspend = AtomMatcher.newBuilder()
            .setId(screenStateDozeSuspendId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(Atom.SCREEN_STATE_CHANGED_FIELD_NUMBER)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(ScreenStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(DisplayStateEnum.DISPLAY_STATE_DOZE_SUSPEND_VALUE)));

        AtomMatcher.Builder screenStateVr = AtomMatcher.newBuilder()
            .setId(screenStateVrId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(Atom.SCREEN_STATE_CHANGED_FIELD_NUMBER)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(ScreenStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(DisplayStateEnum.DISPLAY_STATE_VR_VALUE)));

        AtomMatcher.Builder screenStateOnSuspend = AtomMatcher.newBuilder()
            .setId(screenStateOnSuspendId)
            .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                .setAtomId(Atom.SCREEN_STATE_CHANGED_FIELD_NUMBER)
                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                    .setField(ScreenStateChanged.STATE_FIELD_NUMBER)
                    .setEqInt(DisplayStateEnum.DISPLAY_STATE_ON_SUSPEND_VALUE)));


        AtomMatcher.Builder screenTurnedOff = AtomMatcher.newBuilder()
            .setId(screenTurnedOffId)
            .setCombination(AtomMatcher.Combination.newBuilder()
                .setOperation(LogicalOperation.OR)
                .addMatcher(screenStateOffId)
                .addMatcher(screenStateDozeId)
                .addMatcher(screenStateDozeSuspendId)
                .addMatcher(screenStateUnknownId));

        AtomMatcher.Builder screenTurnedOn = AtomMatcher.newBuilder()
            .setId(screenTurnedOnId)
            .setCombination(AtomMatcher.Combination.newBuilder()
                .setOperation(LogicalOperation.OR)
                .addMatcher(screenStateOnId)
                .addMatcher(screenStateOnSuspendId)
                .addMatcher(screenStateVrId));

        Predicate.Builder screenIsOff = Predicate.newBuilder()
            .setId(screenIsOffId)
            .setSimplePredicate(SimplePredicate.newBuilder()
                .setStart(screenTurnedOffId)
                .setStop(screenTurnedOnId)
                .setCountNesting(false));


        Predicate.Builder screenOffBatteryOn = Predicate.newBuilder()
            .setId(screenOffBatteryOnId)
            .setCombination(Predicate.Combination.newBuilder()
                .setOperation(LogicalOperation.AND)
                .addPredicate(screenIsOffId)
                .addPredicate(deviceIsUnpluggedId));

        StatsdConfig.Builder builder = createConfigBuilder();
        builder.addDurationMetric(DurationMetric.newBuilder()
                .setId(metricId)
                .setWhat(partialWakelockIsOnId)
                .setCondition(screenOffBatteryOnId)
                .setDimensionsInWhat(dimensions)
                .setBucket(bucketsize))
            .addAtomMatcher(wakelockAcquire)
            .addAtomMatcher(wakelockChangeAcquire)
            .addAtomMatcher(wakelockRelease)
            .addAtomMatcher(wakelockChangeRelease)
            .addAtomMatcher(wakelockOn)
            .addAtomMatcher(wakelockOff)
            .addAtomMatcher(pluggedStateBatteryPluggedNone)
            .addAtomMatcher(pluggedStateBatteryPluggedAc)
            .addAtomMatcher(pluggedStateBatteryPluggedUsb)
            .addAtomMatcher(pluggedStateBatteryPluggedWireless)
            .addAtomMatcher(pluggedStateBatteryPlugged)
            .addAtomMatcher(screenStateUnknown)
            .addAtomMatcher(screenStateOff)
            .addAtomMatcher(screenStateOn)
            .addAtomMatcher(screenStateDoze)
            .addAtomMatcher(screenStateDozeSuspend)
            .addAtomMatcher(screenStateVr)
            .addAtomMatcher(screenStateOnSuspend)
            .addAtomMatcher(screenTurnedOff)
            .addAtomMatcher(screenTurnedOn)
            .addPredicate(wakelockPredicate)
            .addPredicate(deviceIsUnplugged)
            .addPredicate(screenIsOff)
            .addPredicate(screenOffBatteryOn);

        uploadConfig(builder);
    }
}
