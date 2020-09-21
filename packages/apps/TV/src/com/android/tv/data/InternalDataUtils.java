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

package com.android.tv.data;

import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;
import com.android.tv.data.Program.CriticScore;
import com.android.tv.dvr.data.RecordedProgram;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;


/**
 * A utility class to parse and store data from the {@link
 * android.media.tv.TvContract.Programs#COLUMN_INTERNAL_PROVIDER_DATA} field in the {@link
 * android.media.tv.TvContract.Programs}.
 */
public final class InternalDataUtils {
    private static final boolean DEBUG = false;
    private static final String TAG = "InternalDataUtils";

    private InternalDataUtils() {
        // do nothing
    }

    /**
     * Deserializes a byte array into objects to be stored in the Program class.
     *
     * <p>Series ID and critic scores are loaded from the bytes.
     *
     * @param bytes the bytes to be deserialized
     * @param builder the builder for the Program class
     */
    public static void deserializeInternalProviderData(byte[] bytes, Program.Builder builder) {
        if (bytes == null || bytes.length == 0) {
            return;
        }
        try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(bytes))) {
            builder.setSeriesId((String) in.readObject());
            builder.setCriticScores((List<CriticScore>) in.readObject());
        } catch (NullPointerException e) {
            if (DEBUG) Log.e(TAG, "no bytes to deserialize");
        } catch (IOException e) {
            if (DEBUG) Log.e(TAG, "Could not deserialize internal provider contents");
        } catch (ClassNotFoundException e) {
            if (DEBUG) Log.e(TAG, "class not found in internal provider contents");
        }
    }

    /**
     * Convenience method for converting relevant data in Program class to a serialized blob type
     * for storage in internal_provider_data field.
     *
     * @param program the program which contains the objects to be serialized
     * @return serialized blob-type data
     */
    @Nullable
    public static byte[] serializeInternalProviderData(Program program) {
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        try (ObjectOutputStream out = new ObjectOutputStream(bos)) {
            if (!TextUtils.isEmpty(program.getSeriesId()) || program.getCriticScores() != null) {
                out.writeObject(program.getSeriesId());
                out.writeObject(program.getCriticScores());
                return bos.toByteArray();
            }
        } catch (IOException e) {
            Log.e(
                    TAG,
                    "Could not serialize internal provider contents for program: "
                            + program.getTitle());
        }
        return null;
    }

    /**
     * Deserializes a byte array into objects to be stored in the RecordedProgram class.
     *
     * <p>Series ID is loaded from the bytes.
     *
     * @param bytes the bytes to be deserialized
     * @param builder the builder for the RecordedProgram class
     */
    public static void deserializeInternalProviderData(
            byte[] bytes, RecordedProgram.Builder builder) {
        if (bytes == null || bytes.length == 0) {
            return;
        }
        try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(bytes))) {
            builder.setSeriesId((String) in.readObject());
        } catch (NullPointerException e) {
            Log.e(TAG, "no bytes to deserialize");
        } catch (IOException e) {
            Log.e(TAG, "Could not deserialize internal provider contents");
        } catch (ClassNotFoundException e) {
            Log.e(TAG, "class not found in internal provider contents");
        }
    }

    /**
     * Serializes relevant objects in {@link android.media.tv.TvContract.Programs} to byte array.
     *
     * @return the serialized byte array
     */
    public static byte[] serializeInternalProviderData(RecordedProgram program) {
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        try (ObjectOutputStream out = new ObjectOutputStream(bos)) {
            if (!TextUtils.isEmpty(program.getSeriesId())) {
                out.writeObject(program.getSeriesId());
                return bos.toByteArray();
            }
        } catch (IOException e) {
            Log.e(
                    TAG,
                    "Could not serialize internal provider contents for program: "
                            + program.getTitle());
        }
        return null;
    }

    @Nullable
    public static byte[] serializeInternalProviderData(String data) {
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        try (ObjectOutputStream out = new ObjectOutputStream(bos)) {
            if (!TextUtils.isEmpty(data)) {
                out.writeObject(data);
                return bos.toByteArray();
            }
        } catch (IOException e) {
            Log.e(
                    TAG,
                    "Could not serialize internal provider contents for data: "
                            + data);
        }
        return null;
    }

    public static String deserializeInternalProviderData(byte[] bytes) {
        if (bytes == null || bytes.length == 0) {
            return null;
        }
        String result = null;
        try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(bytes))) {
            result = (String) in.readObject();
        } catch (Exception e) {
            Log.e(TAG, "Exception deserializeInternalProviderData Exception = " + e.getMessage());
        }
        return result;
    }

    public static byte[] getInternalProviderDataByte(String data) {
        if (data == null) {
            return null;
        }
        byte[] result = null;
        try {
            result = data.getBytes();
        } catch (Exception e) {
            Log.e(TAG, "getInternalProviderDataByte Exception = " + e.getMessage());
        }
        return result;
    }

    public static String getInternalProviderDataString(byte[] bytes) {
        if (bytes == null || bytes.length == 0) {
            return null;
        }
        String result = null;
        try {
            result = new String(bytes);
        } catch (Exception e) {
            Log.e(TAG, "getInternalProviderDataString Exception = " + e.getMessage());
        }
        return result;
    }

    public static class InternalProviderData {
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
        public InternalProviderData(byte[] bytes) {
            try {
                mJsonObject = new JSONObject(new String(bytes));
            } catch (JSONException e) {
                Log.i(TAG, "InternalProviderData JSONException = " + e.getMessage());
                mJsonObject = new JSONObject();
            }
        }

        private int jsonHash(JSONObject jsonObject) {
            int hashSum = 0;
            Iterator<String> keys = jsonObject.keys();
            while (keys.hasNext()) {
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
            while (keys.hasNext()) {
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

        public byte[] getByte() {
            byte[] result = null;
            String objStr = mJsonObject.toString();
            if (objStr != null) {
                result = objStr.getBytes();
            }
            return result;
        }

        /**
         * Adds some custom data to the InternalProviderData.
         *
         * @param key The key for this data
         * @param value The value this data should take
         * @return This InternalProviderData object to allow for chaining of calls
         * @throws ParseException If there is a problem adding custom data
         */
        public InternalProviderData put(String key, Object value) {
            try {
                if (!mJsonObject.has(KEY_CUSTOM_DATA)) {
                    mJsonObject.put(KEY_CUSTOM_DATA, new JSONObject());
                }
                mJsonObject.getJSONObject(KEY_CUSTOM_DATA).put(key, String.valueOf(value));
            } catch (JSONException e) {
                Log.i(TAG, "InternalProviderData put JSONException = " + e.getMessage());
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
        public Object get(String key) {
            if (! mJsonObject.has(KEY_CUSTOM_DATA)) {
                return null;
            }
            try {
                return mJsonObject.getJSONObject(KEY_CUSTOM_DATA).opt(key);
            } catch (JSONException e) {
                Log.i(TAG, "InternalProviderData get JSONException = " + e.getMessage());
            }
            return null;
        }
    }
}
