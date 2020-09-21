/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.tradefed.command.remote;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * A remote operation for freeing a previously remotely allocated device.
 */
class FreeDeviceOp extends RemoteOperation<Void> {

    private static final String SERIAL = "serial";

    static final String ALL_DEVICES = "*";

    private final String mDeviceSerial;

    FreeDeviceOp(String serial) {
        mDeviceSerial = serial;
    }

    /**
     * Factory method for creating a {@link FreeDeviceOp} from JSON data.
     *
     * @param json the data as a {@link JSONObject}
     * @return a {@link FreeDeviceOp}
     * @throws JSONException if failed to extract out data
     */
    static FreeDeviceOp createFromJson(JSONObject json) throws JSONException {
        return new FreeDeviceOp(json.getString(SERIAL));
    }

    @Override
    protected OperationType getType() {
        return OperationType.FREE_DEVICE;
    }

    @Override
    protected void packIntoJson(JSONObject j) throws JSONException {
        j.put(SERIAL, mDeviceSerial);
    }

    public String getDeviceSerial() {
        return mDeviceSerial;
    }
}
