/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.framework.tests;

import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import com.google.common.collect.ImmutableMap;

import org.junit.Assert;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Test that measures the average latency of foreground and background
 * operations in various scenarios.
 */
public class FrameworkPerfTest implements IRemoteTest, IDeviceTest {

    private static final String TEST_PACKAGE_NAME = "com.android.frameworkperffg";
    private static final String TEST_RUNNER_NAME = "android.test.InstrumentationTestRunner";
    private static final int PERF_TIMEOUT = 30 * 60 * 1000; //30 minutes timeout
    private static final int PRE_TEST_SLEEP_MS = 30 *1000; //30s sleep prior to test start

    private static final String LAYOUT = "frameworkfg_perf_layout";
    private static final String GC = "frameworkfg_perf_gc";
    private static final String XML = "frameworkfg_perf_xml";
    private static final String BITMAP = "frameworkfg_perf_bitmap";
    private static final String FILE = "frameworkfg_perf_file";
    private static final String OTHER = "frameworkfg_perf_other";
    private static final String MAP = "frameworkfg_perf_map";
    private static final ImmutableMap<String, String> TEST_TAG_MAP =
            new ImmutableMap.Builder<String, String>()
            .put("ReadFile",FILE)
            .put("CreateWriteFile",FILE)
            .put("CreateWriteSyncFile",FILE)
            .put("WriteFile",FILE)
            .put("CreateFile",FILE)

            .put("CreateRecycleBitmap",BITMAP)
            .put("LoadLargeScaledBitmap",BITMAP)
            .put("LoadSmallScaledBitmap",BITMAP)
            .put("LoadRecycleSmallBitmap",BITMAP)
            .put("LoadLargeBitmap",BITMAP)
            .put("CreateBitmap",BITMAP)
            .put("LoadSmallBitmap",BITMAP)

            .put("LayoutInflaterButton",LAYOUT)
            .put("LayoutInflaterImageButton",LAYOUT)
            .put("LayoutInflaterLarge",LAYOUT)
            .put("LayoutInflaterView",LAYOUT)
            .put("LayoutInflater",LAYOUT)

            .put("Gc",GC)
            .put("PaintGc",GC)
            .put("ObjectGc",GC)
            .put("FinalizingGc",GC)

            .put("OpenXmlRes",XML)
            .put("ParseXmlRes",XML)
            .put("ReadXmlAttrs",XML)
            .put("ParseLargeXmlRes",XML)

            .put("Sched",OTHER)
            .put("CPU",OTHER)
            .put("MethodCall",OTHER)
            .put("Ipc",OTHER)

            .put("GrowLargeArrayMap",MAP)
            .put("GrowLargeHashMap",MAP)
            .put("LookupSmallHashMap",MAP)
            .put("LookupSmallArrayMap",MAP)
            .put("LookupTinyHashMap",MAP)
            .put("GrowTinyHashMap",MAP)
            .put("LookupLargeHashMap",MAP)
            .put("LookupTinyArrayMap",MAP)
            .put("LookupLargeArrayMap",MAP)
            .put("GrowTinyArrayMap",MAP)
            .put("GrowSmallHashMap",MAP)
            .put("GrowSmallArrayMap",MAP)
            .build();

    private ITestDevice mTestDevice = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);

        getDevice().reboot();
        getRunUtil().sleep(PRE_TEST_SLEEP_MS);
        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(TEST_PACKAGE_NAME,
                TEST_RUNNER_NAME, mTestDevice.getIDevice());
        runner.setMaxTimeToOutputResponse(PERF_TIMEOUT, TimeUnit.MILLISECONDS);

        CollectingTestListener collectingListener = new CollectingTestListener();
        Assert.assertTrue(mTestDevice.runInstrumentationTests(runner, collectingListener));

        Collection<TestResult> testResultsCollection =
                collectingListener.getCurrentRunResults().getTestResults().values();

        List<TestResult> testResults =
                new ArrayList<TestResult>(testResultsCollection);

        if (!testResults.isEmpty()) {
            Map<String, String> testMetrics = testResults.get(0).getMetrics();
            if (testMetrics != null) {
                reportMetrics(listener, testMetrics);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mTestDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mTestDevice = device;
    }

    /**
     * Report run metrics by creating an empty test run to stick them in.
     * @param listener The {@link ITestInvocationListener} of test results
     * @param metrics The {@link Map} that contains metrics for the given test
     */
    private void reportMetrics(ITestInvocationListener listener, Map<String, String> metrics) {
        // Parse out only averages
        Map<String, Map<String, String>> allMetrics = new HashMap<String, Map<String, String>>();
        for (String key : metrics.keySet()) {
            String testLabel = TEST_TAG_MAP.get(key);
            if (testLabel == null) {
                testLabel = OTHER;
            }
            if (!allMetrics.containsKey(testLabel)) {
                allMetrics.put(testLabel, new HashMap<String, String>());
            }
            allMetrics.get(testLabel).put(key, metrics.get(key));
        }

        for (String section : allMetrics.keySet()) {
            Map<String, String> sectionMetrics = allMetrics.get(section);
            if (sectionMetrics != null && !sectionMetrics.isEmpty()) {
                for (String section2 : sectionMetrics.keySet()) {
                    CLog.i("%s ::'%s' : %s", section, section2, sectionMetrics.get(section2));
                }
                listener.testRunStarted(section, 0);
                listener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(sectionMetrics));
            }
        }
    }

    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }
}
