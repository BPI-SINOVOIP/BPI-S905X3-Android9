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

import com.google.common.base.Function;
import java.util.ArrayList;
import java.util.Collection;
import org.junit.runner.Runner;
import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.JUnit4;
import org.junit.runners.model.RunnerBuilder;
import vogar.target.junit.junit3.AlternateSuiteMethodBuilder;

/**
 * A composite {@link RunnerBuilder} that will ask each of its list of {@link RunnerBuilder} to
 * create a runner, returning the result of the first that does so, or null if none match.
 */
public class VogarRunnerBuilder extends RunnerBuilder {

    private static final ReplaceRunnerFunction MAPPING_FUNCTION = new ReplaceRunnerFunction();

    private final RunnerParams runnerParams;
    private final Collection<RunnerBuilder> builders;

    public VogarRunnerBuilder(RunnerParams runnerParams) {
        this.runnerParams = runnerParams;
        builders = new ArrayList<>();
        builders.add(new MappingAnnotatedBuilder(this, MAPPING_FUNCTION));
        builders.add(new AlternateSuiteMethodBuilder(runnerParams));
        builders.add(new VogarTestCaseBuilder(runnerParams));
        builders.add(new VogarJUnit4Builder(this));
    }

    public RunnerParams getRunnerParams() {
        return runnerParams;
    }

    @Override
    public Runner runnerForClass(Class<?> testClass) throws Throwable {
        for (RunnerBuilder builder : builders) {
            Runner runner = builder.safeRunnerForClass(testClass);
            if (runner != null) {
                return runner;
            }
        }

        return null;
    }

    private static class ReplaceRunnerFunction
            implements Function<Class<? extends Runner>, Class<? extends Runner>> {
        @Override
        public Class<? extends Runner> apply(Class<? extends Runner> runnerClass) {
            if (runnerClass == JUnit4.class || runnerClass == BlockJUnit4ClassRunner.class) {
                return VogarBlockJUnit4ClassRunner.class;
            } else {
                return runnerClass;
            }
        }
    }
}
