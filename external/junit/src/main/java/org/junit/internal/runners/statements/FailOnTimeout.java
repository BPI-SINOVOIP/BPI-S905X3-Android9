package org.junit.internal.runners.statements;

import java.util.concurrent.Callable;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import org.junit.runners.model.Statement;
import org.junit.runners.model.TestTimedOutException;

public class FailOnTimeout extends Statement {
    private final Statement originalStatement;
    private final TimeUnit timeUnit;
    private final long timeout;

    /**
     * Returns a new builder for building an instance.
     *
     * @since 4.12
     */
    public static Builder builder() {
        return new Builder();
    }

    /**
     * Creates an instance wrapping the given statement with the given timeout in milliseconds.
     *
     * @param statement the statement to wrap
     * @param timeoutMillis the timeout in milliseconds
     * @deprecated use {@link #builder()} instead.
     */
    @Deprecated
    public FailOnTimeout(Statement statement, long timeoutMillis) {
        this(builder().withTimeout(timeoutMillis, TimeUnit.MILLISECONDS), statement);
    }

    private FailOnTimeout(Builder builder, Statement statement) {
        originalStatement = statement;
        timeout = builder.timeout;
        timeUnit = builder.unit;
    }

    /**
     * Builder for {@link FailOnTimeout}.
     *
     * @since 4.12
     */
    public static class Builder {
        private long timeout = 0;
        private TimeUnit unit = TimeUnit.SECONDS;

        private Builder() {
        }

        /**
         * Specifies the time to wait before timing out the test.
         *
         * <p>If this is not called, or is called with a {@code timeout} of
         * {@code 0}, the returned {@code Statement} will wait forever for the
         * test to complete, however the test will still launch from a separate
         * thread. This can be useful for disabling timeouts in environments
         * where they are dynamically set based on some property.
         *
         * @param timeout the maximum time to wait
         * @param unit the time unit of the {@code timeout} argument
         * @return {@code this} for method chaining.
         */
        public Builder withTimeout(long timeout, TimeUnit unit) {
            if (timeout < 0) {
                throw new IllegalArgumentException("timeout must be non-negative");
            }
            if (unit == null) {
                throw new NullPointerException("TimeUnit cannot be null");
            }
            this.timeout = timeout;
            this.unit = unit;
            return this;
        }

        /**
         * Builds a {@link FailOnTimeout} instance using the values in this builder,
         * wrapping the given statement.
         *
         * @param statement
         */
        public FailOnTimeout build(Statement statement) {
            if (statement == null) {
                throw new NullPointerException("statement cannot be null");
            }
            return new FailOnTimeout(this, statement);
        }
    }

    @Override
    public void evaluate() throws Throwable {
        CallableStatement callable = new CallableStatement();
        FutureTask<Throwable> task = new FutureTask<Throwable>(callable);
        Thread thread = new Thread(task, "Time-limited test");
        thread.setDaemon(true);
        thread.start();
        callable.awaitStarted();
        Throwable throwable = getResult(task, thread);
        if (throwable != null) {
            throw throwable;
        }
    }

    /**
     * Wait for the test task, returning the exception thrown by the test if the
     * test failed, an exception indicating a timeout if the test timed out, or
     * {@code null} if the test passed.
     */
    private Throwable getResult(FutureTask<Throwable> task, Thread thread) {
        try {
            if (timeout > 0) {
                return task.get(timeout, timeUnit);
            } else {
                return task.get();
            }
        } catch (InterruptedException e) {
            return e; // caller will re-throw; no need to call Thread.interrupt()
        } catch (ExecutionException e) {
            // test failed; have caller re-throw the exception thrown by the test
            return e.getCause();
        } catch (TimeoutException e) {
            return createTimeoutException(thread);
        }
    }

    private Exception createTimeoutException(Thread thread) {
        StackTraceElement[] stackTrace = thread.getStackTrace();
        Exception currThreadException = new TestTimedOutException(timeout, timeUnit);
        if (stackTrace != null) {
            currThreadException.setStackTrace(stackTrace);
            thread.interrupt();
        }
        return currThreadException;
    }

    private class CallableStatement implements Callable<Throwable> {
        private final CountDownLatch startLatch = new CountDownLatch(1);

        public Throwable call() throws Exception {
            try {
                startLatch.countDown();
                originalStatement.evaluate();
            } catch (Exception e) {
                throw e;
            } catch (Throwable e) {
                return e;
            }
            return null;
        }

        public void awaitStarted() throws InterruptedException {
            startLatch.await();
        }
    }
}
