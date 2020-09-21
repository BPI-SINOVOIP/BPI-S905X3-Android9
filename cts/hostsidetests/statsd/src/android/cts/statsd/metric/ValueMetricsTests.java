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
import com.android.internal.os.StatsdConfigProto.FieldMatcher;
import com.android.internal.os.StatsdConfigProto.ValueMetric;
import com.android.os.AtomsProto.AppBreadcrumbReported;
import com.android.os.AtomsProto.SystemElapsedRealtime;

import com.android.os.AtomsProto.Atom;
import com.android.os.StatsLog.ValueBucketInfo;
import com.android.os.StatsLog.ValueMetricData;
import com.android.os.StatsLog.StatsLogReport;
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
import com.android.os.AtomsProto.Atom;
import com.android.os.AtomsProto.AppBreadcrumbReported;
import com.android.os.AtomsProto.ScreenStateChanged;
import com.android.os.StatsLog.ConfigMetricsReport;
import com.android.os.StatsLog.ConfigMetricsReportList;
import com.android.os.StatsLog.EventMetricData;
import com.android.os.StatsLog.GaugeMetricData;
import com.android.os.StatsLog.StatsLogReport;
import com.android.tradefed.log.LogUtil;

public class ValueMetricsTests extends DeviceAtomTestCase {
  private static final int APP_BREADCRUMB_REPORTED_A_MATCH_START_ID = 0;
  private static final int APP_BREADCRUMB_REPORTED_A_MATCH_STOP_ID = 1;
  private static final int APP_BREADCRUMB_REPORTED_B_MATCH_START_ID = 2;

  public void testValueMetric() throws Exception {
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

    // Add ValueMetric.
    builder.addValueMetric(
        ValueMetric.newBuilder()
            .setId(MetricsUtils.VALUE_METRIC_ID)
            .setWhat(APP_BREADCRUMB_REPORTED_B_MATCH_START_ID)
            .setBucket(StatsdConfigProto.TimeUnit.CTS)
            .setValueField(FieldMatcher.newBuilder()
                               .setField(Atom.APP_BREADCRUMB_REPORTED_FIELD_NUMBER)
                               .addChild(FieldMatcher.newBuilder().setField(
                                   AppBreadcrumbReported.LABEL_FIELD_NUMBER)))
            .setDimensionsInWhat(FieldMatcher.newBuilder()
                                     .setField(APP_BREADCRUMB_REPORTED_B_MATCH_START_ID)
                                     .build())
            .build());

    // Upload config.
    uploadConfig(builder);

    // Create AppBreadcrumbReported Start/Stop events.
    doAppBreadcrumbReportedStart(1);
    Thread.sleep(1000);
    doAppBreadcrumbReportedStop(1);
    doAppBreadcrumbReportedStart(3);
    doAppBreadcrumbReportedStop(3);

    // Wait for the metrics to propagate to statsd.
    Thread.sleep(2000);

    StatsLogReport metricReport = getStatsLogReport();
    LogUtil.CLog.d("Got the following value metric data: " + metricReport.toString());
    assertEquals(MetricsUtils.VALUE_METRIC_ID, metricReport.getMetricId());
    assertTrue(metricReport.hasValueMetrics());
    StatsLogReport.ValueMetricDataWrapper valueData = metricReport.getValueMetrics();
    assertEquals(valueData.getDataCount(), 1);

    int bucketCount = valueData.getData(0).getBucketInfoCount();
    assertTrue(bucketCount > 1);
    ValueMetricData data = valueData.getData(0);
    int totalValue = 0;
    for (ValueBucketInfo bucketInfo : data.getBucketInfoList()) {
      MetricsUtils.assertBucketTimePresent(bucketInfo);
      totalValue += (int) bucketInfo.getValue();
    }
    assertEquals(totalValue, 8);
  }

  // Test value metric with pulled atoms and across multiple buckets
  public void testPullerAcrossBuckets() throws Exception {
    if (statsdDisabled()) {
      return;
    }
    // Add AtomMatcher's.
    final String predicateTrueName = "APP_BREADCRUMB_REPORTED_START";
    final String predicateFalseName = "APP_BREADCRUMB_REPORTED_STOP";
    final String predicateName = "APP_BREADCRUMB_REPORTED_IS_STOP";

    AtomMatcher startAtomMatcher =
            MetricsUtils.startAtomMatcher(predicateTrueName.hashCode());
    AtomMatcher stopAtomMatcher =
            MetricsUtils.stopAtomMatcher(predicateFalseName.hashCode());

    StatsdConfigProto.StatsdConfig.Builder builder = createConfigBuilder();
    builder.addAtomMatcher(startAtomMatcher);
    builder.addAtomMatcher(stopAtomMatcher);
    builder.addPredicate(Predicate.newBuilder()
            .setId(predicateName.hashCode())
            .setSimplePredicate(SimplePredicate.newBuilder()
                    .setStart(predicateTrueName.hashCode())
                    .setStop(predicateFalseName.hashCode())
                    .setCountNesting(false)
            )
    );

    final String atomName = "SYSTEM_ELAPSED_REALTIME";
    SimpleAtomMatcher.Builder sam = SimpleAtomMatcher.newBuilder().setAtomId(Atom.SYSTEM_ELAPSED_REALTIME_FIELD_NUMBER);
    builder.addAtomMatcher(AtomMatcher.newBuilder()
            .setId(atomName.hashCode())
            .setSimpleAtomMatcher(sam));

    // Add ValueMetric.
    builder.addValueMetric(
            ValueMetric.newBuilder()
                    .setId(MetricsUtils.VALUE_METRIC_ID)
                    .setWhat(atomName.hashCode())
                    .setBucket(StatsdConfigProto.TimeUnit.ONE_MINUTE)
                    .setValueField(FieldMatcher.newBuilder()
                            .setField(Atom.SYSTEM_ELAPSED_REALTIME_FIELD_NUMBER)
                            .addChild(FieldMatcher.newBuilder().setField(
                                    SystemElapsedRealtime.TIME_MILLIS_FIELD_NUMBER)))
                    .setCondition(predicateName.hashCode())
                    .build());

    // Upload config.
    uploadConfig(builder);

    // Create AppBreadcrumbReported Start/Stop events.
    doAppBreadcrumbReportedStart(1);
    // Wait for 2 min and 1 sec to capture at least 2 buckets
    Thread.sleep(2*60_000 + 10_000);
    doAppBreadcrumbReportedStop(1);

    // Wait for the metrics to propagate to statsd.
    Thread.sleep(1_000);

    StatsLogReport metricReport = getStatsLogReport();
    LogUtil.CLog.d("Got the following value metric data: " + metricReport.toString());
    assertEquals(MetricsUtils.VALUE_METRIC_ID, metricReport.getMetricId());
    assertTrue(metricReport.hasValueMetrics());
    StatsLogReport.ValueMetricDataWrapper valueData = metricReport.getValueMetrics();
    assertEquals(valueData.getDataCount(), 1);

    int bucketCount = valueData.getData(0).getBucketInfoCount();
    // should have at least 2 buckets
    assertTrue(bucketCount >= 2);
    ValueMetricData data = valueData.getData(0);
    int totalValue = 0;
    for (ValueBucketInfo bucketInfo : data.getBucketInfoList()) {
      MetricsUtils.assertBucketTimePresent(bucketInfo);
      totalValue += (int) bucketInfo.getValue();
    }
    // At most we lose one full min bucket
    assertTrue(totalValue > (130_000 - 60_000));
  }

  // Test value metric with pulled atoms and across multiple buckets
  public void testMultipleEventsPerBucket() throws Exception {
    if (statsdDisabled()) {
      return;
    }
    // Add AtomMatcher's.
    final String predicateTrueName = "APP_BREADCRUMB_REPORTED_START";
    final String predicateFalseName = "APP_BREADCRUMB_REPORTED_STOP";
    final String predicateName = "APP_BREADCRUMB_REPORTED_IS_STOP";

    AtomMatcher startAtomMatcher =
            MetricsUtils.startAtomMatcher(predicateTrueName.hashCode());
    AtomMatcher stopAtomMatcher =
            MetricsUtils.stopAtomMatcher(predicateFalseName.hashCode());

    StatsdConfigProto.StatsdConfig.Builder builder = createConfigBuilder();
    builder.addAtomMatcher(startAtomMatcher);
    builder.addAtomMatcher(stopAtomMatcher);
    builder.addPredicate(Predicate.newBuilder()
            .setId(predicateName.hashCode())
            .setSimplePredicate(SimplePredicate.newBuilder()
                    .setStart(predicateTrueName.hashCode())
                    .setStop(predicateFalseName.hashCode())
                    .setCountNesting(false)
            )
    );

    final String atomName = "SYSTEM_ELAPSED_REALTIME";
    SimpleAtomMatcher.Builder sam = SimpleAtomMatcher.newBuilder().setAtomId(Atom.SYSTEM_ELAPSED_REALTIME_FIELD_NUMBER);
    builder.addAtomMatcher(AtomMatcher.newBuilder()
            .setId(atomName.hashCode())
            .setSimpleAtomMatcher(sam));

    // Add ValueMetric.
    builder.addValueMetric(
            ValueMetric.newBuilder()
                    .setId(MetricsUtils.VALUE_METRIC_ID)
                    .setWhat(atomName.hashCode())
                    .setBucket(StatsdConfigProto.TimeUnit.ONE_MINUTE)
                    .setValueField(FieldMatcher.newBuilder()
                            .setField(Atom.SYSTEM_ELAPSED_REALTIME_FIELD_NUMBER)
                            .addChild(FieldMatcher.newBuilder().setField(
                                    SystemElapsedRealtime.TIME_MILLIS_FIELD_NUMBER)))
                    .setCondition(predicateName.hashCode())
                    .build());

    // Upload config.
    uploadConfig(builder);

    final int NUM_EVENTS = 10;
    final long GAP_INTERVAL = 10_000;
    // Create AppBreadcrumbReported Start/Stop events.
    for (int i = 0; i < NUM_EVENTS; i ++) {
      doAppBreadcrumbReportedStart(1);
      Thread.sleep(GAP_INTERVAL);
      doAppBreadcrumbReportedStop(1);
      Thread.sleep(GAP_INTERVAL);
    }

    // Wait for the metrics to propagate to statsd.
    Thread.sleep(1_000);

    StatsLogReport metricReport = getStatsLogReport();
    LogUtil.CLog.d("Got the following value metric data: " + metricReport.toString());
    assertEquals(MetricsUtils.VALUE_METRIC_ID, metricReport.getMetricId());
    assertTrue(metricReport.hasValueMetrics());
    StatsLogReport.ValueMetricDataWrapper valueData = metricReport.getValueMetrics();
    assertEquals(valueData.getDataCount(), 1);

    int bucketCount = valueData.getData(0).getBucketInfoCount();
    // should have at least 2 buckets
    assertTrue(bucketCount >= 2);
    ValueMetricData data = valueData.getData(0);
    int totalValue = 0;
    for (ValueBucketInfo bucketInfo : data.getBucketInfoList()) {
      MetricsUtils.assertBucketTimePresent(bucketInfo);
      totalValue += (int) bucketInfo.getValue();
    }
    // At most we lose one full min bucket
    assertTrue(totalValue > (GAP_INTERVAL*NUM_EVENTS - 60_000));
  }
}
