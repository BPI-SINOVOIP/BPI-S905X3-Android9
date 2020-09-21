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
 * limitations under the License
 */

package com.android.server.telecom.bluetooth;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadset;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.telecom.Log;
import android.telecom.Logging.Session;

import com.android.internal.os.SomeArgs;

import static com.android.server.telecom.bluetooth.BluetoothRouteManager.HFP_IS_ON;
import static com.android.server.telecom.bluetooth.BluetoothRouteManager.HFP_LOST;


public class BluetoothStateReceiver extends BroadcastReceiver {
    private static final String LOG_TAG = BluetoothStateReceiver.class.getSimpleName();
    public static final IntentFilter INTENT_FILTER;
    static {
        INTENT_FILTER = new IntentFilter();
        INTENT_FILTER.addAction(BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED);
        INTENT_FILTER.addAction(BluetoothHeadset.ACTION_AUDIO_STATE_CHANGED);
        INTENT_FILTER.addAction(BluetoothHeadset.ACTION_ACTIVE_DEVICE_CHANGED);
    }

    // If not in a call, BSR won't listen to the Bluetooth stack's HFP on/off messages, since
    // other apps could be turning it on and off. We don't want to interfere.
    private boolean mIsInCall = false;
    private final BluetoothRouteManager mBluetoothRouteManager;
    private final BluetoothDeviceManager mBluetoothDeviceManager;

    public void onReceive(Context context, Intent intent) {
        Log.startSession("BSR.oR");
        try {
            String action = intent.getAction();
            switch (action) {
                case BluetoothHeadset.ACTION_AUDIO_STATE_CHANGED:
                    handleAudioStateChanged(intent);
                    break;
                case BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED:
                    handleConnectionStateChanged(intent);
                    break;
                case BluetoothHeadset.ACTION_ACTIVE_DEVICE_CHANGED:
                    handleActiveDeviceChanged(intent);
                    break;
            }
        } finally {
            Log.endSession();
        }
    }

    private void handleAudioStateChanged(Intent intent) {
        if (!mIsInCall) {
            Log.i(LOG_TAG, "Ignoring BT audio state change since we're not in a call");
            return;
        }
        int bluetoothHeadsetAudioState =
                intent.getIntExtra(BluetoothHeadset.EXTRA_STATE,
                        BluetoothHeadset.STATE_AUDIO_DISCONNECTED);
        BluetoothDevice device =
                intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        if (device == null) {
            Log.w(LOG_TAG, "Got null device from broadcast. " +
                    "Ignoring.");
            return;
        }

        Log.i(LOG_TAG, "Device %s transitioned to audio state %d",
                device.getAddress(), bluetoothHeadsetAudioState);
        Session session = Log.createSubsession();
        SomeArgs args = SomeArgs.obtain();
        args.arg1 = session;
        args.arg2 = device.getAddress();
        switch (bluetoothHeadsetAudioState) {
            case BluetoothHeadset.STATE_AUDIO_CONNECTED:
                mBluetoothRouteManager.sendMessage(HFP_IS_ON, args);
                break;
            case BluetoothHeadset.STATE_AUDIO_DISCONNECTED:
                mBluetoothRouteManager.sendMessage(HFP_LOST, args);
                break;
        }
    }

    private void handleConnectionStateChanged(Intent intent) {
        int bluetoothHeadsetState = intent.getIntExtra(BluetoothHeadset.EXTRA_STATE,
                BluetoothHeadset.STATE_DISCONNECTED);
        BluetoothDevice device =
                intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);

        if (device == null) {
            Log.w(LOG_TAG, "Got null device from broadcast. " +
                    "Ignoring.");
            return;
        }

        Log.i(LOG_TAG, "Device %s changed state to %d",
                device.getAddress(), bluetoothHeadsetState);

        if (bluetoothHeadsetState == BluetoothHeadset.STATE_CONNECTED) {
            mBluetoothDeviceManager.onDeviceConnected(device);
        } else if (bluetoothHeadsetState == BluetoothHeadset.STATE_DISCONNECTED
                || bluetoothHeadsetState == BluetoothHeadset.STATE_DISCONNECTING) {
            mBluetoothDeviceManager.onDeviceDisconnected(device);
        }
    }

    private void handleActiveDeviceChanged(Intent intent) {
        BluetoothDevice device =
                intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        Log.i(LOG_TAG, "Device %s is now the preferred HFP device", device);
        mBluetoothRouteManager.onActiveDeviceChanged(device);
    }

    public BluetoothStateReceiver(BluetoothDeviceManager deviceManager,
            BluetoothRouteManager routeManager) {
        mBluetoothDeviceManager = deviceManager;
        mBluetoothRouteManager = routeManager;
    }

    public void setIsInCall(boolean isInCall) {
        mIsInCall = isInCall;
    }
}
