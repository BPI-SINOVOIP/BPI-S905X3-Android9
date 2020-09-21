/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.autofillservice.cts;

import androidx.annotation.NonNull;
import android.util.Log;

import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;

/**
 * Rule used to safely run clean up code after a test is finished, so that exceptions thrown by
 * the cleanup code don't hide exception thrown by the test body
 */
// TODO: move to common CTS code
public final class SafeCleanerRule implements TestRule {

    private static final String TAG = "SafeCleanerRule";

    private final List<Runnable> mCleaners = new ArrayList<>();
    private final List<Callable<List<Throwable>>> mExtraThrowables = new ArrayList<>();
    private final List<Throwable> mThrowables = new ArrayList<>();
    private Dumper mDumper;

    /**
     * Runs {@code cleaner} after the test is finished, catching any {@link Throwable} thrown by it.
     */
    public SafeCleanerRule run(@NonNull Runnable cleaner) {
        mCleaners.add(cleaner);
        return this;
    }

    /**
     * Adds exceptions directly.
     *
     * <p>Typically used when exceptions were caught asychronously during the test execution.
     */
    public SafeCleanerRule add(@NonNull Callable<List<Throwable>> exceptions) {
        mExtraThrowables.add(exceptions);
        return this;
    }

    /**
     * Sets a {@link Dumper} used to log errors.
     */
    public SafeCleanerRule setDumper(@NonNull Dumper dumper) {
        mDumper = dumper;
        return this;
    }

    @Override
    public Statement apply(Statement base, Description description) {
        return new Statement() {
            @Override
            public void evaluate() throws Throwable {
                // First run the test
                try {
                    base.evaluate();
                } catch (Throwable t) {
                    Log.w(TAG, "Adding exception from main test");
                    mThrowables.add(t);
                }

                // Then the cleanup runners
                for (Runnable runner : mCleaners) {
                    try {
                        runner.run();
                    } catch (Throwable t) {
                        Log.w(TAG, "Adding exception from cleaner");
                        mThrowables.add(t);
                    }
                }

                // And finally add the extra exceptions
                for (Callable<List<Throwable>> extraThrowablesCallable : mExtraThrowables) {
                    final List<Throwable> extraThrowables = extraThrowablesCallable.call();
                    if (extraThrowables != null) {
                        Log.w(TAG, "Adding " + extraThrowables.size() + " extra exceptions");
                        mThrowables.addAll(extraThrowables);
                    }
                }

                // Finally, throw up!
                if (mThrowables.isEmpty()) return;

                final int numberExceptions = mThrowables.size();
                if (numberExceptions == 1) {
                    fail(description, mThrowables.get(0));
                }
                fail(description, new MultipleExceptions(mThrowables));
            }

        };
    }

    private void fail(Description description, Throwable t) throws Throwable {
        if (mDumper != null) {
            mDumper.dump(description.getDisplayName(), t);
        }
        throw t;
    }

    private static String toMesssage(List<Throwable> throwables) {
        String msg = "D'OH!";
        try {
            try (StringWriter sw = new StringWriter(); PrintWriter pw = new PrintWriter(sw)) {
                sw.write("Caught " + throwables.size() + " exceptions\n");
                for (int i = 0; i < throwables.size(); i++) {
                    sw.write("\n---- Begin of exception #" + (i + 1) + " ----\n");
                    final Throwable exception = throwables.get(i);
                    exception.printStackTrace(pw);
                    sw.write("---- End of exception #" + (i + 1) + " ----\n\n");
                }
                msg = sw.toString();
            }
        } catch (IOException e) {
            // ignore close() errors - should not happen...
            Log.e(TAG, "Exception closing StringWriter: " + e);
        }
        return msg;
    }

    // VisibleForTesting
    static class MultipleExceptions extends AssertionError {
        private final List<Throwable> mThrowables;

        private MultipleExceptions(List<Throwable> throwables) {
            super(toMesssage(throwables));

            this.mThrowables = throwables;
        }

        List<Throwable> getThrowables() {
            return mThrowables;
        }
    }

    /**
     * Optional interface used to dump an error.
     */
    public interface Dumper {

        /**
         * Dumps an error.
         */
        void dump(@NonNull String testName, @NonNull Throwable t);
    }
}
