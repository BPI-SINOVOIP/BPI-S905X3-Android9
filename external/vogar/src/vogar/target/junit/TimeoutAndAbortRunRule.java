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

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;
import vogar.util.Threads;

/**
 * Times a test out and then aborts the test run.
 */
public class TimeoutAndAbortRunRule implements TestRule {

    private final ExecutorService executor = Executors.newCachedThreadPool(
            Threads.daemonThreadFactory(getClass().getName()));

    private final int timeoutSeconds;

    /**
     * @param timeoutSeconds the timeout in seconds, if 0 then never times out.
     */
    public TimeoutAndAbortRunRule(int timeoutSeconds) {
        this.timeoutSeconds = timeoutSeconds;
    }

    @Override
    public Statement apply(final Statement base, Description description) {
        return new Statement() {
            @Override
            public void evaluate() throws Throwable {
                runWithTimeout(base);
            }
        };
    }

    /**
     * Runs the test on another thread. If the test completes before the
     * timeout, this reports the result normally. But if the test times out,
     * this reports the timeout stack trace and begins the process of killing
     * this no-longer-trustworthy process.
     */
    private void runWithTimeout(final Statement base) throws Throwable {
        // Start the test on a background thread.
        final AtomicReference<Thread> executingThreadReference = new AtomicReference<>();
        Future<Throwable> result = executor.submit(new Callable<Throwable>() {
            public Throwable call() throws Exception {
                executingThreadReference.set(Thread.currentThread());
                try {
                    base.evaluate();
                    return null;
                } catch (Throwable throwable) {
                    return throwable;
                }
            }
        });

        // Wait until either the result arrives or the test times out.
        Throwable thrown;
        try {
            thrown = getThrowable(result);
        } catch (TimeoutException e) {
            Thread executingThread = executingThreadReference.get();
            if (executingThread != null) {
                executingThread.interrupt();
                e.setStackTrace(executingThread.getStackTrace());
            }
            // Wrap it in an exception that will cause the current run to be aborted.
            thrown = new VmIsUnstableException(e);
        }

        if (thrown != null) {
            throw thrown;
        }
    }

    @SuppressWarnings("ThrowableResultOfMethodCallIgnored")
    private Throwable getThrowable(Future<Throwable> result)
            throws InterruptedException, ExecutionException, TimeoutException {
        Throwable thrown;
        thrown = timeoutSeconds == 0
                ? result.get()
                : result.get(timeoutSeconds, TimeUnit.SECONDS);
        return thrown;
    }
}

