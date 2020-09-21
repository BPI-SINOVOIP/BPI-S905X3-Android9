/*
 * Copyright (C) 2009 The Guava Authors
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

package com.google.common.collect.testing;

import static java.util.Collections.sort;
import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

import com.google.common.annotations.GwtCompatible;
import com.google.common.annotations.GwtIncompatible;

import junit.framework.Assert;
import junit.framework.AssertionFailedError;

import java.io.Serializable;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

@GwtCompatible(emulated = true)
public class Helpers {
  // Clone of Objects.equal
  static boolean equal(Object a, Object b) {
    return a == b || (a != null && a.equals(b));
  }

  // Clone of Lists.newArrayList
  public static <E> List<E> copyToList(Iterable<? extends E> elements) {
    List<E> list = new ArrayList<E>();
    addAll(list, elements);
    return list;
  }

  public static <E> List<E> copyToList(E[] elements) {
    return copyToList(Arrays.asList(elements));
  }

  // Clone of Sets.newLinkedHashSet
  public static <E> Set<E> copyToSet(Iterable<? extends E> elements) {
    Set<E> set = new LinkedHashSet<E>();
    addAll(set, elements);
    return set;
  }

  public static <E> Set<E> copyToSet(E[] elements) {
    return copyToSet(Arrays.asList(elements));
  }

  // Would use Maps.immutableEntry
  public static <K, V> Entry<K, V> mapEntry(K key, V value) {
    return Collections.singletonMap(key, value).entrySet().iterator().next();
  }

  public static void assertEqualIgnoringOrder(
      Iterable<?> expected, Iterable<?> actual) {
    List<?> exp = copyToList(expected);
    List<?> act = copyToList(actual);
    String actString = act.toString();

    // Of course we could take pains to give the complete description of the
    // problem on any failure.

    // Yeah it's n^2.
    for (Object object : exp) {
      if (!act.remove(object)) {
        Assert.fail("did not contain expected element " + object + ", "
            + "expected = " + exp + ", actual = " + actString);
      }
    }
    assertTrue("unexpected elements: " + act, act.isEmpty());
  }

  public static void assertContentsAnyOrder(
      Iterable<?> actual, Object... expected) {
    assertEqualIgnoringOrder(Arrays.asList(expected), actual);
  }

  public static <E> boolean addAll(
      Collection<E> addTo, Iterable<? extends E> elementsToAdd) {
    boolean modified = false;
    for (E e : elementsToAdd) {
      modified |= addTo.add(e);
    }
    return modified;
  }

  static <T> Iterable<T> reverse(final List<T> list) {
    return new Iterable<T>() {
      @Override
      public Iterator<T> iterator() {
        final ListIterator<T> listIter = list.listIterator(list.size());
        return new Iterator<T>() {
          @Override
          public boolean hasNext() {
            return listIter.hasPrevious();
          }
          @Override
          public T next() {
            return listIter.previous();
          }
          @Override
          public void remove() {
            listIter.remove();
          }
        };
      }
    };
  }

  static <T> Iterator<T> cycle(final Iterable<T> iterable) {
    return new Iterator<T>() {
      Iterator<T> iterator = Collections.<T>emptySet().iterator();
      @Override
      public boolean hasNext() {
        return true;
      }
      @Override
      public T next() {
        if (!iterator.hasNext()) {
          iterator = iterable.iterator();
        }
        return iterator.next();
      }
      @Override
      public void remove() {
        throw new UnsupportedOperationException();
      }
    };
  }

  static <T> T get(Iterator<T> iterator, int position) {
    for (int i = 0; i < position; i++) {
      iterator.next();
    }
    return iterator.next();
  }

  static void fail(Throwable cause, Object message) {
    AssertionFailedError assertionFailedError =
        new AssertionFailedError(String.valueOf(message));
    assertionFailedError.initCause(cause);
    throw assertionFailedError;
  }

  public static <K, V> Comparator<Entry<K, V>> entryComparator(
      final Comparator<? super K> keyComparator) {
    return new Comparator<Entry<K, V>>() {
      @Override
      @SuppressWarnings("unchecked") // no less safe than putting it in the map!
      public int compare(Entry<K, V> a, Entry<K, V> b) {
        return (keyComparator == null)
            ? ((Comparable) a.getKey()).compareTo(b.getKey())
            : keyComparator.compare(a.getKey(), b.getKey());
      }
    };
  }

  public static <T> void testComparator(
      Comparator<? super T> comparator, T... valuesInExpectedOrder) {
    testComparator(comparator, Arrays.asList(valuesInExpectedOrder));
  }

  public static <T> void testComparator(
      Comparator<? super T> comparator, List<T> valuesInExpectedOrder) {
    // This does an O(n^2) test of all pairs of values in both orders
    for (int i = 0; i < valuesInExpectedOrder.size(); i++) {
      T t = valuesInExpectedOrder.get(i);

      for (int j = 0; j < i; j++) {
        T lesser = valuesInExpectedOrder.get(j);
        assertTrue(comparator + ".compare(" + lesser + ", " + t + ")",
            comparator.compare(lesser, t) < 0);
      }

      assertEquals(comparator + ".compare(" + t + ", " + t + ")",
          0, comparator.compare(t, t));

      for (int j = i + 1; j < valuesInExpectedOrder.size(); j++) {
        T greater = valuesInExpectedOrder.get(j);
        assertTrue(comparator + ".compare(" + greater + ", " + t + ")",
            comparator.compare(greater, t) > 0);
      }
    }
  }

  public static <T extends Comparable<? super T>> void testCompareToAndEquals(
      List<T> valuesInExpectedOrder) {
    // This does an O(n^2) test of all pairs of values in both orders
    for (int i = 0; i < valuesInExpectedOrder.size(); i++) {
      T t = valuesInExpectedOrder.get(i);

      for (int j = 0; j < i; j++) {
        T lesser = valuesInExpectedOrder.get(j);
        assertTrue(lesser + ".compareTo(" + t + ')', lesser.compareTo(t) < 0);
        assertFalse(lesser.equals(t));
      }

      assertEquals(t + ".compareTo(" + t + ')', 0, t.compareTo(t));
      assertTrue(t.equals(t));

      for (int j = i + 1; j < valuesInExpectedOrder.size(); j++) {
        T greater = valuesInExpectedOrder.get(j);
        assertTrue(greater + ".compareTo(" + t + ')', greater.compareTo(t) > 0);
        assertFalse(greater.equals(t));
      }
    }
  }

  /**
   * Returns a collection that simulates concurrent modification by
   * having its size method return incorrect values.  This is useful
   * for testing methods that must treat the return value from size()
   * as a hint only.
   *
   * @param delta the difference between the true size of the
   * collection and the values returned by the size method
   */
  public static <T> Collection<T> misleadingSizeCollection(final int delta) {
    // It would be nice to be able to return a real concurrent
    // collection like ConcurrentLinkedQueue, so that e.g. concurrent
    // iteration would work, but that would not be GWT-compatible.
    return new ArrayList<T>() {
      @Override public int size() { return Math.max(0, super.size() + delta); }
    };
  }

  /**
   * Returns a "nefarious" map entry with the specified key and value,
   * meaning an entry that is suitable for testing that map entries cannot be
   * modified via a nefarious implementation of equals. This is used for testing
   * unmodifiable collections of map entries; for example, it should not be
   * possible to access the raw (modifiable) map entry via a nefarious equals
   * method.
   */
  public static <K, V> Map.Entry<K, V> nefariousMapEntry(final K key,
      final V value) {
    return new Map.Entry<K, V>() {
      @Override public K getKey() {
        return key;
      }
      @Override public V getValue() {
        return value;
      }
      @Override public V setValue(V value) {
        throw new UnsupportedOperationException();
      }
      @SuppressWarnings("unchecked")
      @Override public boolean equals(Object o) {
        if (o instanceof Map.Entry) {
          Map.Entry<K, V> e = (Map.Entry<K, V>) o;
          e.setValue(value); // muhahaha!

          return equal(this.getKey(), e.getKey())
              && equal(this.getValue(), e.getValue());
        }
        return false;
      }

      @Override public int hashCode() {
        K k = getKey();
        V v = getValue();
        return ((k == null) ?
            0 : k.hashCode()) ^ ((v == null) ? 0 : v.hashCode());
      }

      /**
       * Returns a string representation of the form <code>{key}={value}</code>.
       */
      @Override public String toString() {
        return getKey() + "=" + getValue();
      }
    };
  }

  static <E> List<E> castOrCopyToList(Iterable<E> iterable) {
    if (iterable instanceof List) {
      return (List<E>) iterable;
    }
    List<E> list = new ArrayList<E>();
    for (E e : iterable) {
      list.add(e);
    }
    return list;
  }

  private static final Comparator<Comparable> NATURAL_ORDER = new Comparator<Comparable>() {
    @SuppressWarnings("unchecked") // assume any Comparable is Comparable<Self>
    @Override public int compare(Comparable left, Comparable right) {
      return left.compareTo(right);
    }
  };

  public static <K extends Comparable, V> Iterable<Entry<K, V>> orderEntriesByKey(
      List<Entry<K, V>> insertionOrder) {
    sort(insertionOrder, Helpers.<K, V>entryComparator(NATURAL_ORDER));
    return insertionOrder;
  }

  /**
   * Private replacement for {@link com.google.gwt.user.client.rpc.GwtTransient} to work around
   * build-system quirks.
   */
  private @interface GwtTransient {}

  /**
   * Compares strings in natural order except that null comes immediately before a given value. This
   * works better than Ordering.natural().nullsFirst() because, if null comes before all other
   * values, it lies outside the submap/submultiset ranges we test, and the variety of tests that
   * exercise null handling fail on those subcollections.
   */
  public abstract static class NullsBefore implements Comparator<String>, Serializable {
    /*
     * We don't serialize this class in GWT, so we don't care about whether GWT will serialize this
     * field.
     */
    @GwtTransient private final String justAfterNull;

    protected NullsBefore(String justAfterNull) {
      if (justAfterNull == null) {
        throw new NullPointerException();
      }

      this.justAfterNull = justAfterNull;
    }

    @Override
    public int compare(String lhs, String rhs) {
      if (lhs == rhs) {
        return 0;
      }
      if (lhs == null) {
        // lhs (null) comes just before justAfterNull.
        // If rhs is b, lhs comes first.
        if (rhs.equals(justAfterNull)) {
          return -1;
        }
        return justAfterNull.compareTo(rhs);
      }
      if (rhs == null) {
        // rhs (null) comes just before justAfterNull.
        // If lhs is b, rhs comes first.
        if (lhs.equals(justAfterNull)) {
          return 1;
        }
        return lhs.compareTo(justAfterNull);
      }
      return lhs.compareTo(rhs);
    }

    @Override
    public boolean equals(Object obj) {
      if (obj instanceof NullsBefore) {
        NullsBefore other = (NullsBefore) obj;
        return justAfterNull.equals(other.justAfterNull);
      }
      return false;
    }

    @Override
    public int hashCode() {
      return justAfterNull.hashCode();
    }
  }

  public static final class NullsBeforeB extends NullsBefore {
    public static final NullsBeforeB INSTANCE = new NullsBeforeB();

    private NullsBeforeB() {
      super("b");
    }
  }

  public static final class NullsBeforeTwo extends NullsBefore {
    public static final NullsBeforeTwo INSTANCE = new NullsBeforeTwo();

    private NullsBeforeTwo() {
      super("two"); // from TestStringSortedMapGenerator's sample keys
    }
  }

  @GwtIncompatible("reflection")
  public static Method getMethod(Class<?> clazz, String name) {
    try {
      return clazz.getMethod(name);
    } catch (Exception e) {
      throw new IllegalArgumentException(e);
    }
  }
}
