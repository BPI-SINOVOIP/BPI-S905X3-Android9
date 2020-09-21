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
 * limitations under the License.
 */

package com.googlecode.android_scripting.facade.bluetooth;


import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothAvrcpController;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothUuid;
import android.os.ParcelUuid;

import com.googlecode.android_scripting.Log;
import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;

public class BluetoothAvrcpFacade extends RpcReceiver {
    static final ParcelUuid[] AVRCP_UUIDS = {
        BluetoothUuid.AvrcpTarget, BluetoothUuid.AvrcpController
    };
    private final Service mService;
    private final BluetoothAdapter mBluetoothAdapter;

    private static boolean sIsAvrcpReady = false;
    private static BluetoothAvrcpController sAvrcpProfile = null;

    public BluetoothAvrcpFacade(FacadeManager manager) {
        super(manager);
        mService = manager.getService();
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mBluetoothAdapter.getProfileProxy(mService, new AvrcpServiceListener(),
        BluetoothProfile.AVRCP_CONTROLLER);
    }

    class AvrcpServiceListener implements BluetoothProfile.ServiceListener {
        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            sAvrcpProfile = (BluetoothAvrcpController) proxy;
            sIsAvrcpReady = true;
        }

        @Override
        public void onServiceDisconnected(int profile) {
            sIsAvrcpReady = false;
        }
    }

    /**
     * Is Avrcp profile ready.
     * @return Avrcp profile is ready or not.
     */
    @Rpc(description = "Is Avrcp profile ready.")
    public Boolean bluetoothAvrcpIsReady() {
        return sIsAvrcpReady;
    }

    /**
     * Get all the devices connected through AVRCP.
     * @return Lsit of the devices connected through AVRCP.
     */
    @Rpc(description = "Get all the devices connected through AVRCP.")
    public List<BluetoothDevice> bluetoothAvrcpGetConnectedDevices() {
        if (!sIsAvrcpReady) {
            Log.d("AVRCP profile is not ready.");
            return null;
        }
        return sAvrcpProfile.getConnectedDevices();
    }

    /**
     * Close AVRCP connection.
     */
    @Rpc(description = "Close AVRCP connection.")
    public void bluetoothAvrcpDisconnect() throws NoSuchMethodException,
            IllegalAccessException, IllegalArgumentException,
                InvocationTargetException {
        if (!sIsAvrcpReady) {
            Log.d("AVRCP profile is not ready.");
            return;
        }
        Method m = sAvrcpProfile.getClass().getMethod("close");
        m.invoke(sAvrcpProfile);
    }

    @Override
    public void shutdown() {
    }
}
