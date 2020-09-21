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

import java.util.ArrayList;
import java.util.List;
import org.junit.rules.TestRule;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;
import org.junit.runners.model.Statement;

/**
 * Defers the validation of test methods until just before they are run.
 *
 * <p>This does not throw an {@link InitializationError} during construction if a test method, i.e.
 * one marked with {@link org.junit.Test @Test} annotation, is not valid. Instead it waits until the
 * test is actually run until it does that. This is better than the standard JUnit behavior, where
 * a single invalid test method would prevent all methods in the class from being run.
 */
public class ValidateTestMethodWhenRunBlockJUnit4ClassRunner
        extends ApplyGlobalRulesBlockJUnit4ClassRunner {

    protected ValidateTestMethodWhenRunBlockJUnit4ClassRunner(Class<?> klass, TestRule testRule)
            throws InitializationError {
        super(klass, testRule);
    }

    @Override
    protected void validateTestMethods(List<Throwable> errors) {
        // Overridden to avoid validation during initialization.
    }

    @Override
    protected Statement methodInvoker(final FrameworkMethod frameworkMethod, Object test) {
        // Overridden to perform validation of the method.
        Statement statement = super.methodInvoker(frameworkMethod, test);

        // Wrap the Statement that will invoke the method with one that will validate that the
        // method is of the correct form.
        return new ValidateMethodStatement(frameworkMethod, statement);
    }

    /**
     * A {@link Statement} that validates the underlying {@link FrameworkMethod}
     */
    protected static class ValidateMethodStatement extends Statement {
        private final FrameworkMethod frameworkMethod;
        private final Statement methodInvoker;

        public ValidateMethodStatement(FrameworkMethod frameworkMethod, Statement methodInvoker) {
            this.frameworkMethod = frameworkMethod;
            this.methodInvoker = methodInvoker;
        }

        @Override
        public void evaluate() throws Throwable {
            validateFrameworkMethod();
            methodInvoker.evaluate();
        }

        private void validateFrameworkMethod() throws Throwable {
            ArrayList<Throwable> errors = new ArrayList<>();
            frameworkMethod.validatePublicVoidNoArg(false, errors);
            if (!errors.isEmpty()) {
                throw errors.get(0);
            }
        }
    }
}
