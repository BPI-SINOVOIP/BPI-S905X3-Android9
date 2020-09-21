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
 * limitations under the License.
 */
package com.android.loganalysis.util;

import java.util.concurrent.TimeUnit;



/**
 * Utility methods for number formatting
 */
public class NumberFormattingUtil {

    private NumberFormattingUtil() {
    }

    /**
     * Convert days/hours/mins/secs/msecs into milliseconds.
     */
    public static long getMs(long days, long hours, long mins, long secs, long msecs) {
        return (((24 * days + hours) * 60 + mins) * 60 + secs) * 1000 + msecs;
    }

    /**
     * Convert hours/mins/secs/msecs into milliseconds.
     */
    public static long getMs(long hours, long mins, long secs, long msecs) {
        return getMs(0, hours, mins, secs, msecs);
    }

    /**
     * Parses a string into a long, or returns 0 if the string is null.
     *
     * @param s a {@link String} containing the long representation to be parsed
     * @return the long represented by the argument in decimal, or 0 if the string is {@code null}.
     * @throws NumberFormatException if the string is not {@code null} or does not contain a
     * parsable long.
     */
    public static long parseLongOrZero(String s) throws NumberFormatException {
        if (s == null) {
            return 0;
        }
        return Long.parseLong(s);
    }

    /**
     * Parses a string into a int, or returns 0 if the string is null.
     *
     * @param s a {@link String} containing the int representation to be parsed
     * @return the int represented by the argument in decimal, or 0 if the string is {@code null}.
     * @throws NumberFormatException if the string is not {@code null} or does not contain a
     * parsable long.
     */
    public static int parseIntOrZero(String s) throws NumberFormatException {
        if (s == null) {
            return 0;
        }
        return Integer.parseInt(s);
    }

    /**
     * Converts milliseconds to days/hours/mins/secs
     *
     * @param ms elapsed time in ms
     * @return the duration in days/hours/mins/secs
     */
    public static String getDuration(long ms) {
        if (ms <= 0) {
            return "Not a valid time";
        }
        final long days = TimeUnit.MILLISECONDS.toDays(ms);
        final long hrs = TimeUnit.MILLISECONDS.toHours(ms - TimeUnit.DAYS.toMillis(days));
        final long mins = TimeUnit.MILLISECONDS.toMinutes(ms - TimeUnit.DAYS.toMillis(days)
                - TimeUnit.HOURS.toMillis(hrs));
        final long secs = TimeUnit.MILLISECONDS.toSeconds(ms - TimeUnit.DAYS.toMillis(days)
                - TimeUnit.HOURS.toMillis(hrs) - TimeUnit.MINUTES.toMillis(mins));

        return String.format("%dd %dh %dm %ds", days, hrs, mins, secs);
    }
}

