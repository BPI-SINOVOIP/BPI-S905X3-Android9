/*
 * Copyright (C) 2015 The Android Open Source Project
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

package vogar.target;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import vogar.target.junit3.WrongSuiteTest;
import vogar.target.mixture.NonTestClass;
import vogar.target.mixture.junit3.AbstractJUnit3Test;
import vogar.target.mixture.junit3.JUnit3Test;
import vogar.target.mixture.junit3.NotPublicConstructorTest;
import vogar.target.mixture.junit3.TwoConstructorsTest;
import vogar.target.mixture.junit4.AbstractJUnit4Test;
import vogar.target.mixture.junit4.JUnit4Test;
import vogar.target.mixture.main.MainApp;

import static org.junit.Assert.assertEquals;

/**
 * Tests for {@link TestRunner}
 */
@RunWith(JUnit4.class)
public class TestRunnerTest extends AbstractTestRunnerTest {

    /**
     * Make sure that the {@code --monitorPort <port>} command line option overrides the default
     * specified in the properties.
     */
    @TestRunnerProperties(testClassOrPackage = "vogar.DummyTest", monitorPort = 2345)
    @Test
    public void testRunner_MonitorPortOverridden() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        assertEquals(2345, (int) runner.monitorPort);

        runner = testRunnerRule.createTestRunner("--monitorPort", "10");
        assertEquals(10, (int) runner.monitorPort);
    }

    @TestRunnerProperties(testClass = Object.class)
    @Test
    public void testRunner_Object() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .noRunner()
                .completedNormally();
    }

    @TestRunnerProperties(testClass = WrongSuiteTest.class)
    @Test
    public void testRunner_WrongSuiteTest() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .noRunner()
                .completedNormally();
    }

    /**
     * If this fails with a "No classes in package: vogar.target.mixture;" error then the tests are
     * not being run from a JAR, add {@code -Dvogar-scan-directories-for-tests=true} to the
     * command line to get it working properly. This is usually only a problem in IDEs.
     */
    @TestRunnerProperties(testClassOrPackage = "vogar.target.mixture")
    @Test
    public void testRunner_Mixture() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        String notPublicClassOrConstructorTest
                = "vogar.target.mixture.junit3.NotPublicClassOrConstructorTest";
        String notPublicClassTest
                = "vogar.target.mixture.junit3.NotPublicClassTest";
        expectedResults()
                .forTestClass(NonTestClass.class)
                .unsupported()
                .forTestClass(AbstractJUnit3Test.class)
                .unsupported()
                .forTestClass(JUnit3Test.class)
                .success("testFoo")
                .forTestClass(notPublicClassOrConstructorTest)
                .failure("test1",
                        "junit.framework.AssertionFailedError: Class "
                                + notPublicClassOrConstructorTest
                                + " has no public constructor TestCase(String name) or TestCase()\n")
                .failure("test2",
                        "junit.framework.AssertionFailedError: Class "
                                + notPublicClassOrConstructorTest
                                + " has no public constructor TestCase(String name) or TestCase()\n")
                .forTestClass(notPublicClassTest)
                .failure("test1",
                        "junit.framework.AssertionFailedError: Class "
                                + notPublicClassTest + " is not public\n")
                .failure("test2",
                        "junit.framework.AssertionFailedError: Class "
                                + notPublicClassTest + " is not public\n")
                .forTestClass(NotPublicConstructorTest.class)
                .failure("test1",
                        "junit.framework.AssertionFailedError: Class "
                                + NotPublicConstructorTest.class.getName()
                                + " has no public constructor TestCase(String name) or TestCase()\n")
                .failure("test2",
                        "junit.framework.AssertionFailedError: Class "
                                + NotPublicConstructorTest.class.getName()
                                + " has no public constructor TestCase(String name) or TestCase()\n")
                .forTestClass(TwoConstructorsTest.class)
                .success("test")
                .forTestClass(AbstractJUnit4Test.class)
                .unsupported()
                .forTestClass(JUnit4Test.class)
                .success("testBar")
                .success("testBaz")
                .forTestClass(MainApp.class)
                .success("main", "Main\n")
                .completedNormally();
    }
}
