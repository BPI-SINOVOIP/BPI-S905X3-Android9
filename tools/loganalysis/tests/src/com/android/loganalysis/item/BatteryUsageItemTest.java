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

import com.android.loganalysis.item.BatteryUsageItem.BatteryUsageInfoItem;

import junit.framework.TestCase;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Unit test for {@link BatteryUsageItem}.
 */
public class BatteryUsageItemTest extends TestCase {

    /**
     * Test that {@link BatteryUsageItem#toJson()} returns correctly.
     */
    public void testToJson() throws JSONException {
        BatteryUsageItem item = new BatteryUsageItem();
        item.addBatteryUsage("Cell standby", 2925);
        item.addBatteryUsage("Uid u0a71", 68.1);
        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        assertTrue(output.has(BatteryUsageItem.BATTERY_CAPACITY));
        assertTrue(output.get(BatteryUsageItem.BATTERY_USAGE) instanceof JSONArray);

        JSONArray usage = output.getJSONArray(BatteryUsageItem.BATTERY_USAGE);

        assertEquals(2, usage.length());
        assertTrue(usage.getJSONObject(0).has(BatteryUsageInfoItem.NAME));
        assertTrue(usage.getJSONObject(0).has(BatteryUsageInfoItem.USAGE));

        assertTrue(usage.getJSONObject(1).has(BatteryUsageInfoItem.NAME));
        assertTrue(usage.getJSONObject(1).has(BatteryUsageInfoItem.USAGE));
    }
}
