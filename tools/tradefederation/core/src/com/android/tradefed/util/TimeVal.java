/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tradefed.util;

import com.google.common.math.LongMath;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * This is a sentinel type which wraps a {@code Long}.  It exists solely as a hint to the
 * options parsing machinery that a particular value should be parsed as if it were a string
 * representing a time value.
 */
@SuppressWarnings("serial")
public class TimeVal extends Number implements Comparable<Long> {
    private static final Pattern TIME_PATTERN =
            Pattern.compile("(?i)" +  // case insensitive
                    "(?:(?<d>\\d+)d)?" +  // a number followed by "d"
                    "(?:(?<h>\\d+)h)?" +
                    "(?:(?<m>\\d+)m)?" +
                    "(?:(?<s>\\d+)s)?" +
                    "(?:(?<ms>\\d+)(?:ms)?)?");  // a number followed by "ms"

    private Long mValue = null;

    /**
     * Constructs a newly allocated TimeVal object that represents the specified Long argument
     */
    public TimeVal(Long value) {
        mValue = value;
    }

    /**
     * Constructs a newly allocated TimeVal object that represents the <emph>timestamp</emph>
     * indicated by the String parameter.  The string is converted to a TimeVal in exactly the
     * manner used by the {@link #fromString(String)} method.
     */
    public TimeVal(String value) throws NumberFormatException {
        mValue = fromString(value);
    }

    /**
     * @return the wrapped {@code Long} value.
     */
    public Long asLong() {
        return mValue;
    }

    /**
     * Parses the string as a hierarchical time value
     * <p />
     * The default unit is millis.  The parser will accept {@code s} for seconds (1000 millis),
     * {@code m} for minutes (60 seconds), {@code h} for hours (60 minutes), or {@code d} for days
     * (24 hours).
     * <p />
     * Units may be mixed and matched, so long as each unit appears at most once, and so long as
     * all units which do appear are listed in decreasing order of scale.  So, for instance,
     * {@code h} may only appear before {@code m}, and may only appear after {@code d}.  As a
     * specific example, "1d2h3m4s5ms" would be a valid time value, as would "4" or "4ms".  All
     * embedded whitespace is discarded.
     * <p />
     * Do note that this method rejects overflows.  So the output number is guaranteed to be
     * non-negative, and to fit within the {@code long} type.
     */
    public static long fromString(String value) throws NumberFormatException {
        if (value == null) throw new NumberFormatException("value is null");

        try {
            value = value.replaceAll("\\s+", "");
            Matcher m = TIME_PATTERN.matcher(value);
            if (m.matches()) {
                // This works by, essentially, modifying the units of timeValue, from the
                // largest supported unit, until we've dropped down to millis.
                long timeValue = 0;
                timeValue = val(m.group("d"));

                // 1 day == 24 hours
                timeValue = LongMath.checkedMultiply(timeValue, 24);
                timeValue = LongMath.checkedAdd(timeValue, val(m.group("h")));

                // 1 hour == 60 minutes
                timeValue = LongMath.checkedMultiply(timeValue, 60);
                timeValue = LongMath.checkedAdd(timeValue, val(m.group("m")));

                // 1 hour == 60 seconds
                timeValue = LongMath.checkedMultiply(timeValue, 60);
                timeValue = LongMath.checkedAdd(timeValue, val(m.group("s")));

                // 1 second == 1000 millis
                timeValue = LongMath.checkedMultiply(timeValue, 1000);
                timeValue = LongMath.checkedAdd(timeValue, val(m.group("ms")));

                return timeValue;
            }
        } catch (ArithmeticException e) {
            throw new NumberFormatException(String.format(
                    "Failed to parse value %s as a time value: %s", value, e.getMessage()));
        }

        throw new NumberFormatException(
                String.format("Failed to parse value %s as a time value", value));
    }

    static long val(String str) throws NumberFormatException {
        if (str == null) return 0;

        Long value = Long.parseLong(str);
        return value;
    }


    // implementing interfaces
    /**
     * {@inheritDoc}
     */
    @Override
    public double doubleValue() {
        return mValue.doubleValue();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public float floatValue() {
        return mValue.floatValue();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int intValue() {
        return mValue.intValue();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public long longValue() {
        return mValue.longValue();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int compareTo(Long other) {
        return mValue.compareTo(other);
    }
}
