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

import android.os.Bundle;
import android.telephony.mbms.DownloadRequest;
import android.telephony.mbms.FileInfo;

interface ICtsDownloadMiddlewareControl {
    // Resets the state of the CTS middleware
    void reset();
    // Get a list of calls made to the middleware binder.
    // Looks like List<List<Object>>, where the first Object is always a String corresponding to
    // the method name.
    List<Bundle> getDownloadSessionCalls();
    // Force all methods that can return an error to return this error.
    void forceErrorCode(int error);
    // Fire the error callback on the download session
    void fireErrorOnSession(int errorCode, String message);
    // Fire the download state callback methods
    void fireOnProgressUpdated(in DownloadRequest request, in FileInfo fileInfo,
            int currentDownloadSize, int fullDownloadSize,
            int currentDecodedSize, int fullDecodedSize);
    void fireOnStateUpdated(in DownloadRequest request, in FileInfo fileInfo, int state);

    // calling download() does nothing, this is to actually start the file-download flow.
    void actuallyStartDownloadFlow();
}