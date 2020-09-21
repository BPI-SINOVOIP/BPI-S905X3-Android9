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

import java.util.Date;

/**
 * Unit test for {@link SmartMonkeyLogItem}.
 */
public class SmartMonkeyLogItemTest extends TestCase {
    /**
     * Test that {@link SmartMonkeyLogItem#toJson()} returns correctly.
     */
    public void testToJson() throws JSONException {
        SmartMonkeyLogItem item = new SmartMonkeyLogItem();
        item.addApplication("application1");
        item.addPackage("package1");
        item.addAnrTime(new Date());
        item.addCrashTime(new Date());

        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        assertTrue(output.has(SmartMonkeyLogItem.APPLICATIONS));
        assertTrue(output.get(SmartMonkeyLogItem.APPLICATIONS) instanceof JSONArray);
        assertTrue(output.has(SmartMonkeyLogItem.PACKAGES));
        assertTrue(output.get(SmartMonkeyLogItem.PACKAGES) instanceof JSONArray);
        assertTrue(output.has(SmartMonkeyLogItem.ANR_TIMES));
        assertTrue(output.get(SmartMonkeyLogItem.ANR_TIMES) instanceof JSONArray);
        assertTrue(output.has(SmartMonkeyLogItem.CRASH_TIMES));
        assertTrue(output.get(SmartMonkeyLogItem.CRASH_TIMES) instanceof JSONArray);
    }
}
