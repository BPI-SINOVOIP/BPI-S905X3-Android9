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
package com.google.common.collect.testing.google;

import static com.google.common.base.Preconditions.checkState;
import static com.google.common.collect.testing.features.CollectionSize.ZERO;
import static com.google.common.collect.testing.features.MapFeature.ALLOWS_NULL_KEYS;
import static com.google.common.collect.testing.features.MapFeature.ALLOWS_NULL_VALUES;
import static com.google.common.collect.testing.features.MapFeature.SUPPORTS_PUT;
import static com.google.common.truth.Truth.assertThat;

import com.google.common.annotations.GwtCompatible;
import com.google.common.collect.Iterators;
import com.google.common.collect.Lists;
import com.google.common.collect.Multimap;
import com.google.common.collect.testing.features.CollectionSize;
import com.google.common.collect.testing.features.MapFeature;

import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;

/**
 * Tests for {@link Multimap#putAll(Object, Iterable)}.
 *
 * @author Louis Wasserman
 */
@GwtCompatible
public class MultimapPutIterableTester<K, V> extends AbstractMultimapTester<K, V, Multimap<K, V>> {
  @CollectionSize.Require(absent = ZERO)
  @MapFeature.Require(SUPPORTS_PUT)
  public void testPutAllNonEmptyIterableOnPresentKey() {
    assertTrue(multimap().putAll(sampleKeys().e0, new Iterable<V>() {
      @Override
      public Iterator<V> iterator() {
        return Lists.newArrayList(sampleValues().e3, sampleValues().e4).iterator();
      }
    }));
    assertGet(sampleKeys().e0, sampleValues().e0, sampleValues().e3, sampleValues().e4);
  }
  
  @CollectionSize.Require(absent = ZERO)
  @MapFeature.Require(SUPPORTS_PUT)
  public void testPutAllNonEmptyCollectionOnPresentKey() {
    assertTrue(multimap().putAll(
        sampleKeys().e0, Lists.newArrayList(sampleValues().e3, sampleValues().e4)));
    assertGet(sampleKeys().e0, sampleValues().e0, sampleValues().e3, sampleValues().e4);
  }

  @MapFeature.Require(SUPPORTS_PUT)
  public void testPutAllNonEmptyIterableOnAbsentKey() {
    assertTrue(multimap().putAll(sampleKeys().e3, new Iterable<V>() {
      @Override
      public Iterator<V> iterator() {
        return Lists.newArrayList(sampleValues().e3, sampleValues().e4).iterator();
      }
    }));
    assertGet(sampleKeys().e3, sampleValues().e3, sampleValues().e4);
  }

  @MapFeature.Require(SUPPORTS_PUT)
  public void testPutAllNonEmptyCollectionOnAbsentKey() {
    assertTrue(multimap().putAll(
        sampleKeys().e3, Lists.newArrayList(sampleValues().e3, sampleValues().e4)));
    assertGet(sampleKeys().e3, sampleValues().e3, sampleValues().e4);
  }
  
  @CollectionSize.Require(absent = ZERO)
  @MapFeature.Require({SUPPORTS_PUT, ALLOWS_NULL_VALUES})
  public void testPutAllNullValueOnPresentKey_supported() {
    assertTrue(multimap().putAll(sampleKeys().e0, Lists.newArrayList(sampleValues().e3, null)));
    assertGet(sampleKeys().e0, sampleValues().e0, sampleValues().e3, null);
  }
  
  @MapFeature.Require({SUPPORTS_PUT, ALLOWS_NULL_VALUES})
  public void testPutAllNullValueOnAbsentKey_supported() {
    assertTrue(multimap().putAll(sampleKeys().e3, Lists.newArrayList(sampleValues().e3, null)));
    assertGet(sampleKeys().e3, sampleValues().e3, null);
  }

  @MapFeature.Require(value = SUPPORTS_PUT, absent = ALLOWS_NULL_VALUES)
  public void testPutAllNullValueSingle_unsupported() {
    multimap().putAll(sampleKeys().e1, Lists.newArrayList((V) null));
    expectUnchanged();
  }

  // In principle, it would be nice to apply these two tests to keys with existing values, too.
  
  @MapFeature.Require(value = SUPPORTS_PUT, absent = ALLOWS_NULL_VALUES)
  public void testPutAllNullValueNullLast_unsupported() {
    int size = getNumElements();

    try {
      multimap().putAll(sampleKeys().e3, Lists.newArrayList(sampleValues().e3, null));
      fail();
    } catch (NullPointerException expected) {
    }

    Collection<V> values = multimap().get(sampleKeys().e3);
    if (values.size() == 0) {
      expectUnchanged();
      // Be extra thorough in case internal state was corrupted by the expected null.
      assertEquals(Lists.newArrayList(), Lists.newArrayList(values));
      assertEquals(size, multimap().size());
    } else {
      assertEquals(Lists.newArrayList(sampleValues().e3), Lists.newArrayList(values));
      assertEquals(size + 1, multimap().size());
    }
  }

  @MapFeature.Require(value = SUPPORTS_PUT, absent = ALLOWS_NULL_VALUES)
  public void testPutAllNullValueNullFirst_unsupported() {
    int size = getNumElements();

    try {
      multimap().putAll(sampleKeys().e3, Lists.newArrayList(null, sampleValues().e3));
      fail();
    } catch (NullPointerException expected) {
    }

    /*
     * In principle, a Multimap implementation could add e3 first before failing on the null. But
     * that seems unlikely enough to be worth complicating the test over, especially if there's any
     * chance that a permissive test could mask a bug.
     */
    expectUnchanged();
    // Be extra thorough in case internal state was corrupted by the expected null.
    assertEquals(Lists.newArrayList(), Lists.newArrayList(multimap().get(sampleKeys().e3)));
    assertEquals(size, multimap().size());
  }

  @MapFeature.Require({SUPPORTS_PUT, ALLOWS_NULL_KEYS})
  public void testPutAllOnPresentNullKey() {
    assertTrue(multimap().putAll(null, Lists.newArrayList(sampleValues().e3, sampleValues().e4)));
    assertGet(null, sampleValues().e3, sampleValues().e4);
  }

  @MapFeature.Require(absent = ALLOWS_NULL_KEYS)
  public void testPutAllNullForbidden() {
    try {
      multimap().putAll(null, Collections.singletonList(sampleValues().e3));
      fail("Expected NullPointerException");
    } catch (NullPointerException expected) {
      // success
    }
  }

  private static final Object[] EMPTY = new Object[0];

  @MapFeature.Require(SUPPORTS_PUT)
  public void testPutAllEmptyCollectionOnAbsentKey() {
    assertFalse(multimap().putAll(sampleKeys().e3, Collections.<V>emptyList()));
    expectUnchanged();
  }

  @MapFeature.Require(SUPPORTS_PUT)
  public void testPutAllEmptyIterableOnAbsentKey() {
    Iterable<V> iterable = new Iterable<V>() {
      @Override
      public Iterator<V> iterator() {
        return Iterators.emptyIterator();
      }
    };

    assertFalse(multimap().putAll(sampleKeys().e3, iterable));
    expectUnchanged();
  }

  @CollectionSize.Require(absent = ZERO)
  @MapFeature.Require(SUPPORTS_PUT)
  public void testPutAllEmptyIterableOnPresentKey() {
    multimap().putAll(sampleKeys().e0, Collections.<V>emptyList());
    expectUnchanged();
  }
  
  @MapFeature.Require(SUPPORTS_PUT)
  public void testPutAllOnlyCallsIteratorOnce() {
    Iterable<V> iterable = new Iterable<V>() {
      private boolean calledIteratorAlready = false;

      @Override
      public Iterator<V> iterator() {
        checkState(!calledIteratorAlready);
        calledIteratorAlready = true;
        return Iterators.forArray(sampleValues().e3);
      }
    };
    
    multimap().putAll(sampleKeys().e3, iterable);
  }

  @MapFeature.Require(SUPPORTS_PUT)
  public void testPutAllPropagatesToGet() {
    Collection<V> getCollection = multimap().get(sampleKeys().e0);
    int getCollectionSize = getCollection.size();
    assertTrue(multimap().putAll(
        sampleKeys().e0, Lists.newArrayList(sampleValues().e3, sampleValues().e4)));
    assertEquals(getCollectionSize + 2, getCollection.size());
    assertThat(getCollection).has().allOf(sampleValues().e3, sampleValues().e4);
  }
}
