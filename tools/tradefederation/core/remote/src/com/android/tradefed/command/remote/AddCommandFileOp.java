/*
 * Copyright (C) 2014 The Android Open Source Project
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

import java.util.ArrayList;
import java.util.List;

/**
 * Remote operation for adding a command file to the local tradefed scheduler.
 */
class AddCommandFileOp extends RemoteOperation<Void> {

    private static final String COMMAND_FILE = "commandFile";
    private static final String EXTRA_ARGS = "extraArgs";
    private final String mCommandFile;
    private final List<String> mExtraArgs;

    AddCommandFileOp(String commandFile, List<String> extraArgs) {
        mCommandFile = commandFile;
        mExtraArgs = extraArgs;
    }

    /**
     * Factory method for creating a {@link AddCommandFileOp} from JSON data.
     *
     * @param jsonData the data as a {@link JSONObject}
     * @return a {@link AddCommandFileOp}
     * @throws JSONException if failed to extract out data
     */
    static AddCommandFileOp createFromJson(JSONObject jsonData) throws JSONException {
        String cmdFile = jsonData.getString(COMMAND_FILE);
        JSONArray argArray = jsonData.getJSONArray(EXTRA_ARGS);
        List<String> argList = new ArrayList<String>(argArray.length());
        for (int i=0; i < argArray.length(); i++) {
            argList.add(argArray.getString(i));
        }
        return new AddCommandFileOp(cmdFile, argList);
    }

    @Override
    protected OperationType getType() {
        return OperationType.ADD_COMMAND_FILE;
    }

    @Override
    protected void packIntoJson(JSONObject j) throws JSONException {
        j.put(COMMAND_FILE, mCommandFile);
        JSONArray argArray = new JSONArray(mExtraArgs);
        j.put(EXTRA_ARGS, argArray);
    }

    public String getCommandFile() {
        return mCommandFile;
    }

    /**
     * Returns the extra arguments to add to each command parsed from command file
     */
    public List<String> getExtraArgs() {
        return mExtraArgs;
    }
}
