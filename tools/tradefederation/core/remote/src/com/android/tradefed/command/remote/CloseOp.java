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
 * Remote operation that instructs the remote manager to shut down. Currently used when
 * handover process is complete.
 */
class CloseOp extends RemoteOperation<Void> {

    CloseOp() {
    }

    /**
     * Factory method for creating a {@link CloseOp} from JSON data.
     *
     * @param json the data as a {@link JSONObject}
     * @return a {@link CloseOp}
     * @throws JSONException if failed to extract out data
     */
    static CloseOp createFromJson(JSONObject json) throws JSONException {
        return new CloseOp();
    }

    @Override
    protected OperationType getType() {
        return OperationType.CLOSE;
    }

    @Override
    protected void packIntoJson(JSONObject j) throws JSONException {
        // nothing to do
    }
}
