/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tv.common;

import android.content.Context;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;
import com.android.tv.common.feature.Feature;
import com.android.tv.common.util.CommonUtils;

/**
 * Simple static methods to be called at the start of your own methods to verify correct arguments
 * and state.
 *
 * <p>{@code checkXXX} methods throw exceptions when {@link BuildConfig#ENG} is true, and logs a
 * warning when it is false.
 *
 * <p>This is based on com.android.internal.util.Preconditions.
 */
public final class SoftPreconditions {
    private static final String TAG = "SoftPreconditions";

    /**
     * Throws or logs if an expression involving the parameter of the calling method is not true.
     *
     * @param expression a boolean expression
     * @param tag Used to identify the source of a log message. It usually identifies the class or
     *     activity where the log call occurs.
     * @param errorMessageTemplate a template for the exception message should the check fail. The
     *     message is formed by replacing each {@code %s} placeholder in the template with an
     *     argument. These are matched by position - the first {@code %s} gets {@code
     *     errorMessageArgs[0]}, etc. Unmatched arguments will be appended to the formatted message
     *     in square braces. Unmatched placeholders will be left as-is.
     * @param errorMessageArgs the arguments to be substituted into the message template. Arguments
     *     are converted to strings using {@link String#valueOf(Object)}.
     * @return the evaluation result of the boolean expression
     * @throws IllegalArgumentException if {@code expression} is true
     */
    public static boolean checkArgument(
            final boolean expression,
            String tag,
            @Nullable String errorMessageTemplate,
            @Nullable Object... errorMessageArgs) {
        if (!expression) {
            String msg = format(errorMessageTemplate, errorMessageArgs);
            warn(tag, "Illegal argument", new IllegalArgumentException(msg), msg);
        }
        return expression;
    }

    /**
     * Throws or logs if an expression involving the parameter of the calling method is not true.
     *
     * @param expression a boolean expression
     * @return the evaluation result of the boolean expression
     * @throws IllegalArgumentException if {@code expression} is true
     */
    public static boolean checkArgument(final boolean expression) {
        checkArgument(expression, null, null);
        return expression;
    }

    /**
     * Throws or logs if an and object is null.
     *
     * @param reference an object reference
     * @param tag Used to identify the source of a log message. It usually identifies the class or
     *     activity where the log call occurs.
     * @param errorMessageTemplate a template for the exception message should the check fail. The
     *     message is formed by replacing each {@code %s} placeholder in the template with an
     *     argument. These are matched by position - the first {@code %s} gets {@code
     *     errorMessageArgs[0]}, etc. Unmatched arguments will be appended to the formatted message
     *     in square braces. Unmatched placeholders will be left as-is.
     * @param errorMessageArgs the arguments to be substituted into the message template. Arguments
     *     are converted to strings using {@link String#valueOf(Object)}.
     * @return true if the object is null
     * @throws NullPointerException if {@code reference} is null
     */
    public static <T> T checkNotNull(
            final T reference,
            String tag,
            @Nullable String errorMessageTemplate,
            @Nullable Object... errorMessageArgs) {
        if (reference == null) {
            String msg = format(errorMessageTemplate, errorMessageArgs);
            warn(tag, "Null Pointer", new NullPointerException(msg), msg);
        }
        return reference;
    }

    /**
     * Throws or logs if an and object is null.
     *
     * @param reference an object reference
     * @return true if the object is null
     * @throws NullPointerException if {@code reference} is null
     */
    public static <T> T checkNotNull(final T reference) {
        return checkNotNull(reference, null, null);
    }

    /**
     * Throws or logs if an expression involving the state of the calling instance, but not
     * involving any parameters to the calling method is not true.
     *
     * @param expression a boolean expression
     * @param tag Used to identify the source of a log message. It usually identifies the class or
     *     activity where the log call occurs.
     * @param errorMessageTemplate a template for the exception message should the check fail. The
     *     message is formed by replacing each {@code %s} placeholder in the template with an
     *     argument. These are matched by position - the first {@code %s} gets {@code
     *     errorMessageArgs[0]}, etc. Unmatched arguments will be appended to the formatted message
     *     in square braces. Unmatched placeholders will be left as-is.
     * @param errorMessageArgs the arguments to be substituted into the message template. Arguments
     *     are converted to strings using {@link String#valueOf(Object)}.
     * @return the evaluation result of the boolean expression
     * @throws IllegalStateException if {@code expression} is true
     */
    public static boolean checkState(
            final boolean expression,
            String tag,
            @Nullable String errorMessageTemplate,
            @Nullable Object... errorMessageArgs) {
        if (!expression) {
            String msg = format(errorMessageTemplate, errorMessageArgs);
            warn(tag, "Illegal State", new IllegalStateException(msg), msg);
        }
        return expression;
    }

    /**
     * Throws or logs if an expression involving the state of the calling instance, but not
     * involving any parameters to the calling method is not true.
     *
     * @param expression a boolean expression
     * @return the evaluation result of the boolean expression
     * @throws IllegalStateException if {@code expression} is true
     */
    public static boolean checkState(final boolean expression) {
        checkState(expression, null, null);
        return expression;
    }

    /**
     * Throws or logs if the Feature is not enabled
     *
     * @param context an android context
     * @param feature the required feature
     * @param tag used to identify the source of a log message. It usually identifies the class or
     *     activity where the log call occurs
     * @throws IllegalStateException if {@code feature} is not enabled
     */
    public static void checkFeatureEnabled(Context context, Feature feature, String tag) {
        checkState(feature.isEnabled(context), tag, feature.toString());
    }

    /**
     * Throws a {@link RuntimeException} if {@link BuildConfig#ENG} is true and not running in a
     * test, else log a warning.
     *
     * @param tag Used to identify the source of a log message. It usually identifies the class or
     *     activity where the log call occurs.
     * @param e The exception to wrap with a RuntimeException when thrown.
     * @param msg The message to be logged
     */
    public static void warn(String tag, String prefix, Exception e, String msg)
            throws RuntimeException {
        if (TextUtils.isEmpty(tag)) {
            tag = TAG;
        }
        String logMessage;
        if (TextUtils.isEmpty(msg)) {
            logMessage = prefix;
        } else if (TextUtils.isEmpty(prefix)) {
            logMessage = msg;
        } else {
            logMessage = prefix + ": " + msg;
        }

        if (BuildConfig.ENG && !CommonUtils.isRunningInTest()) {
            throw new RuntimeException(msg, e);
        } else {
            Log.w(tag, logMessage, e);
        }
    }

    /**
     * Substitutes each {@code %s} in {@code template} with an argument. These are matched by
     * position: the first {@code %s} gets {@code args[0]}, etc. If there are more arguments than
     * placeholders, the unmatched arguments will be appended to the end of the formatted message in
     * square braces.
     *
     * @param template a string containing 0 or more {@code %s} placeholders. null is treated as
     *     "null".
     * @param args the arguments to be substituted into the message template. Arguments are
     *     converted to strings using {@link String#valueOf(Object)}. Arguments can be null.
     */
    static String format(@Nullable String template, @Nullable Object... args) {
        template = String.valueOf(template); // null -> "null"

        args = args == null ? new Object[] {"(Object[])null"} : args;

        // start substituting the arguments into the '%s' placeholders
        StringBuilder builder = new StringBuilder(template.length() + 16 * args.length);
        int templateStart = 0;
        int i = 0;
        while (i < args.length) {
            int placeholderStart = template.indexOf("%s", templateStart);
            if (placeholderStart == -1) {
                break;
            }
            builder.append(template, templateStart, placeholderStart);
            builder.append(args[i++]);
            templateStart = placeholderStart + 2;
        }
        builder.append(template, templateStart, template.length());

        // if we run out of placeholders, append the extra args in square braces
        if (i < args.length) {
            builder.append(" [");
            builder.append(args[i++]);
            while (i < args.length) {
                builder.append(", ");
                builder.append(args[i++]);
            }
            builder.append(']');
        }

        return builder.toString();
    }

    private SoftPreconditions() {}
}
