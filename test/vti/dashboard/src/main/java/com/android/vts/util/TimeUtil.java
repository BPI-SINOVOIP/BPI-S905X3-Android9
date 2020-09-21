/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
package com.android.vts.util;

import java.time.Instant;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.concurrent.TimeUnit;
import java.util.logging.Logger;

/** TimeUtil, a helper class for formatting times. */
public class TimeUtil {
    protected static final Logger logger = Logger.getLogger(TimeUtil.class.getName());

    protected static final String DATE_FORMAT = "yyyy-MM-dd";
    protected static final String DATE_TIME_FORMAT = "yyyy-MM-dd HH:mm:ss (z)";
    public static final ZoneId PT_ZONE = ZoneId.of("America/Los_Angeles");

    /**
     * Create a date time string from the provided timestamp.
     *
     * @param timeMicroseconds The time in microseconds
     * @return A formatted date time string.
     */
    public static String getDateTimeString(long timeMicroseconds) {
        long timeMillis = TimeUnit.MICROSECONDS.toMillis(timeMicroseconds);
        ZonedDateTime zdt = ZonedDateTime.ofInstant(Instant.ofEpochMilli(timeMillis), PT_ZONE);
        return DateTimeFormatter.ofPattern(DATE_TIME_FORMAT).format(zdt);
    }

    /**
     * Create a date string from the provided timestamp.
     *
     * @param timeMicroseconds The time in microseconds
     * @return A formatted date string.
     */
    public static String getDateString(long timeMicroseconds) {
        long timeMillis = TimeUnit.MICROSECONDS.toMillis(timeMicroseconds);
        ZonedDateTime zdt = ZonedDateTime.ofInstant(Instant.ofEpochMilli(timeMillis), PT_ZONE);
        return DateTimeFormatter.ofPattern(DATE_FORMAT).format(zdt);
    }
}
