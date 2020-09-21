/*
 * Copyright (C) 2015 The Android Open Source Project
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


import com.android.loganalysis.item.InterruptItem.InterruptInfoItem;

import junit.framework.TestCase;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Unit test for {@link InterruptItem}.
 */
public class InterruptItemTest extends TestCase {

    /**
     * Test that {@link InterruptItem#toJson()} returns correctly.
     */
    public void testToJson() throws JSONException {
        InterruptItem item = new InterruptItem();
        item.addInterrupt("smd-modem",25, InterruptItem.InterruptCategory.ALARM_INTERRUPT);

        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        assertTrue(output.has(InterruptItem.INTERRUPTS));
        assertTrue(output.get(InterruptItem.INTERRUPTS) instanceof JSONArray);

        JSONArray interruptsInfo = output.getJSONArray(InterruptItem.INTERRUPTS);

        assertEquals(1, interruptsInfo.length());
        assertTrue(interruptsInfo.getJSONObject(0).has(InterruptInfoItem.NAME));
        assertTrue(interruptsInfo.getJSONObject(0).has(InterruptInfoItem.CATEGORY));
        assertTrue(interruptsInfo.getJSONObject(0).has(InterruptInfoItem.INTERRUPT_COUNT));

    }
}
