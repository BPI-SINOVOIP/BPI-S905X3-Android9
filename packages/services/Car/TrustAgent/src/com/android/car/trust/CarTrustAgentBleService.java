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
package com.android.car.trust;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.car.trust.ICarTrustAgentBleCallback;
import android.car.trust.ICarTrustAgentBleService;
import android.car.trust.ICarTrustAgentEnrolmentCallback;
import android.car.trust.ICarTrustAgentTokenRequestDelegate;
import android.car.trust.ICarTrustAgentTokenResponseCallback;
import android.car.trust.ICarTrustAgentUnlockCallback;
import android.content.Intent;
import android.os.IBinder;
import android.os.ParcelUuid;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.util.Log;

import java.nio.ByteBuffer;
import java.util.UUID;

/**
 * Abstracts enrolment and unlock token exchange via BluetoothLE (BLE).
 * {@link CarBleTrustAgent} and any enrolment client should bind to
 * {@link ICarTrustAgentBleService}.
 */
public class CarTrustAgentBleService extends SimpleBleServer {

    private static final String TAG = CarTrustAgentBleService.class.getSimpleName();

    private RemoteCallbackList<ICarTrustAgentBleCallback> mBleCallbacks;
    private RemoteCallbackList<ICarTrustAgentEnrolmentCallback> mEnrolmentCallbacks;
    private RemoteCallbackList<ICarTrustAgentUnlockCallback> mUnlockCallbacks;
    private ICarTrustAgentTokenRequestDelegate mTokenRequestDelegate;
    private ICarTrustAgentTokenResponseCallback mTokenResponseCallback;
    private CarTrustAgentBleWrapper mCarTrustBleService;

    private ParcelUuid mEnrolmentUuid;
    private BluetoothGattCharacteristic mEnrolmentEscrowToken;
    private BluetoothGattCharacteristic mEnrolmentTokenHandle;
    private BluetoothGattService mEnrolmentGattServer;

    private ParcelUuid mUnlockUuid;
    private BluetoothGattCharacteristic mUnlockEscrowToken;
    private BluetoothGattCharacteristic mUnlockTokenHandle;
    private BluetoothGattService mUnlockGattServer;
    private byte[] mCurrentUnlockToken;
    private Long mCurrentUnlockHandle;

    @Override
    public void onCreate() {
        super.onCreate();
        mBleCallbacks = new RemoteCallbackList<>();
        mEnrolmentCallbacks = new RemoteCallbackList<>();
        mUnlockCallbacks = new RemoteCallbackList<>();
        mCarTrustBleService = new CarTrustAgentBleWrapper();

        setupEnrolmentBleServer();
        setupUnlockBleServer();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mCarTrustBleService;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // keep it alive.
        return START_STICKY;
    }

    @Override
    public void onCharacteristicWrite(final BluetoothDevice device, int requestId,
            BluetoothGattCharacteristic characteristic, boolean preparedWrite, boolean
            responseNeeded, int offset, byte[] value) {
        UUID uuid = characteristic.getUuid();
        Log.d(TAG, "onCharacteristicWrite received uuid: " + uuid);
        if (uuid.equals(mEnrolmentEscrowToken.getUuid())) {
            final int callbackCount = mEnrolmentCallbacks.beginBroadcast();
            for (int i = 0; i < callbackCount; i++) {
                try {
                    mEnrolmentCallbacks.getBroadcastItem(i).onEnrolmentDataReceived(value);
                } catch (RemoteException e) {
                    Log.e(TAG, "Error callback onEnrolmentDataReceived", e);
                }
            }
            mEnrolmentCallbacks.finishBroadcast();
        } else if (uuid.equals(mUnlockEscrowToken.getUuid())) {
            mCurrentUnlockToken = value;
            maybeSendUnlockToken();
        } else if (uuid.equals(mUnlockTokenHandle.getUuid())) {
            mCurrentUnlockHandle = getLong(value);
            maybeSendUnlockToken();
        }
    }

    @Override
    public void onCharacteristicRead(BluetoothDevice device,
            int requestId, int offset, final BluetoothGattCharacteristic characteristic) {
        // Ignored read requests.
    }

    @Override
    protected void onAdvertiseStartSuccess() {
        final int callbackCount = mBleCallbacks.beginBroadcast();
        for (int i = 0; i < callbackCount; i++) {
            try {
                mBleCallbacks.getBroadcastItem(i).onBleServerStartSuccess();
            } catch (RemoteException e) {
                Log.e(TAG, "Error callback onBleServerStartSuccess", e);
            }
        }
        mBleCallbacks.finishBroadcast();
    }

    @Override
    protected void onAdvertiseStartFailure(int errorCode) {
        final int callbackCount = mBleCallbacks.beginBroadcast();
        for (int i = 0; i < callbackCount; i++) {
            try {
                mBleCallbacks.getBroadcastItem(i).onBleServerStartFailure(errorCode);
            } catch (RemoteException e) {
                Log.e(TAG, "Error callback onBleServerStartFailure", e);
            }
        }
        mBleCallbacks.finishBroadcast();
    }

    @Override
    protected void onAdvertiseDeviceConnected(BluetoothDevice device) {
        final int callbackCount = mBleCallbacks.beginBroadcast();
        for (int i = 0; i < callbackCount; i++) {
            try {
                mBleCallbacks.getBroadcastItem(i).onBleDeviceConnected(device);
            } catch (RemoteException e) {
                Log.e(TAG, "Error callback onBleDeviceConnected", e);
            }
        }
        mBleCallbacks.finishBroadcast();
    }

    @Override
    protected void onAdvertiseDeviceDisconnected(BluetoothDevice device) {
        final int callbackCount = mBleCallbacks.beginBroadcast();
        for (int i = 0; i < callbackCount; i++) {
            try {
                mBleCallbacks.getBroadcastItem(i).onBleDeviceDisconnected(device);
            } catch (RemoteException e) {
                Log.e(TAG, "Error callback onBleDeviceDisconnected", e);
            }
        }
        mBleCallbacks.finishBroadcast();
    }

    private void setupEnrolmentBleServer() {
        mEnrolmentUuid = new ParcelUuid(
                UUID.fromString(getString(R.string.enrollment_service_uuid)));
        mEnrolmentGattServer = new BluetoothGattService(
                UUID.fromString(getString(R.string.enrollment_service_uuid)),
                BluetoothGattService.SERVICE_TYPE_PRIMARY);

        // Characteristic to describe the escrow token being used for unlock
        mEnrolmentEscrowToken = new BluetoothGattCharacteristic(
                UUID.fromString(getString(R.string.enrollment_token_uuid)),
                BluetoothGattCharacteristic.PROPERTY_WRITE,
                BluetoothGattCharacteristic.PERMISSION_WRITE);

        // Characteristic to describe the handle being used for this escrow token
        mEnrolmentTokenHandle = new BluetoothGattCharacteristic(
                UUID.fromString(getString(R.string.enrollment_handle_uuid)),
                BluetoothGattCharacteristic.PROPERTY_NOTIFY,
                BluetoothGattCharacteristic.PERMISSION_READ);

        mEnrolmentGattServer.addCharacteristic(mEnrolmentEscrowToken);
        mEnrolmentGattServer.addCharacteristic(mEnrolmentTokenHandle);
    }

    private void setupUnlockBleServer() {
        mUnlockUuid = new ParcelUuid(
                UUID.fromString(getString(R.string.unlock_service_uuid)));
        mUnlockGattServer = new BluetoothGattService(
                UUID.fromString(getString(R.string.unlock_service_uuid)),
                BluetoothGattService.SERVICE_TYPE_PRIMARY);

        // Characteristic to describe the escrow token being used for unlock
        mUnlockEscrowToken = new BluetoothGattCharacteristic(
                UUID.fromString(getString(R.string.unlock_escrow_token_uiid)),
                BluetoothGattCharacteristic.PROPERTY_WRITE,
                BluetoothGattCharacteristic.PERMISSION_WRITE);

        // Characteristic to describe the handle being used for this escrow token
        mUnlockTokenHandle = new BluetoothGattCharacteristic(
                UUID.fromString(getString(R.string.unlock_handle_uiid)),
                BluetoothGattCharacteristic.PROPERTY_WRITE,
                BluetoothGattCharacteristic.PERMISSION_WRITE);

        mUnlockGattServer.addCharacteristic(mUnlockEscrowToken);
        mUnlockGattServer.addCharacteristic(mUnlockTokenHandle);
    }

    private synchronized void maybeSendUnlockToken() {
        if (mCurrentUnlockToken == null || mCurrentUnlockHandle == null) {
            return;
        }
        Log.d(TAG, "Handle and token both received, requesting unlock. Time: "
                + System.currentTimeMillis());
        final int callbackCount = mUnlockCallbacks.beginBroadcast();
        for (int i = 0; i < callbackCount; i++) {
            try {
                mUnlockCallbacks.getBroadcastItem(i).onUnlockDataReceived(
                        mCurrentUnlockToken, mCurrentUnlockHandle);
            } catch (RemoteException e) {
                Log.e(TAG, "Error callback onUnlockDataReceived", e);
            }
        }
        mUnlockCallbacks.finishBroadcast();
        mCurrentUnlockHandle = null;
        mCurrentUnlockToken = null;
    }

    private static byte[] getBytes(long primitive) {
        ByteBuffer buffer = ByteBuffer.allocate(Long.SIZE / Byte.SIZE);
        buffer.putLong(0, primitive);
        return buffer.array();
    }

    private static long getLong(byte[] bytes) {
        ByteBuffer buffer = ByteBuffer.allocate(Long.SIZE / Byte.SIZE);
        buffer.put(bytes);
        buffer.flip();
        return buffer.getLong();
    }

    private final class CarTrustAgentBleWrapper extends ICarTrustAgentBleService.Stub {
        @Override
        public void registerBleCallback(ICarTrustAgentBleCallback callback) {
            mBleCallbacks.register(callback);
        }

        @Override
        public void unregisterBleCallback(ICarTrustAgentBleCallback callback) {
            mBleCallbacks.unregister(callback);
        }

        @Override
        public void startEnrolmentAdvertising() {
            Log.d(TAG, "startEnrolmentAdvertising");
            stopUnlockAdvertising();
            startAdvertising(mEnrolmentUuid, mEnrolmentGattServer);
        }

        @Override
        public void stopEnrolmentAdvertising() {
            Log.d(TAG, "stopEnrolmentAdvertising");
            stopAdvertising();
        }

        @Override
        public void sendEnrolmentHandle(BluetoothDevice device, long handle) {
            Log.d(TAG, "sendEnrolmentHandle: " + handle);
            mEnrolmentTokenHandle.setValue(getBytes(handle));
            notifyCharacteristicChanged(device, mEnrolmentTokenHandle, false);
        }

        @Override
        public void registerEnrolmentCallback(ICarTrustAgentEnrolmentCallback callback) {
            mEnrolmentCallbacks.register(callback);
        }

        @Override
        public void unregisterEnrolmentCallback(ICarTrustAgentEnrolmentCallback callback) {
            mEnrolmentCallbacks.unregister(callback);
        }

        @Override
        public void startUnlockAdvertising() {
            Log.d(TAG, "startUnlockAdvertising");
            stopEnrolmentAdvertising();
            startAdvertising(mUnlockUuid, mUnlockGattServer);
        }

        @Override
        public void stopUnlockAdvertising() {
            Log.d(TAG, "stopUnlockAdvertising");
            stopAdvertising();
        }

        @Override
        public void registerUnlockCallback(ICarTrustAgentUnlockCallback callback) {
            mUnlockCallbacks.register(callback);
        }

        @Override
        public void unregisterUnlockCallback(ICarTrustAgentUnlockCallback callback) {
            mUnlockCallbacks.unregister(callback);
        }

        @Override
        public void setTokenRequestDelegate(ICarTrustAgentTokenRequestDelegate delegate) {
            mTokenRequestDelegate = delegate;
        }

        @Override
        public void revokeTrust() throws RemoteException {
            if (mTokenRequestDelegate != null) {
                mTokenRequestDelegate.revokeTrust();
            }
        }

        @Override
        public void addEscrowToken(byte[] token, int uid) throws RemoteException {
            if (mTokenRequestDelegate != null) {
                mTokenRequestDelegate.addEscrowToken(token, uid);
            }
        }

        @Override
        public void removeEscrowToken(long handle, int uid) throws RemoteException {
            if (mTokenRequestDelegate != null) {
                mTokenRequestDelegate.removeEscrowToken(handle, uid);
            }
        }

        @Override
        public void isEscrowTokenActive(long handle, int uid) throws RemoteException {
            if (mTokenRequestDelegate != null) {
                mTokenRequestDelegate.isEscrowTokenActive(handle, uid);
            }
        }

        @Override
        public void setTokenResponseCallback(ICarTrustAgentTokenResponseCallback callback) {
            mTokenResponseCallback = callback;
        }

        @Override
        public void onEscrowTokenAdded(byte[] token, long handle, int uid)
                throws RemoteException {
            Log.d(TAG, "onEscrowTokenAdded handle:" + handle + " uid:" + uid);
            if (mTokenResponseCallback != null) {
                mTokenResponseCallback.onEscrowTokenAdded(token, handle, uid);
            }
        }

        @Override
        public void onEscrowTokenRemoved(long handle, boolean successful) throws RemoteException {
            Log.d(TAG, "onEscrowTokenRemoved handle:" + handle);
            if (mTokenResponseCallback != null) {
                mTokenResponseCallback.onEscrowTokenRemoved(handle, successful);
            }
        }

        @Override
        public void onEscrowTokenActiveStateChanged(long handle, boolean active)
                throws RemoteException {
            if (mTokenResponseCallback != null) {
                mTokenResponseCallback.onEscrowTokenActiveStateChanged(handle, active);
            }
        }
    }
}
