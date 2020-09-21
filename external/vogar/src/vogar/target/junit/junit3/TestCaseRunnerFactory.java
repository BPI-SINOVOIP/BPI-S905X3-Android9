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

package vogar.target.junit.junit3;

import java.lang.annotation.Annotation;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.List;
import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestResult;
import junit.framework.TestSuite;
import org.junit.internal.runners.JUnit38ClassRunner;
import org.junit.runner.Description;
import org.junit.runner.Runner;
import org.junit.runners.model.InitializationError;
import vogar.target.junit.DescribableStatement;
import vogar.target.junit.ErrorRunner;
import vogar.target.junit.ParentStatementRunner;
import vogar.target.junit.RunnerParams;

/**
 * Constructs a {@link ParentStatementRunner} from a {@link TestCase} derived class.
 *
 * <p>This does a similar job to the {@link JUnit38ClassRunner} when it is constructed with a
 * {@link Class Class<? extends TestCase>} but takes a very different approach.
 *
 * <p>The JU38CR takes a {@link Test}, runs it using the standard JUnit3 infrastructure and
 * adapts the JUnit3 events ({@link TestResult}) to the JUnit4 event model. If it is given a Class
 * then it will first turn it into a {@link TestSuite} which is a {@link Test} and then process it
 * as normal.
 *
 * <p>When this is given a Class it converts it directly into a {@link ParentStatementRunner} where
 * each child {@link DescribableStatement} simply runs the {@link TestCase#runBare()} method. That
 * may cause a slight difference in behavior if the class overrides
 * {@link TestCase#run(TestResult)} as that method is never called but if that does become a
 * problem then it can be detected and fallback to treating it as a {@link Test} just as is done
 * by {@link TestSuiteRunnerFactory}.
 *
 * <p>The JU38CR is also used when running the result from a {@code public static Test suite()}
 * method. That functionality is provided by the {@link TestSuiteRunnerFactory}.
 *
 * <p>The advantage of converting JUnit3 style tests into JUnit4 structures is that it makes it
 * easier to treat them all consistently, apply test rules, etc.
 */
public class TestCaseRunnerFactory implements TestCaseFactory<Runner, DescribableStatement> {

    private final RunnerParams runnerParams;

    public TestCaseRunnerFactory(RunnerParams runnerParams) {
        this.runnerParams = runnerParams;
    }

    @Override
    public boolean eagerClassValidation() {
        return false;
    }

    @Override
    public DescribableStatement createTest(
            Class<? extends TestCase> testClass, String methodName, Annotation[] annotations) {
        return new RunTestCaseStatement(testClass, methodName, annotations);
    }

    @Override
    public DescribableStatement createFailingTest(
            Class<? extends Test> testClass, String name, Throwable throwable) {
        Description description = Description.createTestDescription(
                testClass, name, testClass.getAnnotations());
        return new ThrowingStatement(description, throwable);
    }

    @Override
    public Runner createSuite(
            Class<? extends TestCase> testClass, List<DescribableStatement> tests) {

        try {
            return new ParentStatementRunner(testClass, tests, runnerParams);
        } catch (InitializationError e) {
            Description description = Description.createTestDescription(
                    testClass, "initializationError", testClass.getAnnotations());
            return new ErrorRunner(description, e);
        }
    }

    /**
     * Create the test case.
     *
     * <p>Called immediately prior to running the test and the returned object is discarded
     * immediately after the test was run.
     */
    private static TestCase createTestCase(Class<? extends TestCase> testClass, String name)
            throws Throwable {
        Constructor<? extends TestCase> constructor;
        try {
            constructor = testClass.getConstructor(String.class);
        } catch (NoSuchMethodException ignored) {
            constructor = testClass.getConstructor();
        }

        TestCase test;
        try {
            if (constructor.getParameterTypes().length == 0) {
                test = constructor.newInstance();
                test.setName(name);
            } else {
                test = constructor.newInstance(name);
            }
        } catch (InvocationTargetException e) {
            throw e.getCause();
        }

        return test;
    }

    /**
     * Runs a method in a {@link TestCase}.
     */
    private static class RunTestCaseStatement extends DescribableStatement {
        private final Class<? extends TestCase> testClass;
        private final String name;

        public RunTestCaseStatement(
                Class<? extends TestCase> testClass, String name, Annotation[] annotations) {
            super(Description.createTestDescription(testClass, name, annotations));
            this.testClass = testClass;
            this.name = name;
        }

        @Override
        public void evaluate() throws Throwable {
            // Validate the class just before running the test.
            TestCaseTransformer.validateTestClass(testClass);
            TestCase test = createTestCase(testClass, name);
            test.runBare();
        }
    }

    /**
     * Throws the supplied {@link Throwable}.
     */
    private static class ThrowingStatement extends DescribableStatement {
        private final Throwable throwable;

        public ThrowingStatement(Description description, Throwable throwable) {
            super(description);
            this.throwable = throwable;
        }

        @Override
        public void evaluate() throws Throwable {
            throw throwable;
        }
    }
}
