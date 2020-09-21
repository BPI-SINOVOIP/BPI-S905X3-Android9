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

package vogar.target.junit4;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import vogar.target.AbstractTestRunnerTest;
import vogar.target.TestRunner;
import vogar.target.TestRunnerProperties;

/**
 * Tests for using TestRunner to run JUnit 4 classes.
 */
@RunWith(JUnit4.class)
public class TestRunnerJUnit4Test extends AbstractTestRunnerTest {

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
                .success("Simple3")
                .success("simple1")
                .success("simple2")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SimpleTest2.class)
    @Test
    public void testRunner_SkipPast() throws Exception {
        Class<?> testClass = testRunnerRule.testClass();
        String failingTestName = testClass.getName() + "#simple1";
        TestRunner runner = testRunnerRule.createTestRunner("--skipPast", failingTestName);

        runner.run();

        expectedResults()
                .success("simple2")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SimpleTest2.class)
    @Test
    public void testRunner_SkipPastAll() throws Exception {
        Class<?> testClass = testRunnerRule.testClass();
        String failingTestName = testClass.getName() + "#other";
        TestRunner runner = testRunnerRule.createTestRunner("--skipPast", failingTestName);

        runner.run();
        expectedResults().completedNormally();
    }

    @TestRunnerProperties(testClass = SimpleTest2.class)
    @Test
    public void testRunner_SimpleTest2_OneMethod() throws Exception {
        String[] args = {"simple2"};
        TestRunner runner = testRunnerRule.createTestRunner(args);
        runner.run();

        expectedResults()
                .success("simple2")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SimpleTest2.class)
    @Test
    public void testRunner_SimpleTest2_TwoMethod() throws Exception {
        String[] args = {"simple1", "Simple3"};
        TestRunner runner = testRunnerRule.createTestRunner(args);
        runner.run();

        expectedResults()
                .success("Simple3")
                .success("simple1")
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
                .success("Simple3")
                .success("simple1")
                .success("simple2")
                .success("simple4")
                .completedNormally();
    }

    @TestRunnerProperties(
            testClass = ExtendedSimpleTest2.class,
            testClassOrPackage = "vogar.target.junit4.ExtendedSimpleTest2#simple2")
    @Test
    public void testRunner_ExtendedSimple2_QualifiedAndMethodNames() throws Exception {
        String[] args = {"simple1", "simple4"};
        TestRunner runner = testRunnerRule.createTestRunner(args);
        runner.run();

        expectedResults()
                .success("simple1")
                .success("simple2")
                .success("simple4")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = SimpleTest.class)
    @Test
    public void testRunner_SimpleTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .forTestClass(SimpleTest3.class)
                .success("simple")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = FailTest.class)
    @Test
    public void testRunner_FailTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .failure("failure",
                        "java.lang.AssertionError: failed.\n")
                .success("success")
                .success("throwAnotherExpectedException")
                .failure("throwException", "java.lang.RuntimeException: exception\n")
                .success("throwExpectedException")
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

        // The order is different from previous version of Vogar as that sorted a flattened list
        // but JUnit has it organized as a hierarchy and sorts each level so classes which are on
        // a separate level, like SimpleTest2 and SimpleTest3 are not sorted relative to each
        // other.
        expectedResults()
                .forTestClass(SimpleTest3.class)
                .success("simple")
                .forTestClass(SimpleTest2.class)
                .success("Simple3")
                .success("simple1")
                .success("simple2")
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

        String methodName = "parameterized";
        expectedResults()
                .failure(methodName,
                        "java.lang.Exception: Method " + methodName
                                + " should have no parameters\n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = TestMethodWithParameterTest.class)
    @Test
    public void testRunner_TestMethodWithParameterTest_Requested() throws Exception {
        String methodName = "parameterized";
        TestRunner runner = testRunnerRule.createTestRunner(methodName);
        runner.run();

        // Ignores tests with no parameters.
        expectedResults()
                .failure(methodName,
                        "java.lang.Exception: Method " + methodName
                                + " should have no parameters\n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = NoMethodTest.class)
    @Test
    public void testRunner_NoMethodTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        // Ignores tests with no parameters.
        expectedResults()
                .unsupported()
                .completedNormally();
    }

    @TestRunnerProperties(testClass = AnnotatedTestMethodsTest.class)
    @Test
    public void testRunner_AnnotatedTestMethodsTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("test1", "Before\nTest 1\nAfter\n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = AnnotatedMethodsTest.class)
    @Test
    public void testRunner_AnnotatedMethodsTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .text("Before Class\n")
                .success("test1", "Before\nTest 1\nAfter\n")
                .success("test2", "Before\nTest 2\nAfter\n")
                .text("After Class\n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = LazyTestCreationTest.class)
    @Test
    public void testRunner_LazyTestCreationTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .text("Creating\n")
                .success("test1")
                .text("Creating\n")
                .success("test2")
                .completedNormally();
    }

    // =========================================================================================
    // Place all JUnit4 specific test methods after this one.
    // =========================================================================================

    @TestRunnerProperties(testClass = HasIgnoredTest.class)
    @Test
    public void testRunner_HasIgnoredTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("working")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = MockitoFieldTest.class)
    @Test
    public void testRunner_MockitoFieldTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("test")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = AssumeTest.class)
    @Test
    public void testRunner_AssumeTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("assumesCorrectly", "Assumption was correct\n")
                .success("assumesIncorrectly")
                .completedNormally();
    }
}
