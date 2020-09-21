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

import com.android.loganalysis.item.BatteryDischargeItem.BatteryDischargeInfoItem;

import junit.framework.TestCase;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Calendar;

/**
 * Unit test for {@link BatteryDischargeItem}.
 */
public class BatteryDischargeItemTest extends TestCase {

    /**
     * Test that {@link BatteryDischargeItem#toJson()} returns correctly.
     */
    public void testToJson() throws JSONException {
        BatteryDischargeItem item = new BatteryDischargeItem();
        item.addBatteryDischargeInfo(Calendar.getInstance(),25, 95);

        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        assertTrue(output.has(BatteryDischargeItem.BATTERY_DISCHARGE));
        assertTrue(output.get(BatteryDischargeItem.BATTERY_DISCHARGE) instanceof JSONArray);

        JSONArray dischargeInfo = output.getJSONArray(BatteryDischargeItem.BATTERY_DISCHARGE);

        assertEquals(1, dischargeInfo.length());
        assertTrue(dischargeInfo.getJSONObject(0).has(BatteryDischargeInfoItem.BATTERY_LEVEL));
        assertTrue(dischargeInfo.getJSONObject(0).has(
                BatteryDischargeInfoItem.DISCHARGE_ELAPSED_TIME));
        assertTrue(dischargeInfo.getJSONObject(0).has(
                BatteryDischargeInfoItem.CLOCK_TIME_OF_DISCHARGE));

    }
}
