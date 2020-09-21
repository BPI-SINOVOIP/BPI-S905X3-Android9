/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.tradefed.build.BuildSerializedVersion;

import com.google.common.base.Objects;

import java.io.Serializable;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** A {@link Map} that supports multiple values per key. */
public class MultiMap<K, V> implements Serializable {

    private static final long serialVersionUID = BuildSerializedVersion.VERSION;
    private final Map<K, List<V>> mInternalMap;

    public MultiMap() {
        mInternalMap = new HashMap<K, List<V>>();
    }

    /**
     * Clears the map.
     */
    public void clear() {
        mInternalMap.clear();
    }

    /**
     * Checks whether the map contains the specified key.
     *
     * @see Map#containsKey(Object)
     */
    public boolean containsKey(K key) {
        return mInternalMap.containsKey(key);
    }

    /**
     * Checks whether the map contains the specified value.
     *
     * @see Map#containsValue(Object)
     */
    public boolean containsValue(V value) {
        for (List<V> valueList : mInternalMap.values()) {
            if (valueList.contains(value)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Gets the list of values associated with each key.
     */
    public List<V> get(K key) {
        return mInternalMap.get(key);
    }

    /**
     * @see Map#isEmpty()
     */
    public boolean isEmpty() {
        return mInternalMap.isEmpty();
    }

    /**
     * Check if map is empty.
     */
    public Set<K> keySet() {
        return mInternalMap.keySet();
    }

    /**
     * Adds the value to the list associated with a key.
     *
     * @see Map#put(Object, Object)
     */
    public V put(K key, V value) {
        List<V> valueList = mInternalMap.get(key);
        if (valueList == null) {
            valueList = new LinkedList<V>();
            mInternalMap.put(key, valueList);
        }
        valueList.add(value);
        return value;
    }

    /**
     *
     * Adds all entries in given {@link Map} to this {@link MultiMap}.
     */
    public void putAll(Map<? extends K, ? extends V> m) {
        for (Map.Entry<? extends K, ? extends V> entry : m.entrySet()) {
            put(entry.getKey(), entry.getValue());
        }
    }

   /**
    * Adds all entries in given {@link MultiMap} to this {@link MultiMap}.
    */
   public void putAll(MultiMap<K, ? extends V> m) {
       for (K key : m.keySet()) {
           for (V value : m.get(key)) {
               put(key, value);
           }
       }
   }

    /**
     * Removes all values associated with the specified key.
     */
    public List<V> remove(K key) {
        return mInternalMap.remove(key);
    }

    /**
     * Returns the number of keys in the map
     */
    public int size() {
        return mInternalMap.size();
    }

    /**
     * Returns list of all values.
     */
    public List<V> values() {
        List<V> allValues = new LinkedList<V>();
        for (List<V> valueList : mInternalMap.values()) {
            allValues.addAll(valueList);
        }
        return allValues;
    }

    /**
     * Construct a new map, that contains a unique String key for each value.
     *
     * Current algorithm will construct unique key by appending a unique position number to
     * key's toString() value
     *
     * @return a {@link Map}
     */
    public Map<String, V> getUniqueMap() {
        Map<String, V> uniqueMap = new HashMap<String, V>();
        for (Map.Entry<K, List<V>> entry : mInternalMap.entrySet()) {
            int count = 1;
            for (V value : entry.getValue()) {
                if (count == 1) {
                    addUniqueEntry(uniqueMap, entry.getKey().toString(), value);
                } else {
                    // append unique number to key for each value
                    addUniqueEntry(uniqueMap, String.format("%s%d", entry.getKey(), count), value);
                }
                count++;
            }
        }
        return uniqueMap;
    }

    /**
     * Recursive method that will append characters to proposedKey until its unique. Used in case
     * there are collisions with generated key values.
     *
     * @param uniqueMap
     * @param proposedKey
     * @param value
     */
    private String addUniqueEntry(Map<String, V> uniqueMap, String proposedKey, V value) {
        // not the most efficient algorithm, but should work
        if (uniqueMap.containsKey(proposedKey)) {
            return addUniqueEntry(uniqueMap, String.format("%s%s", proposedKey, "X"), value);
        } else {
            uniqueMap.put(proposedKey, value);
            return proposedKey;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        return mInternalMap.hashCode();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        MultiMap<?, ?> other = (MultiMap<?, ?>) obj;
        return Objects.equal(mInternalMap, other.mInternalMap);
    }
}
