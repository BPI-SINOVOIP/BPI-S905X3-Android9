/*
 * Copyright (C) 2007 The Guava Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.google.common.collect;

import static com.google.common.base.Preconditions.checkArgument;

import com.google.common.annotations.GwtCompatible;

import junit.framework.TestCase;

import java.io.Serializable;
import java.util.Iterator;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import javax.annotation.Nullable;

/**
 * Unit test for {@link AbstractMultiset}.
 *
 * @author Kevin Bourrillion
 * @author Louis Wasserman
 */
@SuppressWarnings("serial") // No serialization is used in this test
@GwtCompatible(emulated = true)
public class SimpleAbstractMultisetTest extends TestCase {

  public void testFastAddAllMultiset() {
    final AtomicInteger addCalls = new AtomicInteger();
    Multiset<String> multiset = new NoRemoveMultiset<String>() {
      @Override
      public int add(String element, int occurrences) {
        addCalls.incrementAndGet();
        return super.add(element, occurrences);
      }
    };
    ImmutableMultiset<String> adds =
        new ImmutableMultiset.Builder<String>().addCopies("x", 10).build();
    multiset.addAll(adds);
    assertEquals(addCalls.get(), 1);
  }

  public void testRemoveUnsupported() {
    Multiset<String> multiset = new NoRemoveMultiset<String>();
    multiset.add("a");
    try {
      multiset.remove("a");
      fail();
    } catch (UnsupportedOperationException expected) {}
    assertTrue(multiset.contains("a"));
  }

  private static class NoRemoveMultiset<E> extends AbstractMultiset<E>
      implements Serializable {
    final Map<E, Integer> backingMap = Maps.newHashMap();

    @Override public int add(@Nullable E element, int occurrences) {
      checkArgument(occurrences >= 0);
      Integer frequency = backingMap.get(element);
      if (frequency == null) {
        frequency = 0;
      }
      if (occurrences == 0) {
        return frequency;
      }
      checkArgument(occurrences <= Integer.MAX_VALUE - frequency);
      backingMap.put(element, frequency + occurrences);
      return frequency;
    }

    @Override
    Iterator<Entry<E>> entryIterator() {
      final Iterator<Map.Entry<E, Integer>> backingEntries = backingMap.entrySet().iterator();
      return new UnmodifiableIterator<Multiset.Entry<E>>() {
        @Override
        public boolean hasNext() {
          return backingEntries.hasNext();
        }

        @Override
        public Multiset.Entry<E> next() {
          final Map.Entry<E, Integer> mapEntry = backingEntries.next();
          return new Multisets.AbstractEntry<E>() {
            @Override
            public E getElement() {
              return mapEntry.getKey();
            }

            @Override
            public int getCount() {
              Integer frequency = backingMap.get(getElement());
              return (frequency == null) ? 0 : frequency;
            }
          };
        }
      };
    }

    @Override
    int distinctElements() {
      return backingMap.size();
    }
  }
}

