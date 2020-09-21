/*
 * Copyright (C) 2016 The Android Open Source Project
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

import com.android.loganalysis.item.LocationDumpsItem.LocationInfoItem;

import junit.framework.TestCase;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Unit test for {@link LocationDumpsItem}.
 */
public class LocationDumpsItemTest extends TestCase {

    /**
     * Test that {@link LocationDumpsItem#toJson()} returns correctly.
     */
    public void testToJson() throws JSONException {
        LocationDumpsItem item = new LocationDumpsItem();
        item.addLocationClient("com.google.android.gms", 500, 60, 1000, "PRIORITY_ACCURACY", 45);
        item.addLocationClient("com.google.android.maps", 0, 0, 0, "PRIORITY_ACCURACY", 55);

        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        assertTrue(output.has(LocationDumpsItem.LOCATION_CLIENTS));
        assertTrue(output.get(LocationDumpsItem.LOCATION_CLIENTS) instanceof JSONArray);

        JSONArray locationClients = output.getJSONArray(LocationDumpsItem.LOCATION_CLIENTS);

        assertEquals(2, locationClients.length());
        assertTrue(locationClients.getJSONObject(0).has(LocationInfoItem.PACKAGE));
        assertTrue(locationClients.getJSONObject(0).has(LocationInfoItem.EFFECTIVE_INTERVAL));
        assertTrue(locationClients.getJSONObject(0).has(LocationInfoItem.MIN_INTERVAL));
        assertTrue(locationClients.getJSONObject(0).has(LocationInfoItem.MAX_INTERVAL));
        assertTrue(locationClients.getJSONObject(0).has(LocationInfoItem.REQUEST_PRIORITY));
        assertTrue(locationClients.getJSONObject(0).has(LocationInfoItem.LOCATION_DURATION));

        assertTrue(locationClients.getJSONObject(1).has(LocationInfoItem.PACKAGE));
        assertTrue(locationClients.getJSONObject(1).has(LocationInfoItem.EFFECTIVE_INTERVAL));
        assertTrue(locationClients.getJSONObject(1).has(LocationInfoItem.MIN_INTERVAL));
        assertTrue(locationClients.getJSONObject(1).has(LocationInfoItem.MAX_INTERVAL));
        assertTrue(locationClients.getJSONObject(1).has(LocationInfoItem.REQUEST_PRIORITY));
        assertTrue(locationClients.getJSONObject(1).has(LocationInfoItem.LOCATION_DURATION));
    }

    /**
     * Test that {@link LocationDumpsItem#getLocationClients()} returns correctly.
     */
    public void testGetLocationDumps() {
        LocationDumpsItem item = new LocationDumpsItem();
        item.addLocationClient("com.google.android.gms", 500, 60, 1000, "PRIORITY_ACCURACY", 45);

        assertEquals(item.getLocationClients().size(), 1);
        LocationInfoItem client = item.getLocationClients().iterator().next();
        assertNotNull(client);
        assertEquals(client.getPackage(), "com.google.android.gms");
        assertEquals(client.getEffectiveInterval(), 500);
        assertEquals(client.getMinInterval(), 60);
        assertEquals(client.getMaxInterval(), 1000);
        assertEquals(client.getPriority(), "PRIORITY_ACCURACY");
        assertEquals(client.getDuration(), 45);
    }

}
