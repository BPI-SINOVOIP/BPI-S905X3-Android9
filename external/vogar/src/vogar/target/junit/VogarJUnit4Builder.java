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

import org.junit.runner.Runner;
import org.junit.runners.model.RunnerBuilder;

/**
 * A {@link RunnerBuilder} that always returns a {@link VogarBlockJUnit4ClassRunner}.
 */
class VogarJUnit4Builder extends RunnerBuilder {
    private final VogarRunnerBuilder topRunnerBuilder;

    public VogarJUnit4Builder(VogarRunnerBuilder topRunnerBuilder) {
        this.topRunnerBuilder = topRunnerBuilder;
    }

    @Override
    public Runner runnerForClass(Class<?> testClass) throws Throwable {
        return new VogarBlockJUnit4ClassRunner(testClass, topRunnerBuilder);
    }
}
