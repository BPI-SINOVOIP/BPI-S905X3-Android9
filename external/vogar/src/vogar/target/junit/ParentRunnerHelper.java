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

import org.junit.internal.AssumptionViolatedException;
import org.junit.internal.runners.model.EachTestNotifier;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runner.notification.RunNotifier;
import org.junit.runner.notification.StoppedByUserException;
import org.junit.runners.ParentRunner;
import org.junit.runners.model.Statement;

/**
 * Provides support for modifying the behaviour of {@link ParentRunner} implementations.
 */
public class ParentRunnerHelper {

    private ParentRunnerHelper() {
    }

    /**
     * Runs a {@link Statement} that represents a leaf (aka atomic) test, allowing the test or
     * rather a {@link TestRule} to abort the whole test run.
     */
    public static void abortingRunLeaf(
            Statement statement, Description description, RunNotifier notifier) {
        EachTestNotifier eachNotifier = new EachTestNotifier(notifier, description);
        eachNotifier.fireTestStarted();
        try {
            statement.evaluate();
        } catch (AssumptionViolatedException e) {
            eachNotifier.addFailedAssumption(e);
        } catch (StoppedByUserException e) {
            // Stop running any more tests.
            notifier.pleaseStop();

            // If this exception has a cause then the test failed, otherwise it passed.
            Throwable cause = e.getCause();
            if (cause != null) {
                // The test failed so treat the cause as a normal failure.
                eachNotifier.addFailure(cause);
            }

            // Propagate the exception back up to abort the test run.
            throw e;
        } catch (Throwable e) {
            eachNotifier.addFailure(e);
        } finally {
            eachNotifier.fireTestFinished();
        }
    }
}
