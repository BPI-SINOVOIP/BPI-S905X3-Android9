/*
 * Copyright (C) 2008 The Guava Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.common.primitives;

import com.google.common.annotations.GwtCompatible;
import com.google.common.base.Converter;
import com.google.common.collect.testing.Helpers;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * Unit test for {@link Ints}.
 *
 * @author Kevin Bourrillion
 */
@GwtCompatible(emulated = true)
@SuppressWarnings("cast") // redundant casts are intentional and harmless
public class IntsTest extends TestCase {
  private static final int[] EMPTY = {};
  private static final int[] ARRAY1 = {(int) 1};
  private static final int[] ARRAY234
      = {(int) 2, (int) 3, (int) 4};

  private static final int LEAST = Integer.MIN_VALUE;
  private static final int GREATEST = Integer.MAX_VALUE;

  private static final int[] VALUES =
      { LEAST, (int) -1, (int) 0, (int) 1, GREATEST };

  public void testHashCode() {
    for (int value : VALUES) {
      assertEquals(((Integer) value).hashCode(), Ints.hashCode(value));
    }
  }

  public void testCheckedCast() {
    for (int value : VALUES) {
      assertEquals(value, Ints.checkedCast((long) value));
    }
    assertCastFails(GREATEST + 1L);
    assertCastFails(LEAST - 1L);
    assertCastFails(Long.MAX_VALUE);
    assertCastFails(Long.MIN_VALUE);
  }

  public void testSaturatedCast() {
    for (int value : VALUES) {
      assertEquals(value, Ints.saturatedCast((long) value));
    }
    assertEquals(GREATEST, Ints.saturatedCast(GREATEST + 1L));
    assertEquals(LEAST, Ints.saturatedCast(LEAST - 1L));
    assertEquals(GREATEST, Ints.saturatedCast(Long.MAX_VALUE));
    assertEquals(LEAST, Ints.saturatedCast(Long.MIN_VALUE));
  }

  private static void assertCastFails(long value) {
    try {
      Ints.checkedCast(value);
      fail("Cast to int should have failed: " + value);
    } catch (IllegalArgumentException ex) {
      assertTrue(value + " not found in exception text: " + ex.getMessage(),
          ex.getMessage().contains(String.valueOf(value)));
    }
  }

  public void testCompare() {
    for (int x : VALUES) {
      for (int y : VALUES) {
        // note: spec requires only that the sign is the same
        assertEquals(x + ", " + y,
                     Integer.valueOf(x).compareTo(y),
                     Ints.compare(x, y));
      }
    }
  }

  public void testContains() {
    assertFalse(Ints.contains(EMPTY, (int) 1));
    assertFalse(Ints.contains(ARRAY1, (int) 2));
    assertFalse(Ints.contains(ARRAY234, (int) 1));
    assertTrue(Ints.contains(new int[] {(int) -1}, (int) -1));
    assertTrue(Ints.contains(ARRAY234, (int) 2));
    assertTrue(Ints.contains(ARRAY234, (int) 3));
    assertTrue(Ints.contains(ARRAY234, (int) 4));
  }

  public void testIndexOf() {
    assertEquals(-1, Ints.indexOf(EMPTY, (int) 1));
    assertEquals(-1, Ints.indexOf(ARRAY1, (int) 2));
    assertEquals(-1, Ints.indexOf(ARRAY234, (int) 1));
    assertEquals(0, Ints.indexOf(
        new int[] {(int) -1}, (int) -1));
    assertEquals(0, Ints.indexOf(ARRAY234, (int) 2));
    assertEquals(1, Ints.indexOf(ARRAY234, (int) 3));
    assertEquals(2, Ints.indexOf(ARRAY234, (int) 4));
    assertEquals(1, Ints.indexOf(
        new int[] { (int) 2, (int) 3, (int) 2, (int) 3 },
        (int) 3));
  }

  public void testIndexOf_arrayTarget() {
    assertEquals(0, Ints.indexOf(EMPTY, EMPTY));
    assertEquals(0, Ints.indexOf(ARRAY234, EMPTY));
    assertEquals(-1, Ints.indexOf(EMPTY, ARRAY234));
    assertEquals(-1, Ints.indexOf(ARRAY234, ARRAY1));
    assertEquals(-1, Ints.indexOf(ARRAY1, ARRAY234));
    assertEquals(0, Ints.indexOf(ARRAY1, ARRAY1));
    assertEquals(0, Ints.indexOf(ARRAY234, ARRAY234));
    assertEquals(0, Ints.indexOf(
        ARRAY234, new int[] { (int) 2, (int) 3 }));
    assertEquals(1, Ints.indexOf(
        ARRAY234, new int[] { (int) 3, (int) 4 }));
    assertEquals(1, Ints.indexOf(ARRAY234, new int[] { (int) 3 }));
    assertEquals(2, Ints.indexOf(ARRAY234, new int[] { (int) 4 }));
    assertEquals(1, Ints.indexOf(new int[] { (int) 2, (int) 3,
        (int) 3, (int) 3, (int) 3 },
        new int[] { (int) 3 }
    ));
    assertEquals(2, Ints.indexOf(
        new int[] { (int) 2, (int) 3, (int) 2,
            (int) 3, (int) 4, (int) 2, (int) 3},
        new int[] { (int) 2, (int) 3, (int) 4}
    ));
    assertEquals(1, Ints.indexOf(
        new int[] { (int) 2, (int) 2, (int) 3,
            (int) 4, (int) 2, (int) 3, (int) 4},
        new int[] { (int) 2, (int) 3, (int) 4}
    ));
    assertEquals(-1, Ints.indexOf(
        new int[] { (int) 4, (int) 3, (int) 2},
        new int[] { (int) 2, (int) 3, (int) 4}
    ));
  }

  public void testLastIndexOf() {
    assertEquals(-1, Ints.lastIndexOf(EMPTY, (int) 1));
    assertEquals(-1, Ints.lastIndexOf(ARRAY1, (int) 2));
    assertEquals(-1, Ints.lastIndexOf(ARRAY234, (int) 1));
    assertEquals(0, Ints.lastIndexOf(
        new int[] {(int) -1}, (int) -1));
    assertEquals(0, Ints.lastIndexOf(ARRAY234, (int) 2));
    assertEquals(1, Ints.lastIndexOf(ARRAY234, (int) 3));
    assertEquals(2, Ints.lastIndexOf(ARRAY234, (int) 4));
    assertEquals(3, Ints.lastIndexOf(
        new int[] { (int) 2, (int) 3, (int) 2, (int) 3 },
        (int) 3));
  }

  public void testMax_noArgs() {
    try {
      Ints.max();
      fail();
    } catch (IllegalArgumentException expected) {
    }
  }

  public void testMax() {
    assertEquals(LEAST, Ints.max(LEAST));
    assertEquals(GREATEST, Ints.max(GREATEST));
    assertEquals((int) 9, Ints.max(
        (int) 8, (int) 6, (int) 7,
        (int) 5, (int) 3, (int) 0, (int) 9));
  }

  public void testMin_noArgs() {
    try {
      Ints.min();
      fail();
    } catch (IllegalArgumentException expected) {
    }
  }

  public void testMin() {
    assertEquals(LEAST, Ints.min(LEAST));
    assertEquals(GREATEST, Ints.min(GREATEST));
    assertEquals((int) 0, Ints.min(
        (int) 8, (int) 6, (int) 7,
        (int) 5, (int) 3, (int) 0, (int) 9));
  }

  public void testConcat() {
    assertTrue(Arrays.equals(EMPTY, Ints.concat()));
    assertTrue(Arrays.equals(EMPTY, Ints.concat(EMPTY)));
    assertTrue(Arrays.equals(EMPTY, Ints.concat(EMPTY, EMPTY, EMPTY)));
    assertTrue(Arrays.equals(ARRAY1, Ints.concat(ARRAY1)));
    assertNotSame(ARRAY1, Ints.concat(ARRAY1));
    assertTrue(Arrays.equals(ARRAY1, Ints.concat(EMPTY, ARRAY1, EMPTY)));
    assertTrue(Arrays.equals(
        new int[] {(int) 1, (int) 1, (int) 1},
        Ints.concat(ARRAY1, ARRAY1, ARRAY1)));
    assertTrue(Arrays.equals(
        new int[] {(int) 1, (int) 2, (int) 3, (int) 4},
        Ints.concat(ARRAY1, ARRAY234)));
  }

  public void testEnsureCapacity() {
    assertSame(EMPTY, Ints.ensureCapacity(EMPTY, 0, 1));
    assertSame(ARRAY1, Ints.ensureCapacity(ARRAY1, 0, 1));
    assertSame(ARRAY1, Ints.ensureCapacity(ARRAY1, 1, 1));
    assertTrue(Arrays.equals(
        new int[] {(int) 1, (int) 0, (int) 0},
        Ints.ensureCapacity(ARRAY1, 2, 1)));
  }

  public void testEnsureCapacity_fail() {
    try {
      Ints.ensureCapacity(ARRAY1, -1, 1);
      fail();
    } catch (IllegalArgumentException expected) {
    }
    try {
      // notice that this should even fail when no growth was needed
      Ints.ensureCapacity(ARRAY1, 1, -1);
      fail();
    } catch (IllegalArgumentException expected) {
    }
  }

  public void testJoin() {
    assertEquals("", Ints.join(",", EMPTY));
    assertEquals("1", Ints.join(",", ARRAY1));
    assertEquals("1,2", Ints.join(",", (int) 1, (int) 2));
    assertEquals("123",
        Ints.join("", (int) 1, (int) 2, (int) 3));
  }

  public void testLexicographicalComparator() {
    List<int[]> ordered = Arrays.asList(
        new int[] {},
        new int[] {LEAST},
        new int[] {LEAST, LEAST},
        new int[] {LEAST, (int) 1},
        new int[] {(int) 1},
        new int[] {(int) 1, LEAST},
        new int[] {GREATEST, GREATEST - (int) 1},
        new int[] {GREATEST, GREATEST},
        new int[] {GREATEST, GREATEST, GREATEST});

    Comparator<int[]> comparator = Ints.lexicographicalComparator();
    Helpers.testComparator(comparator, ordered);
  }

  public void testToArray() {
    // need explicit type parameter to avoid javac warning!?
    List<Integer> none = Arrays.<Integer>asList();
    assertTrue(Arrays.equals(EMPTY, Ints.toArray(none)));

    List<Integer> one = Arrays.asList((int) 1);
    assertTrue(Arrays.equals(ARRAY1, Ints.toArray(one)));

    int[] array = {(int) 0, (int) 1, (int) 0xdeadbeef};

    List<Integer> three = Arrays.asList((int) 0, (int) 1, (int) 0xdeadbeef);
    assertTrue(Arrays.equals(array, Ints.toArray(three)));

    assertTrue(Arrays.equals(array, Ints.toArray(Ints.asList(array))));
  }

  public void testToArray_threadSafe() {
    for (int delta : new int[] { +1, 0, -1 }) {
      for (int i = 0; i < VALUES.length; i++) {
        List<Integer> list = Ints.asList(VALUES).subList(0, i);
        Collection<Integer> misleadingSize =
            Helpers.misleadingSizeCollection(delta);
        misleadingSize.addAll(list);
        int[] arr = Ints.toArray(misleadingSize);
        assertEquals(i, arr.length);
        for (int j = 0; j < i; j++) {
          assertEquals(VALUES[j], arr[j]);
        }
      }
    }
  }

  public void testToArray_withNull() {
    List<Integer> list = Arrays.asList((int) 0, (int) 1, null);
    try {
      Ints.toArray(list);
      fail();
    } catch (NullPointerException expected) {
    }
  }

  public void testToArray_withConversion() {
    int[] array = {0, 1, 2};

    List<Byte> bytes = Arrays.asList((byte) 0, (byte) 1, (byte) 2);
    List<Short> shorts = Arrays.asList((short) 0, (short) 1, (short) 2);
    List<Integer> ints = Arrays.asList(0, 1, 2);
    List<Float> floats = Arrays.asList((float) 0, (float) 1, (float) 2);
    List<Long> longs = Arrays.asList((long) 0, (long) 1, (long) 2);
    List<Double> doubles = Arrays.asList((double) 0, (double) 1, (double) 2);

    assertTrue(Arrays.equals(array, Ints.toArray(bytes)));
    assertTrue(Arrays.equals(array, Ints.toArray(shorts)));
    assertTrue(Arrays.equals(array, Ints.toArray(ints)));
    assertTrue(Arrays.equals(array, Ints.toArray(floats)));
    assertTrue(Arrays.equals(array, Ints.toArray(longs)));
    assertTrue(Arrays.equals(array, Ints.toArray(doubles)));
  }

  public void testAsList_isAView() {
    int[] array = {(int) 0, (int) 1};
    List<Integer> list = Ints.asList(array);
    list.set(0, (int) 2);
    assertTrue(Arrays.equals(new int[] {(int) 2, (int) 1}, array));
    array[1] = (int) 3;
    assertEquals(Arrays.asList((int) 2, (int) 3), list);
  }

  public void testAsList_toArray_roundTrip() {
    int[] array = { (int) 0, (int) 1, (int) 2 };
    List<Integer> list = Ints.asList(array);
    int[] newArray = Ints.toArray(list);

    // Make sure it returned a copy
    list.set(0, (int) 4);
    assertTrue(Arrays.equals(
        new int[] { (int) 0, (int) 1, (int) 2 }, newArray));
    newArray[1] = (int) 5;
    assertEquals((int) 1, (int) list.get(1));
  }

  // This test stems from a real bug found by andrewk
  public void testAsList_subList_toArray_roundTrip() {
    int[] array = { (int) 0, (int) 1, (int) 2, (int) 3 };
    List<Integer> list = Ints.asList(array);
    assertTrue(Arrays.equals(new int[] { (int) 1, (int) 2 },
        Ints.toArray(list.subList(1, 3))));
    assertTrue(Arrays.equals(new int[] {},
        Ints.toArray(list.subList(2, 2))));
  }

  public void testAsListEmpty() {
    assertSame(Collections.emptyList(), Ints.asList(EMPTY));
  }

  public void testStringConverter_convert() {
    Converter<String, Integer> converter = Ints.stringConverter();
    assertEquals((Integer) 1, converter.convert("1"));
    assertEquals((Integer) 0, converter.convert("0"));
    assertEquals((Integer) (-1), converter.convert("-1"));
    assertEquals((Integer) 255, converter.convert("0xff"));
    assertEquals((Integer) 255, converter.convert("0xFF"));
    assertEquals((Integer) (-255), converter.convert("-0xFF"));
    assertEquals((Integer) 255, converter.convert("#0000FF"));
    assertEquals((Integer) 438, converter.convert("0666"));
  }

  public void testStringConverter_convertError() {
    try {
      Ints.stringConverter().convert("notanumber");
      fail();
    } catch (NumberFormatException expected) {
    }
  }

  public void testStringConverter_nullConversions() {
    assertNull(Ints.stringConverter().convert(null));
    assertNull(Ints.stringConverter().reverse().convert(null));
  }

  public void testStringConverter_reverse() {
    Converter<String, Integer> converter = Ints.stringConverter();
    assertEquals("1", converter.reverse().convert(1));
    assertEquals("0", converter.reverse().convert(0));
    assertEquals("-1", converter.reverse().convert(-1));
    assertEquals("255", converter.reverse().convert(0xff));
    assertEquals("255", converter.reverse().convert(0xFF));
    assertEquals("-255", converter.reverse().convert(-0xFF));
    assertEquals("438", converter.reverse().convert(0666));
  }

  public void testTryParse() {
    tryParseAndAssertEquals(0, "0");
    tryParseAndAssertEquals(0, "-0");
    tryParseAndAssertEquals(1, "1");
    tryParseAndAssertEquals(-1, "-1");
    tryParseAndAssertEquals(8900, "8900");
    tryParseAndAssertEquals(-8900, "-8900");
    tryParseAndAssertEquals(GREATEST, Integer.toString(GREATEST));
    tryParseAndAssertEquals(LEAST, Integer.toString(LEAST));
    assertNull(Ints.tryParse(""));
    assertNull(Ints.tryParse("-"));
    assertNull(Ints.tryParse("+1"));
    assertNull(Ints.tryParse("9999999999999999"));
    assertNull("Max integer + 1",
        Ints.tryParse(Long.toString(((long) GREATEST) + 1)));
    assertNull("Max integer * 10",
        Ints.tryParse(Long.toString(((long) GREATEST) * 10)));
    assertNull("Min integer - 1",
        Ints.tryParse(Long.toString(((long) LEAST) - 1)));
    assertNull("Min integer * 10",
        Ints.tryParse(Long.toString(((long) LEAST) * 10)));
    assertNull("Max long", Ints.tryParse(Long.toString(Long.MAX_VALUE)));
    assertNull("Min long", Ints.tryParse(Long.toString(Long.MIN_VALUE)));
    assertNull(Ints.tryParse("\u0662\u06f3"));
  }

  /**
   * Applies {@link Ints#tryParse(String)} to the given string and asserts that
   * the result is as expected.
   */
  private static void tryParseAndAssertEquals(Integer expected, String value) {
    assertEquals(expected, Ints.tryParse(value));
  }
}
