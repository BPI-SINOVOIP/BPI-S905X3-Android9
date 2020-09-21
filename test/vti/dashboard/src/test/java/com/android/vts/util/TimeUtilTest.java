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

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class TimeUtilTest {

    /** Test that date/time strings are formatted correctly. */
    @Test
    public void testFormatDateTime() {
        long time = 1504286976352052l;
        String expected = "2017-09-01 10:29:36 (PDT)";
        String timeString = TimeUtil.getDateTimeString(time);
        assertEquals(expected, timeString);
    }

    /** Test that date strings are formatted correctly. */
    @Test
    public void testFormatDate() {
        long time = 1504248634455491l;
        String expected = "2017-08-31";
        String timeString = TimeUtil.getDateString(time);
        assertEquals(expected, timeString);
    }
}
