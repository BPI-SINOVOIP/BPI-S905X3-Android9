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

import org.junit.internal.runners.ErrorReportingRunner;
import org.junit.runner.Description;
import org.junit.runner.Runner;
import org.junit.runner.manipulation.Sortable;
import org.junit.runner.manipulation.Sorter;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.model.InitializationError;

/**
 * A {@link Runner} that will report a failure, used to defer error reporting until tests are run.
 *
 * <p>This allows errors occurring during initialization to be recorded rather than aborting
 * initialization and hence aborting the whole test run.
 *
 * <p>This is a simpler version of {@link ErrorReportingRunner}
 */
public class ErrorRunner extends Runner implements Sortable {

    private final Description description;
    private final Throwable throwable;

    public ErrorRunner(Description description, Throwable throwable) {
        this.description = description;
        this.throwable = throwable;

        // If the throwable is an InitializationError then add any of its causes as suppressed
        // exceptions.
        if (throwable instanceof InitializationError) {
            InitializationError error = (InitializationError) throwable;
            for (Throwable cause : error.getCauses()) {
                throwable.addSuppressed(cause);
            }
        }
    }

    @Override
    public Description getDescription() {
        return description;
    }

    @Override
    public void run(RunNotifier notifier) {
        notifier.fireTestStarted(description);
        notifier.fireTestFailure(new Failure(description, throwable));
        notifier.fireTestFinished(description);
    }

    @Override
    public void sort(Sorter sorter) {
        // Nothing to do.
    }
}
