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
package com.android.tradefed.testtype.suite;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.metric.IMetricCollector;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.ITestFilterReceiver;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.FileSystemLogSaver;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.result.TestRunResult;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mockito;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** Unit tests for {@link com.android.tradefed.testtype.suite.GranularRetriableTestWrapper}. */
@RunWith(JUnit4.class)
public class GranularRetriableTestWrapperTest {

    private class BasicFakeTest implements IRemoteTest {

        protected ArrayList<TestDescription> mTestCases;
        private Map<TestDescription, Boolean> mShouldFail;

        public BasicFakeTest() {
            mTestCases = new ArrayList<>();
            TestDescription defaultTestCase = new TestDescription("ClassFoo", "TestFoo");
            mTestCases.add(defaultTestCase);
            mShouldFail = new HashMap<TestDescription, Boolean>();
            mShouldFail.put(defaultTestCase, false);
        }

        public BasicFakeTest(ArrayList<TestDescription> testCases) {
            mTestCases = testCases;
            mShouldFail = new HashMap<TestDescription, Boolean>();
            for (TestDescription testCase : testCases) {
                mShouldFail.put(testCase, false);
            }
        }

        public void setFailedTestCase(TestDescription testCase) {
            mShouldFail.put(testCase, true);
        }

        @Override
        public void run(ITestInvocationListener listener) throws DeviceUnresponsiveException {
            listener.testRunStarted("test run", mTestCases.size());
            for (TestDescription td : mTestCases) {
                listener.testStarted(td);
                if (mShouldFail.get(td)) {
                    listener.testFailed(td, String.format("Fake failure %s", td.toString()));
                }
                listener.testEnded(td, new HashMap<String, Metric>());
            }
            listener.testRunEnded(0, new HashMap<String, Metric>());
        }
    }

    private class FakeTest extends BasicFakeTest implements ITestFilterReceiver {

        public FakeTest(ArrayList<TestDescription> testCases) {
            super(testCases);
        }

        public FakeTest() {
            super();
        }

        @Override
        public void addIncludeFilter(String filter) {
            String[] descriptionStr = filter.split("#");
            mTestCases = new ArrayList<>();
            mTestCases.add(new TestDescription(descriptionStr[0], descriptionStr[1]));
        }

        @Override
        public void addAllIncludeFilters(Set<String> filters) {}

        @Override
        public void addExcludeFilter(String filters) {}

        @Override
        public void addAllExcludeFilters(Set<String> filters) {}
    }

    public GranularRetriableTestWrapper createGranularTestWrapper(
            IRemoteTest test, int maxRunCount) {
        GranularRetriableTestWrapper granularTestWrapper =
                new GranularRetriableTestWrapper(test, null, null, maxRunCount);
        granularTestWrapper.setModuleId("test module");
        granularTestWrapper.setMarkTestsSkipped(false);
        granularTestWrapper.setMetricCollectors(new ArrayList<IMetricCollector>());
        // Setup InvocationContext.
        granularTestWrapper.setInvocationContext(new InvocationContext());
        // Setup logsaver.
        granularTestWrapper.setLogSaver(new FileSystemLogSaver());
        IConfiguration mockModuleConfiguration = Mockito.mock(IConfiguration.class);
        granularTestWrapper.setModuleConfig(mockModuleConfiguration);
        return granularTestWrapper;
    }

    /**
     * Test the intra module "run" triggers IRemoteTest run method with a dedicated ModuleListener.
     */
    @Test
    public void testIntraModuleRun_pass() throws Exception {
        GranularRetriableTestWrapper granularTestWrapper =
                createGranularTestWrapper(new FakeTest(), 1);
        assertEquals(0, granularTestWrapper.getTestRunResultCollector().size());
        granularTestWrapper.intraModuleRun();
        assertEquals(1, granularTestWrapper.getTestRunResultCollector().size());
        Set<TestDescription> completedTests =
                granularTestWrapper.getFinalTestRunResult().getCompletedTests();
        assertEquals(1, completedTests.size());
        assertEquals("ClassFoo#TestFoo", completedTests.toArray()[0].toString());
    }

    /**
     * Test that the intra module "run" method catches DeviceNotAvailableException and raises it
     * after record the tests.
     */
    @Test(expected = DeviceNotAvailableException.class)
    public void testIntraModuleRun_catchDeviceNotAvailableException() throws Exception {
        IRemoteTest mockTest = Mockito.mock(IRemoteTest.class);
        Mockito.doThrow(new DeviceNotAvailableException("fake message", "serial"))
                .when(mockTest)
                .run(Mockito.any(ITestInvocationListener.class));
        GranularRetriableTestWrapper granularTestWrapper = createGranularTestWrapper(mockTest, 1);
        // Verify.
        granularTestWrapper.intraModuleRun();
    }

    /**
     * Test that the intra module "run" method catches DeviceUnresponsiveException and doesn't raise
     * it again.
     */
    @Test
    public void testIntraModuleRun_catchDeviceUnresponsiveException() throws Exception {
        FakeTest test =
                new FakeTest() {
                    @Override
                    public void run(ITestInvocationListener listener)
                            throws DeviceUnresponsiveException {
                        listener.testRunStarted("test run", 1);
                        throw new DeviceUnresponsiveException("fake message", "serial");
                    }
                };
        GranularRetriableTestWrapper granularTestWrapper = createGranularTestWrapper(test, 1);
        granularTestWrapper.intraModuleRun();
        TestRunResult finalResult = granularTestWrapper.getTestRunResultCollector().get(0);
        assertTrue(finalResult.isRunFailure());
    }

    /**
     * Test that the "run" method has built-in retry logic and each run has an individual
     * ModuleListener and TestRunResult.
     */
    @Test
    public void testRun_withMultipleRun() throws Exception {
        ArrayList<TestDescription> testCases = new ArrayList<>();
        TestDescription fakeTestCase = new TestDescription("Class", "Test");
        testCases.add(fakeTestCase);
        FakeTest test = new FakeTest(testCases);
        test.setFailedTestCase(fakeTestCase);

        int maxRunCount = 5;
        GranularRetriableTestWrapper granularTestWrapper =
                createGranularTestWrapper(test, maxRunCount);
        granularTestWrapper.run(new CollectingTestListener());
        // Verify the test has run 5 times.
        assertEquals(maxRunCount, granularTestWrapper.getTestRunResultCollector().size());
        Map<TestDescription, TestResult> testResults =
                granularTestWrapper.getFinalTestRunResult().getTestResults();
        testResults.containsKey(fakeTestCase);
        // Verify the final TestRunResult is a merged value of every retried TestRunResults.
        assertEquals(TestStatus.FAILURE, testResults.get(fakeTestCase).getStatus());
        String stacktrace =
                String.join("\n", Collections.nCopies(maxRunCount, "Fake failure Class#Test"));
        assertEquals(stacktrace, testResults.get(fakeTestCase).getStackTrace());
    }

    /**
     * Test that the "run" method only retry failed test cases, and merge the multiple test results
     * to a single TestRunResult.
     */
    @Test
    public void testRun_retryOnFailedTestCaseOnly() throws Exception {
        ArrayList<TestDescription> testCases = new ArrayList<>();
        TestDescription fakeTestCase1 = new TestDescription("Class1", "Test1");
        TestDescription fakeTestCase2 = new TestDescription("Class2", "Test2");
        testCases.add(fakeTestCase1);
        testCases.add(fakeTestCase2);
        FakeTest test = new FakeTest(testCases);
        // Only the first testcase is failed and retried.
        test.setFailedTestCase(fakeTestCase1);
        // Run each testcases (if failed) max to 3 times.
        int maxRunCount = 3;
        GranularRetriableTestWrapper granularTestWrapper =
                createGranularTestWrapper(test, maxRunCount);
        granularTestWrapper.run(new CollectingTestListener());

        TestRunResult finalResult = granularTestWrapper.getFinalTestRunResult();
        assertEquals(
                TestStatus.FAILURE, finalResult.getTestResults().get(fakeTestCase1).getStatus());
        assertEquals(
                TestStatus.PASSED, finalResult.getTestResults().get(fakeTestCase2).getStatus());
        // Verify the test has 3 TestRunResults, indicating it runs 3 times. And only failed test
        // case is in the retried TestRunResult.
        assertEquals(maxRunCount, granularTestWrapper.getTestRunResultCollector().size());
        List<TestRunResult> resultCollector = granularTestWrapper.getTestRunResultCollector();
        TestRunResult latestRunResult = resultCollector.get(resultCollector.size() - 1);
        assertEquals(1, latestRunResult.getNumTests());
        latestRunResult.getTestResults().containsKey(fakeTestCase1);
    }

    /**
     * Test that if IRemoteTest doesn't implement ITestFilterReceiver, the "run" method will retry
     * all test cases.
     */
    @Test
    public void testRun_retryAllTestCasesIfNotSupportTestFilterReceiver() throws Exception {
        ArrayList<TestDescription> testCases = new ArrayList<>();
        TestDescription fakeTestCase1 = new TestDescription("Class1", "Test1");
        TestDescription fakeTestCase2 = new TestDescription("Class2", "Test2");
        testCases.add(fakeTestCase1);
        testCases.add(fakeTestCase2);
        BasicFakeTest test = new BasicFakeTest(testCases);
        // Only the first testcase is failed.
        test.setFailedTestCase(fakeTestCase1);
        // Run each testcases (if has failure) max to 3 times.
        int maxRunCount = 3;
        GranularRetriableTestWrapper granularTestWrapper =
                createGranularTestWrapper(test, maxRunCount);
        granularTestWrapper.run(new CollectingTestListener());
        // Verify the test has 3 TestRunResults, indicating it runs 3 times. And all test cases
        // are retried.
        assertEquals(maxRunCount, granularTestWrapper.getTestRunResultCollector().size());
        List<TestRunResult> resultCollector = granularTestWrapper.getTestRunResultCollector();
        for (TestRunResult runResult : resultCollector) {
            assertEquals(2, runResult.getNumTests());
            assertEquals(
                    TestStatus.FAILURE, runResult.getTestResults().get(fakeTestCase1).getStatus());
            assertEquals(
                    TestStatus.PASSED, runResult.getTestResults().get(fakeTestCase2).getStatus());
        }
    }
}
