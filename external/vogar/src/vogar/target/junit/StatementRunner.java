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
import org.junit.runner.Description;
import org.junit.runner.Runner;
import org.junit.runner.manipulation.Filter;
import org.junit.runner.manipulation.Filterable;
import org.junit.runner.manipulation.NoTestsRemainException;
import org.junit.runner.manipulation.Sortable;
import org.junit.runner.manipulation.Sorter;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.model.Statement;

/**
 * Runs a {@link Statement}.
 *
 * <p>This is a generally useful form of {@link Runner} that can easily be customized.
 */
public class StatementRunner extends Runner implements Filterable, Sortable {

    private final Description description;
    private final Statement statement;
    private final TestRule testRule;

    public StatementRunner(RunnerParams runnerParams, Description description,
                           Statement statement) {
        this.description = description;
        this.statement = statement;

        testRule = runnerParams.getTestRule();
    }

    public StatementRunner(RunnerParams runnerParams, DescribableStatement statement) {
        this(runnerParams, statement.getDescription(), statement);
    }

    @Override
    public Description getDescription() {
        return description;
    }

    @Override
    public void run(final RunNotifier notifier) {
        Statement statement = this.statement;
        statement = testRule.apply(statement, description);
        ParentRunnerHelper.abortingRunLeaf(statement, description, notifier);
    }

    @Override
    public void sort(Sorter sorter) {
        // Nothing to do.
    }

    @Override
    public void filter(Filter filter) throws NoTestsRemainException {
        if (!filter.shouldRun(description)) {
            // This only runs one test so it it shouldn't be run then there are no tests left so
            // notify the parent so they can remove this runner from their list.
            throw new NoTestsRemainException();
        }
    }
}
