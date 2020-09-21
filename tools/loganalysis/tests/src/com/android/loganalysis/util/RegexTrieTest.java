/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.loganalysis.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import com.android.loganalysis.util.RegexTrie.CompPattern;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.regex.Pattern;

/** Set of unit tests to verify the behavior of the RegexTrie */
@RunWith(JUnit4.class)
public class RegexTrieTest {
    private RegexTrie<Integer> mTrie = null;
    private static final Integer STORED_VAL = 42;
    private static final List<String> NULL_LIST = Arrays.asList((String)null);

    @Before
    public void setUp() throws Exception {
        mTrie = new RegexTrie<Integer>();
    }

    @Test
    public void testStringPattern() {
        mTrie.put(STORED_VAL, "[p]art1", "[p]art2", "[p]art3");
        Integer retrieved = mTrie.retrieve("part1", "part2", "part3");
        assertEquals(STORED_VAL, retrieved);
    }

    @Test
    public void testAlternation_single() {
        mTrie.put(STORED_VAL, "alpha|beta");
        Integer retrieved;
        retrieved = mTrie.retrieve("alpha");
        assertEquals(STORED_VAL, retrieved);
        retrieved = mTrie.retrieve("beta");
        assertEquals(STORED_VAL, retrieved);
        retrieved = mTrie.retrieve("alpha|beta");
        assertNull(retrieved);
        retrieved = mTrie.retrieve("gamma");
        assertNull(retrieved);
        retrieved = mTrie.retrieve("alph");
        assertNull(retrieved);
    }

    @Test
    public void testAlternation_multiple() {
        mTrie.put(STORED_VAL, "a|alpha", "b|beta");
        Integer retrieved;
        retrieved = mTrie.retrieve("a", "b");
        assertEquals(STORED_VAL, retrieved);
        retrieved = mTrie.retrieve("a", "beta");
        assertEquals(STORED_VAL, retrieved);
        retrieved = mTrie.retrieve("alpha", "b");
        assertEquals(STORED_VAL, retrieved);
        retrieved = mTrie.retrieve("alpha", "beta");
        assertEquals(STORED_VAL, retrieved);

        retrieved = mTrie.retrieve("alpha");
        assertNull(retrieved);
        retrieved = mTrie.retrieve("beta");
        assertNull(retrieved);
        retrieved = mTrie.retrieve("alpha", "bet");
        assertNull(retrieved);
    }

    @Test
    public void testGroups_fullMatch() {
        mTrie.put(STORED_VAL, "a|(alpha)", "b|(beta)");
        Integer retrieved;
        List<List<String>> groups = new ArrayList<List<String>>();

        retrieved = mTrie.retrieve(groups, "a", "b");
        assertEquals(STORED_VAL, retrieved);
        assertEquals(2, groups.size());
        assertEquals(NULL_LIST, groups.get(0));
        assertEquals(NULL_LIST, groups.get(1));

        retrieved = mTrie.retrieve(groups, "a", "beta");
        assertEquals(STORED_VAL, retrieved);
        assertEquals(2, groups.size());
        assertEquals(NULL_LIST, groups.get(0));
        assertEquals(Arrays.asList("beta"), groups.get(1));

        retrieved = mTrie.retrieve(groups, "alpha", "b");
        assertEquals(STORED_VAL, retrieved);
        assertEquals(2, groups.size());
        assertEquals(Arrays.asList("alpha"), groups.get(0));
        assertEquals(NULL_LIST, groups.get(1));

        retrieved = mTrie.retrieve(groups, "alpha", "beta");
        assertEquals(STORED_VAL, retrieved);
        assertEquals(2, groups.size());
        assertEquals(Arrays.asList("alpha"), groups.get(0));
        assertEquals(Arrays.asList("beta"), groups.get(1));
    }

    @Test
    public void testGroups_partialMatch() {
        mTrie.put(STORED_VAL, "a|(alpha)", "b|(beta)");
        Integer retrieved;
        List<List<String>> groups = new ArrayList<List<String>>();

        retrieved = mTrie.retrieve(groups, "alpha");
        assertNull(retrieved);
        assertEquals(1, groups.size());
        assertEquals(Arrays.asList("alpha"), groups.get(0));

        retrieved = mTrie.retrieve(groups, "beta");
        assertNull(retrieved);
        assertEquals(0, groups.size());

        retrieved = mTrie.retrieve(groups, "alpha", "bet");
        assertNull(retrieved);
        assertEquals(1, groups.size());
        assertEquals(Arrays.asList("alpha"), groups.get(0));

        retrieved = mTrie.retrieve(groups, "alpha", "betar");
        assertNull(retrieved);
        assertEquals(1, groups.size());
        assertEquals(Arrays.asList("alpha"), groups.get(0));

        retrieved = mTrie.retrieve(groups, "alpha", "beta", "gamma");
        assertNull(retrieved);
        assertEquals(2, groups.size());
        assertEquals(Arrays.asList("alpha"), groups.get(0));
        assertEquals(Arrays.asList("beta"), groups.get(1));
    }

    /** Make sure that the wildcard functionality works */
    @Test
    public void testWildcard() {
        mTrie.put(STORED_VAL, "a", null);
        Integer retrieved;
        List<List<String>> groups = new ArrayList<List<String>>();

        retrieved = mTrie.retrieve(groups, "a", "b", "c");
        assertEquals(STORED_VAL, retrieved);
        assertEquals(3, groups.size());
        assertTrue(groups.get(0).isEmpty());
        assertEquals(Arrays.asList("b"), groups.get(1));
        assertEquals(Arrays.asList("c"), groups.get(2));

        retrieved = mTrie.retrieve(groups, "a");
        assertNull(retrieved);
        assertEquals(1, groups.size());
        assertTrue(groups.get(0).isEmpty());
    }

    /**
     * Make sure that if a wildcard and a more specific match could both match, that the more
     * specific match takes precedence
     */
    @Test
    public void testWildcard_precedence() {
        // Do one before and one after the wildcard to check for ordering effects
        mTrie.put(STORED_VAL + 1, "a", "(b)");
        mTrie.put(STORED_VAL, "a", null);
        mTrie.put(STORED_VAL + 2, "a", "(c)");
        Integer retrieved;
        List<List<String>> groups = new ArrayList<List<String>>();

        retrieved = mTrie.retrieve(groups, "a", "d");
        assertEquals(STORED_VAL, retrieved);
        assertEquals(2, groups.size());
        assertTrue(groups.get(0).isEmpty());
        assertEquals(Arrays.asList("d"), groups.get(1));

        retrieved = mTrie.retrieve(groups, "a", "b");
        assertEquals((Integer)(STORED_VAL + 1), retrieved);
        assertEquals(2, groups.size());
        assertTrue(groups.get(0).isEmpty());
        assertEquals(Arrays.asList("b"), groups.get(1));

        retrieved = mTrie.retrieve(groups, "a", "c");
        assertEquals((Integer)(STORED_VAL + 2), retrieved);
        assertEquals(2, groups.size());
        assertTrue(groups.get(0).isEmpty());
        assertEquals(Arrays.asList("c"), groups.get(1));
    }

    /**
     * Verify a bugfix: make sure that no NPE results from calling #retrieve with a wildcard but
     * without a place to retrieve captures.
     */
    @Test
    public void testWildcard_noCapture() throws NullPointerException {
        mTrie.put(STORED_VAL, "a", null);
        String[] key = new String[] {"a", "b", "c"};

        mTrie.retrieve(key);
        mTrie.retrieve(null, key);
        // test passes if no exceptions were thrown
    }

    @Test
    public void testMultiChild() {
        mTrie.put(STORED_VAL + 1, "a", "b");
        mTrie.put(STORED_VAL + 2, "a", "c");

        Object retrieved;
        retrieved = mTrie.retrieve("a", "b");
        assertEquals(STORED_VAL + 1, retrieved);
        retrieved = mTrie.retrieve("a", "c");
        assertEquals(STORED_VAL + 2, retrieved);
    }

    /**
     * Make sure that {@link CompPattern#equals} works as expected. Shake a proverbial fist at Java
     */
    @Test
    public void testCompPattern_equality() {
        String regex = "regex";
        Pattern p1 = Pattern.compile(regex);
        Pattern p2 = Pattern.compile(regex);
        Pattern pOther = Pattern.compile("other");
        CompPattern cp1 = new CompPattern(p1);
        CompPattern cp2 = new CompPattern(p2);
        CompPattern cpOther = new CompPattern(pOther);

        // This is the problem with Pattern as implemented
        assertNotEquals(p1, p2);
        assertNotEquals(p2, p1);

        // Make sure that wrapped patterns with the same regex are considered equivalent
        assertEquals(cp2, p1);
        assertEquals(cp2, p1);
        assertEquals(cp2, cp1);

        // And make sure that wrapped patterns with different regexen are still considered different
        assertNotEquals(cp2, pOther);
        assertNotEquals(cp2, cpOther);
    }

    @Test
    public void testCompPattern_hashmap() {
        HashMap<CompPattern, Integer> map = new HashMap<CompPattern, Integer>();
        String regex = "regex";
        Pattern p1 = Pattern.compile(regex);
        Pattern p2 = Pattern.compile(regex);
        Pattern pOther = Pattern.compile("other");
        CompPattern cp1 = new CompPattern(p1);
        CompPattern cp2 = new CompPattern(p2);
        CompPattern cpOther = new CompPattern(pOther);

        map.put(cp1, STORED_VAL);
        assertTrue(map.containsKey(cp1));
        assertTrue(map.containsKey(cp2));
        assertFalse(map.containsKey(cpOther));

        map.put(cpOther, STORED_VAL);
        assertEquals(map.size(), 2);
        assertTrue(map.containsKey(cp1));
        assertTrue(map.containsKey(cp2));
        assertTrue(map.containsKey(cpOther));
    }
}

