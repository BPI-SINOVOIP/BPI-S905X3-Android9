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

import android.bluetooth.BluetoothDevice;
import android.car.trust.ICarTrustAgentBleCallback;
import android.car.trust.ICarTrustAgentEnrolmentCallback;
import android.car.trust.ICarTrustAgentTokenRequestDelegate;
import android.car.trust.ICarTrustAgentTokenResponseCallback;
import android.car.trust.ICarTrustAgentUnlockCallback;

/**
 * Binder interface for CarTrustAgentBleService.
 *
 * @hide
 */
interface ICarTrustAgentBleService {
    /** General BLE */
    void registerBleCallback(in ICarTrustAgentBleCallback callback);
    void unregisterBleCallback(in ICarTrustAgentBleCallback callback);

    /** Enrolment */
    void startEnrolmentAdvertising();
    void stopEnrolmentAdvertising();
    void sendEnrolmentHandle(in BluetoothDevice device, long handle);
    void registerEnrolmentCallback(in ICarTrustAgentEnrolmentCallback callback);
    void unregisterEnrolmentCallback(in ICarTrustAgentEnrolmentCallback callback);

    /** Unlock */
    void startUnlockAdvertising();
    void stopUnlockAdvertising();
    void registerUnlockCallback(in ICarTrustAgentUnlockCallback callback);
    void unregisterUnlockCallback(in ICarTrustAgentUnlockCallback callback);

    /** Token request */
    void setTokenRequestDelegate(in ICarTrustAgentTokenRequestDelegate delegate);
    void revokeTrust();
    void addEscrowToken(in byte[] token, int uid);
    void removeEscrowToken(long handle, int uid);
    void isEscrowTokenActive(long handle, int uid);

    /** Token response */
    void setTokenResponseCallback(in ICarTrustAgentTokenResponseCallback callback);
    void onEscrowTokenAdded(in byte[] token, long handle, int uid);
    void onEscrowTokenRemoved(long handle, boolean successful);
    void onEscrowTokenActiveStateChanged(long handle, boolean active);
}
