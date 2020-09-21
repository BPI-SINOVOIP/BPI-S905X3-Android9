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
package com.android.server.cts;

import android.os.IncidentProto;

/**
 * Tests incidentd reports filters fields correctly based on its privacy tags.
 */
public class IncidentdTest extends ProtoDumpTestCase {
    private static final String TAG = "IncidentdTest";

    public void testIncidentReportDump(final int filterLevel, final String dest) throws Exception {
        final String destArg = dest == null || dest.isEmpty() ? "" : "-p " + dest;
        final IncidentProto dump = getDump(IncidentProto.parser(), "incident " + destArg + " 2>/dev/null");

        SystemPropertiesTest.verifySystemPropertiesProto(dump.getSystemProperties(), filterLevel);

        StackTraceIncidentTest.verifyBackTraceProto(dump.getNativeTraces(), filterLevel);
        StackTraceIncidentTest.verifyBackTraceProto(dump.getHalTraces(), filterLevel);
        StackTraceIncidentTest.verifyBackTraceProto(dump.getJavaTraces(), filterLevel);

        if (FingerprintIncidentTest.supportsFingerprint(getDevice())) {
            FingerprintIncidentTest.verifyFingerprintServiceDumpProto(dump.getFingerprint(), filterLevel);
        }

        NetstatsIncidentTest.verifyNetworkStatsServiceDumpProto(dump.getNetstats(), filterLevel);

        SettingsIncidentTest.verifySettingsServiceDumpProto(dump.getSettings(), filterLevel);

        NotificationIncidentTest.verifyNotificationServiceDumpProto(dump.getNotification(), filterLevel);

        BatteryStatsIncidentTest.verifyBatteryStatsServiceDumpProto(dump.getBatterystats(), filterLevel);

        if (BatteryIncidentTest.hasBattery(getDevice())) {
            BatteryIncidentTest.verifyBatteryServiceDumpProto(dump.getBattery(), filterLevel);
        }

        DiskStatsProtoTest.verifyDiskStatsServiceDumpProto(dump.getDiskstats(), filterLevel, getDevice());

        PackageIncidentTest.verifyPackageServiceDumpProto(dump.getPackage(), filterLevel);

        PowerIncidentTest.verifyPowerManagerServiceDumpProto(dump.getPower(), filterLevel);

        if (PrintProtoTest.supportsPrinting(getDevice())) {
            PrintProtoTest.verifyPrintServiceDumpProto(dump.getPrint(), filterLevel);
        }

        // Procstats currently has no EXPLICIT or LOCAL fields

        // ActivityManagerServiceDumpActivitiesProto has no EXPLICIT or LOCAL fields.

        // ActivityManagerServiceDumpBroadcastsProto has no EXPLICIT or LOCAL fields.

        ActivityManagerIncidentTest.verifyActivityManagerServiceDumpServicesProto(dump.getAmservices(), filterLevel);

        ActivityManagerIncidentTest.verifyActivityManagerServiceDumpProcessesProto(dump.getAmprocesses(), filterLevel);

        AlarmManagerIncidentTest.verifyAlarmManagerServiceDumpProto(dump.getAlarm(), filterLevel);

        MemInfoIncidentTest.verifyMemInfoDumpProto(dump.getMeminfo(), filterLevel);

        // GraphicsStats is expected to be all AUTOMATIC.

        WindowManagerIncidentTest.verifyWindowManagerServiceDumpProto(dump.getWindow(), filterLevel);

        JobSchedulerIncidentTest.verifyJobSchedulerServiceDumpProto(dump.getJobscheduler(), filterLevel);

        UsbIncidentTest.verifyUsbServiceDumpProto(dump.getUsb(), filterLevel);
    }

    // Splitting these into separate methods to make debugging easier.

    public void testIncidentReportDumpAuto() throws Exception {
        testIncidentReportDump(PRIVACY_AUTO, "AUTOMATIC");
    }

    public void testIncidentReportDumpExplicit() throws Exception {
        testIncidentReportDump(PRIVACY_EXPLICIT, "EXPLICIT");
    }

    public void testIncidentReportDumpLocal() throws Exception {
        testIncidentReportDump(PRIVACY_LOCAL, "LOCAL");
    }
}
