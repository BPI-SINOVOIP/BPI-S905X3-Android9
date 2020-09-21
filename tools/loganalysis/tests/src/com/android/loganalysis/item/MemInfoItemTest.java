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
 * Unit test for {@link MemInfoItem}.
 */
public class MemInfoItemTest extends TestCase {

    /**
     * Test that {@link MemInfoItem#toJson()} returns correctly.
     */
    public void testToJson() throws JSONException {
        MemInfoItem item = new MemInfoItem();
        item.put("foo", 123l);
        item.put("bar", 456l);
        item.setText("foo: 123 kB\nbar: 456 kB");

        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        assertTrue(output.has(MemInfoItem.LINES));
        assertTrue(output.get(MemInfoItem.LINES) instanceof JSONObject);
        assertTrue(output.has(MemInfoItem.TEXT));
        assertEquals("foo: 123 kB\nbar: 456 kB", output.get(MemInfoItem.TEXT));

        JSONObject lines = output.getJSONObject(MemInfoItem.LINES);

        assertEquals(2, lines.length());

        assertEquals(123, lines.get("foo"));
        assertEquals(456, lines.get("bar"));
    }
}
