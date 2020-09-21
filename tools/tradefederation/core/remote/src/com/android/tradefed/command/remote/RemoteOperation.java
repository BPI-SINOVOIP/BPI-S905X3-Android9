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
 * Encapsulates data for a remote operation sent over the wire.
 */
abstract class RemoteOperation<T> {
    private static final String TYPE = "type";
    private static final String VERSION = "version";
    /** represents json key for error message */
    static final String ERROR = "error";

    static final int CURRENT_PROTOCOL_VERSION = 8;

    /**
     * Represents all types of remote operations that can be performed
     */
    enum OperationType {
        ALLOCATE_DEVICE,
        FREE_DEVICE,
        CLOSE,
        ADD_COMMAND,
        START_HANDOVER,
        LIST_DEVICES,
        EXEC_COMMAND,
        GET_LAST_COMMAND_RESULT,
        HANDOVER_COMPLETE,
        ADD_COMMAND_FILE,
        HANDOVER_INIT_COMPLETE,
    }

    /**
     * Create and populate a {@link RemoteOperation} from given data.
     *
     * @param data the data to parse
     * @throws RemoteException
     */
    final static RemoteOperation<?> createRemoteOpFromString(String data) throws RemoteException {
        try {
            JSONObject jsonData = new JSONObject(data);
            int protocolVersion = jsonData.getInt(VERSION);
            // to keep things simple for now, just barf when protocol version is unknown
            if (protocolVersion != CURRENT_PROTOCOL_VERSION) {
                throw new RemoteException(String.format(
                        "Remote operation has unknown version '%d'. Expected '%d'",
                        protocolVersion, CURRENT_PROTOCOL_VERSION));
            }
            OperationType op = OperationType.valueOf(jsonData.getString(TYPE));
            RemoteOperation<?> rc = null;
            switch (op) {
                case ALLOCATE_DEVICE:
                    rc = AllocateDeviceOp.createFromJson(jsonData);
                    break;
                case FREE_DEVICE:
                    rc = FreeDeviceOp.createFromJson(jsonData);
                    break;
                case CLOSE:
                    rc = CloseOp.createFromJson(jsonData);
                    break;
                case ADD_COMMAND:
                    rc = AddCommandOp.createFromJson(jsonData);
                    break;
                case START_HANDOVER:
                    rc = StartHandoverOp.createFromJson(jsonData);
                    break;
                case LIST_DEVICES:
                    rc = ListDevicesOp.createFromJson(jsonData);
                    break;
                case EXEC_COMMAND:
                    rc = ExecCommandOp.createFromJson(jsonData);
                    break;
                case GET_LAST_COMMAND_RESULT:
                    rc = GetLastCommandResultOp.createFromJson(jsonData);
                    break;
                case HANDOVER_INIT_COMPLETE:
                    rc = HandoverInitCompleteOp.createFromJson(jsonData);
                    break;
                case HANDOVER_COMPLETE:
                    rc = HandoverCompleteOp.createFromJson(jsonData);
                    break;
                case ADD_COMMAND_FILE:
                    rc = AddCommandFileOp.createFromJson(jsonData);
                    break;
                default:
                    throw new RemoteException(String.format("unknown remote command '%s'", data));

            }
            return rc;
        } catch (JSONException e) {
            throw new RemoteException(e);
        }
    }

    protected abstract OperationType getType();

    /**
     * Returns the RemoteCommand data in its wire protocol format
     */
    String pack() throws RemoteException {
        return pack(CURRENT_PROTOCOL_VERSION);
    }

    /**
     * Returns the RemoteCommand data in its wire protocol format, with given protocol version
     *
     * @VisibleForTesting
     */
    String pack(int protocolVersion) throws RemoteException {
        JSONObject j = new JSONObject();
        try {
            j.put(VERSION, protocolVersion);
            j.put(TYPE, getType().toString());
            packIntoJson(j);
        } catch (JSONException e) {
            throw new RemoteException("Failed to serialize RemoteOperation", e);
        }
        return j.toString();
    }

    /**
     * Callback to add subclass specific data to the JSON object
     *
     * @param j
     * @throws JSONException
     */
    protected abstract void packIntoJson(JSONObject j) throws JSONException;

    /**
     * Optional callback to parse additional response data from the JSON object
     *
     * @param j
     * @throws JSONException
     */
    protected T unpackResponseFromJson(JSONObject j) throws JSONException {
        return null;
    }

    /**
     * Parse out the remote op response data from string
     *
     * @param response
     * @throws JSONException
     */
    T unpackResponseFromString(String response) throws JSONException, RemoteException {
        JSONObject jsonData = new JSONObject(response);
        if (jsonData.has(ERROR)) {
            throw new RemoteException(jsonData.getString(ERROR));
        }
        return unpackResponseFromJson(jsonData);
    }
}
