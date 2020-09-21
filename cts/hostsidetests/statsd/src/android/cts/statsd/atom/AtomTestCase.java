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

import android.os.BatteryStatsProto;
import android.service.batterystats.BatteryStatsServiceDumpProto;
import android.view.DisplayStateEnum;

import com.android.annotations.Nullable;
import com.android.internal.os.StatsdConfigProto.AtomMatcher;
import com.android.internal.os.StatsdConfigProto.EventMetric;
import com.android.internal.os.StatsdConfigProto.FieldFilter;
import com.android.internal.os.StatsdConfigProto.FieldMatcher;
import com.android.internal.os.StatsdConfigProto.FieldValueMatcher;
import com.android.internal.os.StatsdConfigProto.GaugeMetric;
import com.android.internal.os.StatsdConfigProto.Predicate;
import com.android.internal.os.StatsdConfigProto.SimpleAtomMatcher;
import com.android.internal.os.StatsdConfigProto.SimplePredicate;
import com.android.internal.os.StatsdConfigProto.StatsdConfig;
import com.android.internal.os.StatsdConfigProto.TimeUnit;
import com.android.os.AtomsProto.AppBreadcrumbReported;
import com.android.os.AtomsProto.Atom;
import com.android.os.AtomsProto.ScreenStateChanged;
import com.android.os.StatsLog.ConfigMetricsReport;
import com.android.os.StatsLog.ConfigMetricsReportList;
import com.android.os.StatsLog.EventMetricData;
import com.android.os.StatsLog.GaugeMetricData;
import com.android.os.StatsLog.StatsLogReport;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil;

import com.google.common.io.Files;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Date;
import java.util.List;
import java.util.Set;
import java.util.function.Function;

import perfetto.protos.PerfettoConfig.DataSourceConfig;
import perfetto.protos.PerfettoConfig.TraceConfig;
import perfetto.protos.PerfettoConfig.TraceConfig.BufferConfig;
import perfetto.protos.PerfettoConfig.TraceConfig.DataSource;

/**
 * Base class for testing Statsd atoms.
 * Validates reporting of statsd logging based on different events
 */
public class AtomTestCase extends BaseTestCase {

    public static final String UPDATE_CONFIG_CMD = "cmd stats config update";
    public static final String DUMP_REPORT_CMD = "cmd stats dump-report";
    public static final String DUMP_BATTERYSTATS_CMD = "dumpsys batterystats";
    public static final String REMOVE_CONFIG_CMD = "cmd stats config remove";
    public static final String CONFIG_UID = "1000";
    /** ID of the config, which evaluates to -1572883457. */
    public static final long CONFIG_ID = "cts_config".hashCode();

    protected static final int WAIT_TIME_SHORT = 500;
    protected static final int WAIT_TIME_LONG = 2_000;

    protected static final long SCREEN_STATE_CHANGE_TIMEOUT = 4000;
    protected static final long SCREEN_STATE_POLLING_INTERVAL = 500;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        if (statsdDisabled()) {
            return;
        }
        // TODO: need to do these before running real test:
        // 1. compile statsd and push to device
        // 2. make sure StatsCompanionService and incidentd is running
        // 3. start statsd
        // These should go away once we have statsd properly set up.

        // Uninstall to clear the history in case it's still on the device.
        removeConfig(CONFIG_ID);
        getReportList(); // Clears data.
    }

    @Override
    protected void tearDown() throws Exception {
        removeConfig(CONFIG_ID);
        super.tearDown();
    }

    /**
     * Determines whether logcat indicates that incidentd fired since the given device date.
     */
    protected boolean didIncidentdFireSince(String date) throws Exception {
        final String INCIDENTD_TAG = "incidentd";
        final String INCIDENTD_STARTED_STRING = "reportIncident";
        // TODO: Do something more robust than this in case of delayed logging.
        Thread.sleep(1000);
        String log = getLogcatSince(date, String.format(
                "-s %s -e %s", INCIDENTD_TAG, INCIDENTD_STARTED_STRING));
        return log.contains(INCIDENTD_STARTED_STRING);
    }

    /**
     * Determines whether logcat indicates that perfetto fired since the given device date.
     */
    protected boolean didPerfettoStartSince(String date) throws Exception {
        final String PERFETTO_TAG = "perfetto";
        final String PERFETTO_STARTED_STRING = "Enabled tracing";
        final String PERFETTO_STARTED_REGEX = ".*" + PERFETTO_STARTED_STRING + ".*";
        // TODO: Do something more robust than this in case of delayed logging.
        Thread.sleep(1000);
        String log = getLogcatSince(date, String.format(
                "-s %s -e %s", PERFETTO_TAG, PERFETTO_STARTED_REGEX));
        return log.contains(PERFETTO_STARTED_STRING);
    }

    protected static StatsdConfig.Builder createConfigBuilder() {
        return StatsdConfig.newBuilder().setId(CONFIG_ID)
                .addAllowedLogSource("AID_SYSTEM")
                .addAllowedLogSource("AID_BLUETOOTH")
                .addAllowedLogSource(DeviceAtomTestCase.DEVICE_SIDE_TEST_PACKAGE);
    }

    protected void createAndUploadConfig(int atomTag) throws Exception {
        StatsdConfig.Builder conf = createConfigBuilder();
        addAtomEvent(conf, atomTag);
        uploadConfig(conf);
    }

    protected void uploadConfig(StatsdConfig.Builder config) throws Exception {
        uploadConfig(config.build());
    }

    protected void uploadConfig(StatsdConfig config) throws Exception {
        LogUtil.CLog.d("Uploading the following config:\n" + config.toString());
        File configFile = File.createTempFile("statsdconfig", ".config");
        configFile.deleteOnExit();
        Files.write(config.toByteArray(), configFile);
        String remotePath = "/data/local/tmp/" + configFile.getName();
        getDevice().pushFile(configFile, remotePath);
        getDevice().executeShellCommand(
                String.join(" ", "cat", remotePath, "|", UPDATE_CONFIG_CMD,
                        String.valueOf(CONFIG_ID)));
        getDevice().executeShellCommand("rm " + remotePath);
    }

    protected void removeConfig(long configId) throws Exception {
        getDevice().executeShellCommand(
                String.join(" ", REMOVE_CONFIG_CMD, String.valueOf(configId)));
    }

    /** Gets the statsd report and sorts it. Note that this also deletes that report from statsd. */
    protected List<EventMetricData> getEventMetricDataList() throws Exception {
        ConfigMetricsReportList reportList = getReportList();
        assertTrue("Expected one report", reportList.getReportsCount() == 1);
        ConfigMetricsReport report = reportList.getReports(0);

        List<EventMetricData> data = new ArrayList<>();
        for (StatsLogReport metric : report.getMetricsList()) {
            data.addAll(metric.getEventMetrics().getDataList());
        }
        data.sort(Comparator.comparing(EventMetricData::getElapsedTimestampNanos));

        LogUtil.CLog.d("Get EventMetricDataList as following:\n");
        for (EventMetricData d : data) {
            LogUtil.CLog.d("Atom at " + d.getElapsedTimestampNanos() + ":\n" + d.getAtom().toString());
        }
        return data;
    }

    protected List<Atom> getGaugeMetricDataList() throws Exception {
        ConfigMetricsReportList reportList = getReportList();
        assertTrue(reportList.getReportsCount() == 1);
        // only config
        ConfigMetricsReport report = reportList.getReports(0);

        List<Atom> data = new ArrayList<>();
        for (GaugeMetricData gaugeMetricData :
                report.getMetrics(0).getGaugeMetrics().getDataList()) {
            for (Atom atom : gaugeMetricData.getBucketInfo(0).getAtomList()) {
                data.add(atom);
            }
        }

        LogUtil.CLog.d("Get GaugeMetricDataList as following:\n");
        for (Atom d : data) {
            LogUtil.CLog.d("Atom:\n" + d.toString());
        }
        return data;
    }

    protected StatsLogReport getStatsLogReport() throws Exception {
        ConfigMetricsReportList reportList = getReportList();
        assertTrue(reportList.getReportsCount() == 1);
        ConfigMetricsReport report = reportList.getReports(0);
        assertTrue(report.hasUidMap());
        assertEquals(1, report.getMetricsCount());
        return report.getMetrics(0);
    }

    /** Gets the statsd report. Note that this also deletes that report from statsd. */
    protected ConfigMetricsReportList getReportList() throws Exception {
        try {
            ConfigMetricsReportList reportList = getDump(ConfigMetricsReportList.parser(),
                    String.join(" ", DUMP_REPORT_CMD, String.valueOf(CONFIG_ID),
                            "--proto"));
            return reportList;
        } catch (com.google.protobuf.InvalidProtocolBufferException e) {
            LogUtil.CLog.e("Failed to fetch and parse the statsd output report. "
                    + "Perhaps there is not a valid statsd config for the requested "
                    + "uid=" + CONFIG_UID + ", id=" + CONFIG_ID + ".");
            throw (e);
        }
    }

    protected BatteryStatsProto getBatteryStatsProto() throws Exception {
        try {
            BatteryStatsProto batteryStatsProto = getDump(BatteryStatsServiceDumpProto.parser(),
                    String.join(" ", DUMP_BATTERYSTATS_CMD,
                            "--proto")).getBatterystats();
            LogUtil.CLog.d("Got batterystats:\n " + batteryStatsProto.toString());
            return batteryStatsProto;
        } catch (com.google.protobuf.InvalidProtocolBufferException e) {
            LogUtil.CLog.e("Failed to dump batterystats proto");
            throw (e);
        }
    }

    /** Creates a FieldValueMatcher.Builder corresponding to the given field. */
    protected static FieldValueMatcher.Builder createFvm(int field) {
        return FieldValueMatcher.newBuilder().setField(field);
    }

    protected static TraceConfig createPerfettoTraceConfig() {
        return TraceConfig.newBuilder()
            .addBuffers(BufferConfig.newBuilder().setSizeKb(32))
            .addDataSources(DataSource.newBuilder()
                .setConfig(DataSourceConfig.newBuilder()
                    .setName("linux.ftrace")
                    .setTargetBuffer(0)
                    .build()
                )
            )
            .build();
    }

    protected void addAtomEvent(StatsdConfig.Builder conf, int atomTag) throws Exception {
        addAtomEvent(conf, atomTag, new ArrayList<FieldValueMatcher.Builder>());
    }

    /**
     * Adds an event to the config for an atom that matches the given key.
     *
     * @param conf    configuration
     * @param atomTag atom tag (from atoms.proto)
     * @param fvm     FieldValueMatcher.Builder for the relevant key
     */
    protected void addAtomEvent(StatsdConfig.Builder conf, int atomTag,
            FieldValueMatcher.Builder fvm)
            throws Exception {
        addAtomEvent(conf, atomTag, Arrays.asList(fvm));
    }

    /**
     * Adds an event to the config for an atom that matches the given keys.
     *
     * @param conf   configuration
     * @param atomId atom tag (from atoms.proto)
     * @param fvms   list of FieldValueMatcher.Builders to attach to the atom. May be null.
     */
    protected void addAtomEvent(StatsdConfig.Builder conf, int atomId,
            List<FieldValueMatcher.Builder> fvms) throws Exception {

        final String atomName = "Atom" + System.nanoTime();
        final String eventName = "Event" + System.nanoTime();

        SimpleAtomMatcher.Builder sam = SimpleAtomMatcher.newBuilder().setAtomId(atomId);
        if (fvms != null) {
            for (FieldValueMatcher.Builder fvm : fvms) {
                sam.addFieldValueMatcher(fvm);
            }
        }
        conf.addAtomMatcher(AtomMatcher.newBuilder()
                .setId(atomName.hashCode())
                .setSimpleAtomMatcher(sam));
        conf.addEventMetric(EventMetric.newBuilder()
                .setId(eventName.hashCode())
                .setWhat(atomName.hashCode()));
    }

    /**
     * Adds an atom to a gauge metric of a config
     *
     * @param conf      configuration
     * @param atomId    atom id (from atoms.proto)
     * @param dimension dimension is needed for most pulled atoms
     */
    protected void addGaugeAtom(StatsdConfig.Builder conf, int atomId,
            @Nullable FieldMatcher.Builder dimension) throws Exception {
        final String atomName = "Atom" + System.nanoTime();
        final String gaugeName = "Gauge" + System.nanoTime();
        final String predicateName = "SCREEN_IS_ON";
        SimpleAtomMatcher.Builder sam = SimpleAtomMatcher.newBuilder().setAtomId(atomId);
        conf.addAtomMatcher(AtomMatcher.newBuilder()
                .setId(atomName.hashCode())
                .setSimpleAtomMatcher(sam));
        // TODO: change this predicate to something simpler and easier
        final String predicateTrueName = "SCREEN_TURNED_ON";
        final String predicateFalseName = "SCREEN_TURNED_OFF";
        conf.addAtomMatcher(AtomMatcher.newBuilder()
                .setId(predicateTrueName.hashCode())
                .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                        .setAtomId(Atom.SCREEN_STATE_CHANGED_FIELD_NUMBER)
                        .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                                .setField(ScreenStateChanged.STATE_FIELD_NUMBER)
                                .setEqInt(DisplayStateEnum.DISPLAY_STATE_ON_VALUE)
                        )
                )
        )
                // Used to trigger predicate
                .addAtomMatcher(AtomMatcher.newBuilder()
                        .setId(predicateFalseName.hashCode())
                        .setSimpleAtomMatcher(SimpleAtomMatcher.newBuilder()
                                .setAtomId(Atom.SCREEN_STATE_CHANGED_FIELD_NUMBER)
                                .addFieldValueMatcher(FieldValueMatcher.newBuilder()
                                        .setField(ScreenStateChanged.STATE_FIELD_NUMBER)
                                        .setEqInt(DisplayStateEnum.DISPLAY_STATE_OFF_VALUE)
                                )
                        )
                );
        conf.addPredicate(Predicate.newBuilder()
                .setId(predicateName.hashCode())
                .setSimplePredicate(SimplePredicate.newBuilder()
                        .setStart(predicateTrueName.hashCode())
                        .setStop(predicateFalseName.hashCode())
                        .setCountNesting(false)
                )
        );
        GaugeMetric.Builder gaugeMetric = GaugeMetric.newBuilder()
                .setId(gaugeName.hashCode())
                .setWhat(atomName.hashCode())
                .setGaugeFieldsFilter(FieldFilter.newBuilder().setIncludeAll(true).build())
                .setSamplingType(GaugeMetric.SamplingType.ALL_CONDITION_CHANGES)
                .setBucket(TimeUnit.CTS)
                .setCondition(predicateName.hashCode());
        if (dimension != null) {
            gaugeMetric.setDimensionsInWhat(dimension.build());
        }
        conf.addGaugeMetric(gaugeMetric.build());
    }

    /**
     * Asserts that each set of states in stateSets occurs at least once in data.
     * Asserts that the states in data occur in the same order as the sets in stateSets.
     *
     * @param stateSets        A list of set of states, where each set represents an equivalent
     *                         state of the device for the purpose of CTS.
     * @param data             list of EventMetricData from statsd, produced by
     *                         getReportMetricListData()
     * @param wait             expected duration (in ms) between state changes; asserts that the
     *                         actual wait
     *                         time was wait/2 <= actual_wait <= 5*wait. Use 0 to ignore this
     *                         assertion.
     * @param getStateFromAtom expression that takes in an Atom and returns the state it contains
     */
    public void assertStatesOccurred(List<Set<Integer>> stateSets, List<EventMetricData> data,
            int wait, Function<Atom, Integer> getStateFromAtom) {
        // Sometimes, there are more events than there are states.
        // Eg: When the screen turns off, it may go into OFF and then DOZE immediately.
        assertTrue("Too few states found (" + data.size() + ")", data.size() >= stateSets.size());
        int stateSetIndex = 0; // Tracks which state set we expect the data to be in.
        for (int dataIndex = 0; dataIndex < data.size(); dataIndex++) {
            Atom atom = data.get(dataIndex).getAtom();
            int state = getStateFromAtom.apply(atom);
            // If state is in the current state set, we do not assert anything.
            // If it is not, we expect to have transitioned to the next state set.
            if (stateSets.get(stateSetIndex).contains(state)) {
                // No need to assert anything. Just log it.
                LogUtil.CLog.i("The following atom at dataIndex=" + dataIndex + " is "
                        + "in stateSetIndex " + stateSetIndex + ":\n"
                        + data.get(dataIndex).getAtom().toString());
            } else {
                stateSetIndex += 1;
                LogUtil.CLog.i("Assert that the following atom at dataIndex=" + dataIndex + " is"
                        + " in stateSetIndex " + stateSetIndex + ":\n"
                        + data.get(dataIndex).getAtom().toString());
                assertTrue("Missed first state", dataIndex != 0); // should not be on first data
                assertTrue("Too many states (" + (stateSetIndex + 1) + ")",
                        stateSetIndex < stateSets.size());
                assertTrue("Is in wrong state (" + state + ")",
                        stateSets.get(stateSetIndex).contains(state));
                if (wait > 0) {
                    assertTimeDiffBetween(data.get(dataIndex - 1), data.get(dataIndex),
                            wait / 2, wait * 5);
                }
            }
        }
        assertTrue("Too few states (" + (stateSetIndex + 1) + ")",
                stateSetIndex == stateSets.size() - 1);
    }

    /**
     * Removes all elements from data prior to the first occurrence of an element of state. After
     * this method is called, the first element of data (if non-empty) is guaranteed to be an
     * element in state.
     *
     * @param getStateFromAtom expression that takes in an Atom and returns the state it contains
     */
    public void popUntilFind(List<EventMetricData> data, Set<Integer> state,
            Function<Atom, Integer> getStateFromAtom) {
        int firstStateIdx;
        for (firstStateIdx = 0; firstStateIdx < data.size(); firstStateIdx++) {
            Atom atom = data.get(firstStateIdx).getAtom();
            if (state.contains(getStateFromAtom.apply(atom))) {
                break;
            }
        }
        if (firstStateIdx == 0) {
            // First first element already is in state, so there's nothing to do.
            return;
        }
        data.subList(0, firstStateIdx).clear();
    }

    /**
     * Removes all elements from data after to the last occurrence of an element of state. After
     * this method is called, the last element of data (if non-empty) is guaranteed to be an
     * element in state.
     *
     * @param getStateFromAtom expression that takes in an Atom and returns the state it contains
     */
    public void popUntilFindFromEnd(List<EventMetricData> data, Set<Integer> state,
        Function<Atom, Integer> getStateFromAtom) {
        int lastStateIdx;
        for (lastStateIdx = data.size() - 1; lastStateIdx >= 0; lastStateIdx--) {
            Atom atom = data.get(lastStateIdx).getAtom();
            if (state.contains(getStateFromAtom.apply(atom))) {
                break;
            }
        }
        if (lastStateIdx == data.size()-1) {
            // Last element already is in state, so there's nothing to do.
            return;
        }
        data.subList(lastStateIdx+1, data.size()).clear();
    }

    /** Returns the UID of the host, which should always either be SHELL (2000) or ROOT (0). */
    protected int getHostUid() throws DeviceNotAvailableException {
        String strUid = "";
        try {
            strUid = getDevice().executeShellCommand("id -u");
            return Integer.parseInt(strUid.trim());
        } catch (NumberFormatException e) {
            LogUtil.CLog.e("Failed to get host's uid via shell command. Found " + strUid);
            // Fall back to alternative method...
            if (getDevice().isAdbRoot()) {
                return 0; // ROOT
            } else {
                return 2000; // SHELL
            }
        }
    }

    protected void turnScreenOn() throws Exception {
        getDevice().executeShellCommand("input keyevent KEYCODE_WAKEUP");
        getDevice().executeShellCommand("wm dismiss-keyguard");
    }

    protected void turnScreenOff() throws Exception {
        getDevice().executeShellCommand("input keyevent KEYCODE_SLEEP");
    }

    protected void setChargingState(int state) throws Exception {
        getDevice().executeShellCommand("cmd battery set status " + state);
    }

    protected void unplugDevice() throws Exception {
        getDevice().executeShellCommand("cmd battery unplug");
    }

    protected void plugInAc() throws Exception {
        getDevice().executeShellCommand("cmd battery set ac 1");
    }

    protected void plugInUsb() throws Exception {
        getDevice().executeShellCommand("cmd battery set usb 1");
    }

    protected void plugInWireless() throws Exception {
        getDevice().executeShellCommand("cmd battery set wireless 1");
    }

    public void doAppBreadcrumbReportedStart(int label) throws Exception {
        doAppBreadcrumbReported(label, AppBreadcrumbReported.State.START.ordinal());
    }

    public void doAppBreadcrumbReportedStop(int label) throws Exception {
        doAppBreadcrumbReported(label, AppBreadcrumbReported.State.STOP.ordinal());
    }

    public void doAppBreadcrumbReported(int label, int state) throws Exception {
        getDevice().executeShellCommand(String.format(
                "cmd stats log-app-breadcrumb %d %d", label, state));
    }

    protected void setBatteryLevel(int level) throws Exception {
        getDevice().executeShellCommand("cmd battery set level " + level);
    }

    protected void resetBatteryStatus() throws Exception {
        getDevice().executeShellCommand("cmd battery reset");
    }

    protected int getScreenBrightness() throws Exception {
        return Integer.parseInt(
                getDevice().executeShellCommand("settings get system screen_brightness").trim());
    }

    protected void setScreenBrightness(int brightness) throws Exception {
        getDevice().executeShellCommand("settings put system screen_brightness " + brightness);
    }

    protected boolean isScreenBrightnessModeManual() throws Exception {
        String mode = getDevice().executeShellCommand("settings get system screen_brightness_mode");
        return Integer.parseInt(mode.trim()) == 0;
    }

    protected void setScreenBrightnessMode(boolean manual) throws Exception {
        getDevice().executeShellCommand(
                "settings put system screen_brightness_mode " + (manual ? 0 : 1));
    }

    protected void enterDozeModeLight() throws Exception {
        getDevice().executeShellCommand("dumpsys deviceidle force-idle light");
    }

    protected void enterDozeModeDeep() throws Exception {
        getDevice().executeShellCommand("dumpsys deviceidle force-idle deep");
    }

    protected void leaveDozeMode() throws Exception {
        getDevice().executeShellCommand("dumpsys deviceidle unforce");
        getDevice().executeShellCommand("dumpsys deviceidle disable");
        getDevice().executeShellCommand("dumpsys deviceidle enable");
    }

    protected void turnBatterySaverOn() throws Exception {
        getDevice().executeShellCommand("cmd battery unplug");
        getDevice().executeShellCommand("settings put global low_power 1");
    }

    protected void turnBatterySaverOff() throws Exception {
        getDevice().executeShellCommand("settings put global low_power 0");
        getDevice().executeShellCommand("cmd battery reset");
    }

    protected void rebootDevice() throws Exception {
        getDevice().rebootUntilOnline();
    }

    /**
     * Asserts that the two events are within the specified range of each other.
     *
     * @param d0        the event that should occur first
     * @param d1        the event that should occur second
     * @param minDiffMs d0 should precede d1 by at least this amount
     * @param maxDiffMs d0 should precede d1 by at most this amount
     */
    public static void assertTimeDiffBetween(EventMetricData d0, EventMetricData d1,
            int minDiffMs, int maxDiffMs) {
        long diffMs = (d1.getElapsedTimestampNanos() - d0.getElapsedTimestampNanos()) / 1_000_000;
        assertTrue("Illegal time difference (" + diffMs + "ms)", minDiffMs <= diffMs);
        assertTrue("Illegal time difference (" + diffMs + "ms)", diffMs <= maxDiffMs);
    }

    protected String getCurrentLogcatDate() throws Exception {
        // TODO: Do something more robust than this for getting logcat markers.
        long timestampMs = getDevice().getDeviceDate();
        return new SimpleDateFormat("MM-dd HH:mm:ss.SSS")
                .format(new Date(timestampMs));
    }

    protected String getLogcatSince(String date, String logcatParams) throws Exception {
        return getDevice().executeShellCommand(String.format(
                "logcat -v threadtime -t '%s' -d %s", date, logcatParams));
    }

    /**
     * Pulled atoms should have a better way of constructing the config.
     * Remove this config when that happens.
     */
    protected StatsdConfig.Builder getPulledConfig() {
        return StatsdConfig.newBuilder().setId(CONFIG_ID)
                .addAllowedLogSource("AID_SYSTEM")
                .addAllowedLogSource(DeviceAtomTestCase.DEVICE_SIDE_TEST_PACKAGE);
    }

    /**
     * Determines if the device has the given feature.
     * Prints a warning if its value differs from requiredAnswer.
     */
    protected boolean hasFeature(String featureName, boolean requiredAnswer) throws Exception {
        final String features = getDevice().executeShellCommand("pm list features");
        boolean hasIt = features.contains(featureName);
        if (hasIt != requiredAnswer) {
            LogUtil.CLog.w("Device does " + (requiredAnswer ? "not " : "") + "have feature "
                    + featureName);
        }
        return hasIt == requiredAnswer;
    }

}
