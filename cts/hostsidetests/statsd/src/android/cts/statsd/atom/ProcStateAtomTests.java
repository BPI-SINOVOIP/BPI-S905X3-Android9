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
package android.cts.statsd.atom;

import android.app.ProcessStateEnum; // From enums.proto for atoms.proto's UidProcessStateChanged.

import com.android.os.AtomsProto.Atom;
import com.android.os.StatsLog.EventMetricData;

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * Statsd atom tests that are done via app, for atoms that report a uid.
 */
public class ProcStateAtomTests extends DeviceAtomTestCase {

    private static final String TAG = "Statsd.ProcStateAtomTests";

    private static final String DEVICE_SIDE_BG_SERVICE_COMPONENT
            = "com.android.server.cts.device.statsd/.StatsdCtsBackgroundService";
    private static final String DEVICE_SIDE_FG_ACTIVITY_COMPONENT
            = "com.android.server.cts.device.statsd/.StatsdCtsForegroundActivity";
    private static final String DEVICE_SIDE_FG_SERVICE_COMPONENT
            = "com.android.server.cts.device.statsd/.StatsdCtsForegroundService";

    // Constants from the device-side tests (not directly accessible here).
    public static final String KEY_ACTION = "action";
    public static final String ACTION_END_IMMEDIATELY = "action.end_immediately";
    public static final String ACTION_BACKGROUND_SLEEP = "action.background_sleep";
    public static final String ACTION_SLEEP_WHILE_TOP = "action.sleep_top";
    public static final String ACTION_SHOW_APPLICATION_OVERLAY = "action.show_application_overlay";

    // Sleep times (ms) that actions invoke device-side.
    public static final int SLEEP_OF_ACTION_SLEEP_WHILE_TOP = 2_000;
    public static final int SLEEP_OF_ACTION_BACKGROUND_SLEEP = 2_000;
    public static final int SLEEP_OF_FOREGROUND_SERVICE = 2_000;

    private static final int WAIT_TIME_FOR_CONFIG_UPDATE_MS = 200;
    // ActivityManager can take a while to register screen state changes, mandating an extra delay.
    private static final int WAIT_TIME_FOR_CONFIG_AND_SCREEN_MS = 1_000;
    private static final int EXTRA_WAIT_TIME_MS = 1_000; // as buffer when proc state changing.
    private static final int STATSD_REPORT_WAIT_TIME_MS = 500; // make sure statsd finishes log.

    private static final String FEATURE_WATCH = "android.hardware.type.watch";

    // The tests here are using the BatteryStats definition of 'background'.
    private static final Set<Integer> BG_STATES = new HashSet<>(
            Arrays.asList(
                    ProcessStateEnum.PROCESS_STATE_IMPORTANT_BACKGROUND_VALUE,
                    ProcessStateEnum.PROCESS_STATE_TRANSIENT_BACKGROUND_VALUE,
                    ProcessStateEnum.PROCESS_STATE_BACKUP_VALUE,
                    ProcessStateEnum.PROCESS_STATE_SERVICE_VALUE,
                    ProcessStateEnum.PROCESS_STATE_RECEIVER_VALUE,
                    ProcessStateEnum.PROCESS_STATE_HEAVY_WEIGHT_VALUE
            ));

    // Using the BatteryStats definition of 'cached', which is why HOME (etc) are considered cached.
    private static final Set<Integer> CACHED_STATES = new HashSet<>(
            Arrays.asList(
                    ProcessStateEnum.PROCESS_STATE_HOME_VALUE,
                    ProcessStateEnum.PROCESS_STATE_LAST_ACTIVITY_VALUE,
                    ProcessStateEnum.PROCESS_STATE_CACHED_ACTIVITY_VALUE,
                    ProcessStateEnum.PROCESS_STATE_CACHED_ACTIVITY_CLIENT_VALUE,
                    ProcessStateEnum.PROCESS_STATE_CACHED_RECENT_VALUE,
                    ProcessStateEnum.PROCESS_STATE_CACHED_EMPTY_VALUE
            ));

    private static final Set<Integer> MISC_STATES = new HashSet<>(
            Arrays.asList(
                    ProcessStateEnum.PROCESS_STATE_PERSISTENT_VALUE, // TODO: untested
                    ProcessStateEnum.PROCESS_STATE_PERSISTENT_UI_VALUE, // TODO: untested
                    ProcessStateEnum.PROCESS_STATE_TOP_VALUE,
                    ProcessStateEnum.PROCESS_STATE_BOUND_FOREGROUND_SERVICE_VALUE, // TODO: untested
                    ProcessStateEnum.PROCESS_STATE_FOREGROUND_SERVICE_VALUE,
                    ProcessStateEnum.PROCESS_STATE_IMPORTANT_FOREGROUND_VALUE,
                    ProcessStateEnum.PROCESS_STATE_TOP_SLEEPING_VALUE,

                    ProcessStateEnum.PROCESS_STATE_UNKNOWN_VALUE,
                    ProcessStateEnum.PROCESS_STATE_NONEXISTENT_VALUE
            ));

    private static final Set<Integer> ALL_STATES = Stream.of(MISC_STATES, CACHED_STATES, BG_STATES)
            .flatMap(s -> s.stream()).collect(Collectors.toSet());

    private static final Function<Atom, Integer> PROC_STATE_FUNCTION =
            atom -> atom.getUidProcessStateChanged().getState().getNumber();

    private static final int PROC_STATE_ATOM_TAG = Atom.UID_PROCESS_STATE_CHANGED_FIELD_NUMBER;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
    }

    public void testForegroundService() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        Set<Integer> onStates = new HashSet<>(Arrays.asList(
                ProcessStateEnum.PROCESS_STATE_FOREGROUND_SERVICE_VALUE));
        Set<Integer> offStates = complement(onStates);

        List<Set<Integer>> stateSet = Arrays.asList(onStates, offStates); // state sets, in order
        createAndUploadConfig(PROC_STATE_ATOM_TAG, false);  // False: does not use attribution.
        Thread.sleep(WAIT_TIME_FOR_CONFIG_UPDATE_MS);

        executeForegroundService();
        final int waitTime = SLEEP_OF_FOREGROUND_SERVICE + EXTRA_WAIT_TIME_MS;
        Thread.sleep(waitTime + STATSD_REPORT_WAIT_TIME_MS);

        List<EventMetricData> data = getEventMetricDataList();
        popUntilFind(data, onStates, PROC_STATE_FUNCTION); // clear out initial proc states.
        assertStatesOccurred(stateSet, data, waitTime, PROC_STATE_FUNCTION);
    }

    public void testForeground() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        Set<Integer> onStates = new HashSet<>(Arrays.asList(
                ProcessStateEnum.PROCESS_STATE_IMPORTANT_FOREGROUND_VALUE));
        // There are no offStates, since the app remains in foreground until killed.

        List<Set<Integer>> stateSet = Arrays.asList(onStates); // state sets, in order
        createAndUploadConfig(PROC_STATE_ATOM_TAG, false);  // False: does not use attribution.

        Thread.sleep(WAIT_TIME_FOR_CONFIG_AND_SCREEN_MS);

        executeForegroundActivity(ACTION_SHOW_APPLICATION_OVERLAY);
        final int waitTime = EXTRA_WAIT_TIME_MS + 5_000; // Overlay may need to sit there a while.
        Thread.sleep(waitTime + STATSD_REPORT_WAIT_TIME_MS);

        List<EventMetricData> data = getEventMetricDataList();
        popUntilFind(data, onStates, PROC_STATE_FUNCTION); // clear out initial proc states.
        assertStatesOccurred(stateSet, data, 0, PROC_STATE_FUNCTION);
    }

    public void testBackground() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        Set<Integer> onStates = BG_STATES;
        Set<Integer> offStates = complement(onStates);

        List<Set<Integer>> stateSet = Arrays.asList(onStates, offStates); // state sets, in order
        createAndUploadConfig(PROC_STATE_ATOM_TAG, false);  // False: does not use attribution.
        Thread.sleep(WAIT_TIME_FOR_CONFIG_UPDATE_MS);

        executeBackgroundService(ACTION_BACKGROUND_SLEEP);
        final int waitTime = SLEEP_OF_ACTION_BACKGROUND_SLEEP + EXTRA_WAIT_TIME_MS;
        Thread.sleep(waitTime + STATSD_REPORT_WAIT_TIME_MS);

        List<EventMetricData> data = getEventMetricDataList();
        popUntilFind(data, onStates, PROC_STATE_FUNCTION); // clear out initial proc states.
        assertStatesOccurred(stateSet, data, waitTime, PROC_STATE_FUNCTION);
    }

    public void testTop() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        Set<Integer> onStates = new HashSet<>(Arrays.asList(
                ProcessStateEnum.PROCESS_STATE_TOP_VALUE));
        Set<Integer> offStates = complement(onStates);

        List<Set<Integer>> stateSet = Arrays.asList(onStates, offStates); // state sets, in order
        createAndUploadConfig(PROC_STATE_ATOM_TAG, false);  // False: does not use attribution.

        Thread.sleep(WAIT_TIME_FOR_CONFIG_AND_SCREEN_MS);

        executeForegroundActivity(ACTION_SLEEP_WHILE_TOP);
        final int waitTime = SLEEP_OF_ACTION_SLEEP_WHILE_TOP + EXTRA_WAIT_TIME_MS;
        Thread.sleep(waitTime + STATSD_REPORT_WAIT_TIME_MS);

        List<EventMetricData> data = getEventMetricDataList();
        popUntilFind(data, onStates, PROC_STATE_FUNCTION); // clear out initial proc states.
        assertStatesOccurred(stateSet, data, waitTime, PROC_STATE_FUNCTION);
    }

    public void testTopSleeping() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        if (!hasFeature(FEATURE_WATCH, false)) return;
        Set<Integer> onStates = new HashSet<>(Arrays.asList(
                ProcessStateEnum.PROCESS_STATE_TOP_SLEEPING_VALUE));
        Set<Integer> offStates = complement(onStates);

        List<Set<Integer>> stateSet = Arrays.asList(onStates, offStates); // state sets, in order
        createAndUploadConfig(PROC_STATE_ATOM_TAG, false);  //False: does not use attribution.

        turnScreenOn();
        Thread.sleep(WAIT_TIME_FOR_CONFIG_AND_SCREEN_MS);

        executeForegroundActivity(ACTION_SLEEP_WHILE_TOP);
        // ASAP, turn off the screen to make proc state -> top_sleeping.
        turnScreenOff();
        final int waitTime = SLEEP_OF_ACTION_SLEEP_WHILE_TOP + EXTRA_WAIT_TIME_MS;
        Thread.sleep(waitTime + STATSD_REPORT_WAIT_TIME_MS);

        List<EventMetricData> data = getEventMetricDataList();
        popUntilFind(data, new HashSet<>(Arrays.asList(ProcessStateEnum.PROCESS_STATE_TOP_VALUE)),
                PROC_STATE_FUNCTION); // clear out anything prior to it entering TOP.
        popUntilFind(data, onStates, PROC_STATE_FUNCTION); // clear out TOP itself.
        // reset screen back on
        turnScreenOn();
        // Don't check the wait time, since it's up to the system how long top sleeping persists.
        assertStatesOccurred(stateSet, data, 0, PROC_STATE_FUNCTION);
    }

    public void testCached() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        Set<Integer> onStates = CACHED_STATES;
        Set<Integer> offStates = complement(onStates);

        List<Set<Integer>> stateSet = Arrays.asList(onStates, offStates); // state sets, in order
        createAndUploadConfig(PROC_STATE_ATOM_TAG, false);  // False: des not use attribution.
        Thread.sleep(WAIT_TIME_FOR_CONFIG_UPDATE_MS);

        // The schedule is as follows
        // #1. The system may do anything it wants, such as moving the app into a cache state.
        // #2. We move the app into the background.
        // #3. The background process ends, so the app definitely moves to a cache state
        //          (this is the ultimate goal of the test).
        // #4. We start a foreground activity, moving the app out of cache.

        // Start extremely short-lived activity, so app goes into cache state (#1 - #3 above).
        executeBackgroundService(ACTION_END_IMMEDIATELY);
        final int cacheTime = 2_000; // process should be in cached state for up to this long
        Thread.sleep(cacheTime);
        // Now forcibly bring the app out of cache (#4 above).
        executeForegroundActivity(ACTION_SHOW_APPLICATION_OVERLAY);
        // Now check the data *before* the app enters cache again (to avoid another cache event).

        List<EventMetricData> data = getEventMetricDataList();
        // First, clear out any incidental cached states of step #1, prior to step #2.
        popUntilFind(data, BG_STATES, PROC_STATE_FUNCTION);
        // Now clear out the bg state from step #2 (since we are interested in the cache after it).
        popUntilFind(data, onStates, PROC_STATE_FUNCTION);
        // The result is that data should start at step #3, definitively in a cached state.
        assertStatesOccurred(stateSet, data, 1_000, PROC_STATE_FUNCTION);
    }

    public void testValidityOfStates() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        assertFalse("UNKNOWN_TO_PROTO should not be a valid state",
                ALL_STATES.contains(ProcessStateEnum.PROCESS_STATE_UNKNOWN_TO_PROTO_VALUE));
    }

    /**
     * Runs a (background) service to perform the given action.
     * @param actionValue the action code constants indicating the desired action to perform.
     */
    private void executeBackgroundService(String actionValue) throws Exception {
        allowBackgroundServices();
        getDevice().executeShellCommand(String.format(
                "am startservice -n '%s' -e %s %s",
                DEVICE_SIDE_BG_SERVICE_COMPONENT,
                KEY_ACTION, actionValue));
    }

    /**
     * Runs an activity (in the foreground) to perform the given action.
     * @param actionValue the action code constants indicating the desired action to perform.
     */
    private void executeForegroundActivity(String actionValue) throws Exception {
        getDevice().executeShellCommand(String.format(
                "am start -n '%s' -e %s %s",
                DEVICE_SIDE_FG_ACTIVITY_COMPONENT,
                KEY_ACTION, actionValue));
    }

    /**
     * Runs a simple foreground service.
     */
    private void executeForegroundService() throws Exception {
        executeForegroundActivity(ACTION_END_IMMEDIATELY);
        getDevice().executeShellCommand(String.format(
                "am startservice -n '%s'", DEVICE_SIDE_FG_SERVICE_COMPONENT));
    }

    /** Returns the a set containing elements of a that are not elements of b. */
    private Set<Integer> difference(Set<Integer> a, Set<Integer> b) {
        Set<Integer> result = new HashSet<Integer>(a);
        result.removeAll(b);
        return result;
    }

    /** Returns the set of all states that are not in set. */
    private Set<Integer> complement(Set<Integer> set) {
        return difference(ALL_STATES, set);
    }
}
