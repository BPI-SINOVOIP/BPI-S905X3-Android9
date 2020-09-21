/*
 * Copyright (C) 2017 The Android Open Source Project
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

/** Unit test for {@link DumpsysPackageStatsItem}. */
public class DumpsysPackageStatsItemTest extends TestCase {

    /** Test that {@link DumpsysPackageStatsItem#toJson()} returns correctly. */
    public void testToJson() throws JSONException {
        DumpsysPackageStatsItem item = new DumpsysPackageStatsItem();

        item.put("com.google.android.calculator", new AppVersionItem(73000302, "7.3 (3821978)"));
        item.put(
                "com.google.android.googlequicksearchbox",
                new AppVersionItem(300734793, "6.16.35.26.arm64"));

        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        assertTrue(output.has(DumpsysPackageStatsItem.APP_VERSIONS));

        JSONObject appVersionsJson = output.getJSONObject(DumpsysPackageStatsItem.APP_VERSIONS);

        assertEquals(2, appVersionsJson.length());
        final JSONObject calcAppVersionJson =
                appVersionsJson.getJSONObject("com.google.android.calculator");
        assertEquals(73000302, calcAppVersionJson.getInt(AppVersionItem.VERSION_CODE));
        assertEquals("7.3 (3821978)", calcAppVersionJson.getString(AppVersionItem.VERSION_NAME));
        final JSONObject gsaAppVersionJson =
                appVersionsJson.getJSONObject("com.google.android.googlequicksearchbox");
        assertEquals(300734793, gsaAppVersionJson.getInt(AppVersionItem.VERSION_CODE));
        assertEquals("6.16.35.26.arm64", gsaAppVersionJson.getString(AppVersionItem.VERSION_NAME));
    }
}
