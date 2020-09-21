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

package vogar.target.caliper;

import com.google.caliper.Benchmark;
import com.google.caliper.runner.UserCodeException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import vogar.RunnerType;
import vogar.target.AbstractTestRunnerTest;
import vogar.target.TestRunner;
import vogar.target.TestRunnerProperties;

/**
 * Tests for using TestRunner to run Caliper classes.
 */
@RunWith(JUnit4.class)
public class TestRunnerCaliperTest extends AbstractTestRunnerTest {

    @TestRunnerProperties(testClass = CaliperBenchmarkFailing.class)
    @Test
    public void testRunner_CaliperBenchmark_NoRunner() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner("-i", "runtime");
        runner.run();

        expectedResults()
                .unsupported()
                .completedNormally();
    }

    @TestRunnerProperties(testClass = CaliperBenchmarkFailing.class,
            runnerType = RunnerType.CALIPER)
    @Test
    public void testRunner_CaliperBenchmarkFailing() throws Exception {
        TestRunner runner = testRunnerRule.createTestRunner("-i", "runtime");
        runner.run();

        expectedResults()
                .failure(null, (""
                        + "Experiment selection: \n"
                        + "  Benchmark Methods:   [failingBenchmark]\n"
                        + "  Instruments:   [runtime]\n"
                        + "  User parameters:   {}\n"
                        + "  Virtual machines:  [default]\n"
                        + "  Selection type:    Full cartesian product\n"
                        + "\n"
                        + "This selection yields 1 experiments.\n"
                        + UserCodeException.class.getName()
                        + ": An exception was thrown from the benchmark code\n"
                        + "Caused by: " + IllegalStateException.class.getName()
                        + ": " + CaliperBenchmarkFailing.BENCHMARK_FAILED + "\n"))
                .completedNormally();
    }

    public static class CaliperBenchmark {

        @Benchmark
        public long timeMethod(long reps) {
            System.out.println(reps);
            return reps;
        }
    }

    public static class CaliperBenchmarkFailing {

        static final String BENCHMARK_FAILED = "Benchmark failed";

        @Benchmark
        public long failingBenchmark(long reps) {
            throw new IllegalStateException(BENCHMARK_FAILED);
        }
    }
}
