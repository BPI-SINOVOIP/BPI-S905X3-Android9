/*
 * Copyright (C) 2018 Google Inc.
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

package com.android.car.settingslib.log;

import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.util.Log;

/**
 * Helper class that wraps {@link Log} to log messages to logcat. The intended use for a Logger is
 * to include one per file, using the class.getSimpleName as the prefix, like this:
 *     <pre> private static final Logger LOG = new Logger(MyClass.class); </pre>
 *
 * <p>
 * The logger will log statements in this format:
 *     mTag: [PREFIX] MESSAGE
 *
 * <p>
 * Where mTag is defined by the subclass. This helps differentiate logs while staying within the
 * 23 character limit of the log mTag.
 *
 * <p>
 * When logging verbose and debug logs, the logs should either be guarded by {@code if (LOG.isV())},
 * or a constant if (DEBUG). That DEBUG constant should be false on any submitted code.
 */
public abstract class LoggerBase {
    private final String mTag;
    private final String mPrefix;

    public LoggerBase(Class<?> cls) {
        this(cls.getSimpleName());
    }

    public LoggerBase(String prefix) {
        mTag = getTag();
        if (TextUtils.isEmpty(mTag)) {
            throw new IllegalStateException("Tag must be not null or empty");
        }
        if (mTag.length() > 23) {
            throw new IllegalStateException("Tag must be 23 characters or less");
        }
        mPrefix = "[" + prefix + "] ";
    }

    /**
     * Gets the tag that will be used in all logging calls.
     */
    @NonNull
    protected abstract String getTag();

    /**
     * Logs a {@link Log#VERBOSE} log message. Will only be logged if {@link Log#VERBOSE} is
     * loggable. This is a wrapper around {@link Log#v(String, String)}.
     *
     * @param message The message you would like logged.
     */
    public void v(String message) {
        if (isV()) {
            Log.v(mTag, mPrefix.concat(message));
        }
    }

    /**
     * Logs a {@link Log#VERBOSE} log message. Will only be logged if {@link Log#VERBOSE} is
     * loggable. This is a wrapper around {@link Log#v(String, String, Throwable)}.
     *
     * @param message The message you would like logged.
     * @param throwable An exception to log
     */
    public void v(String message, Throwable throwable) {
        if (isV()) {
            Log.v(mTag, mPrefix.concat(message), throwable);
        }
    }

    /**
     * Logs a {@link Log#DEBUG} log message. Will only be logged if {@link Log#DEBUG} is
     * loggable. This is a wrapper around {@link Log#d(String, String)}.
     *
     * @param message The message you would like logged.
     */
    public void d(String message) {
        if (isD()) {
            Log.d(mTag, mPrefix.concat(message));
        }
    }

    /**
     * Logs a {@link Log#DEBUG} log message. Will only be logged if {@link Log#DEBUG} is
     * loggable. This is a wrapper around {@link Log#d(String, String, Throwable)}.
     *
     * @param message The message you would like logged.
     * @param throwable An exception to log
     */
    public void d(String message, Throwable throwable) {
        if (isD()) {
            Log.d(mTag, mPrefix.concat(message), throwable);
        }
    }

    /**
     * Logs a {@link Log#INFO} log message. Will only be logged if {@link Log#INFO} is loggable.
     * This is a wrapper around {@link Log#i(String, String)}.
     *
     * @param message The message you would like logged.
     */
    public void i(String message) {
        if (isI()) {
            Log.i(mTag, mPrefix.concat(message));
        }
    }

    /**
     * Logs a {@link Log#INFO} log message. Will only be logged if {@link Log#INFO} is loggable.
     * This is a wrapper around {@link Log#i(String, String, Throwable)}.
     *
     * @param message The message you would like logged.
     * @param throwable An exception to log
     */
    public void i(String message, Throwable throwable) {
        if (isI()) {
            Log.i(mTag, mPrefix.concat(message), throwable);
        }
    }

    /**
     * Logs a {@link Log#WARN} log message. This is a wrapper around {@link Log#w(String, String)}.
     *
     * @param message The message you would like logged.
     */
    public void w(String message) {
        Log.w(mTag, mPrefix.concat(message));
    }

    /**
     * Logs a {@link Log#WARN} log message. This is a wrapper around
     * {@link Log#w(String, String, Throwable)}.
     *
     * @param message The message you would like logged.
     * @param throwable An exception to log
     */
    public void w(String message, Throwable throwable) {
        Log.w(mTag, mPrefix.concat(message), throwable);
    }

    /**
     * Logs a {@link Log#ERROR} log message. This is a wrapper around {@link Log#e(String, String)}.
     *
     * @param message The message you would like logged.
     */
    public void e(String message) {
        Log.e(mTag, mPrefix.concat(message));
    }

    /**
     * Logs a {@link Log#ERROR} log message. This is a wrapper around
     * {@link Log#e(String, String, Throwable)}.
     *
     * @param message The message you would like logged.
     * @param throwable An exception to log
     */
    public void e(String message, Throwable throwable) {
        Log.e(mTag, mPrefix.concat(message), throwable);
    }

    /**
     * Logs a "What a Terrible Failure" as an {@link Log#ASSERT} log message. This is a wrapper
     * around {@link Log#w(String, String)}.
     *
     * @param message The message you would like logged.
     */
    public void wtf(String message) {
        Log.wtf(mTag, mPrefix.concat(message));
    }

    /**
     * Logs a "What a Terrible Failure" as an {@link Log#ASSERT} log message. This is a wrapper
     * around {@link Log#wtf(String, String, Throwable)}.
     *
     * @param message The message you would like logged.
     * @param throwable An exception to log
     */
    public void wtf(String message, Throwable throwable) {
        Log.wtf(mTag, mPrefix.concat(message), throwable);
    }

    private boolean isV() {
        return Log.isLoggable(mTag, Log.VERBOSE);
    }

    private boolean isD() {
        return Log.isLoggable(mTag, Log.DEBUG);
    }

    private boolean isI() {
        return Log.isLoggable(mTag, Log.INFO);
    }
}
