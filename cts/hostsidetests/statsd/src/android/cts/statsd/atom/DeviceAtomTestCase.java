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

import com.android.internal.os.StatsdConfigProto.FieldValueMatcher;
import com.android.internal.os.StatsdConfigProto.MessageMatcher;
import com.android.internal.os.StatsdConfigProto.Position;
import com.android.internal.os.StatsdConfigProto.StatsdConfig;
import com.android.os.StatsLog.EventMetricData;
import com.android.tradefed.log.LogUtil;

import java.util.Arrays;
import java.util.List;

/**
 * Base class for testing Statsd atoms that report a uid. Tests are performed via a device-side app.
 */
public class DeviceAtomTestCase extends AtomTestCase {

    protected static final String DEVICE_SIDE_TEST_APK = "CtsStatsdApp.apk";
    protected static final String DEVICE_SIDE_TEST_PACKAGE =
            "com.android.server.cts.device.statsd";
    protected static final long DEVICE_SIDE_TEST_PKG_HASH =
            Long.parseUnsignedLong("15694052924544098582");

    protected static final String CONFIG_NAME = "cts_config";

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        getDevice().uninstallPackage(DEVICE_SIDE_TEST_PACKAGE);
        installTestApp();
    }

    @Override
    protected void tearDown() throws Exception {
        getDevice().uninstallPackage(DEVICE_SIDE_TEST_PACKAGE);
        super.tearDown();
    }

    /**
     * Performs a device-side test by calling a method on the app and returns its stats events.
     * @param methodName the name of the method in the app's AtomTests to perform
     * @param atom atom tag (from atoms.proto)
     * @param key atom's field corresponding to state
     * @param stateOn 'on' value
     * @param stateOff 'off' value
     * @param minTimeDiffMs max allowed time between start and stop
     * @param maxTimeDiffMs min allowed time between start and stop
     * @param demandExactlyTwo whether there must be precisely two events logged (1 start, 1 stop)
     * @return list of events with the app's uid matching the configuration defined by the params.
     */
    protected List<EventMetricData> doDeviceMethodOnOff(
            String methodName, int atom, int key, int stateOn, int stateOff,
            int minTimeDiffMs, int maxTimeDiffMs, boolean demandExactlyTwo) throws Exception {
        StatsdConfig.Builder conf = createConfigBuilder();
        addAtomEvent(conf, atom, createFvm(key).setEqInt(stateOn));
        addAtomEvent(conf, atom, createFvm(key).setEqInt(stateOff));
        List<EventMetricData> data = doDeviceMethod(methodName, conf);

        if (demandExactlyTwo) {
            assertTrue(data.size() == 2);
        } else {
            assertTrue(data.size() >= 2);
        }
        assertTimeDiffBetween(data.get(0), data.get(1), minTimeDiffMs, maxTimeDiffMs);
        return data;
    }

    /**
     *
     * @param methodName the name of the method in the app's AtomTests to perform
     * @param cfg statsd configuration
     * @return list of events with the app's uid matching the configuration.
     */
    protected List<EventMetricData> doDeviceMethod(String methodName, StatsdConfig.Builder cfg)
            throws Exception {
        removeConfig(CONFIG_ID);
        getReportList();  // Clears previous data on disk.
        uploadConfig(cfg);
        int appUid = getUid();
        LogUtil.CLog.d("\nPerforming device-side test of " + methodName + " for uid " + appUid);
        runDeviceTests(DEVICE_SIDE_TEST_PACKAGE, ".AtomTests", methodName);

        return getEventMetricDataList();
    }

    protected void createAndUploadConfig(int atomTag, boolean useAttribution) throws Exception {
        StatsdConfig.Builder conf = createConfigBuilder();
        addAtomEvent(conf, atomTag, useAttribution);
        uploadConfig(conf);
    }

    /**
     * Adds an event to the config for an atom that matches the given key AND has the app's uid.
     * @param conf configuration
     * @param atomTag atom tag (from atoms.proto)
     * @param fvm FieldValueMatcher.Builder for the relevant key
     */
    @Override
    protected void addAtomEvent(StatsdConfig.Builder conf, int atomTag, FieldValueMatcher.Builder fvm)
            throws Exception {

        final int UID_KEY = 1;
        FieldValueMatcher.Builder fvmUid = createAttributionFvm(UID_KEY);
        addAtomEvent(conf, atomTag, Arrays.asList(fvm, fvmUid));
    }

    /**
     * Adds an event to the config for an atom that matches the app's uid.
     * @param conf configuration
     * @param atomTag atom tag (from atoms.proto)
     * @param useAttribution if the atom has a uid or AttributionNode
     */
    protected void addAtomEvent(StatsdConfig.Builder conf, int atomTag,
            boolean useAttribution) throws Exception {
        final int UID_KEY = 1;
        FieldValueMatcher.Builder fvmUid;
        if (useAttribution) {
            fvmUid = createAttributionFvm(UID_KEY);
        } else {
            fvmUid = createFvm(UID_KEY).setEqInt(getUid());
        }
        addAtomEvent(conf, atomTag, Arrays.asList(fvmUid));
    }

    /**
     * Creates a FieldValueMatcher for atoms that use AttributionNode
     */
    protected FieldValueMatcher.Builder createAttributionFvm(int field) {
        final int ATTRIBUTION_NODE_UID_KEY = 1;
        return createFvm(field).setPosition(Position.ANY)
                .setMatchesTuple(MessageMatcher.newBuilder()
                        .addFieldValueMatcher(createFvm(ATTRIBUTION_NODE_UID_KEY)
                                .setEqString(DEVICE_SIDE_TEST_PACKAGE)));
    }

    /**
     * Gets the uid of the test app.
     */
    protected int getUid() throws Exception {
        String uidLine = getDevice().executeShellCommand("cmd package list packages -U "
                + DEVICE_SIDE_TEST_PACKAGE);
        String[] uidLineParts = uidLine.split(":");
        // 3rd entry is package uid
        assertTrue(uidLineParts.length > 2);
        int uid = Integer.parseInt(uidLineParts[2].trim());
        assertTrue(uid > 10000);
        return uid;
    }

    /**
     * Installs the test apk.
     */
    protected void installTestApp() throws Exception {
        installPackage(DEVICE_SIDE_TEST_APK, true);
        LogUtil.CLog.i("Installing device-side test app with uid " + getUid());
        allowBackgroundServices();
    }

    /**
     * Required to successfully start a background service from adb in O.
     */
    protected void allowBackgroundServices() throws Exception {
        getDevice().executeShellCommand(String.format(
                "cmd deviceidle tempwhitelist %s", DEVICE_SIDE_TEST_PACKAGE));
    }

    /** Make the test app standby-active so it can run syncs and jobs immediately. */
    protected void allowImmediateSyncs() throws Exception {
        getDevice().executeShellCommand("am set-standby-bucket "
                + DEVICE_SIDE_TEST_PACKAGE + " active");
    }

    /**
     * Runs the specified activity.
     */
    protected void runActivity(String activity, String actionKey, String actionValue)
            throws Exception {
        runActivity(activity, actionKey, actionValue, WAIT_TIME_LONG);
    }

    /**
     * Runs the specified activity.
     */
    protected void runActivity(String activity, String actionKey, String actionValue,
            long waitTime) throws Exception {
        String intentString = null;
        if (actionKey != null && actionValue != null) {
            intentString = actionKey + " " + actionValue;
        }
        if (intentString == null) {
            getDevice().executeShellCommand(
                    "am start -n " + DEVICE_SIDE_TEST_PACKAGE + "/." + activity);
        } else {
            getDevice().executeShellCommand(
                    "am start -n " + DEVICE_SIDE_TEST_PACKAGE + "/." + activity + " -e " +
                            intentString);
        }

        Thread.sleep(waitTime);
        getDevice().executeShellCommand(
                "am force-stop " + DEVICE_SIDE_TEST_PACKAGE);

        Thread.sleep(WAIT_TIME_SHORT);
    }

    protected void resetBatteryStats() throws Exception {
        getDevice().executeShellCommand("dumpsys batterystats --reset");
    }
}
