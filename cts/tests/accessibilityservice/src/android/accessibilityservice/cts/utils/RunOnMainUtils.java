/*
 * Copyright 2018 The Android Open Source Project
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

package android.accessibilityservice.cts.utils;

import android.app.Instrumentation;

import java.util.concurrent.Callable;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Utilities to return values from {@link Instrumentation#runOnMainSync()}
 */
public class RunOnMainUtils {
    /**
     * Execute a callable on main and return its value
     */
    public static <T extends Object> T getOnMain(
            Instrumentation instrumentation, Callable<T> callable) {
        AtomicReference<T> returnValue = new AtomicReference<>(null);
        AtomicReference<Throwable> throwable = new AtomicReference<>(null);
        instrumentation.runOnMainSync(() -> {
            try {
                returnValue.set(callable.call());
            } catch (Throwable e) {
                throwable.set(e);
            }
        });
        if (throwable.get() != null) {
            throw new RuntimeException(throwable.get());
        }
        return returnValue.get();
    }
}
