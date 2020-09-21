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
package com.android.collectors;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;

import android.service.batterystats.BatteryStatsServiceDumpProto;

import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.metric.DeviceMetricData;
import com.android.tradefed.device.metric.FilePullerDeviceMetricCollector;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Measurements;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.Collection;

/**
 * Host side tests for the Batterystats collector, this ensure that we are able to use the
 * collector in a similar way as the infra.
 *
 * Command:
 * mm CollectorHostsideLibTest CollectorDeviceLibTest -j16
 * tradefed.sh run commandAndExit template/local_min --template:map test=CollectorHostsideLibTest
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class BatterystatsCollectorHostTest extends BaseHostJUnit4Test {
    private static final String TEST_APK = "CollectorDeviceLibTest.apk";
    private static final String PACKAGE_NAME = "android.device.collectors";
    private static final String AJUR_RUNNER = "android.support.test.runner.AndroidJUnitRunner";

    private static final String BATTERYSTATS_COLLECTOR =
            "android.device.collectors.BatteryStatsListener";
    private static final String BATTERYSTATS_PROTO = "batterystatsproto";

    private RemoteAndroidTestRunner mTestRunner;
    private IInvocationContext mContext;

    @Before
    public void setUp() throws Exception {
        installPackage(TEST_APK);
        assertTrue(isPackageInstalled(PACKAGE_NAME));
        mTestRunner =
                new RemoteAndroidTestRunner(PACKAGE_NAME, AJUR_RUNNER, getDevice().getIDevice());
        mContext = mock(IInvocationContext.class);
        doReturn(Arrays.asList(getDevice())).when(mContext).getDevices();
        doReturn(Arrays.asList(getBuild())).when(mContext).getBuildInfos();
    }

    /**
     * Test that BatteryStatsListener collects batterystats and records to a file per run.
     */
    @Test
    public void testBatteryStatsListener_perRun() throws Exception {
        mTestRunner.addInstrumentationArg("listener", BATTERYSTATS_COLLECTOR);
        mTestRunner.addInstrumentationArg("batterystats-format", "file:batterystats-log");
        mTestRunner.addInstrumentationArg("batterystats-per-run", "true");
        CollectingTestListener listener = new CollectingTestListener();
        FilePullerDeviceMetricCollector collector = new FilePullerDeviceMetricCollector() {
            @Override
            public void processMetricFile(String key, File metricFile, DeviceMetricData runData) {
                assertTrue(metricFile.getName().contains(BATTERYSTATS_PROTO));

                runData.addMetric(key, Metric.newBuilder().setMeasurements(
                        Measurements.newBuilder().setSingleString(metricFile.getAbsolutePath())
                                .build()));
                try (
                        InputStream is = new BufferedInputStream(new FileInputStream(metricFile))
                ) {
                    BatteryStatsServiceDumpProto bssdp = BatteryStatsServiceDumpProto.parseFrom(is);
                    assertTrue(bssdp.hasBatterystats());
                } catch (IOException e) {
                    throw new RuntimeException(e);
                } finally {
                    assertTrue(metricFile.delete());
                }
            }
            @Override
            public void processMetricDirectory(String key, File metricDirectory,
                    DeviceMetricData runData) {
            }
        };
        OptionSetter optionSetter = new OptionSetter(collector);
        String pattern = String.format("%s_.*", BATTERYSTATS_COLLECTOR);
        optionSetter.setOptionValue("pull-pattern-keys", pattern);
        collector.init(mContext, listener);
        assertTrue(getDevice().runInstrumentationTests(mTestRunner, collector));

        Collection<TestRunResult> results = listener.getRunResults();
        assertEquals(1, results.size());
        TestRunResult result = results.iterator().next();
        assertFalse(result.isRunFailure());
        assertFalse(result.hasFailedTests());

        assertEquals(1, result.getRunMetrics().size());
        String metricFileKey = result.getRunMetrics().keySet().iterator().next();
        assertTrue(metricFileKey.contains(BATTERYSTATS_COLLECTOR));
    }

    /**
     * Test that BatteryStatsListener collects batterystats and records to a file per test.
     */
    @Test
    public void testBatteryStatsListener_perTest() throws Exception {
        mTestRunner.addInstrumentationArg("listener", BATTERYSTATS_COLLECTOR);
        mTestRunner.addInstrumentationArg("batterystats-format", "file:batterystats-log");
        mTestRunner.addInstrumentationArg("batterystats-per-run", "false");
        CollectingTestListener listener = new CollectingTestListener();
        FilePullerDeviceMetricCollector collector = new FilePullerDeviceMetricCollector() {
            @Override
            public void processMetricFile(String key, File metricFile, DeviceMetricData runData) {
                assertTrue(metricFile.getName().contains(BATTERYSTATS_PROTO));
                try (
                        InputStream is = new BufferedInputStream(new FileInputStream(metricFile))
                ) {
                    BatteryStatsServiceDumpProto bssdp = BatteryStatsServiceDumpProto.parseFrom(is);
                    assertTrue(bssdp.hasBatterystats());
                } catch (IOException e) {
                    throw new RuntimeException(e);
                } finally {
                    assertTrue(metricFile.delete());
                }
            }
            @Override
            public void processMetricDirectory(String key, File metricDirectory,
                    DeviceMetricData runData) {
            }
        };
        OptionSetter optionSetter = new OptionSetter(collector);
        String pattern = String.format("%s_.*", BATTERYSTATS_COLLECTOR);
        optionSetter.setOptionValue("pull-pattern-keys", pattern);
        collector.init(mContext, listener);
        assertTrue(getDevice().runInstrumentationTests(mTestRunner, collector));
    }
}
