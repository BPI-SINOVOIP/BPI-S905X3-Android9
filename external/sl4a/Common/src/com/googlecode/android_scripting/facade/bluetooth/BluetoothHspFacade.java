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
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothUuid;
import android.os.ParcelUuid;

import com.googlecode.android_scripting.Log;
import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcParameter;

import java.util.List;

public class BluetoothHspFacade extends RpcReceiver {
    static final ParcelUuid[] UUIDS = {
            BluetoothUuid.HSP, BluetoothUuid.Handsfree
    };

    private final Service mService;
    private final BluetoothAdapter mBluetoothAdapter;

    private static boolean sIsHspReady = false;
    private static BluetoothHeadset sHspProfile = null;

    public BluetoothHspFacade(FacadeManager manager) {
        super(manager);
        mService = manager.getService();
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mBluetoothAdapter.getProfileProxy(mService, new HspServiceListener(),
                BluetoothProfile.HEADSET);
    }

    class HspServiceListener implements BluetoothProfile.ServiceListener {
        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            sHspProfile = (BluetoothHeadset) proxy;
            sIsHspReady = true;
        }

        @Override
        public void onServiceDisconnected(int profile) {
            sIsHspReady = false;
        }
    }

    /**
     * Connect to Hsp Profile
     * @param device - the BluetoothDevice object to connect to.
     * @return if the connection was successfull or not.
     */
    public Boolean hspConnect(BluetoothDevice device) {
        if (sHspProfile == null) return false;
        return sHspProfile.connect(device);
    }

    /**
     * Disconnect to Hsp Profile.
     * @param device - the Bluetooth Device object to disconnect from.
     * @return if the disconnection was successfull or not.
     */
    public Boolean hspDisconnect(BluetoothDevice device) {
        if (sHspProfile == null) return false;
        return sHspProfile.disconnect(device);
    }

    /**
     * Is Hsp profile ready.
     * @return if Hid profile is ready or not.
     */
    @Rpc(description = "Is Hsp profile ready.")
    public Boolean bluetoothHspIsReady() {
        return sIsHspReady;
    }

    /**
     * Set priority of the profile.
     * @param deviceStr - name or MAC address of a Bluetooth device.
     * @param priority - Priority that needs to be set.
     */
    @Rpc(description = "Set priority of the profile.")
    public void bluetoothHspSetPriority(
            @RpcParameter(name = "device", description = "Mac address of a BT device.")
                String deviceStr,
            @RpcParameter(name = "priority", description = "Priority that needs to be set.")
                Integer priority) throws Exception {
        if (sHspProfile == null) return;
        BluetoothDevice device = BluetoothFacade.getDevice(
                mBluetoothAdapter.getBondedDevices(), deviceStr);
        Log.d("Changing priority of device " + device.getAliasName() + " p: " + priority);
        sHspProfile.setPriority(device, priority);
    }

    /**
     * Connect to an HSP device.
     * @param device - Name or MAC address of a bluetooth device.
     * @return True if the connection was successful; otherwise False.
     */
    @Rpc(description = "Connect to an HSP device.")
    public Boolean bluetoothHspConnect(
            @RpcParameter(name = "device", description =
                "Name or MAC address of a bluetooth device.")
                String device) throws Exception {
        if (sHspProfile == null) return false;
        BluetoothDevice mDevice = BluetoothFacade.getDevice(
                mBluetoothAdapter.getBondedDevices(), device);
        Log.d("Connecting to device " + mDevice.getAliasName());
        return hspConnect(mDevice);
    }

    /**
     * Disconnect an HSP device.
     * @param device - Name or MAC address of a bluetooth device.
     * @return True if the disconnection was successful; otherwise False.
     */
    @Rpc(description = "Disconnect an HSP device.")
    public Boolean bluetoothHspDisconnect(
            @RpcParameter(name = "device", description = "Name or MAC address of a device.")
                String device) throws Exception {
        if (sHspProfile == null) return false;
        Log.d("Connected devices: " + sHspProfile.getConnectedDevices());
        BluetoothDevice mDevice = BluetoothFacade.getDevice(
                sHspProfile.getConnectedDevices(), device);
        return hspDisconnect(mDevice);
    }

     /**
     * Get all the devices connected through HSP.
     * @return List of all the devices connected through HSP.
     */
    @Rpc(description = "Get all the devices connected through HSP.")
    public List<BluetoothDevice> bluetoothHspGetConnectedDevices() {
        if (!sIsHspReady) return null;
        return sHspProfile.getConnectedDevices();
    }

    /**
     * Get the connection status of a device.
     * @param deviceID - Name or MAC address of a bluetooth device.
     * @return connection status of a device.
     */
    @Rpc(description = "Get the connection status of a device.")
    public Integer bluetoothHspGetConnectionStatus(
            @RpcParameter(name = "deviceID",
                description = "Name or MAC address of a bluetooth device.")
                    String deviceID) {
        if (sHspProfile == null) {
            return BluetoothProfile.STATE_DISCONNECTED;
        }
        List<BluetoothDevice> deviceList = sHspProfile.getConnectedDevices();
        BluetoothDevice device;
        try {
            device = BluetoothFacade.getDevice(deviceList, deviceID);
        } catch (Exception e) {
            return BluetoothProfile.STATE_DISCONNECTED;
        }
        return sHspProfile.getConnectionState(device);
    }

    /**
     * Force SCO audio on DUT, ignore all other restrictions
     *
     * @param force True to force SCO audio, False to resume normal
     * @return True if the setup is successful
     */
    @Rpc(description = "Force SCO audio connection on DUT.")
    public Boolean bluetoothHspForceScoAudio(
            @RpcParameter(name = "force", description = "whether to force SCO audio")
                Boolean force) {
        if (sHspProfile == null) {
            return false;
        }
        sHspProfile.setForceScoAudio(force);
        return true;
    }

    /**
     * Connect SCO audio to a remote device
     *
     * @param deviceAddress the Bluetooth MAC address of remote device
     * @return True if connection is successful, False otherwise
     */
    @Rpc(description = "Connect SCO audio for a remote device.")
    public Boolean bluetoothHspConnectAudio(
            @RpcParameter(name = "deviceAddress",
                description = "MAC address of a bluetooth device.")
                    String deviceAddress) {
        if (sHspProfile == null) {
            return false;
        }
        Log.d("Connected devices: " + sHspProfile.getConnectedDevices());
        BluetoothDevice device = null;
        if (sHspProfile.getConnectedDevices().size() > 1) {
            Log.d("More than one device available");
        }
        try {
            device = BluetoothFacade.getDevice(
                    sHspProfile.getConnectedDevices(), deviceAddress);
        } catch (Exception e) {
            Log.d("Cannot find device " + deviceAddress);
            return false;
        }
        return sHspProfile.connectAudio();
    }

    /**
     * Disconnect SCO audio for a remote device
     *
     * @param deviceAddress the Bluetooth MAC address of remote device
     * @return True if disconnection is successful, False otherwise
     */
    @Rpc(description = "Disconnect SCO audio for a remote device")
    public Boolean bluetoothHspDisconnectAudio(
            @RpcParameter(name = "deviceAddress",
                description = "MAC address of a bluetooth device.")
                    String deviceAddress) {
        if (sHspProfile == null) {
            return false;
        }
        Log.d("Connected devices: " + sHspProfile.getConnectedDevices());
        BluetoothDevice device = null;
        if (sHspProfile.getConnectedDevices().size() > 1) {
            Log.d("More than one device available");
        }
        try {
            device = BluetoothFacade.getDevice(
                    sHspProfile.getConnectedDevices(), deviceAddress);
        } catch (Exception e) {
            Log.d("Cannot find device " + deviceAddress);
            return false;
        }
        if (!sHspProfile.isAudioConnected(device)) {
            Log.d("SCO audio is not connected for device " + deviceAddress);
            return false;
        }
        return sHspProfile.disconnectAudio();
    }

    /**
     * Check if SCO audio is connected for a remote device
     *
     * @param deviceAddress the Bluetooth MAC address of remote device
     * @return True if device is connected to us via SCO audio, False otherwise
     */
    @Rpc(description = "Check if SCO audio is connected for a remote device")
    public Boolean bluetoothHspIsAudioConnected(
            @RpcParameter(name = "deviceAddress",
                description = "MAC address of a bluetooth device.")
                    String deviceAddress) {
        if (sHspProfile == null) {
            return false;
        }
        Log.d("Connected devices: " + sHspProfile.getConnectedDevices());
        BluetoothDevice device = null;
        if (sHspProfile.getConnectedDevices().size() > 1) {
            Log.d("More than one device available");
        }
        try {
            device = BluetoothFacade.getDevice(
                    sHspProfile.getConnectedDevices(), deviceAddress);
        } catch (Exception e) {
            Log.d("Cannot find device " + deviceAddress);
            return false;
        }
        return sHspProfile.isAudioConnected(device);
    }

    /**
     * Start voice recognition. Send BVRA command.
     *
     * @param deviceAddress the Bluetooth MAC address of remote device
     * @return True if started successfully, False otherwise.
     */
    @Rpc(description = "Start Voice Recognition.")
    public Boolean bluetoothHspStartVoiceRecognition(
            @RpcParameter(name = "deviceAddress",
                    description = "MAC address of a bluetooth device.")
                        String deviceAddress) throws Exception {
        BluetoothDevice device = BluetoothFacade.getDevice(
                sHspProfile.getConnectedDevices(), deviceAddress);
        return sHspProfile.startVoiceRecognition(device);
    }

    /**
     * Stop voice recognition. Send BVRA command.
     *
     * @param deviceAddress the Bluetooth MAC address of remote device
     * @return True if stopped successfully, False otherwise.
     */
    @Rpc(description = "Stop Voice Recognition.")
    public Boolean bluetoothHspStopVoiceRecognition(
            @RpcParameter(name = "deviceAddress",
                description = "MAC address of a bluetooth device.")
                    String deviceAddress) throws Exception {
        BluetoothDevice device = BluetoothFacade.getDevice(
                sHspProfile.getConnectedDevices(), deviceAddress);
        return sHspProfile.stopVoiceRecognition(device);
    }

    /**
     * Determine whether in-band ringtone is enabled or not.
     *
     * @return True if enabled, False otherwise.
     */
    @Rpc(description = "In-band ringtone enabled check.")
    public Boolean bluetoothHspIsInbandRingingEnabled() {
        return sHspProfile.isInbandRingingEnabled();
    }

    /**
     * Send a CLCC response from Sl4a (experimental).
     *
     * @param index the index of the call
     * @param direction the direction of the call
     * @param status the status of the call
     * @param mode the mode
     * @param mpty the mpty value
     * @param number the phone number
     * @param type the type
     */
    @Rpc(description = "Send generic clcc response.")
    public void bluetoothHspClccResponse(
            @RpcParameter(name = "index", description = "") Integer index,
            @RpcParameter(name = "direction", description = "") Integer direction,
            @RpcParameter(name = "status", description = "") Integer status,
            @RpcParameter(name = "mode", description = "") Integer mode,
            @RpcParameter(name = "mpty", description = "") Boolean mpty,
            @RpcParameter(name = "number", description = "") String number,
            @RpcParameter(name = "type", description = "") Integer type
                  ) {
        sHspProfile.clccResponse(index, direction, status, mode, mpty, number, type);
    }

    @Override
    public void shutdown() {
    }
}
