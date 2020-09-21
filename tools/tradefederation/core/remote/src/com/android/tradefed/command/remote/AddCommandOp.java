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
 * Remote operation for adding a command to the local tradefed scheduler.
 */
class AddCommandOp extends RemoteOperation<Void> {

    private static final String COMMAND_ARGS = "commandArgs";
    private static final String TIME = "time";
    private final long mTotalTime;
    private final String[] mCommandArgs;

    AddCommandOp(long totalTime, String... commandArgs) {
        mTotalTime = totalTime;
        mCommandArgs = commandArgs;
    }

    /**
     * Factory method for creating a {@link AddCommandOp} from JSON data.
     *
     * @param jsonData the data as a {@link JSONObject}
     * @return a {@link AddCommandOp}
     * @throws JSONException if failed to extract out data
     */
    static AddCommandOp createFromJson(JSONObject jsonData) throws JSONException {
        long totalTime = jsonData.getLong(TIME);
        JSONArray jsonArgs = jsonData.getJSONArray(COMMAND_ARGS);
        String[] commandArgs = new String[jsonArgs.length()];
        for (int i = 0; i < commandArgs.length; i++) {
            commandArgs[i] = jsonArgs.getString(i);
        }
        return new AddCommandOp(totalTime, commandArgs);
    }

    @Override
    protected OperationType getType() {
        return OperationType.ADD_COMMAND;
    }

    @Override
    protected void packIntoJson(JSONObject j) throws JSONException {
        j.put(TIME, mTotalTime);
        JSONArray jsonArgs = new JSONArray();
        for (String arg : mCommandArgs) {
            jsonArgs.put(arg);
        }
        j.put(COMMAND_ARGS, jsonArgs);
    }

    public String[] getCommandArgs() {
        return mCommandArgs;
    }

    public long getTotalTime() {
        return mTotalTime;
    }
}
