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

package android.view.inputmethod.cts.util;

import android.app.Instrumentation;
import androidx.annotation.NonNull;
import android.support.test.InstrumentationRegistry;

import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BooleanSupplier;
import java.util.function.Supplier;

public final class TestUtils {
    private static final long TIME_SLICE = 100;  // msec

    /**
     * Executes a call on the application's main thread, blocking until it is complete.
     *
     * <p>A simple wrapper for {@link Instrumentation#runOnMainSync(Runnable)}.</p>
     *
     * @param task task to be called on the UI thread
     */
    public static void runOnMainSync(@NonNull Runnable task) {
        InstrumentationRegistry.getInstrumentation().runOnMainSync(task);
    }

    /**
     * Retrieves a value that needs to be obtained on the main thread.
     *
     * <p>A simple utility method that helps to return an object from the UI thread.</p>
     *
     * @param supplier callback to be called on the UI thread to return a value
     * @param <T> Type of the value to be returned
     * @return Value returned from {@code supplier}
     */
    public static <T> T getOnMainSync(@NonNull Supplier<T> supplier) {
        final AtomicReference<T> result = new AtomicReference<>();
        final Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        instrumentation.runOnMainSync(() -> result.set(supplier.get()));
        return result.get();
    }

    /**
     * Does polling loop on the UI thread to wait until the given condition is met.
     *
     * @param condition Condition to be satisfied. This is guaranteed to run on the UI thread.
     * @param timeout timeout in millisecond
     * @param message message to display when timeout occurs.
     * @throws TimeoutException when the no event is matched to the given condition within
     *                          {@code timeout}
     */
    public static void waitOnMainUntil(
            @NonNull BooleanSupplier condition, long timeout, String message)
            throws TimeoutException {
        final AtomicBoolean result = new AtomicBoolean();

        final Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        while (!result.get()) {
            if (timeout < 0) {
                throw new TimeoutException(message);
            }
            instrumentation.runOnMainSync(() -> {
                if (condition.getAsBoolean()) {
                    result.set(true);
                }
            });
            try {
                Thread.sleep(TIME_SLICE);
            } catch (InterruptedException e) {
                throw new IllegalStateException(e);
            }
            timeout -= TIME_SLICE;
        }
    }

    /**
     * Does polling loop on the UI thread to wait until the given condition is met.
     *
     * @param condition Condition to be satisfied. This is guaranteed to run on the UI thread.
     * @param timeout timeout in millisecond
     * @throws TimeoutException when the no event is matched to the given condition within
     *                          {@code timeout}
     */
    public static void waitOnMainUntil(@NonNull BooleanSupplier condition, long timeout)
            throws TimeoutException {
        waitOnMainUntil(condition, timeout, "");
    }
}
