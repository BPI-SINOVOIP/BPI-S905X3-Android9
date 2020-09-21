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

import com.android.loganalysis.item.WakelockItem.WakelockInfoItem;

import junit.framework.TestCase;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Unit test for {@link WakelockItem}.
 */
public class WakelockItemTest extends TestCase {

    /**
     * Test that {@link WakelockItem#toJson()} returns correctly.
     */
    public void testToJson() throws JSONException {
        WakelockItem item = new WakelockItem();
        item.addWakeLock("screen","u100", 150000, 25,
                WakelockItem.WakeLockCategory.PARTIAL_WAKELOCK);
        item.addWakeLock("wlan_rx", 150000, 25,
            WakelockItem.WakeLockCategory.KERNEL_WAKELOCK);

        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        assertTrue(output.has(WakelockItem.WAKELOCKS));
        assertTrue(output.get(WakelockItem.WAKELOCKS) instanceof JSONArray);

        JSONArray wakelockInfo = output.getJSONArray(WakelockItem.WAKELOCKS);

        assertEquals(2, wakelockInfo.length());
        assertTrue(wakelockInfo.getJSONObject(0).has(WakelockInfoItem.NAME));
        assertTrue(wakelockInfo.getJSONObject(0).has(WakelockInfoItem.PROCESS_UID));
        assertTrue(wakelockInfo.getJSONObject(0).has(WakelockInfoItem.HELD_TIME));
        assertTrue(wakelockInfo.getJSONObject(0).has(WakelockInfoItem.LOCKED_COUNT));
        assertTrue(wakelockInfo.getJSONObject(0).has(WakelockInfoItem.CATEGORY));

        assertTrue(wakelockInfo.getJSONObject(1).has(WakelockInfoItem.NAME));
        assertFalse(wakelockInfo.getJSONObject(1).has(WakelockInfoItem.PROCESS_UID));
        assertTrue(wakelockInfo.getJSONObject(1).has(WakelockInfoItem.HELD_TIME));
        assertTrue(wakelockInfo.getJSONObject(1).has(WakelockInfoItem.LOCKED_COUNT));
        assertTrue(wakelockInfo.getJSONObject(1).has(WakelockInfoItem.CATEGORY));

    }
}
