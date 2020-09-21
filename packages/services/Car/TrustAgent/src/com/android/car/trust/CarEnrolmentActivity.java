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

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothDevice;
import android.car.trust.ICarTrustAgentBleCallback;
import android.car.trust.ICarTrustAgentBleService;
import android.car.trust.ICarTrustAgentEnrolmentCallback;
import android.car.trust.ICarTrustAgentTokenResponseCallback;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.UserHandle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.TextView;

/**
 * Setup activity that binds {@link CarTrustAgentBleService} and starts the enrolment process.
 */
public class CarEnrolmentActivity extends Activity {

    private static final String TAG = CarEnrolmentActivity.class.getSimpleName();

    private static final String SP_HANDLE_KEY = "sp-test";
    private static final int FINE_LOCATION_REQUEST_CODE = 42;

    private final ICarTrustAgentTokenResponseCallback mCarTrustAgentTokenResponseCallback =
            new ICarTrustAgentTokenResponseCallback.Stub() {
        @Override
        public void onEscrowTokenAdded(byte[] token, long handle, int uid) {
            runOnUiThread(() -> {
                mPrefs.edit().putLong(SP_HANDLE_KEY, handle).apply();
                Log.d(TAG, "stored new handle");
            });

            if (mBluetoothDevice == null) {
                Log.e(TAG, "No active bluetooth found to add escrow token");
                return;
            }

            try {
                // Notify the enrolment client that escrow token has been added
                mCarTrustAgentBleService.sendEnrolmentHandle(mBluetoothDevice, handle);
                appendOutputText("Escrow Token Added. Handle: " + handle);
                appendOutputText("Lock and unlock the device to activate token");
            } catch (RemoteException e) {
                Log.e(TAG, "Error sendEnrolmentHandle", e);
            }
        }

        @Override
        public void onEscrowTokenRemoved(long handle, boolean successful) {
            appendOutputText("Escrow token Removed. Handle: " + handle);
        }

        @Override
        public void onEscrowTokenActiveStateChanged(long handle, boolean active) {
            appendOutputText("Is token active? " + active + " handle: " + handle);
        }
    };

    private final ICarTrustAgentBleCallback mBleConnectionCallback =
            new ICarTrustAgentBleCallback.Stub() {
        @Override
        public void onBleServerStartSuccess() {
            appendOutputText("Server started");
        }

        @Override
        public void onBleServerStartFailure(int errorCode) {
            appendOutputText("Server failed to start, error code: " + errorCode);
        }

        @Override
        public void onBleDeviceConnected(BluetoothDevice device) {
            mBluetoothDevice = device;
            appendOutputText("Device connected: " + device.getName()
                    + " address: " + device.getAddress());
        }

        @Override
        public void onBleDeviceDisconnected(BluetoothDevice device) {
            mBluetoothDevice = null;
            appendOutputText("Device disconnected: " + device.getName()
                    + " address: " + device.getAddress());
        }
    };

    private final ICarTrustAgentEnrolmentCallback mEnrolmentCallback =
            new ICarTrustAgentEnrolmentCallback.Stub() {
        @Override
        public void onEnrolmentDataReceived(byte[] token) {
            appendOutputText("Enrolment data received ");
            try {
                addEscrowToken(token);
            } catch (RemoteException e) {
                Log.e(TAG, "Error addEscrowToken", e);
            }
        }
    };

    private final ServiceConnection mServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mCarTrustAgentBleServiceBound = true;
            mCarTrustAgentBleService = ICarTrustAgentBleService.Stub.asInterface(service);
            try {
                mCarTrustAgentBleService.registerBleCallback(mBleConnectionCallback);
                mCarTrustAgentBleService.registerEnrolmentCallback(mEnrolmentCallback);
                mCarTrustAgentBleService.setTokenResponseCallback(
                        mCarTrustAgentTokenResponseCallback);
                mCarTrustAgentBleService.startEnrolmentAdvertising();
            } catch (RemoteException e) {
                Log.e(TAG, "Error startEnrolmentAdvertising", e);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            if (mCarTrustAgentBleService != null) {
                try {
                    mCarTrustAgentBleService.unregisterBleCallback(mBleConnectionCallback);
                    mCarTrustAgentBleService.unregisterEnrolmentCallback(mEnrolmentCallback);
                    mCarTrustAgentBleService.setTokenResponseCallback(null);
                    mCarTrustAgentBleService.stopEnrolmentAdvertising();
                } catch (RemoteException e) {
                    Log.e(TAG, "Error unregister callbacks", e);
                }
                mCarTrustAgentBleService = null;
            }
            mCarTrustAgentBleServiceBound = false;
        }
    };

    private TextView mOutputText;
    private BluetoothDevice mBluetoothDevice;
    private ICarTrustAgentBleService mCarTrustAgentBleService;
    private boolean mCarTrustAgentBleServiceBound;
    private SharedPreferences mPrefs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.car_enrolment_activity);
        mOutputText = findViewById(R.id.textfield);
        mPrefs = PreferenceManager.getDefaultSharedPreferences(this /* context */);

        findViewById(R.id.start_button).setOnClickListener((view) -> {
            Intent bindIntent = new Intent(this, CarTrustAgentBleService.class);
            bindService(bindIntent, mServiceConnection, Context.BIND_AUTO_CREATE);
        });

        findViewById(R.id.revoke_trust_button).setOnClickListener((view) -> {
            if (mCarTrustAgentBleServiceBound) {
                try {
                    mCarTrustAgentBleService.revokeTrust();
                } catch (RemoteException e) {
                    Log.e(TAG, "Error revokeTrust", e);
                }
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();

        if (checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) !=
                PackageManager.PERMISSION_GRANTED) {
            requestPermissions(
                    new String[] { android.Manifest.permission.ACCESS_FINE_LOCATION },
                    FINE_LOCATION_REQUEST_CODE);
        } else {
            long tokenHandle = getTokenHandle();
            if (tokenHandle != -1) {
                Log.d(TAG, "onResume, checking handle active: " + tokenHandle);
                if (mCarTrustAgentBleServiceBound) {
                    try {
                        // Due to the asynchronous nature of isEscrowTokenActive in
                        // TrustAgentService, query result will be delivered via
                        // {@link #mCarTrustAgentTokenResponseCallback}
                        mCarTrustAgentBleService.isEscrowTokenActive(tokenHandle,
                                UserHandle.myUserId());
                    } catch (RemoteException e) {
                        Log.e(TAG, "Error isEscrowTokenActive", e);
                    }
                }
            } else {
                appendOutputText("No handles found");
            }
        }
    }

    @Override
    protected void onDestroy() {
        if (mCarTrustAgentBleServiceBound) {
            unbindService(mServiceConnection);
        }
        super.onDestroy();
    }

    private void appendOutputText(final String text) {
        runOnUiThread(() -> mOutputText.append("\n" + text));
    }

    private void addEscrowToken(byte[] token) throws RemoteException {
        if (!mCarTrustAgentBleServiceBound) {
            Log.e(TAG, "No CarTrustAgentBleService bounded");
            return;
        }
        mCarTrustAgentBleService.addEscrowToken(token, UserHandle.myUserId());
    }

    private long getTokenHandle() {
        return mPrefs.getLong(SP_HANDLE_KEY, -1);
    }
}
