/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package org.apache.harmony.tests.java.text;

import java.text.CharacterIterator;
import java.text.CollationElementIterator;
import java.text.CollationKey;
import java.text.Collator;
import java.text.ParseException;
import java.text.RuleBasedCollator;
import java.text.StringCharacterIterator;
import java.util.Locale;

import junit.framework.TestCase;

public class RuleBasedCollatorTest extends TestCase {

  public void test_getCollationKeyLjava_lang_String() throws Exception {
    // Regression test for HARMONY-28
    String source = null;
    String Simple = "&9 < a< b< c< d";
    RuleBasedCollator rbc = new RuleBasedCollator(Simple);
    CollationKey ck = rbc.getCollationKey(source);
    assertNull("Assert 1: getCollationKey (null) does not return null", ck);
  }

  public void testHashCode() throws ParseException {
    String rule = "&9 < a < b < c < d";

    RuleBasedCollator coll = new RuleBasedCollator(rule);
    RuleBasedCollator same = new RuleBasedCollator(rule);
    assertEquals(coll.hashCode(), same.hashCode());
  }

  public void testClone() throws ParseException {
    RuleBasedCollator coll = (RuleBasedCollator) Collator.getInstance(Locale.US);
    RuleBasedCollator clone = (RuleBasedCollator) coll.clone();
    assertNotSame(coll, clone);
    assertEquals(coll.getRules(), clone.getRules());
    assertEquals(coll.getDecomposition(), clone.getDecomposition());
    assertEquals(coll.getStrength(), clone.getStrength());
  }

  public void testEqualsObject() throws ParseException {
    String rule = "&9 < a < b < c < d < e";
    RuleBasedCollator coll = new RuleBasedCollator(rule);

    assertEquals(Collator.TERTIARY, coll.getStrength());
    assertEquals(Collator.NO_DECOMPOSITION, coll.getDecomposition());
    RuleBasedCollator other = new RuleBasedCollator(rule);
    assertTrue(coll.equals(other));

    coll.setStrength(Collator.PRIMARY);
    assertFalse(coll.equals(other));

    coll.setStrength(Collator.TERTIARY);
    coll.setDecomposition(Collator.CANONICAL_DECOMPOSITION);
    assertFalse(coll.equals(other));
  }

  public void testCompareStringString() throws ParseException {
    String rule = "&9 < c < b < a";
    RuleBasedCollator coll = new RuleBasedCollator(rule);
    assertEquals(-1, coll.compare("c", "a"));
  }

  public void testGetCollationKey() {
    RuleBasedCollator coll = (RuleBasedCollator) Collator.getInstance(Locale.GERMAN);
    String source = "abc";
    CollationKey key1 = coll.getCollationKey(source);
    assertEquals(source, key1.getSourceString());
    String source2 = "abb";
    CollationKey key2 = coll.getCollationKey(source2);
    assertEquals(source2, key2.getSourceString());
    assertTrue(key1.compareTo(key2) > 0);
    assertTrue(coll.compare(source, source2) > 0);
  }

  public void testGetRules() throws ParseException {
    String rule = "&9 < a = b < c";
    RuleBasedCollator coll = new RuleBasedCollator(rule);
    assertEquals(rule, coll.getRules());
  }

  public void testGetCollationElementIteratorString() throws Exception {
    {
      Locale locale = Locale.forLanguageTag("es-u-co-trad");
      RuleBasedCollator coll = (RuleBasedCollator) Collator.getInstance(locale);
      String source = "cha";
      CollationElementIterator iterator = coll.getCollationElementIterator(source);
      int[] e_offset = { 0, 2, 3 };
      int offset = iterator.getOffset();
      int i = 0;
      assertEquals(e_offset[i++], offset);
      while (offset != source.length()) {
        iterator.next();
        offset = iterator.getOffset();
        assertEquals(e_offset[i++], offset);
      }
      assertEquals(CollationElementIterator.NULLORDER, iterator.next());
    }

    {
      Locale locale = new Locale("de", "DE");
      RuleBasedCollator coll = (RuleBasedCollator) Collator.getInstance(locale);
      String source = "\u00fcb";
      CollationElementIterator iterator = coll.getCollationElementIterator(source);
      int[] e_offset = { 0, 1, 1, 2 };
      int offset = iterator.getOffset();
      int i = 0;
      assertEquals(e_offset[i++], offset);
      while (offset != source.length()) {
        iterator.next();
        offset = iterator.getOffset();
        assertEquals(e_offset[i++], offset);
      }
      assertEquals(CollationElementIterator.NULLORDER, iterator.next());
    }
    //Regression for HARMONY-1352
    try {
      new RuleBasedCollator("&9 < a< b< c< d").getCollationElementIterator((String)null);
      fail();
    } catch (NullPointerException expected) {
    }
  }

  public void testGetCollationElementIteratorCharacterIterator() throws Exception {
    {
      Locale locale = Locale.forLanguageTag("es-u-co-trad");
      RuleBasedCollator coll = (RuleBasedCollator) Collator.getInstance(locale);
      String text = "cha";
      StringCharacterIterator source = new StringCharacterIterator(text);
      CollationElementIterator iterator = coll.getCollationElementIterator(source);
      int[] e_offset = { 0, 2, 3 };
      int offset = iterator.getOffset();
      int i = 0;
      assertEquals(e_offset[i++], offset);
      while (offset != text.length()) {
        iterator.next();
        offset = iterator.getOffset();
        // System.out.println(offset);
        assertEquals(e_offset[i++], offset);
      }
      assertEquals(CollationElementIterator.NULLORDER, iterator.next());
    }

    {
      Locale locale = new Locale("de", "DE");
      RuleBasedCollator coll = (RuleBasedCollator) Collator.getInstance(locale);
      String text = "\u00fcb";
      StringCharacterIterator source = new StringCharacterIterator(text);
      CollationElementIterator iterator = coll.getCollationElementIterator(source);
      int[] e_offset = { 0, 1, 1, 2 };
      int offset = iterator.getOffset();
      int i = 0;
      assertEquals(e_offset[i++], offset);
      while (offset != text.length()) {
        iterator.next();
        offset = iterator.getOffset();
        assertEquals(e_offset[i++], offset);
      }
      assertEquals(CollationElementIterator.NULLORDER, iterator.next());
    }
    //Regression for HARMONY-1352
    try {
      new RuleBasedCollator("&9 < a< b< c< d").getCollationElementIterator((CharacterIterator)null);
      fail();
    } catch (NullPointerException expected) {
    }
  }

  public void testStrength() {
    RuleBasedCollator coll = (RuleBasedCollator) Collator.getInstance(Locale.US);
    for (int i = 0; i < 4; i++) {
      coll.setStrength(i);
      assertEquals(i, coll.getStrength());
    }
  }

  public void testDecomposition() {
    RuleBasedCollator coll = (RuleBasedCollator) Collator.getInstance(Locale.US);
    for (int i = 0; i < 2; i++) {
      coll.setDecomposition(i);
      assertEquals(i, coll.getDecomposition());
    }
  }

  public void testCollator_GetInstance() {
    Collator coll = Collator.getInstance();
    Object obj1 = "a";
    Object obj2 = "b";
    assertEquals(-1, coll.compare(obj1, obj2));

    Collator.getInstance();
    assertFalse(coll.equals("A", "\uFF21"));
  }

  public void testGetAvailableLocales() {
    assertTrue(Collator.getAvailableLocales().length > 0);
  }

  // Test CollationKey
  public void testCollationKey() {
    Collator coll = Collator.getInstance(Locale.US);
    String text = "abc";
    CollationKey key = coll.getCollationKey(text);
    key.hashCode();

    CollationKey key2 = coll.getCollationKey("abc");

    assertEquals(0, key.compareTo(key2));
  }

  public void testNullPointerException() throws Exception {
    //Regression for HARMONY-241
    try {
      new RuleBasedCollator(null);
      fail();
    } catch (NullPointerException expected) {
    }
  }

  public void testCompareNull() throws Exception {
    //Regression for HARMONY-836
    try {
      new RuleBasedCollator("&9 < a").compare(null, null);
      fail();
    } catch (NullPointerException expected) {
    }
  }

  public void testEmptyRules() throws Exception {
    new RuleBasedCollator("");
    new RuleBasedCollator(" ");
    new RuleBasedCollator("# This is a comment.");
  }
}
