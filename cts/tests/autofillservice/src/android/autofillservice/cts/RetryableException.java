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

import androidx.annotation.Nullable;

/**
 * Exception that cause the {@link RetryRule} to re-try a test.
 */
public class RetryableException extends RuntimeException {

    @Nullable
    private final Timeout mTimeout;

    public RetryableException(String msg) {
        this((Timeout) null, msg);
    }

    public RetryableException(String format, Object...args) {
        this((Timeout) null, String.format(format, args));
    }

    public RetryableException(Throwable cause, String format, Object...args) {
        this((Timeout) null, cause, String.format(format, args), cause);
    }

    public RetryableException(@Nullable Timeout timeout, String msg) {
        super(msg);
        this.mTimeout = timeout;
    }

    public RetryableException(@Nullable Timeout timeout, String format, Object...args) {
        super(String.format(format, args));
        this.mTimeout = timeout;
    }

    public RetryableException(@Nullable Timeout timeout, Throwable cause, String format,
            Object...args) {
        super(String.format(format, args), cause);
        this.mTimeout = timeout;
    }

    @Nullable
    public Timeout getTimeout() {
        return mTimeout;
    }

    @Override
    public String getMessage() {
        final String superMessage = super.getMessage();
        return mTimeout == null ? superMessage : superMessage + " (timeout=" + mTimeout + ")";
    }
}
