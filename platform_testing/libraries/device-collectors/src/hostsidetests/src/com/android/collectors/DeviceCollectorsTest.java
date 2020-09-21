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
package com.android.collectors;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;

import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.testtype.AndroidJUnitTest;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.Collection;
import java.util.Map.Entry;

/**
 * Host side tests for the core device collectors, this ensure that we are able to use the
 * collectors in a similar way as the infra.
 *
 * Command:
 * mm CollectorHostsideLibTest CollectorDeviceLibTest -j16
 * tradefed.sh run commandAndExit template/local_min --template:map test=CollectorHostsideLibTest
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class DeviceCollectorsTest extends BaseHostJUnit4Test {
    private static final String TEST_APK = "CollectorDeviceLibTest.apk";
    private static final String PACKAGE_NAME = "android.device.collectors";
    private static final String AJUR_RUNNER = "android.support.test.runner.AndroidJUnitRunner";

    private static final String STUB_BASE_COLLECTOR =
            "android.device.collectors.StubTestMetricListener";
    private static final String SCHEDULED_COLLECTOR =
            "android.device.collectors.StubScheduledRunMetricListener";

    private RemoteAndroidTestRunner mTestRunner;
    private IInvocationContext mContext;

    @Before
    public void setUp() throws Exception {
        installPackage(TEST_APK);
        assertTrue(isPackageInstalled(PACKAGE_NAME));
        mTestRunner =
                new RemoteAndroidTestRunner(PACKAGE_NAME, AJUR_RUNNER, getDevice().getIDevice());
        // Set the new runListener order to ensure test cases can show their metrics.
        mTestRunner.addInstrumentationArg(AndroidJUnitTest.NEW_RUN_LISTENER_ORDER_KEY, "true");
        mContext = mock(IInvocationContext.class);
        doReturn(Arrays.asList(getDevice())).when(mContext).getDevices();
        doReturn(Arrays.asList(getBuild())).when(mContext).getBuildInfos();
    }

    /**
     * Test that our base metric listener can output metrics.
     */
    @Test
    public void testBaseListenerRuns() throws Exception {
        mTestRunner.addInstrumentationArg("listener", STUB_BASE_COLLECTOR);
        CollectingTestListener listener = new CollectingTestListener();
        assertTrue(getDevice().runInstrumentationTests(mTestRunner, listener));
        Collection<TestRunResult> results = listener.getRunResults();
        assertEquals(1, results.size());
        TestRunResult result = results.iterator().next();
        assertFalse(result.isRunFailure());
        assertFalse(result.hasFailedTests());
        // Ensure the listener added a metric at test run start and end.
        assertTrue(result.getRunMetrics().containsKey("run_start"));
        assertTrue(result.getRunMetrics().containsKey("run_end"));
        // Check that each test case has results with the metrics associated.
        for (TestResult res : result.getTestResults().values()) {
            assertTrue(res.getMetrics().containsKey("test_start"));
            assertTrue(res.getMetrics().containsKey("test_end"));
        }
    }

    /**
     * Test that our base metric listener can filter metrics to run only against some groups tagged
     * with an annotation. All annotation of BaseMetricListenerInstrumentedTest are annotated with
     * the group 'testGroup'.
     */
    @Test
    public void testBaseListenerRuns_withExcludeFilters() throws Exception {
        mTestRunner.addInstrumentationArg("listener", STUB_BASE_COLLECTOR);
        mTestRunner.addInstrumentationArg("exclude-filter-group", "testGroup");
        mTestRunner.setClassName("android.device.collectors.BaseMetricListenerInstrumentedTest");
        CollectingTestListener listener = new CollectingTestListener();
        assertTrue(getDevice().runInstrumentationTests(mTestRunner, listener));
        Collection<TestRunResult> results = listener.getRunResults();
        assertEquals(1, results.size());
        TestRunResult result = results.iterator().next();
        assertFalse(result.isRunFailure());
        assertFalse(result.hasFailedTests());
        // Ensure the listener added a metric at test run start and end.
        assertTrue(result.getRunMetrics().containsKey("run_start"));
        assertTrue(result.getRunMetrics().containsKey("run_end"));
        // We did run some tests
        assertTrue(!result.getTestResults().isEmpty());
        // After filtering none of the test case should contain any of the metrics since it was
        // filtered.
        for (TestResult testCaseResult : result.getTestResults().values()) {
            assertFalse(testCaseResult.getMetrics().containsKey("test_start"));
            assertFalse(testCaseResult.getMetrics().containsKey("test_fail"));
            assertFalse(testCaseResult.getMetrics().containsKey("test_end"));
        }
    }

    /**
     * Test that if an include and exclude filters are provided, the exclude filters has priority.
     */
    @Test
    public void testBaseListenerRuns_withIncludeAndExcludeFilters() throws Exception {
        mTestRunner.addInstrumentationArg("listener", STUB_BASE_COLLECTOR);
        mTestRunner.addInstrumentationArg("include-filter-group", "testGroup");
        mTestRunner.addInstrumentationArg("exclude-filter-group", "testGroup");
        mTestRunner.setClassName("android.device.collectors.BaseMetricListenerInstrumentedTest");
        CollectingTestListener listener = new CollectingTestListener();
        assertTrue(getDevice().runInstrumentationTests(mTestRunner, listener));
        Collection<TestRunResult> results = listener.getRunResults();
        assertEquals(1, results.size());
        TestRunResult result = results.iterator().next();
        assertFalse(result.isRunFailure());
        assertFalse(result.hasFailedTests());
        // Ensure the listener added a metric at test run start and end.
        assertTrue(result.getRunMetrics().containsKey("run_start"));
        assertTrue(result.getRunMetrics().containsKey("run_end"));
        // We did run some tests
        assertTrue(!result.getTestResults().isEmpty());
        // After filtering none of the test case should contain any of the metrics since it was
        // filtered. Exclusion has priority over inclusion.
        for (TestResult testCaseResult : result.getTestResults().values()) {
            assertFalse(testCaseResult.getMetrics().containsKey("test_start"));
            assertFalse(testCaseResult.getMetrics().containsKey("test_fail"));
            assertFalse(testCaseResult.getMetrics().containsKey("test_end"));
        }
    }

    /**
     * Test that if an include filter is provided, only method part of the included group will
     * run the collection.
     */
    @Test
    public void testBaseListenerRuns_withIncludeFilters() throws Exception {
        mTestRunner.addInstrumentationArg("listener", STUB_BASE_COLLECTOR);
        mTestRunner.addInstrumentationArg("include-filter-group", "testGroup1");
        mTestRunner.setClassName("android.device.collectors.BaseMetricListenerInstrumentedTest");
        CollectingTestListener listener = new CollectingTestListener();
        assertTrue(getDevice().runInstrumentationTests(mTestRunner, listener));
        Collection<TestRunResult> results = listener.getRunResults();
        assertEquals(1, results.size());
        TestRunResult result = results.iterator().next();
        assertFalse(result.isRunFailure());
        assertFalse(result.hasFailedTests());
        // Ensure the listener added a metric at test run start and end.
        assertTrue(result.getRunMetrics().containsKey("run_start"));
        assertTrue(result.getRunMetrics().containsKey("run_end"));
        // We did run some tests
        assertTrue(!result.getTestResults().isEmpty());
        // After filtering none of the test case should contain any of the metrics since it was
        // filtered. Exclusion has priority over inclusion.
        for (Entry<TestDescription, TestResult> testResult : result.getTestResults().entrySet()) {
            // testReportMetrics method is the only one annotated with 'testGroup1' it should be the
            // only one collecting metrics.
            if ("testReportMetrics".equals(testResult.getKey().getTestName())) {
                assertTrue(testResult.getValue().getMetrics().containsKey("test_start"));
                assertTrue(testResult.getValue().getMetrics().containsKey("test_end"));
            } else {
                assertFalse(testResult.getValue().getMetrics().containsKey("test_start"));
                assertFalse(testResult.getValue().getMetrics().containsKey("test_fail"));
                assertFalse(testResult.getValue().getMetrics().containsKey("test_end"));
            }
        }
    }

    /**
     * Test that our base scheduled listener can output metrics periodically.
     */
    @Test
    public void testScheduledListenerRuns() throws Exception {
        mTestRunner.addInstrumentationArg("listener", SCHEDULED_COLLECTOR);
        mTestRunner.addInstrumentationArg("interval", "100");
        CollectingTestListener listener = new CollectingTestListener();
        assertTrue(getDevice().runInstrumentationTests(mTestRunner, listener));
        Collection<TestRunResult> results = listener.getRunResults();
        assertEquals(1, results.size());
        TestRunResult result = results.iterator().next();
        assertFalse(result.isRunFailure());
        assertFalse(result.hasFailedTests());
        // There is time during the test to output at least a handful of periodic metrics.
        assertTrue(result.getRunMetrics().containsKey("collect0"));
        assertTrue(result.getRunMetrics().containsKey("collect1"));
        assertTrue(result.getRunMetrics().containsKey("collect2"));
    }

    /**
     * Test that our base scheduled listener can use its default period to run when the interval
     * given is not valid.
     */
    @Test
    public void testScheduledListenerRuns_defaultValue() throws Exception {
        mTestRunner.addInstrumentationArg("listener", SCHEDULED_COLLECTOR);
        // Invalid interval will results in the default period to be used.
        mTestRunner.addInstrumentationArg("interval", "-100");
        CollectingTestListener listener = new CollectingTestListener();
        assertTrue(getDevice().runInstrumentationTests(mTestRunner, listener));
        Collection<TestRunResult> results = listener.getRunResults();
        assertEquals(1, results.size());
        TestRunResult result = results.iterator().next();
        assertFalse(result.isRunFailure());
        assertFalse(result.hasFailedTests());
        // The default interval value is one minute so it will only have time to run once.
        assertEquals(1, result.getRunMetrics().size());
        assertTrue(result.getRunMetrics().containsKey("collect0"));
    }
}
