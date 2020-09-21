/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.car.vehiclehal.test;

import static java.lang.Integer.toHexString;

import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyType;

import com.android.car.vehiclehal.VehiclePropValueBuilder;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;


class VhalJsonReader {

    /**
     * Name of fields presented in JSON file. All of them are required.
     */
    private static final String JSON_FIELD_PROP = "prop";
    private static final String JSON_FIELD_AREA_ID = "areaId";
    private static final String JSON_FIELD_TIMESTAMP = "timestamp";
    private static final String JSON_FIELD_VALUE = "value";

    public static List<VehiclePropValue> readFromJson(InputStream in)
            throws IOException, JSONException {
        JSONArray rawEvents = new JSONArray(readJsonString(in));
        List<VehiclePropValue> events = new ArrayList<>();
        for (int i = 0; i < rawEvents.length(); i++) {
            events.add(getEvent(rawEvents.getJSONObject(i)));
        }
        return events;
    }

    private static String readJsonString(InputStream in) throws IOException {
        StringBuilder builder = new StringBuilder();
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(in, StandardCharsets.UTF_8))) {
            reader.lines().forEach(builder::append);
        }
        return builder.toString();
    }

    private static VehiclePropValue getEvent(JSONObject rawEvent) throws JSONException {
        int prop = rawEvent.getInt(JSON_FIELD_PROP);
        VehiclePropValueBuilder builder = VehiclePropValueBuilder.newBuilder(prop)
                .setAreaId(rawEvent.getInt(JSON_FIELD_AREA_ID))
                .setTimestamp(rawEvent.getLong(JSON_FIELD_TIMESTAMP));

        switch (prop & VehiclePropertyType.MASK) {
            case VehiclePropertyType.BOOLEAN:
            case VehiclePropertyType.INT32:
                builder.addIntValue(rawEvent.getInt(JSON_FIELD_VALUE));
                break;
            case VehiclePropertyType.INT64:
                builder.setInt64Value(rawEvent.getLong(JSON_FIELD_VALUE));
                break;
            case VehiclePropertyType.FLOAT:
                builder.addFloatValue((float) rawEvent.getDouble(JSON_FIELD_VALUE));
                break;
            case VehiclePropertyType.STRING:
                builder.setStringValue(rawEvent.getString(JSON_FIELD_VALUE));
                break;
            //TODO: Add VehiclePropertyType.MIXED type support
            default:
                throw new IllegalArgumentException("Property type 0x"
                        + toHexString(prop & VehiclePropertyType.MASK)
                        + " is not supported in the test.");
        }
        return builder.build();
    }
}
