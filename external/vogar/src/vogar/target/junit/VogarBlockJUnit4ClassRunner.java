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
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import junit.framework.AssertionFailedError;
import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;
import org.junit.runners.model.RunnerBuilder;

/**
 * A specialization of {@link BlockJUnit4ClassRunner} to implement behavior required by Vogar.
 *
 * <ol>
 * <li>Defers validation of test methods,
 * see {@link ValidateTestMethodWhenRunBlockJUnit4ClassRunner}.</li>
 * <li>Applies global rules, see {@link ApplyGlobalRulesBlockJUnit4ClassRunner}</li>
 * <li>Selects either explicitly requested methods, or all methods.</li>
 * </ol>
 */
public class VogarBlockJUnit4ClassRunner
        extends ValidateTestMethodWhenRunBlockJUnit4ClassRunner {

    private final RunnerParams runnerParams;

    /**
     * Used by annotation runner.
     */
    @SuppressWarnings("unused")
    public VogarBlockJUnit4ClassRunner(Class<?> klass, RunnerBuilder suiteBuilder)
            throws InitializationError {
        this(klass, ((VogarRunnerBuilder) suiteBuilder).getRunnerParams());
    }

    public VogarBlockJUnit4ClassRunner(Class<?> klass, RunnerParams runnerParams)
            throws InitializationError {
        super(klass, runnerParams.getTestRule());
        this.runnerParams = runnerParams;
    }

    @Override
    protected List<FrameworkMethod> getChildren() {
        // Overridden to handle requested methods.
        Set<String> requestedMethodNames = JUnitUtils.mergeQualificationAndArgs(
                runnerParams.getQualification(), runnerParams.getArgs());
        List<FrameworkMethod> methods = super.getChildren();

        // If specific methods have been requested then select them from all the methods that were
        // found. If they cannot be found then add a fake one that will report the method as
        // missing.
        if (!requestedMethodNames.isEmpty()) {
            // Store all the methods in a map by name. That should be safe as test methods do not
            // have parameters so there can only be one method in a class with each name.
            Map<String, FrameworkMethod> map = new HashMap<>();
            for (FrameworkMethod method : methods) {
                map.put(method.getName(), method);
            }

            methods = new ArrayList<>();
            for (final String name : requestedMethodNames) {
                FrameworkMethod method = map.get(name);
                if (method == null) {
                    // The method could not be found so add one that when invoked will report the
                    // method as missing.
                    methods.add(new MissingFrameworkMethod(name));
                } else {
                    methods.add(method);
                }
            }
        }
        return methods;
    }

    /**
     * A {@link FrameworkMethod} that is used when a specific method has been requested but no
     * suitable {@link Method} exists.
     *
     * <p>It overrides a number of methods that are called during normal processing in order to
     * avoid throwing a NPE. It also overrides {@link #validatePublicVoidNoArg(boolean, List)} to
     * report the method as being missing. It relies on a {@link ValidateMethodStatement} to call
     * that method immediately prior to invoking the method.
     */
    private static class MissingFrameworkMethod extends FrameworkMethod {
        private static final Annotation[] NO_ANNOTATIONS = new Annotation[0];
        private static final Method DUMMY_METHOD;
        static {
            DUMMY_METHOD = Object.class.getMethods()[0];
        }
        private final String name;

        public MissingFrameworkMethod(String name) {
            super(DUMMY_METHOD);
            this.name = name;
        }

        @Override
        public String getName() {
            // Overridden to avoid NPE.
            return name;
        }

        @Override
        public void validatePublicVoidNoArg(boolean isStatic, List<Throwable> errors) {
            // Overridden to report the method as missing.
            errors.add(new AssertionFailedError("Method \"" + name + "\" not found"));
        }

        @Override
        public Annotation[] getAnnotations() {
            // Overridden to avoid NPE.
            return NO_ANNOTATIONS;
        }

        @Override
        public <T extends Annotation> T getAnnotation(Class<T> annotationType) {
            // Overridden to avoid NPE.
            return null;
        }
    }
}
