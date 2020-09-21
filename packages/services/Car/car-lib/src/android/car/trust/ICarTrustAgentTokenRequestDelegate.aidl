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
 * Delegate interface for escrow token exchange request.
 *
 * TrustAgentService uses async pattern and we therefore divide the
 * request and response into two separate AIDL interfaces.
 *
 * CarBleTrustAgent would implement this interface and set itself as a delegate.
 *
 * @hide
 */
interface ICarTrustAgentTokenRequestDelegate {
    /** Called to revoke trust */
    void revokeTrust();

    /** Called to add escrow token for foreground user */
    void addEscrowToken(in byte[] token, int uid);

    /** Called to remove escrow token for foreground user */
    void removeEscrowToken(long handle, int uid);

    /** Called to query if the foreground user has active escrow token */
    void isEscrowTokenActive(long handle, int uid);
}
