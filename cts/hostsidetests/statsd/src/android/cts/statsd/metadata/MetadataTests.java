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
package android.cts.statsd.metadata;

import android.cts.statsd.atom.AtomTestCase;

import com.android.internal.os.StatsdConfigProto;
import com.android.internal.os.StatsdConfigProto.StatsdConfig;
import com.android.internal.os.StatsdConfigProto.Subscription;
import com.android.internal.os.StatsdConfigProto.TimeUnit;
import com.android.internal.os.StatsdConfigProto.ValueMetric;
import com.android.os.AtomsProto.AnomalyDetected;
import com.android.os.AtomsProto.AppBreadcrumbReported;
import com.android.os.AtomsProto.Atom;
import com.android.os.StatsLog.EventMetricData;
import com.android.os.StatsLog.StatsdStatsReport;
import com.android.os.StatsLog.StatsdStatsReport.ConfigStats;
import com.android.tradefed.log.LogUtil;


import java.util.List;

/**
 * Statsd Anomaly Detection tests.
 */
public class MetadataTests extends MetadataTestCase {

    private static final String TAG = "Statsd.MetadataTests";

    // Tests that anomaly detection for value works.
    public void testConfigTtl() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        final int TTL_TIME_SEC = 8;
        StatsdConfig.Builder config = getBaseConfig();
        config.setTtlInSeconds(TTL_TIME_SEC); // should reset in 3 seconds.
        turnScreenOff();

        uploadConfig(config);
        long startTime = System.currentTimeMillis();
        Thread.sleep(WAIT_TIME_SHORT);
        turnScreenOn();
        Thread.sleep(WAIT_TIME_SHORT);
        StatsdStatsReport report = getStatsdStatsReport(); // Has only been 1 second
        LogUtil.CLog.d("got following statsdstats report: " + report.toString());
        boolean foundActiveConfig = false;
        int creationTime = 0;
        for (ConfigStats stats: report.getConfigStatsList()) {
            if (stats.getId() == CONFIG_ID) {
                if(!stats.hasDeletionTimeSec()) {
                    assertTrue("Found multiple active CTS configs!", foundActiveConfig == false);
                    foundActiveConfig = true;
                    creationTime = stats.getCreationTimeSec();
                }
            }
        }
        assertTrue("Did not find an active CTS config", foundActiveConfig);

        turnScreenOff();
        while(System.currentTimeMillis() - startTime < 8_000) {
            Thread.sleep(10);
        }
        turnScreenOn(); // Force events to make sure the config TTLs.
        report = getStatsdStatsReport();
        LogUtil.CLog.d("got following statsdstats report: " + report.toString());
        foundActiveConfig = false;
        int expectedTime = creationTime + TTL_TIME_SEC;
        for (ConfigStats stats: report.getConfigStatsList()) {
            if (stats.getId() == CONFIG_ID) {
                // Original config should be TTL'd
                if (stats.getCreationTimeSec() == creationTime) {
                    assertTrue("Config should have TTL'd but is still active",
                            stats.hasDeletionTimeSec());
                    assertTrue("Config deletion time should be about " + TTL_TIME_SEC +
                            " after creation",
                            Math.abs(stats.getDeletionTimeSec() - expectedTime) <= 2);
                }
                // There should still be one active config, that is marked as reset.
                if(!stats.hasDeletionTimeSec()) {
                    assertTrue("Found multiple active CTS configs!", foundActiveConfig == false);
                    foundActiveConfig = true;
                    creationTime = stats.getCreationTimeSec();
                    assertTrue("Active config after TTL should be marked as reset",
                            stats.hasResetTimeSec());
                    assertEquals("Reset time and creation time should be equal for TTl'd configs",
                            stats.getResetTimeSec(), stats.getCreationTimeSec());
                    assertTrue("Reset config should be created when the original config TTL'd",
                            Math.abs(stats.getCreationTimeSec() - expectedTime) <= 2);
                }
            }
        }
        assertTrue("Did not find an active CTS config after the TTL", foundActiveConfig);
    }
}
