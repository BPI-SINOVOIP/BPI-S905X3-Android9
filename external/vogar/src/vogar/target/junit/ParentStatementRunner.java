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

import java.util.List;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.ParentRunner;
import org.junit.runners.model.InitializationError;
import org.junit.runners.model.Statement;

/**
 * A {@link ParentRunner} whose children are {@link DescribableStatement} instances.
 *
 * <p>This is a generally useful form of {@link ParentRunner} that can easily be customized.
 *
 * <p>This is used for JUnit3 based classes which may have multiple constructors (a default and
 * one that takes the test name) so it passes a {@link Dummy} class to the parent and overrides the
 * {@link #getName()} method. That works because this does not use any information from the supplied
 * class.
 */
public class ParentStatementRunner extends ParentRunner<DescribableStatement> {

    private final String name;
    private final List<DescribableStatement> statements;
    private final TestRule testRule;

    public ParentStatementRunner(Class<?> testClass, List<DescribableStatement> statements,
                                 RunnerParams runnerParams)
            throws InitializationError {
        super(Dummy.class);
        name = testClass.getName();
        this.statements = statements;
        testRule = runnerParams.getTestRule();
    }

    @Override
    protected String getName() {
        return name;
    }

    @Override
    protected List<DescribableStatement> getChildren() {
        return statements;
    }

    @Override
    protected Description describeChild(DescribableStatement child) {
        return child.getDescription();
    }

    @Override
    protected void runChild(final DescribableStatement child, RunNotifier notifier) {
        Description description = describeChild(child);
        Statement statement = child;
        statement = testRule.apply(statement, description);
        ParentRunnerHelper.abortingRunLeaf(statement, description, notifier);
    }

    /**
     * ParentRunner requires that the class be public.
     */
    public static class Dummy {
    }
}
