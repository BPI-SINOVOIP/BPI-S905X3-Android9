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

package vogar.target.junit;

import java.lang.annotation.Annotation;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import junit.framework.TestCase;
import org.junit.runner.Runner;
import vogar.target.junit.junit3.TestCaseRunnerFactory;

/**
 * Extends {@link TestCaseRunnerFactory} to add support for explicitly requested methods.
 */
public class VogarTestCaseRunnerFactory extends TestCaseRunnerFactory {

    private static final Annotation[] NO_ANNOTATIONS = new Annotation[0];
    private final RunnerParams runnerParams;

    public VogarTestCaseRunnerFactory(RunnerParams runnerParams) {
        super(runnerParams);
        this.runnerParams = runnerParams;
    }

    @Override
    public boolean eagerClassValidation() {
        return false;
    }

    @Override
    public Runner createSuite(
            Class<? extends TestCase> testClass, List<DescribableStatement> tests) {

        Set<String> requestedMethodNames = JUnitUtils.mergeQualificationAndArgs(
                runnerParams.getQualification(), runnerParams.getArgs());
        if (!requestedMethodNames.isEmpty()) {
            // Ignore the tests list provided and create our own.
            tests = new ArrayList<>();
            for (String methodName : requestedMethodNames) {
                tests.add(createTest(testClass, methodName, NO_ANNOTATIONS));
            }
        }

        return super.createSuite(testClass, tests);
    }
}
