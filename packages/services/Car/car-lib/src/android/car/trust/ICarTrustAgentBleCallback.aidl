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

/**
 * Callback interface for BLE connection state changes.
 *
 * @hide
 */
oneway interface ICarTrustAgentBleCallback {
    /**
     * Called when the GATT server is started and BLE is successfully advertising.
     */
    void onBleServerStartSuccess();

    /**
     * Called when the BLE advertisement fails to start.
     * see AdvertiseCallback#ADVERTISE_FAILED_* for possible error codes.
     */
    void onBleServerStartFailure(int errorCode);

    /**
     * Called when a device is connected.
     */
    void onBleDeviceConnected(in BluetoothDevice device);

    /**
     * Called when a device is disconnected.
     */
    void onBleDeviceDisconnected(in BluetoothDevice device);
}
