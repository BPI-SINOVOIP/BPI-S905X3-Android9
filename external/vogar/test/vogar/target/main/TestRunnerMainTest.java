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

package vogar.target.main;

import com.google.common.base.Joiner;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import vogar.RunnerType;
import vogar.target.AbstractTestRunnerTest;
import vogar.target.TestRunner;
import vogar.target.TestRunnerProperties;

/**
 * Tests for using TestRunner to run classes that provide a main method.
 */
@RunWith(JUnit4.class)
public class TestRunnerMainTest extends AbstractTestRunnerTest {

    @TestRunnerProperties(testClass = Main.class, runnerType = RunnerType.MAIN)
    @Test
    public void testConstructor_Main_RunnerType_MAIN() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("main", "Args: \n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = Main.class)
    @Test
    public void testRunner_Main_NoArgs() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("main", "Args: \n")
                .completedNormally();
    }

    @TestRunnerProperties(testClass = Main.class)
    @Test
    public void testRunner_Main_Args() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner("arg1", "arg2");
        runner.run();

        expectedResults()
                .success("main", "Args: arg1, arg2\n")
                .completedNormally();
    }

    public static class Main {
        public static void main(String[] args) {
            System.out.println("Args: " + Joiner.on(", ").join(args));
        }
    }

    @TestRunnerProperties(testClass = MainLong.class)
    @Test
    public void testRunner_MainLong_NoTimeout() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner();
        runner.run();

        expectedResults()
                .success("main", "Args: \n")
                .completedNormally();
    }

    public static class MainLong {
        public static void main(String[] args) throws InterruptedException {
            Thread.sleep(2000);
            System.out.println("Args: " + Joiner.on(", ").join(args));
        }
    }
}
