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

package vogar.target;

import com.google.common.base.Function;
import com.google.common.base.Functions;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import vogar.Result;
import vogar.target.junit.JUnitUtils;
import vogar.testing.InterceptOutputStreams;
import vogar.testing.InterceptOutputStreams.Stream;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * Provides support for testing {@link TestRunner} class.
 *
 * <p>Subclasses provide the individual test methods, each test method has the following structure.
 *
 * <p>It is annotated with {@link TestRunnerProperties @TestRunnerProperties} that specifies the
 * properties that the main Vogar process supplies to the client process that actually runs the
 * tests. It must specify either a {@link TestRunnerProperties#testClass()} or
 * {@link TestRunnerProperties#testClassOrPackage()}, all the remaining properties are optional.
 *
 * <p>It calls {@code testRunnerRule.createTestRunner(...)} to create a {@link TestRunner}; passing
 * in any additional command line arguments that {@link TestRunner#TestRunner(Properties, List)}
 * accepts.
 *
 * <p>It calls {@link TestRunner#run()} to actually run the tests.
 *
 * <p>It calls {@link #expectedResults()} to obtain a {@link ExpectedResults} instance that it uses
 * to specify the expected results for the test. Once it has specified the expected results then it
 * must call either {@link ExpectedResults#completedNormally()} or
 * {@link ExpectedResults#aborted()}. They indicate whether the test process completed normally or
 * would abort (due to a test timing out) and cause the actual results to be checked against the
 * expected results.
 */
public abstract class AbstractTestRunnerTest {

    @Rule
    public InterceptOutputStreams ios = new InterceptOutputStreams(Stream.OUT);

    @Rule
    public TestRunnerRule testRunnerRule = new TestRunnerRule();

    /**
     * Keeps track of number of times {@link #expectedResults()} has been called without
     * {@link ExpectedResults#checkFilteredOutput(String)}
     * also being called. If it is {@code > 0} then the test is in error.
     */
    private AtomicInteger checkCount;

    @Before
    public void beforeTest() {
        checkCount = new AtomicInteger();
    }

    @After
    public void afterTest() {
        if (checkCount.get() != 0) {
            throw new IllegalStateException("Test called expectedResults() but failed to call"
                    + "either aborted() or completedNormally()");
        }
    }

    protected ExpectedResults expectedResults() {
        checkCount.incrementAndGet();
        return new ExpectedResults(testRunnerRule.testClass(), ios,
                checkCount);
    }

    protected static class ExpectedResults {

        private final StringBuilder builder = new StringBuilder();
        private final InterceptOutputStreams ios;
        private final AtomicInteger checkCount;
        private String testClassName;
        private Function<String, String> filter;

        private ExpectedResults(
                Class<?> testClass, InterceptOutputStreams ios, AtomicInteger checkCount) {
            this.testClassName = testClass.getName();
            this.checkCount = checkCount;
            // Automatically strip out methods from a stack trace to avoid making tests dependent
            // on either the call hierarchy or on source line numbers which would make the tests
            // incredibly fragile. If a test fails then the unfiltered output containing the full
            // stack trace will be output so this will not lose information needed to debug errors.
            filter = new Function<String, String>() {
                @Override
                public String apply(String input) {
                    // Remove stack trace from output.
                    return input.replaceAll("\\t(at[^\\n]+|\\.\\.\\. [0-9]+ more)\\n", "");
                }
            };
            this.ios = ios;
        }

        public ExpectedResults text(String message) {
            builder.append(message);
            return this;
        }

        private ExpectedResults addFilter(Function<String, String> function) {
            filter = Functions.compose(filter, function);
            return this;
        }

        public ExpectedResults forTestClass(Class<?> testClass) {
            this.testClassName = testClass.getName();
            return this;
        }

        public ExpectedResults forTestClass(String testClassName) {
            this.testClassName = testClassName;
            return this;
        }

        public ExpectedResults failure(String methodName, String message) {
            String output = outcome(testClassName, methodName, message, Result.EXEC_FAILED);
            return text(output);
        }

        public ExpectedResults success(String methodName) {
            String output = outcome(testClassName, methodName, null, Result.SUCCESS);
            return text(output);
        }

        public ExpectedResults success(String methodName, String message) {
            String output = outcome(testClassName, methodName, message, Result.SUCCESS);
            return text(output);
        }


        public ExpectedResults unsupported() {
            String output = outcome(
                    testClassName, null,
                    "Skipping " + testClassName + ": no associated runner class\n",
                    Result.UNSUPPORTED);
            return text(output);
        }

        public ExpectedResults noRunner() {
            String message =
                    String.format("Skipping %s: no associated runner class\n", testClassName);
            String output = outcome(testClassName, null, message, Result.UNSUPPORTED);
            return text(output);
        }

        public void completedNormally() {
            text("//00xx{\"completedNormally\":true}\n");
            checkFilteredOutput(builder.toString());
        }

        public void aborted() {
            checkFilteredOutput(builder.toString());
        }


        private static String outcome(
                String testClassName, String methodName, String message, Result result) {
            String testName = JUnitUtils.getTestName(testClassName, methodName);

            return String.format("//00xx{\"outcome\":\"%s\"}\n"
                            + "%s"
                            + "//00xx{\"result\":\"%s\"}\n",
                    testName, message == null ? "" : message, result);
        }

        private void checkFilteredOutput(String expected) {
            checkCount.decrementAndGet();
            String output = ios.contents(Stream.OUT);
            String filtered = filter.apply(output);
            if (!expected.equals(filtered)) {
                assertEquals(expected, output);
            }
        }
    }
}
