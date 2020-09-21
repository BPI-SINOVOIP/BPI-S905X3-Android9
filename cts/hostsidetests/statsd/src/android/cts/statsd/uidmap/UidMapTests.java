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
package android.cts.statsd.uidmap;

import static org.junit.Assert.assertTrue;

import android.cts.statsd.atom.DeviceAtomTestCase;
import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.internal.os.StatsdConfigProto;
import com.android.os.AtomsProto;
import com.android.os.StatsLog.ConfigMetricsReportList;
import com.android.os.StatsLog.ConfigMetricsReport;
import com.android.os.StatsLog.UidMapping;
import com.android.os.StatsLog.UidMapping.PackageInfoSnapshot;
import com.android.tradefed.log.LogUtil;

import java.util.List;

public class UidMapTests extends DeviceAtomTestCase {

    // Tests that every report has at least one snapshot.
    public void testUidSnapshotIncluded() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        // There should be at least the test app installed during the test setup.
        createAndUploadConfig(AtomsProto.Atom.UID_PROCESS_STATE_CHANGED_FIELD_NUMBER);

        ConfigMetricsReportList reports = getReportList();
        assertTrue(reports.getReportsCount() > 0);

        for (ConfigMetricsReport report : reports.getReportsList()) {
            UidMapping uidmap = report.getUidMap();
            assertTrue(uidmap.getSnapshotsCount() > 0);
            for (PackageInfoSnapshot snapshot : uidmap.getSnapshotsList()) {
                // There must be at least one element in each snapshot (at least one package is
                // installed).
                assertTrue(snapshot.getPackageInfoCount() > 0);
            }
        }
    }

    private boolean hasMatchingChange(UidMapping uidmap, int uid, boolean expectDeletion) {
        LogUtil.CLog.d("The uid we are looking for is " + uid);
        for (UidMapping.Change change : uidmap.getChangesList()) {
            if (change.getAppHash() == DEVICE_SIDE_TEST_PKG_HASH && change.getUid() == uid) {
                if (change.getDeletion() == expectDeletion) {
                    return true;
                }
            }
        }
        return false;
    }

    // Tests that delta event included during app installation.
    public void testChangeFromInstallation() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        getDevice().uninstallPackage(DEVICE_SIDE_TEST_PACKAGE);
        createAndUploadConfig(AtomsProto.Atom.UID_PROCESS_STATE_CHANGED_FIELD_NUMBER);
        // Install the package after the config is sent to statsd. The uid map is not guaranteed to
        // be updated if there's no config in statsd.
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mCtsBuild);
        final String result = getDevice().installPackage(
                buildHelper.getTestFile(DEVICE_SIDE_TEST_APK), false, true);

        Thread.sleep(WAIT_TIME_SHORT);

        ConfigMetricsReportList reports = getReportList();
        assertTrue(reports.getReportsCount() > 0);

        boolean found = false;
        int uid = getUid();
        for (ConfigMetricsReport report : reports.getReportsList()) {
            LogUtil.CLog.d("Got the following report: \n" + report.toString());
            if (hasMatchingChange(report.getUidMap(), uid, false)) {
                found = true;
            }
        }
        assertTrue(found);
    }

    // We check that a re-installation gives a change event (similar to an app upgrade).
    public void testChangeFromReinstall() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mCtsBuild);
        getDevice().installPackage(buildHelper.getTestFile(DEVICE_SIDE_TEST_APK), false, true);
        createAndUploadConfig(AtomsProto.Atom.UID_PROCESS_STATE_CHANGED_FIELD_NUMBER);
        // Now enable re-installation.
        getDevice().installPackage(buildHelper.getTestFile(DEVICE_SIDE_TEST_APK), true, true);

        Thread.sleep(WAIT_TIME_SHORT);

        ConfigMetricsReportList reports = getReportList();
        assertTrue(reports.getReportsCount() > 0);

        boolean found = false;
        int uid = getUid();
        for (ConfigMetricsReport report : reports.getReportsList()) {
            LogUtil.CLog.d("Got the following report: \n" + report.toString());
            if (hasMatchingChange(report.getUidMap(), uid, false)) {
                found = true;
            }
        }
        assertTrue(found);
    }

    public void testChangeFromUninstall() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mCtsBuild);
        getDevice().installPackage(buildHelper.getTestFile(DEVICE_SIDE_TEST_APK), true, true);
        createAndUploadConfig(AtomsProto.Atom.UID_PROCESS_STATE_CHANGED_FIELD_NUMBER);
        int uid = getUid();
        getDevice().uninstallPackage(DEVICE_SIDE_TEST_PACKAGE);

        Thread.sleep(WAIT_TIME_SHORT);

        ConfigMetricsReportList reports = getReportList();
        assertTrue(reports.getReportsCount() > 0);

        boolean found = false;
        for (ConfigMetricsReport report : reports.getReportsList()) {
            LogUtil.CLog.d("Got the following report: \n" + report.toString());
            if (hasMatchingChange(report.getUidMap(), uid, true)) {
                found = true;
            }
        }
        assertTrue(found);
    }
}
