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
 * Unit test for {@link ProcrankItem}.
 */
public class ProcrankItemTest extends TestCase {

    /**
     * Test that {@link ProcrankItem#toJson()} returns correctly.
     */
    public void testToJson() throws JSONException {
        ProcrankItem item = new ProcrankItem();
        item.addProcrankLine(0, "process0", 1, 2, 3, 4);
        item.addProcrankLine(5, "process1", 6, 7, 8, 9);
        item.setText("foo\nbar");

        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        assertTrue(output.has(ProcrankItem.LINES));
        assertTrue(output.get(ProcrankItem.LINES) instanceof JSONArray);
        assertTrue(output.has(ProcrankItem.TEXT));
        assertEquals("foo\nbar", output.get(ProcrankItem.TEXT));

        JSONArray lines = output.getJSONArray(ProcrankItem.LINES);

        assertEquals(2, lines.length());
        assertTrue(lines.get(0) instanceof JSONObject);

        JSONObject line = lines.getJSONObject(0);

        assertEquals(0, line.get(ProcrankItem.PID));
        assertEquals("process0", line.get(ProcrankItem.PROCESS_NAME));
        assertEquals(1, line.get(ProcrankItem.VSS));
        assertEquals(2, line.get(ProcrankItem.RSS));
        assertEquals(3, line.get(ProcrankItem.PSS));
        assertEquals(4, line.get(ProcrankItem.USS));
    }
}
