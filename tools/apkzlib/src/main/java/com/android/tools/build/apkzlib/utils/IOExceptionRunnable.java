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

package com.android.tools.build.apkzlib.utils;

import java.io.IOException;
import java.io.UncheckedIOException;
import javax.annotation.Nonnull;

/**
 * Runnable that can throw I/O exceptions.
 */
@FunctionalInterface
public interface IOExceptionRunnable {

    /**
     * Runs the runnable.
     *
     * @throws IOException failed to run
     */
    void run() throws IOException;

    /**
     * Wraps a runnable that may throw an IO Exception throwing an {@code UncheckedIOException}.
     *
     * @param r the runnable
     */
    @Nonnull
    public static Runnable asRunnable(@Nonnull IOExceptionRunnable r) {
        return () -> {
            try {
                r.run();
            } catch (IOException e) {
                throw new UncheckedIOException(e);
            }
        };
    }
}
