/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.tradefed.result;

import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import junit.framework.TestCase;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * Unit tests for {@link CollectingTestListener}.
 */
public class CollectingTestListenerTest extends TestCase {

    private static final String METRIC_VALUE = "value";
    private static final String TEST_KEY = "key";
    private static final String METRIC_VALUE2 = "value2";
    private static final String RUN_KEY = "key2";


    private CollectingTestListener mCollectingTestListener;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mCollectingTestListener = new CollectingTestListener();
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo());
        mCollectingTestListener.invocationStarted(context);
    }

    /**
     * Test the listener under a single normal test run.
     */
    public void testSingleRun() {
        final TestDescription test = injectTestRun("run", "testFoo", METRIC_VALUE);
        TestRunResult runResult = mCollectingTestListener.getCurrentRunResults();
        assertTrue(runResult.isRunComplete());
        assertFalse(runResult.isRunFailure());
        assertEquals(1, mCollectingTestListener.getNumTotalTests());
        assertEquals(TestStatus.PASSED,
                runResult.getTestResults().get(test).getStatus());
        assertTrue(runResult.getTestResults().get(test).getStartTime() > 0);
        assertTrue(runResult.getTestResults().get(test).getEndTime() >=
            runResult.getTestResults().get(test).getStartTime());
    }

    /**
     * Test the listener where test run has failed.
     */
    public void testRunFailed() {
        mCollectingTestListener.testRunStarted("foo", 1);
        mCollectingTestListener.testRunFailed("error");
        mCollectingTestListener.testRunEnded(0, new HashMap<String, Metric>());
        TestRunResult runResult = mCollectingTestListener.getCurrentRunResults();
        assertTrue(runResult.isRunComplete());
        assertTrue(runResult.isRunFailure());
        assertEquals("error", runResult.getRunFailureMessage());
    }

    /**
     * Test the listener where test run has failed.
     */
    public void testRunFailed_counting() {
        mCollectingTestListener.testRunStarted("foo1", 1);
        mCollectingTestListener.testRunFailed("error1");
        mCollectingTestListener.testRunEnded(0, new HashMap<String, Metric>());
        mCollectingTestListener.testRunStarted("foo2", 1);
        mCollectingTestListener.testRunEnded(0, new HashMap<String, Metric>());
        mCollectingTestListener.testRunStarted("foo3", 1);
        mCollectingTestListener.testRunFailed("error3");
        mCollectingTestListener.testRunEnded(0, new HashMap<String, Metric>());
        assertEquals(2, mCollectingTestListener.getNumAllFailedTestRuns());
    }

    /**
     * Test the listener when invocation is composed of two test runs.
     */
    public void testTwoRuns() {
        final TestDescription test1 = injectTestRun("run1", "testFoo1", METRIC_VALUE);
        final TestDescription test2 = injectTestRun("run2", "testFoo2", METRIC_VALUE2);
        assertEquals(2, mCollectingTestListener.getNumTotalTests());
        assertEquals(2, mCollectingTestListener.getNumTestsInState(TestStatus.PASSED));
        assertEquals(2, mCollectingTestListener.getRunResults().size());
        Iterator<TestRunResult> runIter = mCollectingTestListener.getRunResults().iterator();
        final TestRunResult runResult1 = runIter.next();
        final TestRunResult runResult2 = runIter.next();

        assertEquals("run1", runResult1.getName());
        assertEquals("run2", runResult2.getName());
        assertEquals(TestStatus.PASSED,
                runResult1.getTestResults().get(test1).getStatus());
        assertEquals(TestStatus.PASSED,
                runResult2.getTestResults().get(test2).getStatus());
        assertEquals(METRIC_VALUE,
                runResult1.getRunMetrics().get(RUN_KEY));
        assertEquals(METRIC_VALUE,
                runResult1.getTestResults().get(test1).getMetrics().get(TEST_KEY));
        assertEquals(METRIC_VALUE2,
                runResult2.getTestResults().get(test2).getMetrics().get(TEST_KEY));
    }

    /**
     * Test the listener when invocation is composed of a re-executed test run.
     */
    public void testReRun() {
        final TestDescription test1 = injectTestRun("run", "testFoo1", METRIC_VALUE);
        final TestDescription test2 = injectTestRun("run", "testFoo2", METRIC_VALUE2);
        assertEquals(2, mCollectingTestListener.getNumTotalTests());
        assertEquals(2, mCollectingTestListener.getNumTestsInState(TestStatus.PASSED));
        assertEquals(1, mCollectingTestListener.getRunResults().size());
        TestRunResult runResult = mCollectingTestListener.getCurrentRunResults();
        assertEquals(2, runResult.getNumTestsInState(TestStatus.PASSED));
        assertTrue(runResult.getCompletedTests().contains(test1));
        assertTrue(runResult.getCompletedTests().contains(test2));
    }

    /**
     * Test the listener when invocation is composed of a re-executed test run, containing the same
     * tests
     */
    public void testReRun_overlap() {
        injectTestRun("run", "testFoo1", METRIC_VALUE);
        injectTestRun("run", "testFoo1", METRIC_VALUE2, true);
        assertEquals(1, mCollectingTestListener.getNumTotalTests());
        assertEquals(0, mCollectingTestListener.getNumTestsInState(TestStatus.PASSED));
        assertEquals(1, mCollectingTestListener.getNumTestsInState(TestStatus.FAILURE));
        assertEquals(1, mCollectingTestListener.getRunResults().size());
        TestRunResult runResult = mCollectingTestListener.getCurrentRunResults();
        assertEquals(0, runResult.getNumTestsInState(TestStatus.PASSED));
        assertEquals(1, runResult.getNumTestsInState(TestStatus.FAILURE));
        assertEquals(1, runResult.getNumTests());
    }

    /**
     * Test run with incomplete tests
     */
    @SuppressWarnings("unchecked")
    public void testSingleRun_incomplete() {
        mCollectingTestListener.testRunStarted("run", 1);
        mCollectingTestListener.testStarted(new TestDescription("FooTest", "incomplete"));
        mCollectingTestListener.testRunEnded(0, new HashMap<String, Metric>());
        assertEquals(1, mCollectingTestListener.getNumTestsInState(TestStatus.INCOMPLETE));
    }

    /**
     * Test aggregating of metrics with long values
     */
    public void testRunEnded_aggregateLongMetrics() {
        mCollectingTestListener.setIsAggregrateMetrics(true);
        injectTestRun("run", "testFoo1", "1");
        injectTestRun("run", "testFoo1", "1");
        assertEquals("2", mCollectingTestListener.getCurrentRunResults().getRunMetrics().get(
                RUN_KEY));
    }

    /**
     * Test aggregating of metrics with double values
     */
    public void testRunEnded_aggregateDoubleMetrics() {
        mCollectingTestListener.setIsAggregrateMetrics(true);
        injectTestRun("run", "testFoo1", "1.1");
        injectTestRun("run", "testFoo1", "1.1");
        assertEquals("2.2", mCollectingTestListener.getCurrentRunResults().getRunMetrics().get(
                RUN_KEY));
    }

    /**
     * Test aggregating of metrics with different data types
     */
    public void testRunEnded_aggregateMixedMetrics() {
        mCollectingTestListener.setIsAggregrateMetrics(true);
        injectTestRun("run", "testFoo1", "1");
        injectTestRun("run", "testFoo1", "1.1");
        mCollectingTestListener.invocationEnded(0);
        assertEquals("2.1", mCollectingTestListener.getCurrentRunResults().getRunMetrics().get(
                RUN_KEY));
    }

    /**
     * Test aggregating of metrics when new metric isn't a number
     */
    public void testRunEnded_aggregateNewStringMetrics() {
        mCollectingTestListener.setIsAggregrateMetrics(true);
        injectTestRun("run", "testFoo1", "1");
        injectTestRun("run", "testFoo1", "bar");
        mCollectingTestListener.invocationEnded(0);
        assertEquals("bar", mCollectingTestListener.getCurrentRunResults().getRunMetrics().get(
                RUN_KEY));
    }

    /**
     * Test aggregating of metrics when existing metric isn't a number
     */
    public void testRunEnded_aggregateExistingStringMetrics() {
        mCollectingTestListener.setIsAggregrateMetrics(true);
        injectTestRun("run", "testFoo1", "bar");
        injectTestRun("run", "testFoo1", "1");
        mCollectingTestListener.invocationEnded(0);
        assertEquals("1", mCollectingTestListener.getCurrentRunResults().getRunMetrics().get(
                RUN_KEY));
    }

    public void testGetNumTestsInState() {
        injectTestRun("run", "testFoo1", "1");
        injectTestRun("run", "testFoo2", "1");
        int testsPassed = mCollectingTestListener.getNumTestsInState(TestStatus.PASSED);
        assertEquals(2, testsPassed);
        injectTestRun("run", "testFoo3", "1");
        testsPassed = mCollectingTestListener.getNumTestsInState(TestStatus.PASSED);
        assertEquals(3, testsPassed);
    }

    public void testGetNumTotalTests() {
        injectTestRun("run", "testFoo1", "1");
        injectTestRun("run", "testFoo2", "1");
        int total = mCollectingTestListener.getNumTotalTests();
        assertEquals(2, total);
        injectTestRun("run", "testFoo3", "1", true);
        total = mCollectingTestListener.getNumTotalTests();
        assertEquals(3, total);
    }

    /**
     * Injects a single test run with 1 passed test into the {@link CollectingTestListener} under
     * test
     *
     * @return the {@link TestDescription} of added test
     */
    private TestDescription injectTestRun(String runName, String testName, String metricValue) {
        return injectTestRun(runName, testName, metricValue, false);
    }

    /**
     * Injects a single test run with 1 test into the {@link CollectingTestListener} under test.
     *
     * @return the {@link TestDescription} of added test
     */
    private TestDescription injectTestRun(
            String runName, String testName, String metricValue, boolean failtest) {
        Map<String, String> runMetrics = new HashMap<String, String>(1);
        runMetrics.put(RUN_KEY, metricValue);
        Map<String, String> testMetrics = new HashMap<String, String>(1);
        testMetrics.put(TEST_KEY, metricValue);

        mCollectingTestListener.testRunStarted(runName, 1);
        final TestDescription test = new TestDescription("FooTest", testName);
        mCollectingTestListener.testStarted(test);
        if (failtest) {
            mCollectingTestListener.testFailed(test, "trace");
        }
        mCollectingTestListener.testEnded(test, TfMetricProtoUtil.upgradeConvert(testMetrics));
        mCollectingTestListener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(runMetrics));
        return test;
    }
}
