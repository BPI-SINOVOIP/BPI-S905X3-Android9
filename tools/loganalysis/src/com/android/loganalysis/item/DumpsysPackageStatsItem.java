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

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Map;

/** An {@link IItem} used to store apps and their version codes and names. */
public class DumpsysPackageStatsItem extends GenericMapItem<AppVersionItem> {
    private static final long serialVersionUID = 1L;

    /** Constant for JSON output */
    public static final String APP_VERSIONS = "APP_VERSIONS";

    /** {@inheritDoc} */
    @Override
    public JSONObject toJson() {
        JSONObject object = new JSONObject();
        try {
            JSONObject appVersions = new JSONObject();
            for (Map.Entry<String, AppVersionItem> entry : entrySet()) {
                appVersions.put(entry.getKey(), entry.getValue().toJson());
            }
            object.put(APP_VERSIONS, appVersions);
        } catch (JSONException e) {
            // Ignore
        }
        return object;
    }
}
