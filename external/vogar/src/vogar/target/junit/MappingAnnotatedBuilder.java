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
import org.junit.internal.builders.AnnotatedBuilder;
import org.junit.runner.RunWith;
import org.junit.runner.Runner;
import org.junit.runners.model.RunnerBuilder;

/**
 * An {@link AnnotatedBuilder} that can map the {@link Runner} class specified in the
 * {@link RunWith} annotation to a replacement one.
 */
public class MappingAnnotatedBuilder extends AnnotatedBuilder {

    private final Function<Class<? extends Runner>, Class<? extends Runner>> mappingFunction;

    public MappingAnnotatedBuilder(
            RunnerBuilder suiteBuilder,
            Function<Class<? extends Runner>, Class<? extends Runner>> mappingFunction) {
        super(suiteBuilder);
        this.mappingFunction = mappingFunction;
    }

    @Override
    public Runner runnerForClass(Class<?> testClass) throws Exception {
        RunWith runWith = testClass.getAnnotation(RunWith.class);
        if (runWith != null) {
            Class<? extends Runner> runnerClass = runWith.value();

            runnerClass = mappingFunction.apply(runnerClass);
            if (runnerClass != null) {
                return buildRunner(runnerClass, testClass);
            }
        }

        return null;
    }
}
