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
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothPbapClient;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothUuid;
import android.os.ParcelUuid;

import com.googlecode.android_scripting.Log;
import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcParameter;

import java.util.ArrayList;
import java.util.List;

/**
 * BluetoothPbapClientFacade
 */
public class BluetoothPbapClientFacade extends RpcReceiver {
    static final ParcelUuid[] UUIDS = {
        BluetoothUuid.PBAP_PSE,
    };

    private final Service mService;
    private final BluetoothAdapter mBluetoothAdapter;

    private static boolean sIsPbapClientReady = false;
    private static BluetoothPbapClient sPbapClientProfile = null;

    public BluetoothPbapClientFacade(FacadeManager manager) {
        super(manager);
        mService = manager.getService();
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mBluetoothAdapter.getProfileProxy(mService,
                new PbapClientServiceListener(), BluetoothProfile.PBAP_CLIENT);
    }

    class PbapClientServiceListener implements
            BluetoothProfile.ServiceListener {
        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            sPbapClientProfile = (BluetoothPbapClient) proxy;
            sIsPbapClientReady = true;
        }

        @Override
        public void onServiceDisconnected(int profile) {
            sIsPbapClientReady = false;
        }
    }

    /**
     * Connect PbapClient profile.
     * @param device - the BluetoothDevice object to connect to.
     * @return if the connection was successfull or not.
     */
    public Boolean pbapClientConnect(BluetoothDevice device) {
        if (sPbapClientProfile == null) return false;
        return sPbapClientProfile.connect(device);
    }

    /**
     * Disconnect PbapClient profile.
     * @param device - the Bluetooth Device object to disconnect from.
     * @return if the disconnection was successfull or not.
     */
    public Boolean pbapClientDisconnect(BluetoothDevice device) {
        if (sPbapClientProfile == null) return false;
        return sPbapClientProfile.disconnect(device);
    }

    /**
     * Is PbapClient profile ready.
     * @return if PbapClient profile is ready or not.
     */
    @Rpc(description = "Is PbapClient profile ready.")
    public Boolean bluetoothPbapClientIsReady() {
        return sIsPbapClientReady;
    }

    /**
     * Set priority of the profile.
     * @param deviceStr - name or MAC address of a Bluetooth device.
     * @param priority - Priority that needs to be set.
     */
    @Rpc(description = "Set priority of the profile")
    public void bluetoothPbapClientSetPriority(@RpcParameter(name = "device",
                description = "Mac address of a BT device.") String deviceStr,
            @RpcParameter(name = "priority",
                description = "Priority that needs to be set.")
                Integer priority) throws Exception {
        if (sPbapClientProfile == null) return;
        BluetoothDevice device =
                BluetoothFacade.getDevice(mBluetoothAdapter.getBondedDevices(),
                deviceStr);
        Log.d("Changing priority of device " + device.getAliasName()
                + " p: " + priority);
        sPbapClientProfile.setPriority(device, priority);
    }

    /**
     * Get priority of the profile.
     * @return Priority of the device.
     */
    @Rpc(description = "Get priority of the profile")
    public Integer bluetoothPbapClientGetPriority(
            @RpcParameter(name = "device",
                description = "Mac address of a BT device.") String deviceStr)
                throws Exception {
        if (sPbapClientProfile == null) {
            return BluetoothProfile.PRIORITY_UNDEFINED;
        }
        BluetoothDevice device =
                BluetoothFacade.getDevice(mBluetoothAdapter.getBondedDevices(),
                    deviceStr);
        return sPbapClientProfile.getPriority(device);
    }

    /**
     * Connect to an PBAP Client device.
     * @param deviceStr - Name or MAC address of a bluetooth device.
     * @return True if the connection was successful; otherwise False.
     */
    @Rpc(description = "Connect to an PBAP Client device.")
    public Boolean bluetoothPbapClientConnect(
                @RpcParameter(name = "device",
                    description = "Name or MAC address of a bluetooth device.")
                    String deviceStr) throws Exception {
        if (sPbapClientProfile == null) return false;
        try {
            BluetoothDevice device =
                    BluetoothFacade.getDevice(
                            mBluetoothAdapter.getBondedDevices(), deviceStr);
            Log.d("Connecting to device " + device.getAliasName());
            return pbapClientConnect(device);
        } catch (Exception e) {
            Log.e("bluetoothPbapClientConnect failed on getDevice "
                    + deviceStr + " with " + e);
            return false;
        }
    }

    /**
     * Disconnect an PBAP Client device.
     * @param deviceStr - Name or MAC address of a bluetooth device.
     * @return True if the disconnection was successful; otherwise False.
     */
    @Rpc(description = "Disconnect an PBAP Client device.")
    public Boolean bluetoothPbapClientDisconnect(
            @RpcParameter(name = "device",
                description = "Name or MAC address of a device.")
                String deviceStr) {
        if (sPbapClientProfile == null) return false;
        Log.d("Connected devices: " + sPbapClientProfile.getConnectedDevices());
        try {
            BluetoothDevice device =
                    BluetoothFacade.getDevice(
                        sPbapClientProfile.getConnectedDevices(), deviceStr);
            return pbapClientDisconnect(device);
        } catch (Exception e) {
            // Do nothing since it is disconnect and the above call
            // should force disconnect.
            Log.e("bluetoothPbapClientConnect getDevice failed " + e);
        }
        return false;
    }

    /**
     * Get all the devices connected through PBAP Client.
     * @return List of all the devices connected through PBAP client.
     */
    @Rpc(description = "Get all the devices connected through PBAP Client.")
    public List<BluetoothDevice> bluetoothPbapClientGetConnectedDevices() {
        if (sPbapClientProfile == null) return new ArrayList<BluetoothDevice>();
        return sPbapClientProfile.getConnectedDevices();
    }

     /**
     * Get the connection status of a device.
     * @return connection status of the device.
     */
    @Rpc(description = "Get the connection status of a device.")
    public Integer bluetoothPbapClientGetConnectionStatus(
            @RpcParameter(name = "deviceID",
            description = "Name or MAC address of a bluetooth device.")
            String deviceID) {
        if (sPbapClientProfile == null) {
            return BluetoothProfile.STATE_DISCONNECTED;
        }
        List<BluetoothDevice> deviceList =
                sPbapClientProfile.getConnectedDevices();
        BluetoothDevice device;
        try {
            device = BluetoothFacade.getDevice(deviceList, deviceID);
        } catch (Exception e) {
            Log.e(e);
            return BluetoothProfile.STATE_DISCONNECTED;
        }
        return sPbapClientProfile.getConnectionState(device);
    }

    @Override
    public void shutdown() {
    }
}
