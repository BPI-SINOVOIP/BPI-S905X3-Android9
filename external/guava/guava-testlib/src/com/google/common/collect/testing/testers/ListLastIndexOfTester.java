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

package com.google.common.collect.testing.testers;

import static com.google.common.collect.testing.features.CollectionFeature.REJECTS_DUPLICATES_AT_CREATION;
import static com.google.common.collect.testing.features.CollectionSize.ONE;
import static com.google.common.collect.testing.features.CollectionSize.ZERO;

import com.google.common.annotations.GwtCompatible;
import com.google.common.collect.testing.features.CollectionFeature;
import com.google.common.collect.testing.features.CollectionSize;

/**
 * A generic JUnit test which tests {@code lastIndexOf()} operations on a list.
 * Can't be invoked directly; please see
 * {@link com.google.common.collect.testing.ListTestSuiteBuilder}.
 *
 * @author Chris Povirk
 */
@GwtCompatible
public class ListLastIndexOfTester<E> extends AbstractListIndexOfTester<E> {
  @Override protected int find(Object o) {
    return getList().lastIndexOf(o);
  }

  @Override protected String getMethodName() {
    return "lastIndexOf";
  }

  @CollectionFeature.Require(absent = REJECTS_DUPLICATES_AT_CREATION)
  @CollectionSize.Require(absent = {ZERO, ONE})
  public void testLastIndexOf_duplicate() {
    E[] array = createSamplesArray();
    array[getNumElements() / 2] = samples.e0;
    collection = getSubjectGenerator().create(array);
    assertEquals(
        "lastIndexOf(duplicate) should return index of last occurrence",
        getNumElements() / 2, getList().lastIndexOf(samples.e0));
  }
}
