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
package com.android.tradefed.util.hostmetric;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.Map;

/** This class represents a host metric sample to be reported. */
public class HostMetric {
    private String mName;
    private long mTimestamp;
    private long mValue;
    private Map<String, String> mData;

    /**
     * Constructor.
     *
     * @param name name of the metric
     * @param timestamp timestamp of the sample
     * @param value value of the sample
     * @param data data associated with the sample
     */
    public HostMetric(String name, long timestamp, long value, Map<String, String> data) {
        mName = name;
        mTimestamp = timestamp;
        mValue = value;
        mData = new HashMap<String, String>(data);
    }

    /**
     * Returns a JSON object for the metric sample.
     *
     * @return a JSON object.
     * @throws JSONException
     */
    public JSONObject toJson() throws JSONException {
        final JSONObject json = new JSONObject();
        json.put("name", mName);
        json.put("timestamp", mTimestamp);
        json.put("value", mValue);
        json.put("fields", new JSONObject(mData));
        return json;
    }
}
