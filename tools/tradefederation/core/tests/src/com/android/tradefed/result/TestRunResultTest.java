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
package com.android.tradefed.result;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Unit tests for {@link TestRunResult} */
@RunWith(JUnit4.class)
public class TestRunResultTest {

    /** Check basic storing of results when events are coming in. */
    @Test
    public void testGetNumTestsInState() {
        TestDescription test = new TestDescription("FooTest", "testBar");
        TestRunResult result = new TestRunResult();
        assertEquals(0, result.getNumTestsInState(TestStatus.PASSED));
        result.testStarted(test);
        assertEquals(0, result.getNumTestsInState(TestStatus.PASSED));
        assertEquals(1, result.getNumTestsInState(TestStatus.INCOMPLETE));
        result.testEnded(test, new HashMap<String, Metric>());
        assertEquals(1, result.getNumTestsInState(TestStatus.PASSED));
        assertEquals(0, result.getNumTestsInState(TestStatus.INCOMPLETE));
        // Ensure our test was recorded.
        assertNotNull(result.getTestResults().get(test));
    }

    /** Check basic storing of results when events are coming in and there is a test failure. */
    @Test
    public void testGetNumTestsInState_failed() {
        TestDescription test = new TestDescription("FooTest", "testBar");
        TestRunResult result = new TestRunResult();
        assertEquals(0, result.getNumTestsInState(TestStatus.PASSED));
        result.testStarted(test);
        assertEquals(0, result.getNumTestsInState(TestStatus.PASSED));
        assertEquals(1, result.getNumTestsInState(TestStatus.INCOMPLETE));
        result.testFailed(test, "I failed!");
        // Test status immediately switch to failure.
        assertEquals(0, result.getNumTestsInState(TestStatus.PASSED));
        assertEquals(1, result.getNumTestsInState(TestStatus.FAILURE));
        assertEquals(0, result.getNumTestsInState(TestStatus.INCOMPLETE));
        result.testEnded(test, new HashMap<String, Metric>());
        assertEquals(0, result.getNumTestsInState(TestStatus.PASSED));
        assertEquals(1, result.getNumTestsInState(TestStatus.FAILURE));
        assertEquals(0, result.getNumTestsInState(TestStatus.INCOMPLETE));
        // Ensure our test was recorded.
        assertNotNull(result.getTestResults().get(test));
    }

    /** Test that we are able to specify directly the start and end time of a test. */
    @Test
    public void testSpecifyElapsedTime() {
        TestDescription test = new TestDescription("FooTest", "testBar");
        TestRunResult result = new TestRunResult();
        result.testStarted(test, 5L);
        assertEquals(5L, result.getTestResults().get(test).getStartTime());
        result.testEnded(test, 25L, new HashMap<String, Metric>());
        assertEquals(25L, result.getTestResults().get(test).getEndTime());
    }

    /**
     * Test that when a same {@link TestRunResult} is re-run (new testRunStart/End) we keep the
     * failure state, since we do not want to override it.
     */
    @Test
    public void testMultiRun() {
        TestRunResult result = new TestRunResult();
        // Initially not failed and not completed
        assertFalse(result.isRunFailure());
        assertFalse(result.isRunComplete());
        result.testRunStarted("run", 0);
        result.testRunFailed("failure");
        result.testRunEnded(0, new HashMap<String, Metric>());
        assertTrue(result.isRunFailure());
        assertEquals("failure", result.getRunFailureMessage());
        assertTrue(result.isRunComplete());
        // If a re-run is triggered.
        result.testRunStarted("run", 0);
        // Not complete anymore, but still failed
        assertFalse(result.isRunComplete());
        assertTrue(result.isRunFailure());
        result.testRunEnded(0, new HashMap<String, Metric>());
        assertTrue(result.isRunFailure());
        assertEquals("failure", result.getRunFailureMessage());
        assertTrue(result.isRunComplete());
    }

    /**
     * Test that when logging of files occurs during a test case in progress, files are associated
     * to the test case results.
     */
    @Test
    public void testLogSavedFile_testCases() {
        TestDescription test = new TestDescription("FooTest", "testBar");
        TestRunResult result = new TestRunResult();
        result.testStarted(test);
        // Check that there is no logged file at first.
        TestResult testRes = result.getTestResults().get(test);
        assertEquals(0, testRes.getLoggedFiles().size());
        result.testLogSaved("test", new LogFile("path", "url", LogDataType.TEXT));
        assertEquals(1, testRes.getLoggedFiles().size());
        result.testFailed(test, "failure");
        result.testLogSaved("afterFailure", new LogFile("path", "url", LogDataType.TEXT));
        assertEquals(2, testRes.getLoggedFiles().size());
        result.testEnded(test, new HashMap<String, Metric>());
        // Once done, the results are still available.
        assertEquals(2, testRes.getLoggedFiles().size());
    }

    /**
     * Ensure that files logged from outside a test case (testStart/testEnd) are tracked by the run
     * itself.
     */
    @Test
    public void testLogSavedFile_runLogs() {
        TestRunResult result = new TestRunResult();
        result.testRunStarted("run", 1);
        result.testLogSaved("outsideTestCase", new LogFile("path", "url", LogDataType.TEXT));

        TestDescription test = new TestDescription("FooTest", "testBar");
        result.testStarted(test);
        // Check that there is no logged file at first.
        TestResult testRes = result.getTestResults().get(test);
        assertEquals(0, testRes.getLoggedFiles().size());

        result.testLogSaved("insideTestCase", new LogFile("path", "url", LogDataType.TEXT));
        result.testLogSaved("insideTestCase2", new LogFile("path", "url", LogDataType.TEXT));
        result.testEnded(test, new HashMap<String, Metric>());
        result.testLogSaved("outsideTestCase2", new LogFile("path", "url", LogDataType.TEXT));
        // Once done, the results are still available and the test cases has its two files.
        assertEquals(2, testRes.getLoggedFiles().size());
        assertTrue(testRes.getLoggedFiles().containsKey("insideTestCase"));
        assertTrue(testRes.getLoggedFiles().containsKey("insideTestCase2"));
        // the Run has its file too
        assertEquals(2, result.getRunLoggedFiles().size());
        assertTrue(result.getRunLoggedFiles().containsKey("outsideTestCase"));
        assertTrue(result.getRunLoggedFiles().containsKey("outsideTestCase2"));
    }

    /** Ensure that the merging logic among multiple testRunResults for the same test is correct. */
    @Test
    public void testMergeRetriedRunResults_fullMergeLogicCheck() {
        /*
         *|-----------------------------------------------------------------|
         *|                |  test1     |   test2           |   test3       |
         *| testcase       |  Foo#foo   |  Bar#bar          | Baz#baz       |
         *| -------------- -------------------------------------------------|
         *| 1st Run        |            |                   |               |
         *|    status      |   PASSED   |     FAILED        |   FAILED      |
         *|    stack trace |    null    |    "flaky 1"      |"bad_code1"    |
         *| -------------- -------------------------------------------------|
         *| 2nd Run        |            |                   |               |
         *|    status      |  (skipped) |     FAILED        |   FAILED      |
         *|    stack trace |    null    |    "flaky 2"      |"bad_code2"    |
         *| -------------- -------------------------------------------------|
         *| 3rd Run        |            |                   |               |
         *|    status      |  (skipped) |      PASSED       |   FAILED      |
         *|    stack trace |    null    |       null        |"bad_code2"    |
         *| -------------- -------------------------------------------------|
         *| Expected       |            |                   |               |
         *|    status      |   PASSED   |      PASSED       |   FAILED      |
         *|    stack trace |    null    |"flaky 1\nflaky 2" |"bad_code1\n   |
         *|                |            |                   |+ "bad_code2\n"|
         *|                |            |                   |+ "bad_code3"  |
         *|-----------------------------------------------------------------|
         */
        // Test Setup.
        TestDescription testcase1 = new TestDescription("Foo", "foo");
        TestDescription testcase2 = new TestDescription("Bar", "bar");
        TestDescription testcase3 = new TestDescription("Baz", "baz");
        TestRunResult result1 = new TestRunResult();
        TestRunResult result2 = new TestRunResult();
        TestRunResult result3 = new TestRunResult();
        // Mimic the ModuleDefinition run.
        result1.testRunStarted("fake run", 3);
        result1.testStarted(testcase1);
        result1.testEnded(testcase1, new HashMap<String, Metric>());
        result1.testStarted(testcase2);
        result1.testFailed(testcase2, "flaky 1");
        result1.testEnded(testcase2, new HashMap<String, Metric>());
        result1.testStarted(testcase3);
        result1.testFailed(testcase3, "bad_code1");
        result1.testRunEnded(0, new HashMap<String, Metric>());

        result2.testRunStarted("fake run", 2);
        result2.testStarted(testcase2);
        result2.testFailed(testcase2, "flaky 2");
        result2.testEnded(testcase2, new HashMap<String, Metric>());
        result2.testStarted(testcase3);
        result2.testFailed(testcase3, "bad_code2");
        result2.testRunEnded(0, new HashMap<String, Metric>());

        result3.testRunStarted("fake run", 2);
        result3.testStarted(testcase2);
        result3.testEnded(testcase2, new HashMap<String, Metric>());
        result3.testStarted(testcase3);
        result3.testFailed(testcase3, "bad_code3");
        result3.testRunEnded(0, new HashMap<String, Metric>());

        List<TestRunResult> testResultList =
                new ArrayList<>(Arrays.asList(result1, result2, result3));
        TestRunResult result = TestRunResult.merge(testResultList);

        // Verify result.
        assertEquals("fake run", result.getName());
        Map<TestDescription, TestResult> testResult = result.getTestResults();
        assertTrue(testResult.containsKey(testcase1));
        assertTrue(testResult.containsKey(testcase2));
        assertTrue(testResult.containsKey(testcase3));

        TestResult test1Result = testResult.get(testcase1);
        TestResult test2Result = testResult.get(testcase2);
        TestResult test3Result = testResult.get(testcase3);

        assertEquals(TestStatus.PASSED, test1Result.getStatus());
        assertEquals(TestStatus.PASSED, test2Result.getStatus());
        assertEquals(TestStatus.FAILURE, test3Result.getStatus());

        assertEquals(null, test1Result.getStackTrace());
        assertEquals("flaky 1\nflaky 2", test2Result.getStackTrace());
        assertEquals("bad_code1\nbad_code2\nbad_code3", test3Result.getStackTrace());
    }

    /** Ensure that the merging logic among multiple testRunResults for the same test is correct. */
    @Test
    public void testMergeRetriedRunResults_checkMergingStackTraces() {
        // Test Setup.
        TestDescription testcase = new TestDescription("Foo", "foo");
        TestRunResult result1 = new TestRunResult();
        TestRunResult result2 = new TestRunResult();
        TestRunResult result3 = new TestRunResult();
        TestRunResult result4 = new TestRunResult();
        // Mimic the ModuleDefinition run.
        result1.testRunStarted("fake run", 1);
        result1.testStarted(testcase);
        result1.testEnded(testcase, new HashMap<String, Metric>());
        result1.testRunEnded(0, new HashMap<String, Metric>());

        result2.testRunStarted("fake run", 1);
        result2.testStarted(testcase);
        result2.testFailed(testcase, "Second run failed.");
        result2.testEnded(testcase, new HashMap<String, Metric>());
        result2.testRunEnded(0, new HashMap<String, Metric>());

        result3.testRunStarted("fake run", 1);
        result3.testStarted(testcase);
        result3.testEnded(testcase, new HashMap<String, Metric>());
        result3.testRunEnded(0, new HashMap<String, Metric>());

        result4.testRunStarted("fake run", 1);
        result4.testStarted(testcase);
        result4.testFailed(testcase, "Fourth run failed.");
        result4.testEnded(testcase, new HashMap<String, Metric>());
        result4.testRunEnded(0, new HashMap<String, Metric>());

        List<TestRunResult> testResultList =
                new ArrayList<>(Arrays.asList(result1, result2, result3, result4));
        TestRunResult result = TestRunResult.merge(testResultList);

        // Verify result.
        assertEquals("fake run", result.getName());
        Map<TestDescription, TestResult> testRunResult = result.getTestResults();
        assertTrue(testRunResult.containsKey(testcase));
        TestResult testResult = testRunResult.get(testcase);
        assertEquals(TestStatus.FAILURE, testResult.getStatus());

        assertEquals("Second run failed.\nFourth run failed.", testResult.getStackTrace());
    }

    /** Ensure that merge will raise exceptions if merge TestRunResult for different test. */
    @Test(expected = IllegalArgumentException.class)
    public void testMergeRetriedRunResults_RaiseErrorIfDiffTests() {
        // Test Setup.
        TestDescription testcase1 = new TestDescription("Foo", "foo");
        TestRunResult result1 = new TestRunResult();
        TestRunResult result2 = new TestRunResult();

        // Mimic the ModuleDefinition run.
        result1.testRunStarted("Fake run 1", 1);
        result1.testStarted(testcase1);
        result1.testEnded(testcase1, new HashMap<String, Metric>());
        result1.testRunEnded(0, new HashMap<String, Metric>());

        result2.testRunStarted("Fake run 2", 1);
        result2.testStarted(testcase1);
        result2.testEnded(testcase1, new HashMap<String, Metric>());
        result2.testRunEnded(0, new HashMap<String, Metric>());

        // Verify raise exceptoin.
        List<TestRunResult> testResultList = new ArrayList<>(Arrays.asList(result1, result2));
        TestRunResult.merge(testResultList);
    }

    /**
     * Ensure that merge will use the last TestRunResult's runMetrics and fileLogs as the merged
     * TestRunResults.
     */
    @Test
    public void testMergeRetriedRunResults_CheckMergeMapAttributes() {
        // Test Setup.
        TestDescription testcase1 = new TestDescription("Foo", "foo");
        TestRunResult result1 = new TestRunResult();
        TestRunResult result2 = new TestRunResult();
        Map<String, String> runMetrics1 = new HashMap<String, String>();
        Map<String, String> runMetrics2 = new HashMap<String, String>();

        // Mimic the ModuleDefinition run.
        result1.testRunStarted("Fake run", 1);
        result1.testLogSaved("run log", new LogFile("path1", "url", LogDataType.TEXT));
        result1.testStarted(testcase1);
        result1.testEnded(testcase1, new HashMap<String, Metric>());
        runMetrics1.put("metric1", "10");
        runMetrics1.put("metric2", "1000");
        result1.testRunEnded(0, runMetrics1);

        result2.testRunStarted("Fake run", 1);
        result1.testLogSaved("run log", new LogFile("path2", "url", LogDataType.TEXT));
        result2.testStarted(testcase1);
        result2.testEnded(testcase1, new HashMap<String, Metric>());
        runMetrics2.put("metric1", "5");
        runMetrics2.put("metric2", "5000");
        result2.testRunEnded(0, runMetrics2);

        List<TestRunResult> testResultList = new ArrayList<>(Arrays.asList(result1, result2));
        TestRunResult result = TestRunResult.merge(testResultList);
        Map<String, String> runMetrics = result.getRunMetrics();
        assertTrue(runMetrics.containsKey("metric1"));
        assertTrue(runMetrics.get("metric1").equals("5"));
        assertTrue(runMetrics.containsKey("metric2"));
        assertTrue(runMetrics.get("metric2").equals("5000"));

        assertTrue(result.getRunLoggedFiles().containsKey("run log"));
        assertEquals("path2", result.getRunLoggedFiles().get("run log").getPath());
    }
}
