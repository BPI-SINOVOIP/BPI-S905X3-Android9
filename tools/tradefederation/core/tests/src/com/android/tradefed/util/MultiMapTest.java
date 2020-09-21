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

import junit.framework.TestCase;

import java.util.Map;

/**
 * Unit tests for {@link MultiMap}.
 */
public class MultiMapTest extends TestCase {

    /**
     * Test for {@link MultiMap#getUniqueMap()}.
     */
    public void testGetUniqueMap() {
        MultiMap<String, String> multiMap = new MultiMap<String, String>();
        multiMap.put("key", "value1");
        multiMap.put("key", "value2");
        multiMap.put("uniquekey", "value");
        multiMap.put("key2", "collisionKeyvalue");
        Map<String, String> uniqueMap = multiMap.getUniqueMap();
        assertEquals(4, uniqueMap.size());
        // key's for value1, value2 and collisionKeyvalue might be one of three possible values,
        // depending on order of collision resolvement
        assertTrue(checkKeyForValue(uniqueMap, "value1"));
        assertTrue(checkKeyForValue(uniqueMap, "value2"));
        assertTrue(checkKeyForValue(uniqueMap, "collisionKeyvalue"));
        // uniquekey should be unmodified
        assertEquals("value", uniqueMap.get("uniquekey"));
    }

    /**
     * Helper method testGetUniqueMap() for that will check that the given value's key matches
     * one of the expected values
     *
     * @param uniqueMap
     * @param value
     * @return <code>true</code> if key matched one of the expected values
     */
    private boolean checkKeyForValue(Map<String, String> uniqueMap, String value) {
        for (Map.Entry<String, String> entry : uniqueMap.entrySet()) {
            if (entry.getValue().equals(value)) {
                String key = entry.getKey();
                return key.equals("key") || key.equals("key2") || key.equals("key2X");
            }
        }
        return false;
    }
}
