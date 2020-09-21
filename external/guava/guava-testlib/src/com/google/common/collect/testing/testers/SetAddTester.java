/*
 * Copyright (C) 2007 The Guava Authors
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

import static com.google.common.collect.testing.features.CollectionFeature.ALLOWS_NULL_VALUES;
import static com.google.common.collect.testing.features.CollectionFeature.SUPPORTS_ADD;
import static com.google.common.collect.testing.features.CollectionSize.ZERO;

import com.google.common.annotations.GwtCompatible;
import com.google.common.annotations.GwtIncompatible;
import com.google.common.collect.testing.Helpers;
import com.google.common.collect.testing.features.CollectionFeature;
import com.google.common.collect.testing.features.CollectionSize;

import java.lang.reflect.Method;

/**
 * A generic JUnit test which tests add operations on a set. Can't be
 * invoked directly; please see
 * {@link com.google.common.collect.testing.SetTestSuiteBuilder}.
 *
 * @author Kevin Bourrillion
 */
@GwtCompatible(emulated = true)
public class SetAddTester<E> extends AbstractSetTester<E> {
  @CollectionFeature.Require(SUPPORTS_ADD)
  @CollectionSize.Require(absent = ZERO)
  public void testAdd_supportedPresent() {
    assertFalse("add(present) should return false", getSet().add(samples.e0));
    expectUnchanged();
  }

  @CollectionFeature.Require(value = {SUPPORTS_ADD, ALLOWS_NULL_VALUES})
  @CollectionSize.Require(absent = ZERO)
  public void testAdd_supportedNullPresent() {
    E[] array = createArrayWithNullElement();
    collection = getSubjectGenerator().create(array);
    assertFalse("add(nullPresent) should return false", getSet().add(null));
    expectContents(array);
  }

  /**
   * Returns the {@link Method} instance for
   * {@link #testAdd_supportedNullPresent()} so that tests can suppress it. See
   * {@link CollectionAddTester#getAddNullSupportedMethod()} for details.
   */
  @GwtIncompatible("reflection")
  public static Method getAddSupportedNullPresentMethod() {
    return Helpers.getMethod(SetAddTester.class, "testAdd_supportedNullPresent");
  }
}
