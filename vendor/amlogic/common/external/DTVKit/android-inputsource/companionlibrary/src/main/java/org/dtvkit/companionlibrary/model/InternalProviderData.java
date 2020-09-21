/*
 * Copyright 2016 Google Inc. All rights reserved.
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
 *
 * Modifications copyright (C) 2018 DTVKit
 */

package org.dtvkit.companionlibrary.model;

import android.support.annotation.NonNull;
import android.util.Log;

import org.dtvkit.companionlibrary.utils.TvContractUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * This is a serialized class used for storing and retrieving serialized data from
 * {@link android.media.tv.TvContract.Channels#COLUMN_INTERNAL_PROVIDER_DATA},
 * {@link android.media.tv.TvContract.Programs#COLUMN_INTERNAL_PROVIDER_DATA}, and
 * {@link android.media.tv.TvContract.RecordedPrograms#COLUMN_INTERNAL_PROVIDER_DATA}.
 *
 * In addition to developers being able to add custom attributes to this data type, there are
 * pre-defined values.
 */
public class InternalProviderData {
    private static final String TAG = "InternalProviderData";
    private static final String KEY_CUSTOM_DATA = "custom";

    private JSONObject mJsonObject;

    /**
     * Creates a new empty object
     */
    public InternalProviderData() {
        mJsonObject = new JSONObject();
    }

    /**
     * Creates a new object and attempts to populate by obtaining the String representation of the
     * provided byte array
     *
     * @param bytes Byte array corresponding to a correctly formatted String representation of
     * InternalProviderData
     * @throws ParseException If data is not formatted correctly
     */
    public InternalProviderData(@NonNull byte[] bytes) throws ParseException {
        try {
            mJsonObject = new JSONObject(new String(bytes));
        } catch (JSONException e) {
            Log.i(TAG, "InternalProviderData JSONException = " + e.getMessage());
            throw new ParseException(e.getMessage());
        }
    }

    private int jsonHash(JSONObject jsonObject) {
        int hashSum = 0;
        Iterator<String> keys = jsonObject.keys();
        while(keys.hasNext()) {
            String key = keys.next();
            try {
                if (jsonObject.get(key) instanceof JSONObject) {
                    // This is a branch, get hash of this object recursively
                    JSONObject branch = jsonObject.getJSONObject(key);
                    hashSum += jsonHash(branch);
                } else {
                    // If this key does not link to a JSONObject, get hash of leaf
                    hashSum += key.hashCode() + jsonObject.get(key).hashCode();
                }
            } catch (JSONException ignored) {
                Log.i(TAG, "jsonHash JSONException = " + ignored.getMessage());
            }
        }
        return hashSum;
    }

    @Override
    public int hashCode() {
        // Recursively get the hashcode from all internal JSON keys and values
        return jsonHash(mJsonObject);
    }

    private boolean jsonEquals(JSONObject json1, JSONObject json2) {
        Iterator<String> keys = json1.keys();
        while(keys.hasNext()) {
            String key = keys.next();
            try {
                if (json1.get(key) instanceof JSONObject) {
                    // This is a branch, check equality of this object recursively
                    JSONObject thisBranch = json1.getJSONObject(key);
                    JSONObject otherBranch = json2.getJSONObject(key);
                    return jsonEquals(thisBranch, otherBranch);
                } else {
                    // If this key does not link to a JSONObject, check equality of leaf
                    if (!json1.get(key).equals(json2.get(key))) {
                        // The VALUE of the KEY does not match
                        return false;
                    }
                }
            } catch (JSONException e) {
                Log.i(TAG, "jsonEquals JSONException = " + e.getMessage());
                return false;
            }
        }
        // Confirm that no key has been missed in the check
        return json1.length() == json2.length();
    }

    /**
     * Tests that the value of each key is equal. Order does not matter.
     *
     * @param obj The object you are comparing to.
     * @return Whether the value of each key between both objects is equal.
     */
    @Override
    public boolean equals(Object obj) {
        if (obj == null || ! (obj instanceof InternalProviderData)) {
            return false;
        }
        JSONObject otherJsonObject = ((InternalProviderData) obj).mJsonObject;
        return jsonEquals(mJsonObject, otherJsonObject);
    }

    @Override
    public String toString() {
        return mJsonObject.toString();
    }

    /**
     * Adds some custom data to the InternalProviderData.
     *
     * @param key The key for this data
     * @param value The value this data should take
     * @return This InternalProviderData object to allow for chaining of calls
     * @throws ParseException If there is a problem adding custom data
     */
    public InternalProviderData put(String key, Object value) throws ParseException {
        try {
            if (!mJsonObject.has(KEY_CUSTOM_DATA)) {
                mJsonObject.put(KEY_CUSTOM_DATA, new JSONObject());
            }
            mJsonObject.getJSONObject(KEY_CUSTOM_DATA).put(key, String.valueOf(value));
        } catch (JSONException e) {
            Log.i(TAG, "put JSONException = " + e.getMessage());
            throw new ParseException(e.getMessage());
        }
        return this;
    }

    /**
     * Gets some previously added custom data stored in InternalProviderData.
     *
     * @param key The key assigned to this data
     * @return The value of this key if it has been defined. Returns null if the key is not found.
     * @throws ParseException If there is a problem getting custom data
     */
    public Object get(String key) throws ParseException {
        if (! mJsonObject.has(KEY_CUSTOM_DATA)) {
            return null;
        }
        try {
            return mJsonObject.getJSONObject(KEY_CUSTOM_DATA).opt(key);
        } catch (JSONException e) {
            Log.i(TAG, "get JSONException = " + e.getMessage());
            throw new ParseException(e.getMessage());
        }
    }

    /**
     * This exception is thrown when an error occurs in getting or setting data for the
     * InternalProviderData.
     */
    public class ParseException extends JSONException {
        public ParseException(String s) {
            super(s);
        }
    }
}
