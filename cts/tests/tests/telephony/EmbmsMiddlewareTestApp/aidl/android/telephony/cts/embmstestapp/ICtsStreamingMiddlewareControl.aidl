/*
 * Copyright (C) 2017 The Android Open Source Project
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
 * limitations under the License
 */

package android.telephony.cts.embmstestapp;

import android.os.Parcel;

interface ICtsStreamingMiddlewareControl {
    // Resets the state of the CTS middleware
    void reset();
    // Get a list of calls made to the middleware binder.
    // Looks like List<List<Object>>, where the first Object is always a String corresponding to
    // the method name.
    List getStreamingSessionCalls();
    // Force all methods that can return an error to return this error.
    void forceErrorCode(int error);
    // Fire the error callback on the current active stream
    void fireErrorOnStream(int errorCode, String message);
    // Fire the error callback on the streaming session
    void fireErrorOnSession(int errorCode, String message);
    // The following fire callbacks on the active stream, using the provided arguments
    void fireOnMediaDescriptionUpdated();
    void fireOnBroadcastSignalStrengthUpdated(int signalStrength);
    void fireOnStreamMethodUpdated(int methodType);
}