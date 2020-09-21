/*
 * Copyright (C) 2016 The Android Open Source Project
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

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGattServer;
import android.bluetooth.BluetoothGattServerCallback;
import android.bluetooth.BluetoothManager;
import android.car.trust.ICarTrustAgentBleService;
import android.car.trust.ICarTrustAgentTokenRequestDelegate;
import android.car.trust.ICarTrustAgentUnlockCallback;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.service.trust.TrustAgentService;
import android.util.Log;

import java.util.concurrent.TimeUnit;

/**
 * A BluetoothLE (BLE) based {@link TrustAgentService} that uses the escrow token unlock APIs. </p>
 *
 * This trust agent runs during direct boot and binds to a BLE service that listens for remote
 * devices to trigger an unlock. <p/>
 *
 * The permissions for this agent must be enabled as priv-app permissions for it to start.
 */
public class CarBleTrustAgent extends TrustAgentService {

    private static final String TAG = CarBleTrustAgent.class.getSimpleName();

    private static final long TRUST_DURATION_MS = TimeUnit.MINUTES.toMicros(5);
    private static final long BLE_RETRY_MS = TimeUnit.SECONDS.toMillis(1);

    private Handler mHandler;
    private BluetoothManager mBluetoothManager;
    private ICarTrustAgentBleService mCarTrustAgentBleService;
    private boolean mCarTrustAgentBleServiceBound;

    private final ICarTrustAgentUnlockCallback mUnlockCallback =
            new ICarTrustAgentUnlockCallback.Stub() {
        @Override
        public void onUnlockDataReceived(byte[] token, long handle) {
            UserManager um = (UserManager) getSystemService(Context.USER_SERVICE);
            // TODO(b/77854782): get the actual user to unlock by token
            UserHandle userHandle = getForegroundUserHandle();

            Log.d(TAG, "About to unlock user. Handle: " + handle
                    + " Time: " + System.currentTimeMillis());
            unlockUserWithToken(handle, token, userHandle);

            Log.d(TAG, "Attempted to unlock user, is user unlocked: "
                    + um.isUserUnlocked(userHandle)
                    + " Time: " + System.currentTimeMillis());
            setManagingTrust(true);

            if (um.isUserUnlocked(userHandle)) {
                Log.d(TAG, getString(R.string.trust_granted_explanation));
                grantTrust("Granting trust from escrow token",
                        TRUST_DURATION_MS, FLAG_GRANT_TRUST_DISMISS_KEYGUARD);
            }
        }
    };

    private final ICarTrustAgentTokenRequestDelegate mTokenRequestDelegate =
            new ICarTrustAgentTokenRequestDelegate.Stub() {
        @Override
        public void revokeTrust() {
            CarBleTrustAgent.this.revokeTrust();
        }

        @Override
        public void addEscrowToken(byte[] token, int uid) {
            CarBleTrustAgent.this.addEscrowToken(token, UserHandle.of(uid));
        }

        @Override
        public void removeEscrowToken(long handle, int uid) {
            CarBleTrustAgent.this.removeEscrowToken(handle, UserHandle.of(uid));
        }

        @Override
        public void isEscrowTokenActive(long handle, int uid) {
            CarBleTrustAgent.this.isEscrowTokenActive(handle, UserHandle.of(uid));
        }
    };

    private final ServiceConnection mServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.d(TAG, "CarTrustAgentBleService connected");
            mCarTrustAgentBleServiceBound = true;
            mCarTrustAgentBleService = ICarTrustAgentBleService.Stub.asInterface(service);
            try {
                mCarTrustAgentBleService.registerUnlockCallback(mUnlockCallback);
                mCarTrustAgentBleService.setTokenRequestDelegate(mTokenRequestDelegate);
                maybeStartBleUnlockService();
            } catch (RemoteException e) {
                Log.e(TAG, "Error registerUnlockCallback", e);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            if (mCarTrustAgentBleService != null) {
                try {
                    mCarTrustAgentBleService.unregisterUnlockCallback(mUnlockCallback);
                    mCarTrustAgentBleService.setTokenRequestDelegate(null);
                } catch (RemoteException e) {
                    Log.e(TAG, "Error unregisterUnlockCallback", e);
                }
                mCarTrustAgentBleService = null;
                mCarTrustAgentBleServiceBound = false;
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();

        Log.d(TAG, "Bluetooth trust agent starting up");
        mHandler = new Handler();
        mBluetoothManager = (BluetoothManager) getSystemService(BLUETOOTH_SERVICE);

        // If the user is already unlocked, don't bother starting the BLE service.
        UserManager um = (UserManager) getSystemService(Context.USER_SERVICE);
        if (!um.isUserUnlocked(getForegroundUserHandle())) {
            Log.d(TAG, "User locked, will now bind CarTrustAgentBleService");
            Intent intent = new Intent(this, CarTrustAgentBleService.class);
            bindService(intent, mServiceConnection, Context.BIND_AUTO_CREATE);
        } else {
            setManagingTrust(true);
        }
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "Car Trust agent shutting down");
        mHandler.removeCallbacks(null);

        // Unbind the service to avoid leaks from BLE stack.
        if (mCarTrustAgentBleServiceBound) {
            unbindService(mServiceConnection);
        }
        super.onDestroy();
    }

    @Override
    public void onDeviceLocked() {
        super.onDeviceLocked();
        if (mCarTrustAgentBleServiceBound) {
            try {
                // Only one BLE advertising is allowed, ensure enrolment advertising is stopped
                // before start unlock advertising.
                mCarTrustAgentBleService.stopEnrolmentAdvertising();
                mCarTrustAgentBleService.startUnlockAdvertising();
            } catch (RemoteException e) {
                Log.e(TAG, "Error startUnlockAdvertising", e);
            }
        }
    }

    @Override
    public void onDeviceUnlocked() {
        super.onDeviceUnlocked();
        if (mCarTrustAgentBleServiceBound) {
            try {
                // Only one BLE advertising is allowed, ensure unlock advertising is stopped.
                mCarTrustAgentBleService.stopUnlockAdvertising();
            } catch (RemoteException e) {
                Log.e(TAG, "Error stopUnlockAdvertising", e);
            }
        }
    }

    private void maybeStartBleUnlockService() {
        Log.d(TAG, "Trying to open a Ble GATT server");
        BluetoothGattServer gattServer = mBluetoothManager.openGattServer(
                this, new BluetoothGattServerCallback() {
            @Override
            public void onConnectionStateChange(BluetoothDevice device, int status, int newState) {
                super.onConnectionStateChange(device, status, newState);
            }
        });

        // The BLE stack is started up before the trust agent service, however Gatt capabilities
        // might not be ready just yet. Keep trying until a GattServer can open up before proceeding
        // to start the rest of the BLE services.
        if (gattServer == null) {
            Log.e(TAG, "Gatt not available, will try again...in " + BLE_RETRY_MS + "ms");
            mHandler.postDelayed(this::maybeStartBleUnlockService, BLE_RETRY_MS);
        } else {
            mHandler.removeCallbacks(null);
            gattServer.close();
            Log.d(TAG, "GATT available, starting up UnlockService");
            try {
                mCarTrustAgentBleService.startUnlockAdvertising();
            } catch (RemoteException e) {
                Log.e(TAG, "Error startUnlockAdvertising", e);
            }
        }
    }

    @Override
    public void onEscrowTokenRemoved(long handle, boolean successful) {
        if (mCarTrustAgentBleServiceBound) {
            try {
                mCarTrustAgentBleService.onEscrowTokenRemoved(handle, successful);
                Log.v(TAG, "Callback onEscrowTokenRemoved");
            } catch (RemoteException e) {
                Log.e(TAG, "Error callback onEscrowTokenRemoved", e);
            }
        }
    }

    @Override
    public void onEscrowTokenStateReceived(long handle, int tokenState) {
        boolean isActive = tokenState == TOKEN_STATE_ACTIVE;
        if (mCarTrustAgentBleServiceBound) {
            try {
                mCarTrustAgentBleService.onEscrowTokenActiveStateChanged(handle, isActive);
                Log.v(TAG, "Callback onEscrowTokenActiveStateChanged");
            } catch (RemoteException e) {
                Log.e(TAG, "Error callback onEscrowTokenActiveStateChanged", e);
            }
        }
    }

    @Override
    public void onEscrowTokenAdded(byte[] token, long handle, UserHandle user) {
        if (mCarTrustAgentBleServiceBound) {
            try {
                mCarTrustAgentBleService.onEscrowTokenAdded(token, handle, user.getIdentifier());
                Log.v(TAG, "Callback onEscrowTokenAdded");
            } catch (RemoteException e) {
                Log.e(TAG, "Error callback onEscrowTokenAdded", e);
            }
        }
    }

    /**
     * TODO(b/77854782): return the {@link UserHandle} of foreground user.
     * CarBleTrustAgent itself runs as user-0
     */
    private UserHandle getForegroundUserHandle() {
        Log.d(TAG, "getForegroundUserHandle for " + UserHandle.myUserId());
        return UserHandle.of(UserHandle.myUserId());
    }
}
