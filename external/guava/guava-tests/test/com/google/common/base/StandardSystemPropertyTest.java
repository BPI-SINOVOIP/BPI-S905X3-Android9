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

package com.google.common.base;

import junit.framework.TestCase;

/**
 * Tests for {@link StandardSystemProperty}.
 *
 * @author Kurt Alfred Kluever
 */
public class StandardSystemPropertyTest extends TestCase {

  public void testGetKeyMatchesString() {
    for (StandardSystemProperty property : StandardSystemProperty.values()) {
      String fieldName = property.name();
      String expected = Ascii.toLowerCase(fieldName).replaceAll("_", ".");
      assertEquals(expected, property.key());
    }
  }

  public void testGetValue() {
    for (StandardSystemProperty property : StandardSystemProperty.values()) {
      assertEquals(System.getProperty(property.key()), property.value());
    }
  }

  public void testToString() {
    for (StandardSystemProperty property : StandardSystemProperty.values()) {
      assertEquals(property.key() + "=" + property.value(), property.toString());
    }
  }

  public void testNoNullValues() {
    for (StandardSystemProperty property : StandardSystemProperty.values()) {
      // Even though the contract in System.getProperties() specifies that a value will exist for
      // all of the listed keys, for some reason the "java.compiler" key returns null in some JVMs.
      if (property != StandardSystemProperty.JAVA_COMPILER) {
        assertNotNull(property.value());
      }
    }
  }
}
