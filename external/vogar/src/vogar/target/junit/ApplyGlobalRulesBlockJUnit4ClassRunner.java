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

import org.junit.Ignore;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;
import org.junit.runners.model.Statement;

/**
 * Applies global rules, i.e. those provided externally to the tests.
 */
public abstract class ApplyGlobalRulesBlockJUnit4ClassRunner extends BlockJUnit4ClassRunner {

    private final TestRule testRule;

    public ApplyGlobalRulesBlockJUnit4ClassRunner(Class<?> klass, TestRule testRule)
            throws InitializationError {
        super(klass);
        this.testRule = testRule;
    }

    @Override
    protected void runChild(final FrameworkMethod method, RunNotifier notifier) {
        // Override to allow it to call abortingRunLeaf as runLeaf is final and so its behavior
        // cannot be modified by overriding it.
        Description description = describeChild(method);
        if (method.getAnnotation(Ignore.class) != null) {
            notifier.fireTestIgnored(description);
        } else {
            ParentRunnerHelper.abortingRunLeaf(methodBlock(method), description, notifier);
        }
    }

    @Override
    protected Statement methodBlock(FrameworkMethod method) {
        // Override to apply any global TestRules.
        Statement statement = super.methodBlock(method);
        statement = testRule.apply(statement, getDescription());
        return statement;
    }
}
