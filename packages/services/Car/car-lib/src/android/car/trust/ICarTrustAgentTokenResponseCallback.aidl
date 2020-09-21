/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.car.trust;

/**
 * Callback interface for escrow token exchange response.
 *
 * TrustAgentService uses async pattern and we therefore divide the
 * request and response into two separate AIDL interfaces.
 *
 * @hide
 */
interface ICarTrustAgentTokenResponseCallback {
    /** Called after escrow token is added for foreground user */
    void onEscrowTokenAdded(out byte[] token, long handle, int uid);

    /** Called after escrow token is removed for foreground user */
    void onEscrowTokenRemoved(long handle, boolean successful);

    /** Called after escrow token active state is changed for foreground user */
    void onEscrowTokenActiveStateChanged(long handle, boolean active);
}
