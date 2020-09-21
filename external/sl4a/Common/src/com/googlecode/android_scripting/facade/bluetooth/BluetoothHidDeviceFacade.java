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
import android.bluetooth.BluetoothHidDevice;
import android.bluetooth.BluetoothHidDeviceAppQosSettings;
import android.bluetooth.BluetoothHidDeviceAppSdpSettings;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothUuid;
import android.os.Bundle;
import android.os.ParcelUuid;

import com.googlecode.android_scripting.Log;
import com.googlecode.android_scripting.facade.EventFacade;
import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcParameter;

import java.util.List;

public class BluetoothHidDeviceFacade extends RpcReceiver {

    public static final ParcelUuid[] UUIDS = {BluetoothUuid.Hid};

    public static final byte ID_KEYBOARD = 1;
    public static final byte ID_MOUSE = 2;

    public static final byte[] HIDD_REPORT_DESC = {
            (byte) 0x05,
            (byte) 0x01, // Usage page (Generic Desktop)
            (byte) 0x09,
            (byte) 0x06, // Usage (Keyboard)
            (byte) 0xA1,
            (byte) 0x01, // Collection (Application)
            (byte) 0x85,
            ID_KEYBOARD, //    Report ID
            (byte) 0x05,
            (byte) 0x07, //       Usage page (Key Codes)
            (byte) 0x19,
            (byte) 0xE0, //       Usage minimum (224)
            (byte) 0x29,
            (byte) 0xE7, //       Usage maximum (231)
            (byte) 0x15,
            (byte) 0x00, //       Logical minimum (0)
            (byte) 0x25,
            (byte) 0x01, //       Logical maximum (1)
            (byte) 0x75,
            (byte) 0x01, //       Report size (1)
            (byte) 0x95,
            (byte) 0x08, //       Report count (8)
            (byte) 0x81,
            (byte) 0x02, //       Input (Data, Variable, Absolute) ; Modifier byte
            (byte) 0x75,
            (byte) 0x08, //       Report size (8)
            (byte) 0x95,
            (byte) 0x01, //       Report count (1)
            (byte) 0x81,
            (byte) 0x01, //       Input (Constant)                 ; Reserved byte
            (byte) 0x75,
            (byte) 0x08, //       Report size (8)
            (byte) 0x95,
            (byte) 0x06, //       Report count (6)
            (byte) 0x15,
            (byte) 0x00, //       Logical Minimum (0)
            (byte) 0x25,
            (byte) 0x65, //       Logical Maximum (101)
            (byte) 0x05,
            (byte) 0x07, //       Usage page (Key Codes)
            (byte) 0x19,
            (byte) 0x00, //       Usage Minimum (0)
            (byte) 0x29,
            (byte) 0x65, //       Usage Maximum (101)
            (byte) 0x81,
            (byte) 0x00, //       Input (Data, Array)              ; Key array (6 keys)
            (byte) 0xC0, // End Collection
            (byte) 0x05,
            (byte) 0x01, // Usage Page (Generic Desktop)
            (byte) 0x09,
            (byte) 0x02, // Usage (Mouse)
            (byte) 0xA1,
            (byte) 0x01, // Collection (Application)
            (byte) 0x85,
            ID_MOUSE, //    Report ID
            (byte) 0x09,
            (byte) 0x01, //    Usage (Pointer)
            (byte) 0xA1,
            (byte) 0x00, //    Collection (Physical)
            (byte) 0x05,
            (byte) 0x09, //       Usage Page (Buttons)
            (byte) 0x19,
            (byte) 0x01, //       Usage minimum (1)
            (byte) 0x29,
            (byte) 0x03, //       Usage maximum (3)
            (byte) 0x15,
            (byte) 0x00, //       Logical minimum (0)
            (byte) 0x25,
            (byte) 0x01, //       Logical maximum (1)
            (byte) 0x75,
            (byte) 0x01, //       Report size (1)
            (byte) 0x95,
            (byte) 0x03, //       Report count (3)
            (byte) 0x81,
            (byte) 0x02, //       Input (Data, Variable, Absolute)
            (byte) 0x75,
            (byte) 0x05, //       Report size (5)
            (byte) 0x95,
            (byte) 0x01, //       Report count (1)
            (byte) 0x81,
            (byte) 0x01, //       Input (constant)                 ; 5 bit padding
            (byte) 0x05,
            (byte) 0x01, //       Usage page (Generic Desktop)
            (byte) 0x09,
            (byte) 0x30, //       Usage (X)
            (byte) 0x09,
            (byte) 0x31, //       Usage (Y)
            (byte) 0x09,
            (byte) 0x38, //       Usage (Wheel)
            (byte) 0x15,
            (byte) 0x81, //       Logical minimum (-127)
            (byte) 0x25,
            (byte) 0x7F, //       Logical maximum (127)
            (byte) 0x75,
            (byte) 0x08, //       Report size (8)
            (byte) 0x95,
            (byte) 0x03, //       Report count (3)
            (byte) 0x81,
            (byte) 0x06, //       Input (Data, Variable, Relative)
            (byte) 0xC0, //    End Collection
            (byte) 0xC0 // End Collection
    };

    // Default values.
    private static final int QOS_TOKEN_RATE = 800; // 9 bytes * 1000000 us / 11250 us
    private static final int QOS_TOKEN_BUCKET_SIZE = 9;
    private static final int QOS_PEAK_BANDWIDTH = 0;
    private static final int QOS_LATENCY = 11250;

    private final Service mService;
    private final BluetoothAdapter mBluetoothAdapter;
    private final EventFacade mEventFacade;

    private static boolean sIsHidDeviceReady = false;
    private static BluetoothHidDevice sHidDeviceProfile = null;

    private BluetoothHidDevice.Callback mCallback = new BluetoothHidDevice.Callback() {
        @Override
        public void onAppStatusChanged(BluetoothDevice pluggedDevice, boolean registered) {
            Log.d("onAppStatusChanged: pluggedDevice=" + pluggedDevice + " registered="
                    + registered);
            Bundle result = new Bundle();
            result.putBoolean("registered", registered);
            mEventFacade.postEvent("onAppStatusChanged", result);
        }

        @Override
        public void onConnectionStateChanged(BluetoothDevice device, int state) {
            Log.d("onConnectionStateChanged: device=" + device + " state=" + state);
            Bundle result = new Bundle();
            result.putInt("state", state);
            mEventFacade.postEvent("onConnectionStateChanged", result);
        }

        @Override
        public void onGetReport(BluetoothDevice device, byte type, byte id, int bufferSize) {
            Log.d("onGetReport: device=" + device + " type=" + type + " id=" + id + " bufferSize="
                    + bufferSize);
            Bundle result = new Bundle();
            result.putByte("type", type);
            result.putByte("id", id);
            result.putInt("bufferSize", bufferSize);
            mEventFacade.postEvent("onGetReport", result);
        }

        @Override
        public void onSetReport(BluetoothDevice device, byte type, byte id, byte[] data) {
            Log.d("onSetReport: device=" + device + " type=" + type + " id=" + id);
            Bundle result = new Bundle();
            result.putByte("type", type);
            result.putByte("id", id);
            result.putByteArray("data", data);
            mEventFacade.postEvent("onSetReport", result);
        }

        @Override
        public void onSetProtocol(BluetoothDevice device, byte protocol) {
            Log.d("onSetProtocol: device=" + device + " protocol=" + protocol);
            Bundle result = new Bundle();
            result.putByte("protocol", protocol);
            mEventFacade.postEvent("onSetProtocol", result);
        }

        @Override
        public void onInterruptData(BluetoothDevice device, byte reportId, byte[] data) {
            Log.d("onInterruptData: device=" + device + " reportId=" + reportId);
            Bundle result = new Bundle();
            result.putByte("registered", reportId);
            result.putByteArray("data", data);
            mEventFacade.postEvent("onInterruptData", result);
        }

        @Override
        public void onVirtualCableUnplug(BluetoothDevice device) {
            Log.d("onVirtualCableUnplug: device=" + device);
            Bundle result = new Bundle();
            mEventFacade.postEvent("onVirtualCableUnplug", result);
        }
    };

    private static BluetoothHidDeviceAppSdpSettings sSdpSettings =
            new BluetoothHidDeviceAppSdpSettings("Mock App", "Mock", "Google",
                    BluetoothHidDevice.SUBCLASS1_COMBO, HIDD_REPORT_DESC);

    private static BluetoothHidDeviceAppQosSettings sQos =
            new BluetoothHidDeviceAppQosSettings(
                    BluetoothHidDeviceAppQosSettings.SERVICE_BEST_EFFORT,
                    QOS_TOKEN_RATE,
                    QOS_TOKEN_BUCKET_SIZE,
                    QOS_PEAK_BANDWIDTH,
                    QOS_LATENCY,
                    BluetoothHidDeviceAppQosSettings.MAX);

    public BluetoothHidDeviceFacade(FacadeManager manager) {
        super(manager);
        mService = manager.getService();
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mBluetoothAdapter.getProfileProxy(mService, new HidDeviceServiceListener(),
                BluetoothProfile.HID_DEVICE);
        mEventFacade = manager.getReceiver(EventFacade.class);
        Log.w("Init HID Device Facade");
    }

    class HidDeviceServiceListener implements BluetoothProfile.ServiceListener {

        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            Log.d("BluetoothHidDeviceFacade: onServiceConnected");
            sHidDeviceProfile = (BluetoothHidDevice) proxy;
            sIsHidDeviceReady = true;
            if (proxy == null) {
                Log.e("proxy is still null");
            }
        }

        @Override
        public void onServiceDisconnected(int profile) {
            sIsHidDeviceReady = false;
        }
    }

    public Boolean hidDeviceConnect(BluetoothDevice device) {
        return sHidDeviceProfile != null && sHidDeviceProfile.connect(device);
    }

    public Boolean hidDeviceDisconnect(BluetoothDevice device) {
        return sHidDeviceProfile != null && sHidDeviceProfile.disconnect(device);
    }

    /**
     * Check whether the HID Device profile service is ready to use.
     * @return true if HID Device profile is ready to use; otherwise false
     */
    @Rpc(description = "Is HID Device profile ready.")
    public Boolean bluetoothHidDeviceIsReady() {
        Log.d("isReady");
        return sHidDeviceProfile != null && sIsHidDeviceReady;
    }

    /**
     * Connect to a Bluetooth HID input host.
     * @param device name or MAC address or the HID input host
     * @return true if successfully connected to the HID host; otherwise false
     * @throws Exception error from Bluetooth HidDevService
     */
    @Rpc(description = "Connect to an HID host.")
    public Boolean bluetoothHidDeviceConnect(
            @RpcParameter(name = "device",
                    description = "Name or MAC address of a bluetooth device.")
                    String device)
            throws Exception {
        if (sHidDeviceProfile == null) {
            return false;
        }
        BluetoothDevice mDevice =
                BluetoothFacade.getDevice(BluetoothFacade.DiscoveredDevices, device);
        Log.d("Connecting to device " + mDevice.getAliasName());
        return hidDeviceConnect(mDevice);
    }

    /**
     * Disconnect a Bluetooth HID input host.
     * @param device name or MAC address or the HID input host
     * @return true if successfully disconnected the HID host; otherwise false
     * @throws Exception error from Bluetooth HidDevService
     */
    @Rpc(description = "Disconnect an HID host.")
    public Boolean bluetoothHidDeviceDisconnect(
            @RpcParameter(name = "device",
                    description = "Name or MAC address of a device.")
                    String device)
            throws Exception {
        if (sHidDeviceProfile == null) {
            return false;
        }
        Log.d("Connected devices: " + sHidDeviceProfile.getConnectedDevices());
        BluetoothDevice mDevice = BluetoothFacade.getDevice(sHidDeviceProfile.getConnectedDevices(),
                device);
        return hidDeviceDisconnect(mDevice);
    }

    /**
     * Get all the devices connected through HID Device Service.
     * @return a list of all the devices connected through HID Device Service,
     * or null if the HID device profile is not ready.
     */
    @Rpc(description = "Get all the devices connected through HID Device Service.")
    public List<BluetoothDevice> bluetoothHidDeviceGetConnectedDevices() {
        if (sHidDeviceProfile == null) {
            return null;
        }
        return sHidDeviceProfile.getConnectedDevices();
    }

    /**
     * Get the connection status of the specified device
     * @param deviceID name or MAC address or the HID input host
     * @return the status of the device
     */
    @Rpc(description = "Get the connection status of a device.")
    public Integer bluetoothHidDeviceGetConnectionStatus(
            @RpcParameter(name = "deviceID",
                    description = "Name or MAC address of a bluetooth device.")
                    String deviceID) {
        if (sHidDeviceProfile == null) {
            return BluetoothProfile.STATE_DISCONNECTED;
        }
        List<BluetoothDevice> deviceList = sHidDeviceProfile.getConnectedDevices();
        BluetoothDevice device;
        try {
            device = BluetoothFacade.getDevice(deviceList, deviceID);
        } catch (Exception e) {
            return BluetoothProfile.STATE_DISCONNECTED;
        }
        return sHidDeviceProfile.getConnectionState(device);
    }

    /**
     * Register app for the HID Device service using default settings. This adds a SDP record.
     * @return true if successfully registered the app; otherwise false
     * @throws Exception error from Bluetooth HidDevService
     */
    @Rpc(description = "Register app for the HID Device service using default settings.")
    public Boolean bluetoothHidDeviceRegisterApp() throws Exception {
        return sHidDeviceProfile != null
                && sHidDeviceProfile.registerApp(
                        sSdpSettings, null, sQos, command -> command.run(), mCallback);
    }

    /**
     * Unregister app for the HID Device service.
     *
     * @return true if successfully unregistered the app; otherwise false
     * @throws Exception error from Bluetooth HidDevService
     */
    @Rpc(description = "Unregister app.")
    public Boolean bluetoothHidDeviceUnregisterApp() throws Exception {
        return sHidDeviceProfile != null && sHidDeviceProfile.unregisterApp();
    }

    /**
     * Send a data report to a connected HID host using interrupt channel.
     * @param deviceID name or MAC address or the HID input host
     * @param id report Id, as defined in descriptor. Can be 0 in case Report Id are not defined in
     * descriptor.
     * @param report report data
     * @return true if successfully sent the report; otherwise false
     * @throws Exception error from Bluetooth HidDevService
     */
    @Rpc(description = "Send report to a connected HID host using interrupt channel.")
    public Boolean bluetoothHidDeviceSendReport(
            @RpcParameter(name = "deviceID",
                    description = "Name or MAC address of a bluetooth device.")
                    String deviceID,
            @RpcParameter(name = "descriptor",
                    description = "Descriptor of the report")
                    Integer id,
            @RpcParameter(name = "report")
                    String report) throws Exception {
        if (sHidDeviceProfile == null) {
            return false;
        }

        BluetoothDevice device = BluetoothFacade.getDevice(sHidDeviceProfile.getConnectedDevices(),
                deviceID);
        byte[] reportByteArray = report.getBytes();
        return sHidDeviceProfile.sendReport(device, id, reportByteArray);
    }

    /**
     * Send a report to the connected HID host as reply for GET_REPORT request from the HID host.
     * @param deviceID name or MAC address or the HID input host
     * @param type type of the report, as in request
     * @param id id of the report, as in request
     * @param report report data
     * @return true if successfully sent the reply report; otherwise false
     * @throws Exception error from Bluetooth HidDevService
     */
    @Rpc(description = "Send reply report to a connected HID..")
    public Boolean bluetoothHidDeviceReplyReport(
            @RpcParameter(name = "deviceID",
                    description = "Name or MAC address of a bluetooth device.")
                    String deviceID,
            @RpcParameter(name = "type",
                    description = "Type as in the report.")
                    Integer type,
            @RpcParameter(name = "id",
                    description = "id as in the report.")
                    Integer id,
            @RpcParameter(name = "report")
                    String report) throws Exception {
        if (sHidDeviceProfile == null) {
            return false;
        }

        BluetoothDevice device = BluetoothFacade.getDevice(sHidDeviceProfile.getConnectedDevices(),
                deviceID);
        byte[] reportByteArray = report.getBytes();
        return sHidDeviceProfile.replyReport(
                device, (byte) (int) type, (byte) (int) id, reportByteArray);
    }

    /**
     * Send error handshake message as reply for invalid SET_REPORT request from the HID host.
     * @param deviceID name or MAC address or the HID input host
     * @param error error byte
     * @return true if successfully sent the error handshake message; otherwise false
     * @throws Exception error from Bluetooth HidDevService
     */
    @Rpc(description = "Send error handshake message to a connected HID host.")
    public Boolean bluetoothHidDeviceReportError(
            @RpcParameter(name = "deviceID",
                    description = "Name or MAC address of a bluetooth device.")
                    String deviceID,
            @RpcParameter(name = "error",
                    description = "Error byte")
                    Integer error) throws Exception {
        if (sHidDeviceProfile == null) {
            return false;
        }

        BluetoothDevice device = BluetoothFacade.getDevice(sHidDeviceProfile.getConnectedDevices(),
                deviceID);
        return sHidDeviceProfile.reportError(device, (byte) (int) error);
    }

    @Override
    public void shutdown() {
    }

}
