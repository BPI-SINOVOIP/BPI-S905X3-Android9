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

import java.util.List;
import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import org.junit.internal.runners.JUnit38ClassRunner;
import org.junit.runner.Description;
import org.junit.runner.Runner;
import org.junit.runners.Suite;
import org.junit.runners.model.InitializationError;
import vogar.target.junit.DescribableStatement;
import vogar.target.junit.ErrorRunner;
import vogar.target.junit.ExtendedSuiteRunner;
import vogar.target.junit.ParentStatementRunner;
import vogar.target.junit.RunnerParams;
import vogar.target.junit.StatementRunner;

/**
 * Transforms a hierarchy of {@link Test} objects into a hierarchy of {@link Runner} objects.
 *
 * <p>This provides some of the functionality provided by {@link JUnit38ClassRunner}; see
 * {@link TestCaseRunnerFactory} for an overview of that class.
 *
 * <p>Just like {@link TestCaseRunnerFactory} this converts the JUnit3 classes into JUnit4
 * structures and then runs them rather than run them as JUnit3 and adapt the events back into
 * JUnit4 event model. This traverses the hierarchy of {@link Test} and converts them directly into
 * {@link Runner} classes.
 *
 * <ol>
 * <li>A {@link TestSuite} is converted into a {@link Suite}. If it contained {@link TestCase}
 * instances that all had the same class and so were likely created by calling
 * {@link TestSuite#addTestSuite(Class)} or calling any of the {@link TestSuite} constructors that
 * take a Class then it should probably be converted to a {@link ParentStatementRunner}</li>
 * <li>A {@link TestCase} is converted into a {@link StatementRunner}.</li>
 * <li>No other {@link Test} classes are supported. They could be but they would require something
 * like {@link JUnit38ClassRunner}.</li>
 * </ol>
 */
public class TestSuiteRunnerFactory implements TestSuiteFactory<Runner> {

    private final RunnerParams runnerParams;

    public TestSuiteRunnerFactory(RunnerParams runnerParams) {
        this.runnerParams = runnerParams;
    }

    @Override
    public Runner createSuite(String name, List<Runner> children) {
        try {
            return new ExtendedSuiteRunner(name, children);
        } catch (InitializationError e) {
            throw new IllegalStateException(e);
        }
    }

    @Override
    public Runner createTestCase(final TestCase testCase, Description description) {
        return new StatementRunner(runnerParams,
                new DescribableStatement(description) {
                    @Override
                    public void evaluate() throws Throwable {
                        testCase.runBare();
                    }
                });
    }

    @Override
    public Runner createCustomTest(Test test, Description description) {
        return new ErrorRunner(description,
                new IllegalStateException("Unknown suite() result: " + test));
    }
}
