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
 * Tests {@link java.util.Collection#equals}.
 *
 * @author George van den Driessche
 */
@GwtCompatible
public class CollectionEqualsTester<E> extends AbstractCollectionTester<E> {
  public void testEquals_self() {
    assertTrue("An Object should be equal to itself.",
        collection.equals(collection));
  }

  public void testEquals_null() {
    //noinspection ObjectEqualsNull
    assertFalse("An object should not be equal to null.",
        collection.equals(null));
  }

  public void testEquals_notACollection() {
    //noinspection EqualsBetweenInconvertibleTypes
    assertFalse("A Collection should never equal an "
        + "object that is not a Collection.",
        collection.equals("huh?"));
  }
}
