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

import com.google.common.annotations.GwtCompatible;
import com.google.common.collect.testing.AbstractCollectionTester;

/**
 * A generic JUnit test which tests {@code size()} operations on a collection.
 * Can't be invoked directly; please see
 * {@link com.google.common.collect.testing.CollectionTestSuiteBuilder}.
 *
 * @author Kevin Bourrillion
 */
@GwtCompatible
public class CollectionSizeTester<E> extends AbstractCollectionTester<E> {
  public void testSize() {
    assertEquals("size():", getNumElements(), collection.size());
  }
}
