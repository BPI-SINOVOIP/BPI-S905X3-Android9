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

package com.android.tradefed.util;

import com.android.tradefed.log.LogUtil.CLog;
import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;
import java.util.Iterator;

/**
 * A helper class for JSON related operations
 */
public class JsonUtil {
    /**
     * Deep merge two JSONObjects.
     * This method merge source JSONObject fields into the target JSONObject without overwriting.
     * JSONArray will not be deep merged.
     *
     * @param target the target json object to merge
     * @param source the source json object to read
     * @throws JSONException
     */
    public static void deepMergeJsonObjects(JSONObject target, JSONObject source)
            throws JSONException {
        Iterator<String> iter = source.keys();
        while (iter.hasNext()) {
            String key = iter.next();
            Object source_value = source.get(key);
            Object target_value = null;
            try {
                target_value = target.get(key);
            } catch (JSONException e) {
                CLog.i("Merging Json key '%s' into target object.", key);
                target.put(key, source_value);
                continue;
            }

            if (JSONObject.class.isInstance(target_value)
                    && JSONObject.class.isInstance(source_value)) {
                deepMergeJsonObjects((JSONObject) target_value, (JSONObject) source_value);
            }
        }
    }
}