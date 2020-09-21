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
package android.cts.statsd.metric;

import android.cts.statsd.atom.DeviceAtomTestCase;

import com.android.internal.os.StatsdConfigProto;
import com.android.os.AtomsProto.Atom;
import com.android.os.AtomsProto.AppBreadcrumbReported;
import com.android.os.StatsLog;
import com.android.os.StatsLog.ConfigMetricsReport;
import com.android.os.StatsLog.ConfigMetricsReportList;
import com.android.os.StatsLog.CountBucketInfo;
import com.android.os.StatsLog.CountMetricData;
import com.android.os.StatsLog.StatsLogReport;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil;

import java.util.Arrays;
import java.util.List;

public class CountMetricsTests extends DeviceAtomTestCase {

    public void testSimpleEventCountMetric() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        int matcherId = 1;
        StatsdConfigProto.StatsdConfig.Builder builder = createConfigBuilder();
        builder.addCountMetric(StatsdConfigProto.CountMetric.newBuilder()
                .setId(MetricsUtils.COUNT_METRIC_ID)
                .setBucket(StatsdConfigProto.TimeUnit.CTS)
                .setWhat(matcherId))
                .addAtomMatcher(MetricsUtils.simpleAtomMatcher(matcherId));
        uploadConfig(builder);

        doAppBreadcrumbReportedStart(0);
        doAppBreadcrumbReportedStop(0);
        Thread.sleep(2000);  // Wait for the metrics to propagate to statsd.

        StatsLogReport metricReport = getStatsLogReport();
        LogUtil.CLog.d("Got the following stats log report: \n" + metricReport.toString());
        assertEquals(MetricsUtils.COUNT_METRIC_ID, metricReport.getMetricId());
        assertTrue(metricReport.hasCountMetrics());

        StatsLogReport.CountMetricDataWrapper countData = metricReport.getCountMetrics();

        assertTrue(countData.getDataCount() > 0);
        assertEquals(2, countData.getData(0).getBucketInfo(0).getCount());
    }
    public void testEventCountWithCondition() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        int startMatcherId = 1;
        int endMatcherId = 2;
        int whatMatcherId = 3;
        int conditionId = 4;

        StatsdConfigProto.AtomMatcher whatMatcher =
               MetricsUtils.unspecifiedAtomMatcher(whatMatcherId);

        StatsdConfigProto.AtomMatcher predicateStartMatcher =
                MetricsUtils.startAtomMatcher(startMatcherId);

        StatsdConfigProto.AtomMatcher predicateEndMatcher =
                MetricsUtils.stopAtomMatcher(endMatcherId);

        StatsdConfigProto.Predicate p = StatsdConfigProto.Predicate.newBuilder()
                .setSimplePredicate(StatsdConfigProto.SimplePredicate.newBuilder()
                        .setStart(startMatcherId)
                        .setStop(endMatcherId)
                        .setCountNesting(false))
                .setId(conditionId)
                .build();

        StatsdConfigProto.StatsdConfig.Builder builder = createConfigBuilder()
                .addCountMetric(StatsdConfigProto.CountMetric.newBuilder()
                        .setId(MetricsUtils.COUNT_METRIC_ID)
                        .setBucket(StatsdConfigProto.TimeUnit.CTS)
                        .setWhat(whatMatcherId)
                        .setCondition(conditionId))
                .addAtomMatcher(whatMatcher)
                .addAtomMatcher(predicateStartMatcher)
                .addAtomMatcher(predicateEndMatcher)
                .addPredicate(p);

        uploadConfig(builder);

        doAppBreadcrumbReported(0, AppBreadcrumbReported.State.UNSPECIFIED.ordinal());
        Thread.sleep(10);
        doAppBreadcrumbReportedStart(0);
        Thread.sleep(10);
        doAppBreadcrumbReported(0, AppBreadcrumbReported.State.UNSPECIFIED.ordinal());
        Thread.sleep(10);
        doAppBreadcrumbReportedStop(0);
        Thread.sleep(10);
        doAppBreadcrumbReported(0, AppBreadcrumbReported.State.UNSPECIFIED.ordinal());
        Thread.sleep(2000);  // Wait for the metrics to propagate to statsd.

        StatsLogReport metricReport = getStatsLogReport();
        assertEquals(MetricsUtils.COUNT_METRIC_ID, metricReport.getMetricId());
        assertTrue(metricReport.hasCountMetrics());

        StatsLogReport.CountMetricDataWrapper countData = metricReport.getCountMetrics();

        assertTrue(countData.getDataCount() > 0);
        assertEquals(1, countData.getData(0).getBucketInfo(0).getCount());
    }

    public void testPartialBucketCountMetric() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        int matcherId = 1;
        StatsdConfigProto.StatsdConfig.Builder builder = createConfigBuilder();
        builder.addCountMetric(StatsdConfigProto.CountMetric.newBuilder()
                .setId(MetricsUtils.COUNT_METRIC_ID)
                .setBucket(StatsdConfigProto.TimeUnit.ONE_DAY)  // Should ensure partial bucket.
                .setWhat(matcherId))
                .addAtomMatcher(MetricsUtils.simpleAtomMatcher(matcherId));
        uploadConfig(builder);

        doAppBreadcrumbReportedStart(0);

        builder.getCountMetricBuilder(0).setBucket(StatsdConfigProto.TimeUnit.CTS);
        uploadConfig(builder);  // The count metric had a partial bucket.
        doAppBreadcrumbReportedStart(0);
        Thread.sleep(10);
        doAppBreadcrumbReportedStart(0);
        Thread.sleep(WAIT_TIME_LONG); // Finish the current bucket.

        ConfigMetricsReportList reports = getReportList();
        LogUtil.CLog.d("Got following report list: " + reports.toString());

        assertEquals("Expected 2 reports, got " + reports.getReportsCount(),
                2, reports.getReportsCount());
        boolean inOrder = reports.getReports(0).getCurrentReportWallClockNanos() <
                reports.getReports(1).getCurrentReportWallClockNanos();

        // Only 1 metric, so there should only be 1 StatsLogReport.
        for (ConfigMetricsReport report : reports.getReportsList()) {
            assertEquals("Expected 1 StatsLogReport in each ConfigMetricsReport",
                    1, report.getMetricsCount());
            assertEquals("Expected 1 CountMetricData in each report",
                    1, report.getMetrics(0).getCountMetrics().getDataCount());
        }
        CountMetricData data1 =
                reports.getReports(inOrder? 0 : 1).getMetrics(0).getCountMetrics().getData(0);
        CountMetricData data2 =
                reports.getReports(inOrder? 1 : 0).getMetrics(0).getCountMetrics().getData(0);
        // Data1 should have only 1 bucket, and it should be a partial bucket.
        // The count should be 1.
        assertEquals("First report should only have 1 bucket", 1, data1.getBucketInfoCount());
        CountBucketInfo bucketInfo = data1.getBucketInfo(0);
        assertEquals("First report should have a count of 1", 1, bucketInfo.getCount());
        assertTrue("First report's bucket should be less than 1 day",
                bucketInfo.getEndBucketElapsedNanos() <
                (bucketInfo.getStartBucketElapsedNanos() + 1_000_000_000L * 60L * 60L * 24L));

        //Second report should have a count of 2.
        assertTrue("Second report should have at most 2 buckets", data2.getBucketInfoCount() < 3);
        int totalCount = 0;
        for (CountBucketInfo bucket : data2.getBucketInfoList()) {
            totalCount += bucket.getCount();
        }
        assertEquals("Second report should have a count of 2", 2, totalCount);
    }
}
