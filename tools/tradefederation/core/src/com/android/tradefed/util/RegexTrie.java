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
package com.android.tradefed.util;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * The RegexTrie is a trie where each <emph>stored</emph> segment of the key is a regex
 * {@link Pattern}.  Thus, the full <emph>stored</emph> key is a {@code List&lt;Pattern&gt;} rather
 * than a {@code List&lt;String&gt;} as in a standard trie.  Note that the
 * {@link #retrieve(String...)} method will be pointwise matched against the {@code Pattern}s,
 * rather than checked for pointwise equality as in a standard trie.  Because of this, it may
 * perform poorly for large datasets.
 * <p />
 * One can also use a {@code null} entry in the {@code Pattern} sequence to serve as a wildcard.  If
 * a {@code null} is encountered, all subsequent entries in the sequence will be ignored.
 * When the retrieval code encounters a {@code null} {@code Pattern}, it will first wait to see if a
 * more-specific entry matches the sequence.  If one does, that more-specific entry will proceed,
 * even if it subsequently fails to match.
 * <p />
 * If no more-specific entry matches, the wildcard match will add all remaining {@code String}s
 * to the list of captures (if enabled) and return the value associated with the wildcard.
 * <p />
 * A short sample of the wildcard functionality:
 * <pre>
 * List&lt;List&lt;String&gt;&gt; captures = new LinkedList&lt;List&lt;String&gt;&gt;();
 * RegexTrie&lt;Integer&gt; trie = new RegexTrie&lt;Integer&gt;();
 * trie.put(2, "a", null);
 * trie.put(4, "a", "b");
 * trie.retrieve(captures, "a", "c", "e");
 * // returns 2.  captures is now [[], ["c"], ["e"]]
 * trie.retrieve(captures, "a", "b");
 * // returns 4.  captures is now [[], []]
 * trie.retrieve(captures, "a", "b", "c");
 * // returns null.  captures is now [[], []]
 * </pre>
 */
public class RegexTrie<V> {
    private V mValue = null;
    private Map<CompPattern, RegexTrie<V>> mChildren =
            new LinkedHashMap<CompPattern, RegexTrie<V>>();

    /**
     * Patterns aren't comparable by default, which prevents you from retrieving them from a
     * HashTable.  This is a simple stub class that makes a Pattern with a working
     * {@link com.android.tradefed.util.RegexTrie.CompPattern#equals(Object)} method.
     */
    static class CompPattern {
        protected final Pattern mPattern;

        CompPattern(Pattern pattern) {
            if (pattern == null) {
                throw new NullPointerException();
            }
            mPattern = pattern;
        }

        @Override
        public boolean equals(Object other) {
            Pattern otherPat;
            if (other instanceof Pattern) {
                otherPat = (Pattern) other;
            } else if (other instanceof CompPattern) {
                CompPattern otherCPat = (CompPattern) other;
                otherPat = otherCPat.mPattern;
            } else {
                return false;
            }
            return mPattern.toString().equals(otherPat.toString());
        }

        @Override
        public int hashCode() {
            return mPattern.toString().hashCode();
        }

        @Override
        public String toString() {
            return String.format("CP(%s)", mPattern.toString());
        }

        public Matcher matcher(String string) {
            return mPattern.matcher(string);
        }
    }

    public void clear() {
        mValue = null;
        for (RegexTrie<V> child : mChildren.values()) {
            child.clear();
        }
        mChildren.clear();
    }

    boolean containsKey(String... strings) {
        return retrieve(strings) != null;
    }

    V recursivePut(V value, List<CompPattern> patterns) {
        // Cases:
        // 1) patterns is empty -- set our value
        // 2) patterns is non-empty -- recurse downward, creating a child if necessary
        if (patterns.isEmpty()) {
            V oldValue = mValue;
            mValue = value;
            return oldValue;
        } else {
            CompPattern curKey = patterns.get(0);
            List<CompPattern> nextKeys = patterns.subList(1, patterns.size());

            // Create a new child to handle
            RegexTrie<V> nextChild = mChildren.get(curKey);
            if (nextChild == null) {
                nextChild = new RegexTrie<V>();
                mChildren.put(curKey, nextChild);
            }
            return nextChild.recursivePut(value, nextKeys);
        }
    }

    /**
     * A helper method to consolidate validation before adding an entry to the trie.
     *
     * @param value The value to set
     * @param pList The sequence of {@link CompPattern}s that must be sequentially matched to
     *        retrieve the associated {@code value}
     */
    private V validateAndPut(V value, List<CompPattern> pList) {
        if (pList.size() == 0) {
            throw new IllegalArgumentException("pattern list must be non-empty.");
        }
        return recursivePut(value, pList);
    }

    /**
     * Add an entry to the trie.
     *
     * @param value The value to set
     * @param patterns The sequence of {@link Pattern}s that must be sequentially matched to
     *        retrieve the associated {@code value}
     */
    public V put(V value, Pattern... patterns) {
        List<CompPattern> pList = new ArrayList<CompPattern>(patterns.length);
        for (Pattern pat : patterns) {
            if (pat == null) {
                pList.add(null);
                break;
            }
            pList.add(new CompPattern(pat));
        }
        return validateAndPut(value, pList);
    }

    /**
     * This helper method takes a list of regular expressions as {@link String}s and compiles them
     * on-the-fly before adding the subsequent {@link Pattern}s to the trie
     *
     * @param value The value to set
     * @param regexen The sequence of regular expressions (as {@link String}s) that must be
     *        sequentially matched to retrieve the associated {@code value}.  Each String will be
     *        compiled as a {@link Pattern} before invoking {@link #put(Object, Pattern...)}.
     */
    public V put(V value, String... regexen) {
        List<CompPattern> pList = new ArrayList<CompPattern>(regexen.length);
        for (String regex : regexen) {
            if (regex == null) {
                pList.add(null);
                break;
            }
            Pattern pat = Pattern.compile(regex);
            pList.add(new CompPattern(pat));
        }
        return validateAndPut(value, pList);
    }

    V recursiveRetrieve(List<List<String>> captures, List<String> strings) {
        // Cases:
        // 1) strings is empty -- return our value
        // 2) strings is non-empty -- find the first child that matches, recurse downward
        if (strings.isEmpty()) {
            return mValue;
        } else {
            boolean wildcardMatch = false;
            V wildcardValue = null;
            String curKey = strings.get(0);
            List<String> nextKeys = strings.subList(1, strings.size());

            for (Map.Entry<CompPattern, RegexTrie<V>> child : mChildren.entrySet()) {
                CompPattern pattern = child.getKey();
                if (pattern == null) {
                    wildcardMatch = true;
                    wildcardValue = child.getValue().getValue();
                    continue;
                }

                Matcher matcher = pattern.matcher(curKey);
                if (matcher.matches()) {
                    if (captures != null) {
                        List<String> curCaptures = new ArrayList<String>(matcher.groupCount());
                        for (int i = 0; i < matcher.groupCount(); i++) {
                            // i+1 since group 0 is the entire matched string
                            curCaptures.add(matcher.group(i+1));
                        }
                        captures.add(curCaptures);
                    }

                    return child.getValue().recursiveRetrieve(captures, nextKeys);
                }
            }

            if (wildcardMatch) {
                // Stick the rest of the query string into the captures list and return
                if (captures != null) {
                    for (String str : strings) {
                        captures.add(Arrays.asList(str));
                    }
                }
                return wildcardValue;
            }

            // no match
            return null;
        }
    }

    /**
     * Fetch a value from the trie, by matching the provided sequence of {@link String}s to a
     * sequence of {@link Pattern}s stored in the trie.
     *
     * @param strings A sequence of {@link String}s to match
     * @return The associated value, or {@code null} if no value was found
     */
    public V retrieve(String... strings) {
        return retrieve(null, strings);
    }

    /**
     * Fetch a value from the trie, by matching the provided sequence of {@link String}s to a
     * sequence of {@link Pattern}s stored in the trie.  This version of the method also returns
     * a {@link List} of capture groups for each {@link Pattern} that was matched.
     * <p />
     * Each entry in the outer List corresponds to one level of {@code Pattern} in the trie.
     * For each level, the list of capture groups will be stored.  If there were no captures
     * for a particular level, an empty list will be stored.
     * <p />
     * Note that {@code captures} will be {@link List#clear()}ed before the retrieval begins.
     * Also, if the retrieval fails after a partial sequence of matches, {@code captures} will
     * still reflect the capture groups from the partial match.
     *
     * @param captures A {@code List<List<String>>} through which capture groups will be returned.
     * @param strings A sequence of {@link String}s to match
     * @return The associated value, or {@code null} if no value was found
     */
    public V retrieve(List<List<String>> captures, String... strings) {
        if (strings.length == 0) {
            throw new IllegalArgumentException("string list must be non-empty");
        }
        List<String> sList = Arrays.asList(strings);
        if (captures != null) {
            captures.clear();
        }
        return recursiveRetrieve(captures, sList);
    }

    private V getValue() {
        return mValue;
    }

    @Override
    public String toString() {
        return String.format("{V: %s, C: %s}", mValue, mChildren);
    }
}

