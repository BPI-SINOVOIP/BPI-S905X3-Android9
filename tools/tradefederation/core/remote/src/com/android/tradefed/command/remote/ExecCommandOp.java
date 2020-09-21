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

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Remote operation for asynchronously executing a command on an already allocated device.
 */
class ExecCommandOp extends RemoteOperation<Void> {

    private static final String SERIAL = "serial";
    private static final String COMMAND_ARGS = "commandArgs";

    private final String mSerial;
    private final String[] mCommandArgs;

    ExecCommandOp(String serial, String... commandArgs) {
        mSerial = serial;
        mCommandArgs = commandArgs;
    }

    /**
     * Factory method for creating a {@link ExecCommandOp} from JSON data.
     *
     * @param json the data as a {@link JSONObject}
     * @return a {@link ExecCommandOp}
     * @throws JSONException if failed to extract out data
     */
    static ExecCommandOp createFromJson(JSONObject json) throws JSONException {
        String serial = json.getString(SERIAL);
        JSONArray jsonArgs = json.getJSONArray(COMMAND_ARGS);
        String[] commandArgs = new String[jsonArgs.length()];
        for (int i = 0; i < commandArgs.length; i++) {
            commandArgs[i] = jsonArgs.getString(i);
        }
        return new ExecCommandOp(serial, commandArgs);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected OperationType getType() {
        return OperationType.EXEC_COMMAND;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void packIntoJson(JSONObject j) throws JSONException {
        j.put(SERIAL, mSerial);
        JSONArray jsonArgs = new JSONArray();
        for (String arg : mCommandArgs) {
            jsonArgs.put(arg);
        }
        j.put(COMMAND_ARGS, jsonArgs);
    }

    public String[] getCommandArgs() {
        return mCommandArgs;
    }

    public String getDeviceSerial() {
        return mSerial;
    }
}
