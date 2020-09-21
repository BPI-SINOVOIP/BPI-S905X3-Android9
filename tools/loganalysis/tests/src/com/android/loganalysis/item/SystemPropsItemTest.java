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

import org.json.JSONException;
import org.json.JSONObject;

/**
 * Unit test for {@link SystemPropsItem}.
 */
public class SystemPropsItemTest extends TestCase {

    /**
     * Test that {@link SystemPropsItem#toJson()} returns correctly.
     */
    public void testToJson() throws JSONException {
        SystemPropsItem item = new SystemPropsItem();
        item.put("foo", "123");
        item.put("bar", "456");
        item.setText("[foo]: [123]\n[bar]: [456]");

        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        assertTrue(output.has(SystemPropsItem.LINES));
        assertTrue(output.get(SystemPropsItem.LINES) instanceof JSONObject);
        assertTrue(output.has(SystemPropsItem.TEXT));
        assertEquals("[foo]: [123]\n[bar]: [456]", output.get(SystemPropsItem.TEXT));

        JSONObject lines = output.getJSONObject(SystemPropsItem.LINES);

        assertEquals(2, lines.length());

        assertEquals("123", lines.get("foo"));
        assertEquals("456", lines.get("bar"));
    }
}
