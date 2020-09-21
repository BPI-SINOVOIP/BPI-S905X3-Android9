/*
 * Copyright (C) 2011 The Guava Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.common.primitives;

import com.google.common.annotations.GwtCompatible;
import com.google.common.collect.testing.Helpers;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

/**
 * Tests for UnsignedInts
 *
 * @author Louis Wasserman
 */
@GwtCompatible(emulated = true)
public class UnsignedIntsTest extends TestCase {
  private static final long[] UNSIGNED_INTS = {
      0L,
      1L,
      2L,
      3L,
      0x12345678L,
      0x5a4316b8L,
      0x6cf78a4bL,
      0xff1a618bL,
      0xfffffffdL,
      0xfffffffeL,
      0xffffffffL};
  
  private static final int LEAST = (int) 0L;
  private static final int GREATEST = (int) 0xffffffffL;

  public void testToLong() {
    for (long a : UNSIGNED_INTS) {
      assertEquals(a, UnsignedInts.toLong((int) a));
    }
  }

  public void testCompare() {
    for (long a : UNSIGNED_INTS) {
      for (long b : UNSIGNED_INTS) {
        int cmpAsLongs = Longs.compare(a, b);
        int cmpAsUInt = UnsignedInts.compare((int) a, (int) b);
        assertEquals(Integer.signum(cmpAsLongs), Integer.signum(cmpAsUInt));
      }
    }
  }
  
  public void testMax_noArgs() {
    try {
      UnsignedInts.max();
      fail();
    } catch (IllegalArgumentException expected) {
    }
  }
  
  public void testMax() {
    assertEquals(LEAST, UnsignedInts.max(LEAST));
    assertEquals(GREATEST, UnsignedInts.max(GREATEST));
    assertEquals((int) 0xff1a618bL, UnsignedInts.max(
        (int) 8L, (int) 6L, (int) 7L,
        (int) 0x12345678L, (int) 0x5a4316b8L,
        (int) 0xff1a618bL, (int) 0L));
  }
  
  public void testMin_noArgs() {
    try {
      UnsignedInts.min();
      fail();
    } catch (IllegalArgumentException expected) {
    }
  }
  
  public void testMin() {
    assertEquals(LEAST, UnsignedInts.min(LEAST));
    assertEquals(GREATEST, UnsignedInts.min(GREATEST));
    assertEquals((int) 0L, UnsignedInts.min(
        (int) 8L, (int) 6L, (int) 7L,
        (int) 0x12345678L, (int) 0x5a4316b8L,
        (int) 0xff1a618bL, (int) 0L));
  }
  
  public void testLexicographicalComparator() {
    List<int[]> ordered = Arrays.asList(
        new int[] {},
        new int[] {LEAST},
        new int[] {LEAST, LEAST},
        new int[] {LEAST, (int) 1L},
        new int[] {(int) 1L},
        new int[] {(int) 1L, LEAST},
        new int[] {GREATEST, (GREATEST - (int) 1L)},
        new int[] {GREATEST, GREATEST},
        new int[] {GREATEST, GREATEST, GREATEST}
        );

    Comparator<int[]> comparator = UnsignedInts.lexicographicalComparator();
    Helpers.testComparator(comparator, ordered);
  }

  public void testDivide() {
    for (long a : UNSIGNED_INTS) {
      for (long b : UNSIGNED_INTS) {
        try {
          assertEquals((int) (a / b), UnsignedInts.divide((int) a, (int) b));
          assertFalse(b == 0);
        } catch (ArithmeticException e) {
          assertEquals(0, b);
        }
      }
    }
  }

  public void testRemainder() {
    for (long a : UNSIGNED_INTS) {
      for (long b : UNSIGNED_INTS) {
        try {
          assertEquals((int) (a % b), UnsignedInts.remainder((int) a, (int) b));
          assertFalse(b == 0);
        } catch (ArithmeticException e) {
          assertEquals(0, b);
        }
      }
    }
  }

  public void testParseInt() {
    try {
      for (long a : UNSIGNED_INTS) {
        assertEquals((int) a, UnsignedInts.parseUnsignedInt(Long.toString(a)));
      }
    } catch (NumberFormatException e) {
      fail(e.getMessage());
    }

    try {
      UnsignedInts.parseUnsignedInt(Long.toString(1L << 32));
      fail("Expected NumberFormatException");
    } catch (NumberFormatException expected) {}
  }

  public void testParseIntWithRadix() throws NumberFormatException {
    for (long a : UNSIGNED_INTS) {
      for (int radix = Character.MIN_RADIX; radix <= Character.MAX_RADIX; radix++) {
        assertEquals((int) a, UnsignedInts.parseUnsignedInt(Long.toString(a, radix), radix));
      }
    }

    // loops through all legal radix values.
    for (int radix = Character.MIN_RADIX; radix <= Character.MAX_RADIX; radix++) {
      // tests can successfully parse a number string with this radix.
      String maxAsString = Long.toString((1L << 32) - 1, radix);
      assertEquals(-1, UnsignedInts.parseUnsignedInt(maxAsString, radix));

      try {
        // tests that we get exception whre an overflow would occur.
        long overflow = 1L << 32;
        String overflowAsString = Long.toString(overflow, radix);
        UnsignedInts.parseUnsignedInt(overflowAsString, radix);
        fail();
      } catch (NumberFormatException expected) {}
    }
  }

  public void testParseIntThrowsExceptionForInvalidRadix() {
    // Valid radix values are Character.MIN_RADIX to Character.MAX_RADIX,
    // inclusive.
    try {
      UnsignedInts.parseUnsignedInt("0", Character.MIN_RADIX - 1);
      fail();
    } catch (NumberFormatException expected) {}

    try {
      UnsignedInts.parseUnsignedInt("0", Character.MAX_RADIX + 1);
      fail();
    } catch (NumberFormatException expected) {}

    // The radix is used as an array index, so try a negative value.
    try {
      UnsignedInts.parseUnsignedInt("0", -1);
      fail();
    } catch (NumberFormatException expected) {}
  }

  public void testDecodeInt() {
    assertEquals(0xffffffff, UnsignedInts.decode("0xffffffff"));
    assertEquals(01234567, UnsignedInts.decode("01234567")); // octal
    assertEquals(0x12345678, UnsignedInts.decode("#12345678"));
    assertEquals(76543210, UnsignedInts.decode("76543210"));
    assertEquals(0x13579135, UnsignedInts.decode("0x13579135"));
    assertEquals(0x13579135, UnsignedInts.decode("0X13579135"));
    assertEquals(0, UnsignedInts.decode("0"));
  }

  public void testDecodeIntFails() {
    try {
      // One more than maximum value
      UnsignedInts.decode("0xfffffffff");
      fail();
    } catch (NumberFormatException expected) {
    }

    try {
      UnsignedInts.decode("-5");
      fail();
    } catch (NumberFormatException expected) {
    }

    try {
      UnsignedInts.decode("-0x5");
      fail();
    } catch (NumberFormatException expected) {
    }

    try {
      UnsignedInts.decode("-05");
      fail();
    } catch (NumberFormatException expected) {
    }
  }

  public void testToString() {
    int[] bases = {2, 5, 7, 8, 10, 16};
    for (long a : UNSIGNED_INTS) {
      for (int base : bases) {
        assertEquals(UnsignedInts.toString((int) a, base), Long.toString(a, base));
      }
    }
  }

  public void testJoin() {
    assertEquals("", join());
    assertEquals("1", join(1));
    assertEquals("1,2", join(1, 2));
    assertEquals("4294967295,2147483648", join(-1, Integer.MIN_VALUE));

    assertEquals("123", UnsignedInts.join("", 1, 2, 3));
  }

  private static String join(int... values) {
    return UnsignedInts.join(",", values);
  }
}

