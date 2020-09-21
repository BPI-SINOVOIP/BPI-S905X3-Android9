/*
 * Copyright (C) 2012 The Guava Authors
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

import com.google.common.collect.testing.SafeTreeSet;
import com.google.common.collect.testing.SetTestSuiteBuilder;
import com.google.common.collect.testing.TestStringSetGenerator;
import com.google.common.collect.testing.features.CollectionFeature;
import com.google.common.collect.testing.features.CollectionSize;

import junit.framework.Test;
import junit.framework.TestSuite;

import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.NavigableSet;
import java.util.Set;
import java.util.SortedSet;

/**
 * Tests for {@code ForwardingNavigableSet}.
 *
 * @author Louis Wasserman
 */
public class ForwardingNavigableSetTest extends ForwardingSortedSetTest {
  static class StandardImplForwardingNavigableSet<T>
      extends ForwardingNavigableSet<T> {
    private final NavigableSet<T> backingSet;

    StandardImplForwardingNavigableSet(NavigableSet<T> backingSet) {
      this.backingSet = backingSet;
    }

    @Override protected NavigableSet<T> delegate() {
      return backingSet;
    }

    @Override public boolean equals(Object object) {
      return standardEquals(object);
    }

    @Override public int hashCode() {
      return standardHashCode();
    }

    @Override public boolean addAll(Collection<? extends T> collection) {
      return standardAddAll(collection);
    }

    @Override public void clear() {
      standardClear();
    }

    @Override public boolean contains(Object object) {
      return standardContains(object);
    }

    @Override public boolean containsAll(Collection<?> collection) {
      return standardContainsAll(collection);
    }

    @Override public boolean remove(Object object) {
      return standardRemove(object);
    }

    @Override public boolean removeAll(Collection<?> collection) {
      return standardRemoveAll(collection);
    }

    @Override public boolean retainAll(Collection<?> collection) {
      return standardRetainAll(collection);
    }

    @Override public Object[] toArray() {
      return standardToArray();
    }

    @Override public <T> T[] toArray(T[] array) {
      return standardToArray(array);
    }

    @Override public String toString() {
      return standardToString();
    }

    @Override public SortedSet<T> subSet(T fromElement, T toElement) {
      return standardSubSet(fromElement, toElement);
    }

    @Override
    public T lower(T e) {
      return standardLower(e);
    }

    @Override
    public T floor(T e) {
      return standardFloor(e);
    }

    @Override
    public T ceiling(T e) {
      return standardCeiling(e);
    }

    @Override
    public T higher(T e) {
      return standardHigher(e);
    }

    @Override
    public T pollFirst() {
      return standardPollFirst();
    }

    @Override
    public T pollLast() {
      return standardPollLast();
    }

    @Override
    public SortedSet<T> headSet(T toElement) {
      return standardHeadSet(toElement);
    }

    @Override
    public SortedSet<T> tailSet(T fromElement) {
      return standardTailSet(fromElement);
    }
  }
  
  public static Test suite() {
    TestSuite suite = new TestSuite();
    
    suite.addTestSuite(ForwardingNavigableSetTest.class);
    suite.addTest(
        SetTestSuiteBuilder.using(new TestStringSetGenerator() {
          @Override protected Set<String> create(String[] elements) {
            return new StandardImplForwardingNavigableSet<String>(
                new SafeTreeSet<String>(Arrays.asList(elements)));
          }

          @Override public List<String> order(List<String> insertionOrder) {
            return Lists.newArrayList(Sets.newTreeSet(insertionOrder));
          }
        }).named(
            "ForwardingNavigableSet[SafeTreeSet] with standard implementations")
            .withFeatures(CollectionSize.ANY, CollectionFeature.KNOWN_ORDER,
                CollectionFeature.GENERAL_PURPOSE).createTestSuite());
    suite.addTest(
        SetTestSuiteBuilder.using(new TestStringSetGenerator() {
          @Override protected Set<String> create(String[] elements) {
            SafeTreeSet<String> set = new SafeTreeSet<String>(Ordering.natural().nullsFirst());
            Collections.addAll(set, elements);
            return new StandardImplForwardingNavigableSet<String>(set);
          }

          @Override public List<String> order(List<String> insertionOrder) {
            return Lists.newArrayList(Sets.newTreeSet(insertionOrder));
          }
        }).named(
            "ForwardingNavigableSet[SafeTreeSet[Ordering.natural.nullsFirst]]"
                + " with standard implementations")
            .withFeatures(CollectionSize.ANY, CollectionFeature.KNOWN_ORDER,
                CollectionFeature.GENERAL_PURPOSE, CollectionFeature.ALLOWS_NULL_VALUES)
                .createTestSuite());
    
    return suite;
  }
  
  @Override public void setUp() throws Exception {
    super.setUp();
    /*
     * Class parameters must be raw, so we can't create a proxy with generic
     * type arguments. The created proxy only records calls and returns null, so
     * the type is irrelevant at runtime.
     */
    @SuppressWarnings("unchecked")
    final NavigableSet<String> navigableSet
        = createProxyInstance(NavigableSet.class);
    forward = new ForwardingNavigableSet<String>() {
      @Override protected NavigableSet<String> delegate() {
        return navigableSet;
      }
    };
  }

  public void testLower() {
    forward().lower("a");
    assertEquals("[lower(Object)]", getCalls());
  }

  public void testFloor() {
    forward().floor("a");
    assertEquals("[floor(Object)]", getCalls());
  }

  public void testCeiling() {
    forward().ceiling("a");
    assertEquals("[ceiling(Object)]", getCalls());
  }

  public void testHigher() {
    forward().higher("a");
    assertEquals("[higher(Object)]", getCalls());
  }

  public void testPollFirst() {
    forward().pollFirst();
    assertEquals("[pollFirst]", getCalls());
  }

  public void testPollLast() {
    forward().pollLast();
    assertEquals("[pollLast]", getCalls());
  }

  public void testDescendingIterator() {
    forward().descendingIterator();
    assertEquals("[descendingIterator]", getCalls());
  }
  
  public void testHeadSet_K_Boolean() {
    forward().headSet("key", false);
    assertEquals("[headSet(Object,boolean)]", getCalls());
  }
  
  public void testSubSet_K_Boolean_K_Boolean() {
    forward().subSet("a", true, "b", false);
    assertEquals("[subSet(Object,boolean,Object,boolean)]", getCalls());
  }
  
  public void testTailSet_K_Boolean() {
    forward().tailSet("key", false);
    assertEquals("[tailSet(Object,boolean)]", getCalls());
  }

  @Override NavigableSet<String> forward() {
    return (NavigableSet<String>) super.forward();
  }
}
