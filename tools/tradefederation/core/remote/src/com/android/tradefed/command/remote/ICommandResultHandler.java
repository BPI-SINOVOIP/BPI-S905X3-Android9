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
 * Callback for handling the result of a {@link GetLastCommandResultOp}. Although commands can have
 * multiple runs, we only return one set of metrics and replace any currently stored metrics with
 * the same key.
 */
public interface ICommandResultHandler {
    public void success(Map<String, String> runMetrics);
    public void failure(String errorDetails, FreeDeviceState deviceState,
            Map<String, String> runMetrics);
    public void stillRunning();
    public void notAllocated();
    public void noActiveCommand();
}
