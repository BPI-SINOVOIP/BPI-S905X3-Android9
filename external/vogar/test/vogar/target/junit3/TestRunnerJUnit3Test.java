/*
 * Copyright (C) 2016 The Android Open Source Project
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

package vogar.target.junit3;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import vogar.RunnerType;
import vogar.target.AbstractTestRunnerTest;
import vogar.target.TestRunner;
import vogar.target.TestRunnerProperties;
import vogar.target.junit3.SuiteReturnsCustomTest.CustomTest;
import vogar.target.mixture.junit3.JUnit3Test;

/**
 * Tests for using TestRunner to run JUnit 3 classes.
 */
@RunWith(JUnit4.class)
public class TestRunnerJUnit3Test extends AbstractTestRunnerTest {

    @TestRunnerProperties(testClass = JUnit3Test.class, runnerType = RunnerType.JUNIT)
    @Test
    public void testConstructor_JUnit3Test_RunnerType_JUNIT() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("testFoo")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = ChangeDefaultLocaleTest.class)
    @Test
    public void testRunner_ChangeDefaultLocaleTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        // Note, that this doesn't complete normally. That's correct behavior as that will trigger
        // the vogar process to restart the VM and run the tests from after this one.
        expectedResults()
                .success("testDefault_Locale_CANADA")
                .success("testDefault_Locale_CHINA")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SimpleTest2.class)
    @Test
    public void testRunner_SimpleTest2() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("testSimple1")
                .success("testSimple2")
                .success("testSimple3")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SimpleTest2.class)
    @Test
    public void testRunner_SkipPast() throws Exception {
        Class<?> testClass = testRunnerRule.testClass();
        String failingTestName = testClass.getName() + "#testSimple2";
        TestRunner runner = testRunnerRule.createTestRunner("--skipPast", failingTestName);

        runner.run();
        expectedResults()
                .success("testSimple3")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SimpleTest2.class)
    @Test
    public void testRunner_SkipPastAll() throws Exception {
        Class<?> testClass = testRunnerRule.testClass();
        String failingTestName = testClass.getName() + "#other";
        TestRunner runner = testRunnerRule.createTestRunner("--skipPast", failingTestName);

        runner.run();
        expectedResults()
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SimpleTest2.class)
    @Test
    public void testRunner_SimpleTest2_OneMethod() throws Exception {
        String[] args = {"testSimple2"};
        TestRunner runner = testRunnerRule.createTestRunner(args);
        runner.run();

        expectedResults()
                .success("testSimple2")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SimpleTest2.class)
    @Test
    public void testRunner_SimpleTest2_TwoMethod() throws Exception {
        String[] args = {"testSimple1", "testSimple3"};
        TestRunner runner = testRunnerRule.createTestRunner(args);
        runner.run();

        expectedResults()
                .success("testSimple1")
                .success("testSimple3")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SimpleTest2.class)
    @Test
    public void testRunner_SimpleTest2_WrongMethod() throws Exception {
        String args = "testSimple5";
        TestRunner runner = testRunnerRule.createTestRunner(args);
        runner.run();

        expectedResults()
                .failure("testSimple5",
                        "junit.framework.AssertionFailedError: Method \"" + args
                                + "\" not found\n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = ExtendedSimpleTest2.class)
    @Test
    public void testRunner_ExtendedSimple2() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("testSimple1")
                .success("testSimple2")
                .success("testSimple3")
                .success("testSimple4")
                .completedNormally();
    }

    @TestRunnerProperties(
            testClass = ExtendedSimpleTest2.class,
            testClassOrPackage = "vogar.target.junit3.ExtendedSimpleTest2#testSimple2")
    @Test
    public void testRunner_ExtendedSimple2_QualifiedAndMethodNames() throws Exception {
        String[] args = {"testSimple1", "testSimple4"};
        TestRunner runner = testRunnerRule.createTestRunner(args);
        runner.run();

        expectedResults()
                .success("testSimple1")
                .success("testSimple2")
                .success("testSimple4")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SimpleTest.class)
    @Test
    public void testRunner_SimpleTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("testSimple")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = FailTest.class)
    @Test
    public void testRunner_FailTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .failure("testFail",
                        "junit.framework.AssertionFailedError: failed.\n")
                .success("testSuccess")
                .failure("testThrowException", "java.lang.RuntimeException: exception\n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = FailConstructorTest.class)
    @Test
    public void testRunner_FailConstructorTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .failure("testSuccess", ""
                        + "java.lang.IllegalStateException: Constructor failed\n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SuiteTest.class)
    @Test
    public void testRunner_SuiteTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .forTestClass(SimpleTest.class)
                .success("testSimple")
                .forTestClass(SimpleTest2.class)
                .success("testSimple1")
                .success("testSimple2")
                .success("testSimple3")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = LongTest.class, timeout = 1)
    @Test
    public void testRunner_LongTest_WithTimeout() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        // Note, that this doesn't complete normally. That's correct behavior as that will trigger
        // the vogar process to restart the VM and run the tests from after this one.
        expectedResults()
                .failure("test", "java.util.concurrent.TimeoutException\n")
                .aborted();
    }

    @TestRunnerProperties(testClass = LongTest2.class)
    @Test
    public void testRunner_LongTest2_WithoutTimeout() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("test1")
                .success("test2")
                .success("test3")
                .success("test4")
                .success("test5")
                .success("test6")
                .success("test7")
                .success("test8")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = TestMethodWithParameterTest.class)
    @Test
    public void testRunner_TestMethodWithParameterTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        // Ignores test methods with parameters.
        expectedResults().completedNormally();
    }

    @TestRunnerProperties(testClass = TestMethodWithParameterTest.class)
    @Test
    public void testRunner_TestMethodWithParameterTest_Requested() throws Exception {
        String methodName = "testParameterized";
        TestRunner runner = testRunnerRule.createTestRunner(methodName);
        runner.run();

        // Ignores tests with no parameters.
        expectedResults()
                .failure(methodName,
                        "junit.framework.AssertionFailedError: Method \"" + methodName
                                + "\" not found\n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = NoMethodTest.class)
    @Test
    public void testRunner_NoMethodTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        // Ignores tests with no methods.
        expectedResults().completedNormally();
    }

    /**
     * Verifies that when a timeout occurs with the first test that it does not run subsequent
     * tests as the client JVM is assumed to be in an indeterminate state. In that case the client
     * process exits and the main Vogar process restarts it skipping past the timing out test.
     */
    @TestRunnerProperties(testClass = LongSuite.class, timeout = 1)
    @Test
    public void testRunner_LongSuite_WithTimeout() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        // Ignores tests with no parameters.
        expectedResults()
                .forTestClass(LongTest.class)
                .failure("test", "java.util.concurrent.TimeoutException\n")
                .aborted();
    }

    @TestRunnerProperties(testClass = LazyTestCreationTest.class)
    @Test
    public void testRunner_LazyTestCreationTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("test1", "Creating: test1\n")
                .success("test2", "Creating: test2\n")
                .completedNormally();
    }

    // =========================================================================================
    // Place all JUnit3 specific test methods after this one.
    // =========================================================================================

    @TestRunnerProperties(testClass = FailingSuiteTest.class)
    @Test
    public void testRunner_FailingSuiteTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        // Ignores tests with no parameters.
        expectedResults()
                .failure("suite",
                        "java.lang.IllegalStateException: Cannot create suite\n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SuiteReturnsTestCaseTest.class)
    @Test
    public void testRunner_SuiteReturnsTestCaseTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .forTestClass(ParameterizedTestCase.class)
                .success("wilma", "wilma\n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SuiteReturnsTestSuiteWithTestCasesTest.class)
    @Test
    public void testRunner_SuiteReturnsTestSuiteWithTestCasesTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .forTestClass(ParameterizedTestCase.class)
                .success("fred", "fred\n")
                .success("null", "null\n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SuiteReturnsCustomTest.class)
    @Test
    public void testRunner_SuiteReturnsCustomTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .failure("suite", ""
                        + "java.lang.IllegalStateException: Unknown suite() result: "
                        + CustomTest.class.getName()
                        + "\n")
                .completedNormally();
    }
}
