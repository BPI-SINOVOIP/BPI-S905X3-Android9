/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.loganalysis.item;

import junit.framework.TestCase;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Unit test for {@link MonkeyLogItem}.
 */
public class MonkeyLogItemTest extends TestCase {
    /**
     * Test that {@link MonkeyLogItem#toJson()} returns correctly.
     */
    public void testToJson() throws JSONException {
        MonkeyLogItem item = new MonkeyLogItem();
        item.addCategory("category1");
        item.addCategory("category2");
        item.addPackage("package1");
        item.addPackage("package2");
        item.addPackage("package3");

        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        assertTrue(output.has(MonkeyLogItem.CATEGORIES));
        assertTrue(output.get(MonkeyLogItem.CATEGORIES) instanceof JSONArray);

        JSONArray categories = output.getJSONArray(MonkeyLogItem.CATEGORIES);

        assertEquals(2, categories.length());
        assertTrue(in("category1", categories));
        assertTrue(in("category2", categories));

        JSONArray packages = output.getJSONArray(MonkeyLogItem.PACKAGES);

        assertEquals(3, packages.length());
        assertTrue(in("package1", packages));
        assertTrue(in("package2", packages));
        assertTrue(in("package3", packages));
    }

    private boolean in(String value, JSONArray array) throws JSONException {
        for (int i = 0; i < array.length(); i++) {
            if (value.equals(array.get(i))) {
                return true;
            }
        }
        return false;
    }
}
