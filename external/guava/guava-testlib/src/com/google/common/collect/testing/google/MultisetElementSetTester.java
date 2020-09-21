/*
 * Copyright (C) 2013 The Guava Authors
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

import static com.google.common.collect.testing.features.CollectionFeature.SUPPORTS_ADD;
import static com.google.common.collect.testing.features.CollectionFeature.SUPPORTS_REMOVE;
import static com.google.common.collect.testing.features.CollectionSize.SEVERAL;
import static com.google.common.collect.testing.features.CollectionSize.ZERO;
import static com.google.common.truth.Truth.assertThat;

import com.google.common.annotations.GwtCompatible;
import com.google.common.collect.testing.features.CollectionFeature;
import com.google.common.collect.testing.features.CollectionSize;

import java.util.Collections;
import java.util.Set;

/**
 * Tests for {@code Multiset.elementSet()} not covered by the derived {@code SetTestSuiteBuilder}.
 * 
 * @author Louis Wasserman
 */
@GwtCompatible
public class MultisetElementSetTester<E> extends AbstractMultisetTester<E> {
  @CollectionFeature.Require(SUPPORTS_ADD)
  public void testElementSetReflectsAddAbsent() {
    Set<E> elementSet = getMultiset().elementSet();
    assertFalse(elementSet.contains(samples.e3));
    getMultiset().add(samples.e3, 4);
    assertTrue(elementSet.contains(samples.e3));
  }
  
  @CollectionSize.Require(absent = ZERO)
  @CollectionFeature.Require(SUPPORTS_REMOVE)
  public void testElementSetReflectsRemove() {
    Set<E> elementSet = getMultiset().elementSet();
    assertTrue(elementSet.contains(samples.e0));
    getMultiset().removeAll(Collections.singleton(samples.e0));
    assertFalse(elementSet.contains(samples.e0));
  }

  @CollectionSize.Require(absent = ZERO)
  @CollectionFeature.Require(SUPPORTS_REMOVE)
  public void testElementSetRemovePropagatesToMultiset() {
    Set<E> elementSet = getMultiset().elementSet();
    assertTrue(elementSet.remove(samples.e0));
    assertFalse(getMultiset().contains(samples.e0));
  }

  @CollectionSize.Require(SEVERAL)
  @CollectionFeature.Require(SUPPORTS_REMOVE)
  public void testElementSetRemoveDuplicatePropagatesToMultiset() {
    initThreeCopies();
    Set<E> elementSet = getMultiset().elementSet();
    assertTrue(elementSet.remove(samples.e0));
    assertThat(getMultiset()).isEmpty();
  }

  @CollectionFeature.Require(SUPPORTS_REMOVE)
  public void testElementSetRemoveAbsent() {
    Set<E> elementSet = getMultiset().elementSet();
    assertFalse(elementSet.remove(samples.e3));
    expectUnchanged();
  }
  
  @CollectionFeature.Require(SUPPORTS_REMOVE)
  public void testElementSetClear() {
    getMultiset().elementSet().clear();
    assertThat(getMultiset()).isEmpty();    
  }
}
