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

import org.junit.rules.TestRule;
import org.junit.runner.Runner;
import org.junit.runners.model.RunnerBuilder;

/**
 * Parameters for the various vogar {@link Runner} implementation classes.
 *
 * <p>RunnerParams can be used to configure {@link RunnerBuilder} instances. RunnerBuilder doesn't
 * have a method that allows parameters so they must be passed, for example, as a constructor
 * argument to the RunnerBuilder implementation.
 */
public class RunnerParams {

    private final String qualification;
    private final String[] args;
    private final TestRule testRule;

    public RunnerParams(String qualification, String[] args, TestRule testRule) {
        this.qualification = qualification;
        this.args = args;
        this.testRule = testRule;
    }

    public String getQualification() {
        return qualification;
    }

    public String[] getArgs() {
        return args;
    }

    public TestRule getTestRule() {
        return testRule;
    }
}
