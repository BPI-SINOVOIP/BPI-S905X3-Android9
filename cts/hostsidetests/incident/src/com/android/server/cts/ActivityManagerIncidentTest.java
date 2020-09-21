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

import com.android.server.am.ActiveInstrumentationProto;
import com.android.server.am.ActiveServicesProto;
import com.android.server.am.ActiveServicesProto.ServicesByUser;
import com.android.server.am.ActivityManagerServiceDumpBroadcastsProto;
import com.android.server.am.ActivityManagerServiceDumpProcessesProto;
import com.android.server.am.ActivityManagerServiceDumpProcessesProto.LruProcesses;
import com.android.server.am.ActivityManagerServiceDumpServicesProto;
import com.android.server.am.AppErrorsProto;
import com.android.server.am.AppTimeTrackerProto;
import com.android.server.am.BroadcastQueueProto;
import com.android.server.am.BroadcastQueueProto.BroadcastSummary;
import com.android.server.am.BroadcastRecordProto;
import com.android.server.am.ConnectionRecordProto;
import com.android.server.am.GrantUriProto;
import com.android.server.am.ImportanceTokenProto;
import com.android.server.am.NeededUriGrantsProto;
import com.android.server.am.ProcessRecordProto;
import com.android.server.am.ServiceRecordProto;
import com.android.server.am.UidRecordProto;
import com.android.server.am.UriPermissionOwnerProto;
import com.android.server.am.VrControllerProto;

/**
 * Test to check that the activity manager service properly outputs its dump state.
 *
 * make -j32 CtsIncidentHostTestCases
 * cts-tradefed run singleCommand cts-dev -d --module CtsIncidentHostTestCases
 */
public class ActivityManagerIncidentTest extends ProtoDumpTestCase {

    private static final String TEST_BROADCAST = "com.android.mybroadcast";
    private static final String SYSTEM_PROC = "system";
    private static final int SYSTEM_UID = 1000;

    /**
     * Tests activity manager dumps broadcasts.
     */
    public void testDumpBroadcasts() throws Exception {
        getDevice().executeShellCommand("am broadcast -a " + TEST_BROADCAST);
        Thread.sleep(100);
        final ActivityManagerServiceDumpBroadcastsProto dump = getDump(
                ActivityManagerServiceDumpBroadcastsProto.parser(),
                "dumpsys activity --proto broadcasts");

        assertTrue(dump.getReceiverListCount() > 0);
        assertTrue(dump.getBroadcastQueueCount() > 0);
        assertTrue(dump.getStickyBroadcastsCount() > 0);

        boolean found = false;
mybroadcast:
        for (BroadcastQueueProto queue : dump.getBroadcastQueueList()) {
            for (BroadcastRecordProto record : queue.getHistoricalBroadcastsList()) {
                if (record.getIntentAction().equals(TEST_BROADCAST)) {
                    found = true;
                    break mybroadcast;
                }
            }
            for (BroadcastSummary summary : queue.getHistoricalBroadcastsSummaryList()) {
                if (summary.getIntent().getAction().equals(TEST_BROADCAST)) {
                    found = true;
                    break mybroadcast;
                }
            }
        }
        assertTrue(found);
        ActivityManagerServiceDumpBroadcastsProto.MainHandler mainHandler = dump.getHandler();
        assertTrue(mainHandler.getHandler().contains(
            "com.android.server.am.ActivityManagerService"));
    }

    /**
     * Tests activity manager dumps services.
     */
    public void testDumpServices() throws Exception {
        final ActivityManagerServiceDumpServicesProto dump = getDump(
                ActivityManagerServiceDumpServicesProto.parser(),
                "dumpsys activity --proto service");
        ActiveServicesProto activeServices = dump.getActiveServices();
        assertTrue(activeServices.getServicesByUsersCount() > 0);

        for (ServicesByUser perUserServices : activeServices.getServicesByUsersList()) {
            assertTrue(perUserServices.getServiceRecordsCount() > 0);
            for (ServiceRecordProto service : perUserServices.getServiceRecordsList()) {
                assertFalse(service.getShortName().isEmpty());
                assertFalse(service.getPackageName().isEmpty());
                assertFalse(service.getProcessName().isEmpty());
                assertFalse(service.getAppinfo().getBaseDir().isEmpty());
                assertFalse(service.getAppinfo().getDataDir().isEmpty());
            }
        }

        verifyActivityManagerServiceDumpServicesProto(dump, PRIVACY_NONE);
    }

    static void verifyActivityManagerServiceDumpServicesProto(ActivityManagerServiceDumpServicesProto dump, final int filterLevel) throws Exception {
        for (ServicesByUser sbu : dump.getActiveServices().getServicesByUsersList()) {
            for (ServiceRecordProto srp : sbu.getServiceRecordsList()) {
                verifyServiceRecordProto(srp, filterLevel);
            }
        }
    }

    private static void verifyServiceRecordProto(ServiceRecordProto srp, final int filterLevel) throws Exception {
        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(srp.getAppinfo().getBaseDir().isEmpty());
            assertTrue(srp.getAppinfo().getResDir().isEmpty());
            assertTrue(srp.getAppinfo().getDataDir().isEmpty());
        } else {
            assertFalse(srp.getAppinfo().getBaseDir().isEmpty());
            assertFalse(srp.getAppinfo().getDataDir().isEmpty());
        }
        for (ServiceRecordProto.StartItem si : srp.getDeliveredStartsList()) {
            verifyServiceRecordProtoStartItem(si, filterLevel);
        }
        for (ServiceRecordProto.StartItem si : srp.getPendingStartsList()) {
            verifyServiceRecordProtoStartItem(si, filterLevel);
        }
        for (ConnectionRecordProto crp : srp.getConnectionsList()) {
            verifyConnectionRecordProto(crp, filterLevel);
        }
    }

    private static void verifyServiceRecordProtoStartItem(ServiceRecordProto.StartItem si, final int filterLevel) throws Exception {
        verifyNeededUriGrantsProto(si.getNeededGrants(), filterLevel);
        verifyUriPermissionOwnerProto(si.getUriPermissions(), filterLevel);
    }

    private static void verifyNeededUriGrantsProto(NeededUriGrantsProto nugp, final int filterLevel) throws Exception {
        for (GrantUriProto gup : nugp.getGrantsList()) {
            verifyGrantUriProto(gup, filterLevel);
        }
    }

    private static void verifyUriPermissionOwnerProto(UriPermissionOwnerProto upop, final int filterLevel) throws Exception {
        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(upop.getOwner().isEmpty());
        }
        for (GrantUriProto gup : upop.getReadPermsList()) {
            verifyGrantUriProto(gup, filterLevel);
        }
        for (GrantUriProto gup : upop.getWritePermsList()) {
            verifyGrantUriProto(gup, filterLevel);
        }
    }

    private static void verifyGrantUriProto(GrantUriProto gup, final int filterLevel) throws Exception {
        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(gup.getUri().isEmpty());
        }
    }

    private static void verifyConnectionRecordProto(ConnectionRecordProto crp, final int filterLevel) throws Exception {
        for (ConnectionRecordProto.Flag f : crp.getFlagsList()) {
            assertTrue(ConnectionRecordProto.Flag.getDescriptor().getValues().contains(f.getValueDescriptor()));
        }
    }

    /**
     * Tests activity manager dumps processes.
     */
    public void testDumpProcesses() throws Exception {
        final ActivityManagerServiceDumpProcessesProto dump = getDump(
                ActivityManagerServiceDumpProcessesProto.parser(),
                "dumpsys activity --proto processes");

        assertTrue(dump.getProcsCount() > 0);
        boolean procFound = false;
        for (ProcessRecordProto proc : dump.getProcsList()) {
            if (proc.getProcessName().equals(SYSTEM_PROC) && proc.getUid() == SYSTEM_UID) {
                procFound = true;
                break;
            }
        }
        assertTrue(procFound);

        assertTrue(dump.getActiveUidsCount() > 0);
        boolean uidFound = false;
        for (UidRecordProto uid : dump.getActiveUidsList()) {
            if (uid.getUid() == SYSTEM_UID) {
                uidFound = true;
                break;
            }
        }
        assertTrue(uidFound);

        LruProcesses lruProcs = dump.getLruProcs();
        assertTrue(lruProcs.getSize() == lruProcs.getListCount());
        assertTrue(dump.getUidObserversCount() > 0);
        assertTrue(dump.getAdjSeq() > 0);
        assertTrue(dump.getLruSeq() > 0);

        verifyActivityManagerServiceDumpProcessesProto(dump, PRIVACY_NONE);
    }

    static void verifyActivityManagerServiceDumpProcessesProto(ActivityManagerServiceDumpProcessesProto dump, final int filterLevel) throws Exception {
        for (ActiveInstrumentationProto aip : dump.getActiveInstrumentationsList()) {
            verifyActiveInstrumentationProto(aip, filterLevel);
        }
        for (UidRecordProto urp : dump.getActiveUidsList()) {
            verifyUidRecordProto(urp, filterLevel);
        }
        for (UidRecordProto urp : dump.getValidateUidsList()) {
            verifyUidRecordProto(urp, filterLevel);
        }
        for (ImportanceTokenProto itp : dump.getImportantProcsList()) {
            verifyImportanceTokenProto(itp, filterLevel);
        }
        verifyAppErrorsProto(dump.getAppErrors(), filterLevel);
        verifyVrControllerProto(dump.getVrController(), filterLevel);
        verifyAppTimeTrackerProto(dump.getCurrentTracker(), filterLevel);
        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(dump.getMemWatchProcesses().getDump().getFile().isEmpty());
        }
    }

    private static void verifyActiveInstrumentationProto(ActiveInstrumentationProto aip, final int filterLevel) throws Exception {
        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(aip.getArguments().isEmpty());
        }
    }

    private static void verifyUidRecordProto(UidRecordProto urp, final int filterLevel) throws Exception {
        for (UidRecordProto.Change c : urp.getLastReportedChangesList()) {
            assertTrue(UidRecordProto.Change.getDescriptor().getValues().contains(c.getValueDescriptor()));
        }
        assertTrue(urp.getNumProcs() >= 0);
    }

    private static void verifyImportanceTokenProto(ImportanceTokenProto itp, final int filterLevel) throws Exception {
        if (filterLevel == PRIVACY_AUTO) {
            // The entire message is tagged as EXPLICIT, so even the pid should be stripped out.
            assertTrue(itp.getPid() == 0);
            assertTrue(itp.getToken().isEmpty());
            assertTrue(itp.getReason().isEmpty());
        }
    }

    private static void verifyAppErrorsProto(AppErrorsProto aep, final int filterLevel) throws Exception {
        assertTrue(aep.getNowUptimeMs() >= 0);
        if (filterLevel == PRIVACY_AUTO) {
            for (AppErrorsProto.BadProcess bp : aep.getBadProcessesList()) {
                for (AppErrorsProto.BadProcess.Entry e : bp.getEntriesList()) {
                    assertTrue(e.getLongMsg().isEmpty());
                    assertTrue(e.getStack().isEmpty());
                }
            }
        }
    }

    private static void verifyVrControllerProto(VrControllerProto vcp, final int filterLevel) throws Exception {
        for (VrControllerProto.VrMode vm : vcp.getVrModeList()) {
            assertTrue(VrControllerProto.VrMode.getDescriptor().getValues().contains(vm.getValueDescriptor()));
        }
    }

    private static void verifyAppTimeTrackerProto(AppTimeTrackerProto attp, final int filterLevel) throws Exception {
        assertTrue(attp.getTotalDurationMs() >= 0);
        for (AppTimeTrackerProto.PackageTime pt : attp.getPackageTimesList()) {
            assertTrue(pt.getDurationMs() >= 0);
        }
    }
}
