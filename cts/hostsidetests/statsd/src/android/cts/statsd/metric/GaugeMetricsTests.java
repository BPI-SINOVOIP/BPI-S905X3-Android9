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
import com.android.internal.os.StatsdConfigProto.AtomMatcher;
import com.android.internal.os.StatsdConfigProto.FieldFilter;
import com.android.internal.os.StatsdConfigProto.FieldMatcher;
import com.android.internal.os.StatsdConfigProto.FieldValueMatcher;
import com.android.internal.os.StatsdConfigProto.Predicate;
import com.android.internal.os.StatsdConfigProto.SimpleAtomMatcher;
import com.android.internal.os.StatsdConfigProto.SimplePredicate;
import com.android.os.AtomsProto.AppBreadcrumbReported;
import com.android.os.AtomsProto.Atom;
import com.android.os.StatsLog.GaugeMetricData;
import com.android.os.StatsLog.StatsLogReport;
import com.android.tradefed.log.LogUtil;

public class GaugeMetricsTests extends DeviceAtomTestCase {

  private static final int APP_BREADCRUMB_REPORTED_A_MATCH_START_ID = 0;
  private static final int APP_BREADCRUMB_REPORTED_A_MATCH_STOP_ID = 1;
  private static final int APP_BREADCRUMB_REPORTED_B_MATCH_START_ID = 2;

  public void testGaugeMetric() throws Exception {
    if (statsdDisabled()) {
      return;
    }
    // Add AtomMatcher's.
    AtomMatcher startAtomMatcher =
        MetricsUtils.startAtomMatcher(APP_BREADCRUMB_REPORTED_A_MATCH_START_ID);
    AtomMatcher stopAtomMatcher =
        MetricsUtils.stopAtomMatcher(APP_BREADCRUMB_REPORTED_A_MATCH_STOP_ID);
    AtomMatcher atomMatcher =
        MetricsUtils.simpleAtomMatcher(APP_BREADCRUMB_REPORTED_B_MATCH_START_ID);

    StatsdConfigProto.StatsdConfig.Builder builder = createConfigBuilder();
    builder.addAtomMatcher(startAtomMatcher);
    builder.addAtomMatcher(stopAtomMatcher);
    builder.addAtomMatcher(atomMatcher);

    // Add Predicate's.
    SimplePredicate simplePredicate = SimplePredicate.newBuilder()
                                          .setStart(APP_BREADCRUMB_REPORTED_A_MATCH_START_ID)
                                          .setStop(APP_BREADCRUMB_REPORTED_A_MATCH_STOP_ID)
                                          .build();
    Predicate predicate = Predicate.newBuilder()
                              .setId(MetricsUtils.StringToId("Predicate"))
                              .setSimplePredicate(simplePredicate)
                              .build();
    builder.addPredicate(predicate);

    // Add GaugeMetric.
    FieldMatcher fieldMatcher =
        FieldMatcher.newBuilder().setField(APP_BREADCRUMB_REPORTED_B_MATCH_START_ID).build();
    builder.addGaugeMetric(
        StatsdConfigProto.GaugeMetric.newBuilder()
            .setId(MetricsUtils.GAUGE_METRIC_ID)
            .setWhat(APP_BREADCRUMB_REPORTED_B_MATCH_START_ID)
            .setCondition(predicate.getId())
            .setGaugeFieldsFilter(
                FieldFilter.newBuilder().setIncludeAll(false).setFields(fieldMatcher).build())
            .setDimensionsInWhat(
                FieldMatcher.newBuilder()
                    .setField(APP_BREADCRUMB_REPORTED_B_MATCH_START_ID)
                    .addChild(FieldMatcher.newBuilder()
                                  .setField(AppBreadcrumbReported.STATE_FIELD_NUMBER)
                                  .build())
                    .build())
            .setBucket(StatsdConfigProto.TimeUnit.CTS)
            .build());

    // Upload config.
    uploadConfig(builder);

    // Create AppBreadcrumbReported Start/Stop events.
    doAppBreadcrumbReportedStart(0);
    Thread.sleep(10);
    doAppBreadcrumbReportedStart(1);
    Thread.sleep(10);
    doAppBreadcrumbReportedStart(2);
    Thread.sleep(2000);
    doAppBreadcrumbReportedStop(2);
    Thread.sleep(10);
    doAppBreadcrumbReportedStop(0);
    Thread.sleep(10);
    doAppBreadcrumbReportedStop(1);
    doAppBreadcrumbReportedStart(2);
    Thread.sleep(10);
    doAppBreadcrumbReportedStart(1);
    Thread.sleep(2000);
    doAppBreadcrumbReportedStop(2);
    Thread.sleep(10);
    doAppBreadcrumbReportedStop(1);

    // Wait for the metrics to propagate to statsd.
    Thread.sleep(2000);

    StatsLogReport metricReport = getStatsLogReport();
    LogUtil.CLog.d("Got the following gauge metric data: " + metricReport.toString());
    assertEquals(MetricsUtils.GAUGE_METRIC_ID, metricReport.getMetricId());
    assertTrue(metricReport.hasGaugeMetrics());
    StatsLogReport.GaugeMetricDataWrapper gaugeData = metricReport.getGaugeMetrics();
    assertEquals(gaugeData.getDataCount(), 1);

    int bucketCount = gaugeData.getData(0).getBucketInfoCount();
    GaugeMetricData data = gaugeData.getData(0);
    assertTrue(bucketCount > 2);
    MetricsUtils.assertBucketTimePresent(data.getBucketInfo(0));
    assertEquals(data.getBucketInfo(0).getAtomCount(), 1);
    assertEquals(data.getBucketInfo(0).getAtom(0).getAppBreadcrumbReported().getLabel(), 0);
    assertEquals(data.getBucketInfo(0).getAtom(0).getAppBreadcrumbReported().getState(),
        AppBreadcrumbReported.State.START);

    MetricsUtils.assertBucketTimePresent(data.getBucketInfo(1));
    assertEquals(data.getBucketInfo(1).getAtomCount(), 1);

    MetricsUtils.assertBucketTimePresent(data.getBucketInfo(bucketCount-1));
    assertEquals(data.getBucketInfo(bucketCount - 1).getAtomCount(), 1);
    assertEquals(
        data.getBucketInfo(bucketCount - 1).getAtom(0).getAppBreadcrumbReported().getLabel(), 2);
    assertEquals(
        data.getBucketInfo(bucketCount - 1).getAtom(0).getAppBreadcrumbReported().getState(),
        AppBreadcrumbReported.State.STOP);
  }
}
