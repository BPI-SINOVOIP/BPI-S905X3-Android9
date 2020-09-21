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
package com.android.tradefed.util;

import static org.junit.Assert.*;

import org.junit.Test;

/**
 * Unit Tests for {@link TimeUtil}
 */
public class TimeUtilTest {

    @Test
    public void testFormatElapsedTime() {
        assertEquals("1 ms", TimeUtil.formatElapsedTime(1));
        assertEquals("999 ms", TimeUtil.formatElapsedTime(999));
        assertEquals("1s", TimeUtil.formatElapsedTime(1001));
        assertEquals("1m 1s", TimeUtil.formatElapsedTime(61001));
        assertEquals("1h 1m 0s", TimeUtil.formatElapsedTime(60 * 60 * 1000 + 60 * 1000));
    }

    @Test
    public void testFormatTimeStampGMT() {
        assertEquals("2016-10-25 13:31:58", TimeUtil.formatTimeStampGMT(1477402318000L));
    }
}
