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
package com.android.car.trust;

import static android.bluetooth.BluetoothProfile.GATT_SERVER;

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattServer;
import android.bluetooth.BluetoothGattServerCallback;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.ParcelUuid;
import android.util.Log;

/**
 * A generic service to start a BLE
 */
public abstract class SimpleBleServer extends Service {

    private static final String TAG = SimpleBleServer.class.getSimpleName();

    private static final int BLE_RETRY_LIMIT = 5;
    private static final int BLE_RETRY_INTERVAL_MS = 1000;

    private final AdvertiseCallback mAdvertisingCallback = new AdvertiseCallback() {
        @Override
        public void onStartSuccess(AdvertiseSettings settingsInEffect) {
            super.onStartSuccess(settingsInEffect);
            Log.d(TAG, "Successfully started advertising service");
            onAdvertiseStartSuccess();
        }

        @Override
        public void onStartFailure(int errorCode) {
            super.onStartFailure(errorCode);
            Log.e(TAG, "Failed to advertise, errorCode: " + errorCode);
            onAdvertiseStartFailure(errorCode);
        }
    };

    private final BluetoothGattServerCallback mGattServerCallback =
            new BluetoothGattServerCallback() {
        @Override
        public void onConnectionStateChange(BluetoothDevice device,
                final int status, final int newState) {
            Log.d(TAG, "GattServer connection change status: " + status
                    + " newState: " + newState
                    + " device name: " + device.getName());
            switch (newState) {
                case BluetoothProfile.STATE_CONNECTED:
                    onAdvertiseDeviceConnected(device);
                    break;
                case BluetoothProfile.STATE_DISCONNECTED:
                    onAdvertiseDeviceDisconnected(device);
                    break;
            }
        }

        @Override
        public void onServiceAdded(final int status, BluetoothGattService service) {
            Log.d(TAG, "Service added status: " + status + " uuid: " + service.getUuid());
        }

        @Override
        public void onCharacteristicReadRequest(BluetoothDevice device,
                int requestId, int offset, final BluetoothGattCharacteristic characteristic) {
            Log.d(TAG, "Read request for characteristic: " + characteristic.getUuid());
            mGattServer.sendResponse(device, requestId,
                    BluetoothGatt.GATT_SUCCESS, offset, characteristic.getValue());
            onCharacteristicRead(device, requestId, offset, characteristic);
        }

        @Override
        public void onCharacteristicWriteRequest(final BluetoothDevice device, int requestId,
                BluetoothGattCharacteristic characteristic, boolean preparedWrite, boolean
                responseNeeded, int offset, byte[] value) {
            Log.d(TAG, "Write request for characteristic: " + characteristic.getUuid());
            mGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS,
                    offset, value);
            onCharacteristicWrite(device, requestId, characteristic,
                    preparedWrite, responseNeeded, offset, value);
        }
    };

    private final Handler mHandler = new Handler();

    private BluetoothManager mBluetoothManager;
    private BluetoothLeAdvertiser mAdvertiser;
    private BluetoothGattServer mGattServer;
    private int mAdvertiserStartCount;

    /**
     * Starts the GATT server with the given {@link BluetoothGattService} and begins
     * advertising with the {@link ParcelUuid}.
     * <p>It is possible that BLE service is still in TURNING_ON state when this method is invoked.
     * Therefore, several retries will be made to ensure advertising is started.
     *
     * @param advertiseUuid Service Uuid used in the {@link AdvertiseData}
     * @param service {@link BluetoothGattService} that will be discovered by clients
     */
    protected void startAdvertising(ParcelUuid advertiseUuid, BluetoothGattService service) {
        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Log.e(TAG, "System does not support BLE");
            return;
        }

        mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mGattServer = mBluetoothManager.openGattServer(this, mGattServerCallback);
        if (mGattServer == null) {
            Log.e(TAG, "Gatt Server not created");
            return;
        }

        // We only allow adding one service in this implementation. If multiple services need
        // to be added, then they need to be queued up and added only after
        // BluetoothGattServerCallback.onServiceAdded is called.
        mGattServer.addService(service);

        AdvertiseSettings settings = new AdvertiseSettings.Builder()
                .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY)
                .setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_HIGH)
                .setConnectable(true)
                .build();

        AdvertiseData data = new AdvertiseData.Builder()
                .setIncludeDeviceName(true)
                .addServiceUuid(advertiseUuid)
                .build();

        mAdvertiserStartCount = 0;
        startAdvertisingInternally(settings, data);
    }

    private void startAdvertisingInternally(AdvertiseSettings settings, AdvertiseData data) {
        mAdvertiserStartCount += 1;
        mAdvertiser = BluetoothAdapter.getDefaultAdapter().getBluetoothLeAdvertiser();
        if (mAdvertiser == null && mAdvertiserStartCount < BLE_RETRY_LIMIT) {
            mHandler.postDelayed(() -> startAdvertisingInternally(settings, data),
                    BLE_RETRY_INTERVAL_MS);
        } else {
            mHandler.removeCallbacks(null);
            mAdvertiser.startAdvertising(settings, data, mAdvertisingCallback);
            mAdvertiserStartCount = 0;
        }
    }

    protected void stopAdvertising() {
        if (mAdvertiser != null) {
            mAdvertiser.stopAdvertising(mAdvertisingCallback);
        }
    }

    /**
     * Notifies the characteristic change via {@link BluetoothGattServer}
     */
    protected void notifyCharacteristicChanged(BluetoothDevice device,
            BluetoothGattCharacteristic characteristic, boolean confirm) {
        if (mGattServer != null) {
            mGattServer.notifyCharacteristicChanged(device, characteristic, confirm);
        }
    }

    @Override
    public void onDestroy() {
        // Stops the advertiser and GATT server. This needs to be done to avoid leaks
        if (mAdvertiser != null) {
            mAdvertiser.stopAdvertising(mAdvertisingCallback);
            mAdvertiser.cleanup();
        }

        if (mGattServer != null) {
            mGattServer.clearServices();
            try {
                for (BluetoothDevice d : mBluetoothManager.getConnectedDevices(GATT_SERVER)) {
                    mGattServer.cancelConnection(d);
                }
            } catch (UnsupportedOperationException e) {
                Log.e(TAG, "Error getting connected devices", e);
            } finally {
                mGattServer.close();
            }
        }
        super.onDestroy();
    }

    // Delegate to subclass
    protected void onAdvertiseStartSuccess() { }
    protected void onAdvertiseStartFailure(int errorCode) { }
    protected void onAdvertiseDeviceConnected(BluetoothDevice device) { }
    protected void onAdvertiseDeviceDisconnected(BluetoothDevice device) { }

    /**
     * Triggered when this BleService receives a write request from a remote
     * device. Sub-classes should implement how to handle requests.
     */
    protected abstract void onCharacteristicWrite(BluetoothDevice device, int requestId,
            BluetoothGattCharacteristic characteristic, boolean preparedWrite, boolean
            responseNeeded, int offset, byte[] value);

    /**
     * Triggered when this BleService receives a read request from a remote device.
     */
    protected abstract void onCharacteristicRead(BluetoothDevice device,
            int requestId, int offset, final BluetoothGattCharacteristic characteristic);

}
