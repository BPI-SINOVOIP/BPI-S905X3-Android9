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

import com.android.tradefed.device.FreeDeviceState;

import java.util.Map;

/**
 * Encapsulates 'last command execution' data sent over the wire
 */
class CommandResult {

    public enum Status {
        NO_ACTIVE_COMMAND,
        EXECUTING,
        NOT_ALLOCATED,
        INVOCATION_ERROR,
        INVOCATION_SUCCESS
    }

    private final Status mStatus;
    private final String mError;
    private final FreeDeviceState mState;
    private final Map<String, String> mRunMetrics;

    CommandResult(Status status, String errorDetails, FreeDeviceState state,
            Map<String, String> runMetrics) {
        mStatus = status;
        mError = errorDetails;
        mState = state;
        mRunMetrics = runMetrics;
    }

    public CommandResult(Status status) {
        this(status, null, null, null);
    }

    Status getStatus() {
        return mStatus;
    }

    String getInvocationErrorDetails() {
        return mError;
    }

    FreeDeviceState getFreeDeviceState() {
        return mState;
    }

    /*
     * Although commands can have multiple runs, we only return one set of metrics and replace any
     * currently stored metrics with the same key.
     */
    Map<String, String> getRunMetrics() {
        return mRunMetrics;
    }
}
