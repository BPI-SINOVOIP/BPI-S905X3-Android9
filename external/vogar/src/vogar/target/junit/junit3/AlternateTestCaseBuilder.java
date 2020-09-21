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

import junit.framework.TestCase;
import org.junit.runner.Runner;
import org.junit.runners.model.RunnerBuilder;
import vogar.target.junit.DescribableComparator;
import vogar.target.junit.DescribableStatement;
import vogar.target.junit.RunnerParams;

/**
 * Creates a {@link Runner} for {@link TestCase} derived classes.
 */
public class AlternateTestCaseBuilder extends RunnerBuilder {

    private final TestCaseRunnerFactory testCaseRunnerFactory;

    public AlternateTestCaseBuilder(RunnerParams runnerParams) {
        this(new TestCaseRunnerFactory(runnerParams));
    }

    public AlternateTestCaseBuilder(TestCaseRunnerFactory testCaseRunnerFactory) {
        this.testCaseRunnerFactory = testCaseRunnerFactory;
    }


    @Override
    public Runner runnerForClass(Class<?> testClass) throws Throwable {
        if (junit.framework.TestCase.class.isAssignableFrom(testClass)) {
            // Transform the test case into a Runner.
            Class<? extends TestCase> testCaseClass = testClass.asSubclass(TestCase.class);
            TestCaseTransformer<Runner, DescribableStatement> testCaseTransformer =
                    new TestCaseTransformer<>(testCaseRunnerFactory,
                            DescribableComparator.getInstance());
            return testCaseTransformer.createSuite(testCaseClass);
        }
        return null;
    }
}
