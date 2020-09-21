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

package com.google.common.collect;

import static com.google.common.truth.Truth.assertThat;

import com.google.common.collect.testing.IteratorFeature;
import com.google.common.collect.testing.IteratorTester;
import com.google.common.testing.NullPointerTester;

import junit.framework.TestCase;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.ConcurrentModificationException;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.PriorityQueue;
import java.util.Random;
import java.util.SortedMap;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Unit test for {@link MinMaxPriorityQueue}.
 *
 * @author Alexei Stolboushkin
 * @author Sverre Sundsdal
 */
public class MinMaxPriorityQueueTest extends TestCase {
  private Ordering<Integer> SOME_COMPARATOR = Ordering.natural().reverse();

  // Overkill alert!  Test all combinations of 0-2 options during creation.

  public void testCreation_simple() {
    MinMaxPriorityQueue<Integer> queue = MinMaxPriorityQueue
        .create();
    assertEquals(11, queue.capacity());
    checkUnbounded(queue);
    checkNatural(queue);
  }

  public void testCreation_comparator() {
    MinMaxPriorityQueue<Integer> queue = MinMaxPriorityQueue
        .orderedBy(SOME_COMPARATOR)
        .create();
    assertEquals(11, queue.capacity());
    checkUnbounded(queue);
    assertSame(SOME_COMPARATOR, queue.comparator());
  }

  public void testCreation_expectedSize() {
    MinMaxPriorityQueue<Integer> queue = MinMaxPriorityQueue
        .expectedSize(8)
        .create();
    assertEquals(8, queue.capacity());
    checkUnbounded(queue);
    checkNatural(queue);
  }

  public void testCreation_expectedSize_comparator() {
    MinMaxPriorityQueue<Integer> queue = MinMaxPriorityQueue
        .orderedBy(SOME_COMPARATOR)
        .expectedSize(8)
        .create();
    assertEquals(8, queue.capacity());
    checkUnbounded(queue);
    assertSame(SOME_COMPARATOR, queue.comparator());
  }

  public void testCreation_maximumSize() {
    MinMaxPriorityQueue<Integer> queue = MinMaxPriorityQueue
        .maximumSize(42)
        .create();
    assertEquals(11, queue.capacity());
    assertEquals(42, queue.maximumSize);
    checkNatural(queue);
  }

  public void testCreation_comparator_maximumSize() {
    MinMaxPriorityQueue<Integer> queue = MinMaxPriorityQueue
        .orderedBy(SOME_COMPARATOR)
        .maximumSize(42)
        .create();
    assertEquals(11, queue.capacity());
    assertEquals(42, queue.maximumSize);
    assertSame(SOME_COMPARATOR, queue.comparator());
  }

  public void testCreation_expectedSize_maximumSize() {
    MinMaxPriorityQueue<Integer> queue = MinMaxPriorityQueue
        .expectedSize(8)
        .maximumSize(42)
        .create();
    assertEquals(8, queue.capacity());
    assertEquals(42, queue.maximumSize);
    checkNatural(queue);
  }

  private static final List<Integer> NUMBERS =
      ImmutableList.of(4, 8, 15, 16, 23, 42);

  public void testCreation_withContents() {
    MinMaxPriorityQueue<Integer> queue = MinMaxPriorityQueue
        .create(NUMBERS);
    assertEquals(6, queue.size());
    assertEquals(11, queue.capacity());
    checkUnbounded(queue);
    checkNatural(queue);
  }

  public void testCreation_comparator_withContents() {
    MinMaxPriorityQueue<Integer> queue = MinMaxPriorityQueue
        .orderedBy(SOME_COMPARATOR)
        .create(NUMBERS);
    assertEquals(6, queue.size());
    assertEquals(11, queue.capacity());
    checkUnbounded(queue);
    assertSame(SOME_COMPARATOR, queue.comparator());
  }

  public void testCreation_expectedSize_withContents() {
    MinMaxPriorityQueue<Integer> queue = MinMaxPriorityQueue
        .expectedSize(8)
        .create(NUMBERS);
    assertEquals(6, queue.size());
    assertEquals(8, queue.capacity());
    checkUnbounded(queue);
    checkNatural(queue);
  }

  public void testCreation_maximumSize_withContents() {
    MinMaxPriorityQueue<Integer> queue = MinMaxPriorityQueue
        .maximumSize(42)
        .create(NUMBERS);
    assertEquals(6, queue.size());
    assertEquals(11, queue.capacity());
    assertEquals(42, queue.maximumSize);
    checkNatural(queue);
  }

  // Now test everything at once

  public void testCreation_allOptions() {
    MinMaxPriorityQueue<Integer> queue = MinMaxPriorityQueue
        .orderedBy(SOME_COMPARATOR)
        .expectedSize(8)
        .maximumSize(42)
        .create(NUMBERS);
    assertEquals(6, queue.size());
    assertEquals(8, queue.capacity());
    assertEquals(42, queue.maximumSize);
    assertSame(SOME_COMPARATOR, queue.comparator());
  }

  // TODO: tests that check the weird interplay between expected size,
  // maximum size, size of initial contents, default capacity...

  private static void checkNatural(MinMaxPriorityQueue<Integer> queue) {
    assertSame(Ordering.natural(), queue.comparator());
  }

  private static void checkUnbounded(MinMaxPriorityQueue<Integer> queue) {
    assertEquals(Integer.MAX_VALUE, queue.maximumSize);
  }

  public void testHeapIntact() {
    Random random = new Random(0);
    int heapSize = 999;
    int numberOfModifications = 500;
    MinMaxPriorityQueue<Integer> mmHeap =
        MinMaxPriorityQueue.expectedSize(heapSize).create();
    /*
     * this map would contain the same exact elements as the MinMaxHeap; the
     * value in the map is the number of occurrences of the key.
     */
    SortedMap<Integer, AtomicInteger> replica = Maps.newTreeMap();
    assertTrue("Empty heap should be OK", mmHeap.isIntact());
    for (int i = 0; i < heapSize; i++) {
      int randomInt = random.nextInt();
      mmHeap.offer(randomInt);
      insertIntoReplica(replica, randomInt);
    }
    assertTrue("MinMaxHeap not intact after " + heapSize + " insertions",
        mmHeap.isIntact());
    assertEquals(heapSize, mmHeap.size());
    int currentHeapSize = heapSize;
    for (int i = 0; i < numberOfModifications; i++) {
      if (random.nextBoolean()) {
        /* insert a new element */
        int randomInt = random.nextInt();
        mmHeap.offer(randomInt);
        insertIntoReplica(replica, randomInt);
        currentHeapSize++;
      } else {
        /* remove either min or max */
        if (random.nextBoolean()) {
          removeMinFromReplica(replica, mmHeap.poll());
        } else {
          removeMaxFromReplica(replica, mmHeap.pollLast());
        }
        for (Integer v : replica.keySet()) {
          assertTrue("MinMax queue has lost " + v, mmHeap.contains(v));
        }
        assertTrue(mmHeap.isIntact());
        currentHeapSize--;
        assertEquals(currentHeapSize, mmHeap.size());
      }
    }
    assertEquals(currentHeapSize, mmHeap.size());
    assertTrue("Heap not intact after " + numberOfModifications +
        " random mixture of operations", mmHeap.isIntact());
  }

  public void testSmall() {
    MinMaxPriorityQueue<Integer> mmHeap = MinMaxPriorityQueue.create();
    mmHeap.add(1);
    mmHeap.add(4);
    mmHeap.add(2);
    mmHeap.add(3);
    assertEquals(4, (int) mmHeap.pollLast());
    assertEquals(3, (int) mmHeap.peekLast());
    assertEquals(3, (int) mmHeap.pollLast());
    assertEquals(1, (int) mmHeap.peek());
    assertEquals(2, (int) mmHeap.peekLast());
    assertEquals(2, (int) mmHeap.pollLast());
    assertEquals(1, (int) mmHeap.peek());
    assertEquals(1, (int) mmHeap.peekLast());
    assertEquals(1, (int) mmHeap.pollLast());
    assertNull(mmHeap.peek());
    assertNull(mmHeap.peekLast());
    assertNull(mmHeap.pollLast());
  }

  public void testSmallMinHeap() {
    MinMaxPriorityQueue<Integer> mmHeap = MinMaxPriorityQueue.create();
    mmHeap.add(1);
    mmHeap.add(3);
    mmHeap.add(2);
    assertEquals(1, (int) mmHeap.peek());
    assertEquals(1, (int) mmHeap.poll());
    assertEquals(3, (int) mmHeap.peekLast());
    assertEquals(2, (int) mmHeap.peek());
    assertEquals(2, (int) mmHeap.poll());
    assertEquals(3, (int) mmHeap.peekLast());
    assertEquals(3, (int) mmHeap.peek());
    assertEquals(3, (int) mmHeap.poll());
    assertNull(mmHeap.peekLast());
    assertNull(mmHeap.peek());
    assertNull(mmHeap.poll());
  }

  public void testRemove() {
    MinMaxPriorityQueue<Integer> mmHeap = MinMaxPriorityQueue.create();
    mmHeap.addAll(Lists.newArrayList(1, 2, 3, 4, 47, 1, 5, 3, 0));
    assertTrue("Heap is not intact initally", mmHeap.isIntact());
    assertEquals(9, mmHeap.size());
    mmHeap.remove(5);
    assertEquals(8, mmHeap.size());
    assertTrue("Heap is not intact after remove()", mmHeap.isIntact());
    assertEquals(47, (int) mmHeap.pollLast());
    assertEquals(4, (int) mmHeap.pollLast());
    mmHeap.removeAll(Lists.newArrayList(2, 3));
    assertEquals(3, mmHeap.size());
    assertTrue("Heap is not intact after removeAll()", mmHeap.isIntact());
  }

  public void testContains() {
    MinMaxPriorityQueue<Integer> mmHeap = MinMaxPriorityQueue.create();
    mmHeap.addAll(Lists.newArrayList(1, 1, 2));
    assertEquals(3, mmHeap.size());
    assertFalse("Heap does not contain null", mmHeap.contains(null));
    assertFalse("Heap does not contain 3", mmHeap.contains(3));
    assertFalse("Heap does not contain 3", mmHeap.remove(3));
    assertEquals(3, mmHeap.size());
    assertTrue("Heap is not intact after remove()", mmHeap.isIntact());
    assertTrue("Heap contains two 1's", mmHeap.contains(1));
    assertTrue("Heap contains two 1's", mmHeap.remove(1));
    assertTrue("Heap contains 1", mmHeap.contains(1));
    assertTrue("Heap contains 1", mmHeap.remove(1));
    assertFalse("Heap does not contain 1", mmHeap.contains(1));
    assertTrue("Heap contains 2", mmHeap.remove(2));
    assertEquals(0, mmHeap.size());
    assertFalse("Heap does not contain anything", mmHeap.contains(1));
    assertFalse("Heap does not contain anything", mmHeap.remove(2));
  }

  public void testIteratorPastEndException() {
    MinMaxPriorityQueue<Integer> mmHeap = MinMaxPriorityQueue.create();
    mmHeap.addAll(Lists.newArrayList(1, 2));
    Iterator<Integer> it = mmHeap.iterator();
    assertTrue("Iterator has reached end prematurely", it.hasNext());
    it.next();
    it.next();
    try {
      it.next();
      fail("No exception thrown when iterating past end of heap");
    } catch (NoSuchElementException e) {
      // expected error
    }
  }

  public void testIteratorConcurrentModification() {
    MinMaxPriorityQueue<Integer> mmHeap = MinMaxPriorityQueue.create();
    mmHeap.addAll(Lists.newArrayList(1, 2, 3, 4));
    Iterator<Integer> it = mmHeap.iterator();
    assertTrue("Iterator has reached end prematurely", it.hasNext());
    it.next();
    it.next();
    mmHeap.remove(4);
    try {
      it.next();
      fail("No exception thrown when iterating a modified heap");
    } catch (ConcurrentModificationException e) {
      // expected error
    }
  }

  /**
   * Tests a failure caused by fix to childless uncle issue.
   */
  public void testIteratorRegressionChildlessUncle() {
    final ArrayList<Integer> initial = Lists.newArrayList(
        1, 15, 13, 8, 9, 10, 11, 14);
    MinMaxPriorityQueue<Integer> q = MinMaxPriorityQueue.create(initial);
    assertTrue("State " + Arrays.toString(q.toArray()), q.isIntact());
    q.remove(9);
    q.remove(11);
    q.remove(10);
    // Now we're in the critical state: [1, 15, 13, 8, 14]
    // Removing 8 while iterating caused duplicates in iteration result.
    List<Integer> result = Lists.newArrayListWithCapacity(initial.size());
    for (Iterator<Integer> iter = q.iterator(); iter.hasNext();) {
      Integer value = iter.next();
      result.add(value);
      if (value == 8) {
        iter.remove();
      }
    }
    assertTrue(q.isIntact());
    assertThat(result).has().exactly(1, 15, 13, 8, 14);
  }

  /**
   * This tests a special case of the removeAt() call. Moving an element
   * sideways on the heap could break the invariants. Sometimes we need to
   * bubble an element up instead of trickling down. See implementation.
   */
  public void testInvalidatingRemove() {
    MinMaxPriorityQueue<Integer> mmHeap = MinMaxPriorityQueue.create();
    mmHeap.addAll(Lists.newArrayList(
            1, 20, 1000, 2, 3, 30, 40, 10, 11, 12, 13, 300, 400, 500, 600));
    assertEquals(15, mmHeap.size());
    assertTrue("Heap is not intact initially", mmHeap.isIntact());
    mmHeap.remove(12);
    assertEquals(14, mmHeap.size());
    assertTrue("Heap is not intact after remove()", mmHeap.isIntact());
  }

  /**
   * This tests a more obscure special case, but otherwise similar to above.
   */
  public void testInvalidatingRemove2() {
    MinMaxPriorityQueue<Integer> mmHeap = MinMaxPriorityQueue.create();
    List<Integer> values = Lists.newArrayList(
        1, 20, 1000, 2, 3, 30, 40, 10, 11, 12, 13, 300, 400, 500, 600, 4, 5,
        6, 7, 8, 9, 4, 5, 200, 250);
    mmHeap.addAll(values);
    assertEquals(25, mmHeap.size());
    assertTrue("Heap is not intact initially", mmHeap.isIntact());
    mmHeap.remove(2);
    assertEquals(24, mmHeap.size());
    assertTrue("Heap is not intact after remove()", mmHeap.isIntact());
    values.removeAll(Lists.newArrayList(2));
    assertEquals(values.size(), mmHeap.size());
    assertTrue(values.containsAll(mmHeap));
    assertTrue(mmHeap.containsAll(values));
  }

  public void testIteratorInvalidatingIteratorRemove() {
    MinMaxPriorityQueue<Integer> mmHeap = MinMaxPriorityQueue.create();
    mmHeap.addAll(Lists.newArrayList(
            1, 20, 100, 2, 3, 30, 40));
    assertEquals(7, mmHeap.size());
    assertTrue("Heap is not intact initially", mmHeap.isIntact());
    Iterator<Integer> it = mmHeap.iterator();
    assertEquals((Integer) 1, it.next());
    assertEquals((Integer) 20, it.next());
    assertEquals((Integer) 100, it.next());
    assertEquals((Integer) 2, it.next());
    it.remove();
    assertFalse(mmHeap.contains(2));
    assertTrue(it.hasNext());
    assertEquals((Integer) 3, it.next());
    assertTrue(it.hasNext());
    assertEquals((Integer) 30, it.next());
    assertTrue(it.hasNext());
    assertEquals((Integer) 40, it.next());
    assertFalse(it.hasNext());
    assertEquals(6, mmHeap.size());
    assertTrue("Heap is not intact after remove()", mmHeap.isIntact());
    assertFalse(mmHeap.contains(2));

    // This tests that it.remove() above actually changed the order. It
    // indicates that the value 40 was stored in forgetMeNot, so it is
    // returned in the last call to it.next(). Without it, 30 should be the last
    // item returned by the iterator.
    Integer lastItem = 0;
    for (Integer tmp : mmHeap) {
      lastItem = tmp;
    }
    assertEquals((Integer) 30, lastItem);
  }

  /**
   * This tests a special case where removeAt has to trickle an element
   * first down one level from a min to a max level, then up one level above
   * the index of the removed element.
   * It also tests that skipMe in the iterator plays nicely with
   * forgetMeNot.
   */
  public void testIteratorInvalidatingIteratorRemove2() {
    MinMaxPriorityQueue<Integer> mmHeap = MinMaxPriorityQueue.create();
    mmHeap.addAll(Lists.newArrayList(
        1, 20, 1000, 2, 3, 30, 40, 10, 11, 12, 13, 200, 300, 500, 400));
    assertTrue("Heap is not intact initially", mmHeap.isIntact());
    Iterator<Integer> it = mmHeap.iterator();
    assertEquals((Integer) 1, it.next());
    assertEquals((Integer) 20, it.next());
    assertEquals((Integer) 1000, it.next());
    assertEquals((Integer) 2, it.next());
    it.remove();
    assertTrue("Heap is not intact after remove", mmHeap.isIntact());
    assertEquals((Integer) 10, it.next());
    assertEquals((Integer) 3, it.next());
    it.remove();
    assertTrue("Heap is not intact after remove", mmHeap.isIntact());
    assertEquals((Integer) 12, it.next());
    assertEquals((Integer) 30, it.next());
    assertEquals((Integer) 40, it.next());
    // Skipping 20
    assertEquals((Integer) 11, it.next());
    // Skipping 400
    assertEquals((Integer) 13, it.next());
    assertEquals((Integer) 200, it.next());
    assertEquals((Integer) 300, it.next());
    // Last two from forgetMeNot.
    assertEquals((Integer) 400, it.next());
    assertEquals((Integer) 500, it.next());
  }

  public void testRemoveFromStringHeap() {
    MinMaxPriorityQueue<String> mmHeap =
        MinMaxPriorityQueue.expectedSize(5).create();
    Collections.addAll(mmHeap,
        "foo", "bar", "foobar", "barfoo", "larry", "sergey", "eric");
    assertTrue("Heap is not intact initially", mmHeap.isIntact());
    assertEquals("bar", mmHeap.peek());
    assertEquals("sergey", mmHeap.peekLast());
    assertEquals(7, mmHeap.size());
    assertTrue("Could not remove larry", mmHeap.remove("larry"));
    assertEquals(6, mmHeap.size());
    assertFalse("heap contains larry which has been removed",
        mmHeap.contains("larry"));
    assertTrue("heap does not contain sergey",
        mmHeap.contains("sergey"));
    assertTrue("Could not remove larry", mmHeap.removeAll(
        Lists.newArrayList("sergey", "eric")));
    assertFalse("Could remove nikesh which is not in the heap",
        mmHeap.remove("nikesh"));
    assertEquals(4, mmHeap.size());
  }

  public void testCreateWithOrdering() {
    MinMaxPriorityQueue<String> mmHeap =
        MinMaxPriorityQueue.orderedBy(Ordering.natural().reverse()).create();
    Collections.addAll(mmHeap,
        "foo", "bar", "foobar", "barfoo", "larry", "sergey", "eric");
    assertTrue("Heap is not intact initially", mmHeap.isIntact());
    assertEquals("sergey", mmHeap.peek());
    assertEquals("bar", mmHeap.peekLast());
  }

  public void testCreateWithCapacityAndOrdering() {
    MinMaxPriorityQueue<Integer> mmHeap = MinMaxPriorityQueue.orderedBy(
        Ordering.natural().reverse()).expectedSize(5).create();
    Collections.addAll(mmHeap, 1, 7, 2, 56, 2, 5, 23, 68, 0, 3);
    assertTrue("Heap is not intact initially", mmHeap.isIntact());
    assertEquals(68, (int) mmHeap.peek());
    assertEquals(0, (int) mmHeap.peekLast());
  }

  private <T extends Comparable<T>> void runIterator(
      final List<T> values, int steps) throws Exception {
    IteratorTester<T> tester =
        new IteratorTester<T>(
            steps,
            IteratorFeature.MODIFIABLE,
            Lists.newLinkedList(values),
            IteratorTester.KnownOrder.UNKNOWN_ORDER) {
          private MinMaxPriorityQueue<T> mmHeap;
          @Override protected Iterator<T> newTargetIterator() {
            mmHeap = MinMaxPriorityQueue.create(values);
            return mmHeap.iterator();
          }
          @Override protected void verify(List<T> elements) {
            assertEquals(Sets.newHashSet(elements),
                Sets.newHashSet(mmHeap.iterator()));
            assertTrue("Invalid MinMaxHeap: " + mmHeap, mmHeap.isIntact());
          }
        };
    tester.test();
  }

  public void testIteratorTester() throws Exception {
    Random random = new Random(0);
    List<Integer> list = Lists.newArrayList();
    for (int i = 0; i < 3; i++) {
      list.add(random.nextInt());
    }
    runIterator(list, 6);
  }

  public void testIteratorTesterLarger() throws Exception {
    runIterator(Lists.newArrayList(1, 2, 3, 4, 5, 6, 7, 8, 9, 10), 5);
  }

  public void testRemoveAt() {
    long seed = new Random().nextLong();
    Random random = new Random(seed);
    int heapSize = 999;
    int numberOfModifications = 500;
    MinMaxPriorityQueue<Integer> mmHeap =
        MinMaxPriorityQueue.expectedSize(heapSize).create();
    for (int i = 0; i < heapSize; i++) {
      mmHeap.add(random.nextInt());
    }
    for (int i = 0; i < numberOfModifications; i++) {
      mmHeap.removeAt(random.nextInt(mmHeap.size()));
      assertTrue("Modification " + i + " of seed " + seed, mmHeap.isIntact());
      mmHeap.add(random.nextInt());
      assertTrue("Modification " + i + " of seed " + seed, mmHeap.isIntact());
    }
  }

  public void testRemoveAt_exhaustive() {
    int size = 8;
    List<Integer> expected = createOrderedList(size);
    for (Collection<Integer> perm : Collections2.permutations(expected)) {
      for (int i = 0; i < perm.size(); i++) {
        MinMaxPriorityQueue<Integer> q = MinMaxPriorityQueue.create(perm);
        q.removeAt(i);
        assertTrue("Remove at " + i + " perm " + perm, q.isIntact());
      }
    }
  }

  /**
   * Regression test for bug found.
   */
  public void testCorrectOrdering_regression() {
    MinMaxPriorityQueue<Integer> q = MinMaxPriorityQueue
        .create(ImmutableList.of(3, 5, 1, 4, 7));
    List<Integer> expected = ImmutableList.of(1, 3, 4, 5, 7);
    List<Integer> actual = new ArrayList<Integer>(5);
    for (int i = 0; i < expected.size(); i++) {
      actual.add(q.pollFirst());
    }
    assertEquals(expected, actual);
  }

  public void testCorrectOrdering_smallHeapsPollFirst() {
    for (int size = 2; size < 16; size++) {
      for (int attempts = 0; attempts < size * (size - 1); attempts++) {
        ArrayList<Integer> elements = createOrderedList(size);
        List<Integer> expected = ImmutableList.copyOf(elements);
        MinMaxPriorityQueue<Integer> q = MinMaxPriorityQueue.create();
        long seed = insertRandomly(elements, q);
        while (!q.isEmpty()) {
          elements.add(q.pollFirst());
        }
        assertEquals("Using seed " + seed, expected, elements);
      }
    }
  }

  public void testCorrectOrdering_smallHeapsPollLast() {
    for (int size = 2; size < 16; size++) {
      for (int attempts = 0; attempts < size * (size - 1); attempts++) {
        ArrayList<Integer> elements = createOrderedList(size);
        List<Integer> expected = ImmutableList.copyOf(elements);
        MinMaxPriorityQueue<Integer> q = MinMaxPriorityQueue.create();
        long seed = insertRandomly(elements, q);
        while (!q.isEmpty()) {
          elements.add(0, q.pollLast());
        }
        assertEquals("Using seed " + seed, expected, elements);
      }
    }
  }

  public void testCorrectOrdering_mediumHeapsPollFirst() {
    for (int attempts = 0; attempts < 5000; attempts++) {
      int size = new Random().nextInt(256) + 16;
      ArrayList<Integer> elements = createOrderedList(size);
      List<Integer> expected = ImmutableList.copyOf(elements);
      MinMaxPriorityQueue<Integer> q = MinMaxPriorityQueue.create();
      long seed = insertRandomly(elements, q);
      while (!q.isEmpty()) {
        elements.add(q.pollFirst());
      }
      assertEquals("Using seed " + seed, expected, elements);
    }
  }

  /**
   * Regression test for bug found in random testing.
   */
  public void testCorrectOrdering_73ElementBug() {
    int size = 73;
    long seed = 7522346378524621981L;
    ArrayList<Integer> elements = createOrderedList(size);
    List<Integer> expected = ImmutableList.copyOf(elements);
    MinMaxPriorityQueue<Integer> q = MinMaxPriorityQueue.create();
    insertRandomly(elements, q, new Random(seed));
    assertTrue(q.isIntact());
    while (!q.isEmpty()) {
      elements.add(q.pollFirst());
      assertTrue("State " + Arrays.toString(q.toArray()), q.isIntact());
    }
    assertEquals("Using seed " + seed, expected, elements);
  }

  public void testCorrectOrdering_mediumHeapsPollLast() {
    for (int attempts = 0; attempts < 5000; attempts++) {
      int size = new Random().nextInt(256) + 16;
      ArrayList<Integer> elements = createOrderedList(size);
      List<Integer> expected = ImmutableList.copyOf(elements);
      MinMaxPriorityQueue<Integer> q = MinMaxPriorityQueue.create();
      long seed = insertRandomly(elements, q);
      while (!q.isEmpty()) {
        elements.add(0, q.pollLast());
      }
      assertEquals("Using seed " + seed, expected, elements);
    }
  }

  public void testCorrectOrdering_randomAccess() {
    long seed = new Random().nextLong();
    Random random = new Random(seed);
    PriorityQueue<Integer> control = new PriorityQueue<Integer>();
    MinMaxPriorityQueue<Integer> q = MinMaxPriorityQueue.create();
    for (int i = 0; i < 73; i++) { // 73 is a childless uncle case.
      Integer element = random.nextInt();
      control.add(element);
      assertTrue(q.add(element));
    }
    assertTrue("State " + Arrays.toString(q.toArray()), q.isIntact());
    for (int i = 0; i < 500000; i++) {
      if (random.nextBoolean()) {
        Integer element = random.nextInt();
        control.add(element);
        q.add(element);
      } else {
        assertEquals("Using seed " + seed, control.poll(), q.pollFirst());
      }
    }
    while (!control.isEmpty()) {
      assertEquals("Using seed " + seed, control.poll(), q.pollFirst());
    }
    assertTrue(q.isEmpty());
  }

  public void testExhaustive_pollAndPush() {
    int size = 8;
    List<Integer> expected = createOrderedList(size);
    for (Collection<Integer> perm : Collections2.permutations(expected)) {
      MinMaxPriorityQueue<Integer> q = MinMaxPriorityQueue.create(perm);
      List<Integer> elements = Lists.newArrayListWithCapacity(size);
      while (!q.isEmpty()) {
        Integer next = q.pollFirst();
        for (int i = 0; i <= size; i++) {
          assertTrue(q.add(i));
          assertTrue(q.add(next));
          assertTrue(q.remove(i));
          assertEquals(next, q.poll());
        }
        elements.add(next);
      }
      assertEquals("Started with " + perm, expected, elements);
    }
  }

  /**
   * Regression test for b/4124577
   */
  public void testRegression_dataCorruption() {
    int size = 8;
    List<Integer> expected = createOrderedList(size);
    MinMaxPriorityQueue<Integer> q = MinMaxPriorityQueue.create(expected);
    List<Integer> contents = Lists.newArrayList(expected);
    List<Integer> elements = Lists.newArrayListWithCapacity(size);
    while (!q.isEmpty()) {
      assertThat(q).has().exactlyAs(contents);
      Integer next = q.pollFirst();
      contents.remove(next);
      assertThat(q).has().exactlyAs(contents);
      for (int i = 0; i <= size; i++) {
        q.add(i);
        contents.add(i);
        assertThat(q).has().exactlyAs(contents);
        q.add(next);
        contents.add(next);
        assertThat(q).has().exactlyAs(contents);
        q.remove(i);
        assertTrue(contents.remove(Integer.valueOf(i)));
        assertThat(q).has().exactlyAs(contents);
        assertEquals(next, q.poll());
        contents.remove(next);
        assertThat(q).has().exactlyAs(contents);
      }
      elements.add(next);
    }
    assertEquals(expected, elements);
  }

  /**
   * Returns the seed used for the randomization.
   */
  private long insertRandomly(ArrayList<Integer> elements,
      MinMaxPriorityQueue<Integer> q) {
    long seed = new Random().nextLong();
    Random random = new Random(seed);
    insertRandomly(elements, q, random);
    return seed;
  }

  private void insertRandomly(ArrayList<Integer> elements, MinMaxPriorityQueue<Integer> q,
      Random random) {
    while (!elements.isEmpty()) {
      int selectedIndex = random.nextInt(elements.size());
      q.offer(elements.remove(selectedIndex));
    }
  }

  private ArrayList<Integer> createOrderedList(int size) {
    ArrayList<Integer> elements = new ArrayList<Integer>(size);
    for (int i = 0; i < size; i++) {
      elements.add(i);
    }
    return elements;
  }

  public void testIsEvenLevel() {
    assertTrue(MinMaxPriorityQueue.isEvenLevel(0));
    assertFalse(MinMaxPriorityQueue.isEvenLevel(1));
    assertFalse(MinMaxPriorityQueue.isEvenLevel(2));
    assertTrue(MinMaxPriorityQueue.isEvenLevel(3));

    assertFalse(MinMaxPriorityQueue.isEvenLevel((1 << 10) - 2));
    assertTrue(MinMaxPriorityQueue.isEvenLevel((1 << 10) - 1));

    int i = 1 << 29;
    assertTrue(MinMaxPriorityQueue.isEvenLevel(i - 2));
    assertFalse(MinMaxPriorityQueue.isEvenLevel(i - 1));
    assertFalse(MinMaxPriorityQueue.isEvenLevel(i));

    i = 1 << 30;
    assertFalse(MinMaxPriorityQueue.isEvenLevel(i - 2));
    assertTrue(MinMaxPriorityQueue.isEvenLevel(i - 1));
    assertTrue(MinMaxPriorityQueue.isEvenLevel(i));

    // 1 << 31 is negative because of overflow, 1 << 31 - 1 is positive
    // since isEvenLevel adds 1, we need to do - 2.
    assertTrue(MinMaxPriorityQueue.isEvenLevel((1 << 31) - 2));
    assertTrue(MinMaxPriorityQueue.isEvenLevel(Integer.MAX_VALUE - 1));
    try {
      MinMaxPriorityQueue.isEvenLevel((1 << 31) - 1);
      fail("Should overflow");
    } catch (IllegalStateException e) {
      // expected
    }
    try {
      MinMaxPriorityQueue.isEvenLevel(Integer.MAX_VALUE);
      fail("Should overflow");
    } catch (IllegalStateException e) {
      // expected
    }
    try {
      MinMaxPriorityQueue.isEvenLevel(1 << 31);
      fail("Should be negative");
    } catch (IllegalStateException e) {
      // expected
    }
    try {
      MinMaxPriorityQueue.isEvenLevel(Integer.MIN_VALUE);
      fail("Should be negative");
    } catch (IllegalStateException e) {
      // expected
    }
  }

  public void testNullPointers() {
    NullPointerTester tester = new NullPointerTester();
    tester.testAllPublicConstructors(MinMaxPriorityQueue.class);
    tester.testAllPublicStaticMethods(MinMaxPriorityQueue.class);
    tester.testAllPublicInstanceMethods(MinMaxPriorityQueue.<String>create());
  }

  private static void insertIntoReplica(
      Map<Integer, AtomicInteger> replica,
      int newValue) {
    if (replica.containsKey(newValue)) {
      replica.get(newValue).incrementAndGet();
    } else {
      replica.put(newValue, new AtomicInteger(1));
    }
  }

  private static void removeMinFromReplica(
      SortedMap<Integer, AtomicInteger> replica,
      int minValue) {
    Integer replicatedMinValue = replica.firstKey();
    assertEquals(replicatedMinValue, (Integer) minValue);
    removeFromReplica(replica, replicatedMinValue);
  }

  private static void removeMaxFromReplica(
      SortedMap<Integer, AtomicInteger> replica,
      int maxValue) {
    Integer replicatedMaxValue = replica.lastKey();
    assertTrue("maxValue is incorrect", replicatedMaxValue == maxValue);
    removeFromReplica(replica, replicatedMaxValue);
  }

  private static void removeFromReplica(
      Map<Integer, AtomicInteger> replica, int value) {
    AtomicInteger numOccur = replica.get(value);
    if (numOccur.decrementAndGet() == 0) {
      replica.remove(value);
    }
  }
}
