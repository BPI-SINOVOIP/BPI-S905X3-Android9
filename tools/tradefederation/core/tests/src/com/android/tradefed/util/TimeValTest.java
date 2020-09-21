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

import junit.framework.TestCase;

/**
 * Unit tests for {@link TimeVal}.
 */
public class TimeValTest extends TestCase {

    public void testBasic() {
        assertEquals(0, TimeVal.fromString("0"));
        assertEquals(1, TimeVal.fromString("1"));

        assertEquals(1000, TimeVal.fromString("1000"));
        assertEquals(1000, TimeVal.fromString("1000ms"));
        assertEquals(1000, TimeVal.fromString("1000Ms"));
        assertEquals(1000, TimeVal.fromString("1000mS"));
        assertEquals(1000, TimeVal.fromString("1000MS"));

        assertEquals(1000, TimeVal.fromString("1s"));
        assertEquals(1000, TimeVal.fromString("1S"));

        assertEquals(1000 * 60, TimeVal.fromString("1m"));
        assertEquals(1000 * 60, TimeVal.fromString("1M"));

        assertEquals(1000 * 3600, TimeVal.fromString("1h"));
        assertEquals(1000 * 3600, TimeVal.fromString("1H"));

        assertEquals(1000 * 86400, TimeVal.fromString("1d"));
        assertEquals(1000 * 86400, TimeVal.fromString("1D"));
    }

    public void testComposition() {
        assertEquals(1303, TimeVal.fromString("1s303"));
        assertEquals(1000 * 60 + 303, TimeVal.fromString("1m303"));
        assertEquals(1000 * 3600 + 303, TimeVal.fromString("1h303ms"));
        assertEquals(1000 * 86400 + 303, TimeVal.fromString("1d303MS"));

        assertEquals(4 * 1000 * 86400 + 5 * 1000 * 3600 + 303, TimeVal.fromString("4D5h303mS"));
        assertEquals(5 + 1000 * (4 + 60 * (3 + 60 * (2 + 24 * 1))),
                TimeVal.fromString("1d2h3m4s5ms"));
    }

    public void testWhitespace() {
        assertEquals(1, TimeVal.fromString("1 ms"));
        assertEquals(1, TimeVal.fromString("  1  ms  "));

        assertEquals(1002, TimeVal.fromString("1s2"));
        assertEquals(1002, TimeVal.fromString("1s2 ms"));
        assertEquals(1002, TimeVal.fromString(" 1s2ms"));
        assertEquals(1002, TimeVal.fromString("1s 2 ms"));
        // This is non-ideal, but is a side-effect of discarding all whitespace prior to parsing
        assertEquals(3 * 1000 * 60 + 1002, TimeVal.fromString(" 3 m 1 s 2 m s"));

        assertEquals(5 + 1000 * (4 + 60 * (3 + 60 * (2 + 24 * 1))),
                TimeVal.fromString("1d 2h 3m 4s 5ms"));
    }

    public void testInvalid() {
        assertInvalid("-1");
        assertInvalid("1m1h");
        assertInvalid("+5");
    }

    /**
     * Because of TimeVal's multiplication features, it can be easier for a user to unexpectedly
     * trigger an overflow without realizing that their value has become quite so large.  Here,
     * we verify that various kinds of overflows and ensure that we detect them.
     */
    public void testOverflow() {
        // 2**63 - 1
        assertEquals(Long.MAX_VALUE, TimeVal.fromString("9223372036854775807"));
        // 2**63
        assertInvalid("9223372036854775808");

        // (2**63 - 1).divmod(1000 * 86400) = [106751991167, 25975807] (days, msecs)
        // This should be the greatest number of days that does not overflow
        assertEquals(106751991167L * 1000L * 86400L, TimeVal.fromString("106751991167d"));
        // This should be 2**63 - 1
        assertEquals(Long.MAX_VALUE, TimeVal.fromString("106751991167d 25975807ms"));
        // Adding 1 more ms should cause an overflow.
        assertInvalid("106751991167d 25975808ms");

        // 2**64 + 1 should be a positive value after an overflow.  Make sure we can still detect
        // this non-negative overflow
        long l = 1<<62;
        l *= 2;
        l *= 2;
        l += 1;
        assertTrue(l > 0);
        // 2**64 + 1 == 18446744073709551617
        assertInvalid("18446744073709551617ms");
    }

    private void assertInvalid(String input) {
        try {
            final long val = TimeVal.fromString(input);
            fail(String.format("Did not reject input: %s.  Produced value: %d", input, val));
        } catch (NumberFormatException e) {
            // expected
        }
    }
}
