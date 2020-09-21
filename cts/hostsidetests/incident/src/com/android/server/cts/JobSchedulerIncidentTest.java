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

import android.app.job.StopReasonEnum;
import android.net.NetworkCapabilitiesProto;
import android.net.NetworkRequestProto;
import com.android.server.job.ConstantsProto;
import com.android.server.job.DataSetProto;
import com.android.server.job.JobPackageHistoryProto;
import com.android.server.job.JobPackageTrackerDumpProto;
import com.android.server.job.JobSchedulerServiceDumpProto;
import com.android.server.job.JobStatusDumpProto;
import com.android.server.job.JobStatusShortInfoProto;
import com.android.server.job.StateControllerProto;

/** Test to check that the jobscheduler service properly outputs its dump state. */
public class JobSchedulerIncidentTest extends ProtoDumpTestCase {
    public void testJobSchedulerServiceDump() throws Exception {
        final JobSchedulerServiceDumpProto dump =
                getDump(JobSchedulerServiceDumpProto.parser(), "dumpsys jobscheduler --proto");

        verifyJobSchedulerServiceDumpProto(dump, PRIVACY_NONE);
    }

    static void verifyJobSchedulerServiceDumpProto(JobSchedulerServiceDumpProto dump, final int filterLevel) throws Exception {
        testConstantsProto(dump.getSettings());

        for (int u : dump.getStartedUsersList()) {
            assertTrue(0 <= u);
        }

        for (JobSchedulerServiceDumpProto.RegisteredJob rj : dump.getRegisteredJobsList()) {
            testJobStatusShortInfoProto(rj.getInfo(), filterLevel);
            testJobStatusDumpProto(rj.getDump());
        }

        for (StateControllerProto c : dump.getControllersList()) {
            testStateControllerProto(c, filterLevel);
        }

        for (JobSchedulerServiceDumpProto.PriorityOverride po : dump.getPriorityOverridesList()) {
            assertTrue(0 <= po.getUid());
        }

        for (int buu : dump.getBackingUpUidsList()) {
            assertTrue(0 <= buu);
        }

        testJobPackageHistoryProto(dump.getHistory(), filterLevel);

        testJobPackageTrackerDumpProto(dump.getPackageTracker());

        for (JobSchedulerServiceDumpProto.PendingJob pj : dump.getPendingJobsList()) {
            testJobStatusShortInfoProto(pj.getInfo(), filterLevel);
            testJobStatusDumpProto(pj.getDump());
            assertTrue(0 <= pj.getEnqueuedDurationMs());
        }

        for (JobSchedulerServiceDumpProto.ActiveJob aj : dump.getActiveJobsList()) {
            JobSchedulerServiceDumpProto.ActiveJob.InactiveJob ajIj = aj.getInactive();
            assertTrue(0 <= ajIj.getTimeSinceStoppedMs());

            JobSchedulerServiceDumpProto.ActiveJob.RunningJob ajRj = aj.getRunning();
            testJobStatusShortInfoProto(ajRj.getInfo(), filterLevel);
            assertTrue(0 <= ajRj.getRunningDurationMs());
            assertTrue(0 <= ajRj.getTimeUntilTimeoutMs());
            testJobStatusDumpProto(ajRj.getDump());
            assertTrue(0 <= ajRj.getTimeSinceMadeActiveMs());
            assertTrue(0 <= ajRj.getPendingDurationMs());
        }

        assertTrue(0 <= dump.getMaxActiveJobs());
    }

    private static void testConstantsProto(ConstantsProto c) throws Exception {
        assertNotNull(c);

        assertTrue(0 <= c.getMinIdleCount());
        assertTrue(0 <= c.getMinChargingCount());
        assertTrue(0 <= c.getMinBatteryNotLowCount());
        assertTrue(0 <= c.getMinStorageNotLowCount());
        assertTrue(0 <= c.getMinConnectivityCount());
        assertTrue(0 <= c.getMinContentCount());
        assertTrue(0 <= c.getMinReadyJobsCount());
        assertTrue(0 <= c.getHeavyUseFactor());
        assertTrue(0 <= c.getModerateUseFactor());
        assertTrue(0 <= c.getFgJobCount());
        assertTrue(0 <= c.getBgNormalJobCount());
        assertTrue(0 <= c.getBgModerateJobCount());
        assertTrue(0 <= c.getBgLowJobCount());
        assertTrue(0 <= c.getBgCriticalJobCount());
        assertTrue(0 <= c.getMaxStandardRescheduleCount());
        assertTrue(0 <= c.getMaxWorkRescheduleCount());
        assertTrue(0 <= c.getMinLinearBackoffTimeMs());
        assertTrue(0 <= c.getMinExpBackoffTimeMs());
        assertTrue(0 <= c.getStandbyHeartbeatTimeMs());
        for (int sb : c.getStandbyBeatsList()) {
            assertTrue(0 <= sb);
        }
    }

    private static void testDataSetProto(DataSetProto ds) throws Exception {
        assertNotNull(ds);

        assertTrue(0 <= ds.getStartClockTimeMs());
        assertTrue(0 <= ds.getElapsedTimeMs());
        assertTrue(0 <= ds.getPeriodMs());

        for (DataSetProto.PackageEntryProto pe : ds.getPackageEntriesList()) {
            assertTrue(0 <= pe.getUid());

            assertTrue(0 <= pe.getPendingState().getDurationMs());
            assertTrue(0 <= pe.getPendingState().getCount());
            assertTrue(0 <= pe.getActiveState().getDurationMs());
            assertTrue(0 <= pe.getActiveState().getCount());
            assertTrue(0 <= pe.getActiveTopState().getDurationMs());
            assertTrue(0 <= pe.getActiveTopState().getCount());

            for (DataSetProto.PackageEntryProto.StopReasonCount src : pe.getStopReasonsList()) {
                assertTrue(StopReasonEnum.getDescriptor().getValues()
                        .contains(src.getReason().getValueDescriptor()));
                assertTrue(0 <= src.getCount());
            }
        }
        assertTrue(0 <= ds.getMaxConcurrency());
        assertTrue(0 <= ds.getMaxForegroundConcurrency());
    }

    private static void testJobPackageHistoryProto(JobPackageHistoryProto jph, int filterLevel) throws Exception {
        assertNotNull(jph);

        for (JobPackageHistoryProto.HistoryEvent he : jph.getHistoryEventList()) {
            assertTrue(JobPackageHistoryProto.Event.getDescriptor().getValues()
                    .contains(he.getEvent().getValueDescriptor()));
            assertTrue(0 <= he.getTimeSinceEventMs()); // Should be positive.
            assertTrue(0 <= he.getUid());
            assertTrue(StopReasonEnum.getDescriptor().getValues()
                    .contains(he.getStopReason().getValueDescriptor()));
        }
    }

    private static void testJobPackageTrackerDumpProto(JobPackageTrackerDumpProto jptd) throws Exception {
        assertNotNull(jptd);

        for (DataSetProto ds : jptd.getHistoricalStatsList()) {
            testDataSetProto(ds);
        }
        testDataSetProto(jptd.getCurrentStats());
    }

    private static void testJobStatusShortInfoProto(JobStatusShortInfoProto jssi, final int filterLevel) throws Exception {
        assertNotNull(jssi);

        assertTrue(0 <= jssi.getCallingUid());
    }

    private static void testJobStatusDumpProto(JobStatusDumpProto jsd) throws Exception {
        assertNotNull(jsd);

        assertTrue(0 <= jsd.getCallingUid());
        assertTrue(0 <= jsd.getSourceUid());
        assertTrue(0 <= jsd.getSourceUserId());

        JobStatusDumpProto.JobInfo ji = jsd.getJobInfo();
        if (ji.getIsPeriodic()) {
            assertTrue(0 <= ji.getPeriodIntervalMs());
            assertTrue(0 <= ji.getPeriodFlexMs());
        }
        assertTrue(0 <= ji.getTriggerContentUpdateDelayMs());
        assertTrue(0 <= ji.getTriggerContentMaxDelayMs());
        testNetworkRequestProto(ji.getRequiredNetwork());
        assertTrue(0 <= ji.getTotalNetworkBytes());
        assertTrue(0 <= ji.getMinLatencyMs());
        assertTrue(0 <= ji.getMaxExecutionDelayMs());
        JobStatusDumpProto.JobInfo.Backoff bp = ji.getBackoffPolicy();
        assertTrue(JobStatusDumpProto.JobInfo.Backoff.Policy.getDescriptor().getValues()
                .contains(bp.getPolicy().getValueDescriptor()));
        assertTrue(0 <= bp.getInitialBackoffMs());

        for (JobStatusDumpProto.Constraint c : jsd.getRequiredConstraintsList()) {
            assertTrue(JobStatusDumpProto.Constraint.getDescriptor().getValues()
                    .contains(c.getValueDescriptor()));
        }
        for (JobStatusDumpProto.Constraint c : jsd.getSatisfiedConstraintsList()) {
            assertTrue(JobStatusDumpProto.Constraint.getDescriptor().getValues()
                    .contains(c.getValueDescriptor()));
        }
        for (JobStatusDumpProto.Constraint c : jsd.getUnsatisfiedConstraintsList()) {
            assertTrue(JobStatusDumpProto.Constraint.getDescriptor().getValues()
                    .contains(c.getValueDescriptor()));
        }

        for (JobStatusDumpProto.TrackingController tc : jsd.getTrackingControllersList()) {
            assertTrue(JobStatusDumpProto.TrackingController.getDescriptor().getValues()
                    .contains(tc.getValueDescriptor()));
        }

        for (JobStatusDumpProto.JobWorkItem jwi : jsd.getPendingWorkList()) {
            assertTrue(0 <= jwi.getDeliveryCount());
        }
        for (JobStatusDumpProto.JobWorkItem jwi : jsd.getExecutingWorkList()) {
            assertTrue(0 <= jwi.getDeliveryCount());
        }

        assertTrue(JobStatusDumpProto.Bucket.getDescriptor().getValues()
                .contains(jsd.getStandbyBucket().getValueDescriptor()));

        assertTrue(0 <= jsd.getEnqueueDurationMs());

        assertTrue(0 <= jsd.getNumFailures());

        assertTrue(0 <= jsd.getLastSuccessfulRunTime());
        assertTrue(0 <= jsd.getLastFailedRunTime());
    }

    private static void testNetworkRequestProto(NetworkRequestProto nr) throws Exception {
        assertNotNull(nr);

        assertTrue(NetworkRequestProto.Type.getDescriptor().getValues()
                .contains(nr.getType().getValueDescriptor()));
        testNetworkCapabilitesProto(nr.getNetworkCapabilities());
    }

    private static void testNetworkCapabilitesProto(NetworkCapabilitiesProto nc) throws Exception {
        assertNotNull(nc);

        for (NetworkCapabilitiesProto.Transport t : nc.getTransportsList()) {
            assertTrue(NetworkCapabilitiesProto.Transport.getDescriptor().getValues()
                .contains(t.getValueDescriptor()));
        }
        for (NetworkCapabilitiesProto.NetCapability c : nc.getCapabilitiesList()) {
            assertTrue(NetworkCapabilitiesProto.NetCapability.getDescriptor().getValues()
                .contains(c.getValueDescriptor()));
        }

        assertTrue(0 <= nc.getLinkUpBandwidthKbps());
        assertTrue(0 <= nc.getLinkDownBandwidthKbps());
    }

    private static void testStateControllerProto(StateControllerProto sc, int filterLevel) throws Exception {
        assertNotNull(sc);

        StateControllerProto.BackgroundJobsController bjc = sc.getBackground();
        for (StateControllerProto.BackgroundJobsController.TrackedJob tj : bjc.getTrackedJobsList()) {
            testJobStatusShortInfoProto(tj.getInfo(), filterLevel);
            assertTrue(0 <= tj.getSourceUid());
        }
        StateControllerProto.BatteryController bc = sc.getBattery();
        for (StateControllerProto.BatteryController.TrackedJob tj : bc.getTrackedJobsList()) {
            testJobStatusShortInfoProto(tj.getInfo(), filterLevel);
            assertTrue(0 <= tj.getSourceUid());
        }
        StateControllerProto.ConnectivityController cc = sc.getConnectivity();
        for (StateControllerProto.ConnectivityController.TrackedJob tj : cc.getTrackedJobsList()) {
            testJobStatusShortInfoProto(tj.getInfo(), filterLevel);
            assertTrue(0 <= tj.getSourceUid());
            testNetworkRequestProto(tj.getRequiredNetwork());
        }
        StateControllerProto.ContentObserverController coc = sc.getContentObserver();
        for (StateControllerProto.ContentObserverController.TrackedJob tj : coc.getTrackedJobsList()) {
            testJobStatusShortInfoProto(tj.getInfo(), filterLevel);
            assertTrue(0 <= tj.getSourceUid());
        }
        for (StateControllerProto.ContentObserverController.Observer o : coc.getObserversList()) {
            assertTrue(0 <= o.getUserId());

            for (StateControllerProto.ContentObserverController.Observer.TriggerContentData tcd : o.getTriggersList()) {
                if (filterLevel == PRIVACY_AUTO) {
                    assertTrue(tcd.getUri().isEmpty());
                }
                for (StateControllerProto.ContentObserverController.Observer.TriggerContentData.JobInstance ji : tcd.getJobsList()) {
                    testJobStatusShortInfoProto(ji.getInfo(), filterLevel);

                    assertTrue(0 <= ji.getSourceUid());
                    assertTrue(0 <= ji.getTriggerContentUpdateDelayMs());
                    assertTrue(0 <= ji.getTriggerContentMaxDelayMs());

                    if (filterLevel == PRIVACY_AUTO) {
                        for (String ca : ji.getChangedAuthoritiesList()) {
                            assertTrue(ca.isEmpty());
                        }
                        for (String ca : ji.getChangedUrisList()) {
                            assertTrue(ca.isEmpty());
                        }
                    }
                }
            }
        }
        StateControllerProto.DeviceIdleJobsController dijc = sc.getDeviceIdle();
        for (StateControllerProto.DeviceIdleJobsController.TrackedJob tj : dijc.getTrackedJobsList()) {
            testJobStatusShortInfoProto(tj.getInfo(), filterLevel);
            assertTrue(0 <= tj.getSourceUid());
        }
        StateControllerProto.IdleController ic = sc.getIdle();
        for (StateControllerProto.IdleController.TrackedJob tj : ic.getTrackedJobsList()) {
            testJobStatusShortInfoProto(tj.getInfo(), filterLevel);
            assertTrue(0 <= tj.getSourceUid());
        }
        StateControllerProto.StorageController scr = sc.getStorage();
        for (StateControllerProto.StorageController.TrackedJob tj : scr.getTrackedJobsList()) {
            testJobStatusShortInfoProto(tj.getInfo(), filterLevel);
            assertTrue(0 <= tj.getSourceUid());
        }
        StateControllerProto.TimeController tc = sc.getTime();
        assertTrue(0 <=  tc.getNowElapsedRealtime());
        for (StateControllerProto.TimeController.TrackedJob tj : tc.getTrackedJobsList()) {
            testJobStatusShortInfoProto(tj.getInfo(), filterLevel);
            assertTrue(0 <= tj.getSourceUid());
        }
    }
}
