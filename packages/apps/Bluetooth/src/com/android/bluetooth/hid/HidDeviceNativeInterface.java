/*
 * Copyright 2017 The Android Open Source Project
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

/*
 * Defines the native inteface that is used by HID Device service to
 * send or receive messages from the native stack. This file is registered
 * for the native methods in the corresponding JNI C++ file.
 */

package com.android.bluetooth.hid;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import com.android.bluetooth.Utils;
import com.android.internal.annotations.GuardedBy;

/**
 * HID Device Native Interface to/from JNI.
 */
public class HidDeviceNativeInterface {
    private static final String TAG = "HidDeviceNativeInterface";
    private BluetoothAdapter mAdapter;

    @GuardedBy("INSTANCE_LOCK")
    private static HidDeviceNativeInterface sInstance;
    private static final Object INSTANCE_LOCK = new Object();

    static {
        classInitNative();
    }

    @VisibleForTesting
    private HidDeviceNativeInterface() {
        mAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mAdapter == null) {
            Log.wtfStack(TAG, "No Bluetooth Adapter Available");
        }
    }

    /**
     * Get the singleton instance.
     */
    public static HidDeviceNativeInterface getInstance() {
        synchronized (INSTANCE_LOCK) {
            if (sInstance == null) {
                setInstance(new HidDeviceNativeInterface());
            }
            return sInstance;
        }
    }

    /**
     * Set the singleton instance.
     *
     * @param nativeInterface native interface
     */
    private static void setInstance(HidDeviceNativeInterface nativeInterface) {
        sInstance = nativeInterface;
    }

    /**
     * Initializes the native interface.
     */
    public void init() {
        initNative();
    }

    /**
     * Cleanup the native interface.
     */
    public void cleanup() {
        cleanupNative();
    }

    /**
     * Registers the application
     *
     * @param name name of the HID Device application
     * @param description description of the HID Device application
     * @param provider provider of the HID Device application
     * @param subclass subclass of the HID Device application
     * @param descriptors HID descriptors
     * @param inQos incoming QoS settings
     * @param outQos outgoing QoS settings
     * @return the result of the native call
     */
    public boolean registerApp(String name, String description, String provider,
            byte subclass, byte[] descriptors, int[] inQos, int[] outQos) {
        return registerAppNative(name, description, provider, subclass, descriptors, inQos, outQos);
    }

    /**
     * Unregisters the application
     *
     * @return the result of the native call
     */
    public boolean unregisterApp() {
        return unregisterAppNative();
    }

    /**
     * Send report to the remote host
     *
     * @param id report ID
     * @param data report data array
     * @return the result of the native call
     */
    public boolean sendReport(int id, byte[] data) {
        return sendReportNative(id, data);
    }

    /**
     * Reply report to the remote host
     *
     * @param type report type
     * @param id report ID
     * @param data report data array
     * @return the result of the native call
     */
    public boolean replyReport(byte type, byte id, byte[] data) {
        return replyReportNative(type, id, data);
    }

    /**
     * Send virtual unplug to the remote host
     *
     * @return the result of the native call
     */
    public boolean unplug() {
        return unplugNative();
    }

    /**
     * Connect to the remote host
     *
     * @param device remote host device
     * @return the result of the native call
     */
    public boolean connect(BluetoothDevice device) {
        return connectNative(getByteAddress(device));
    }

    /**
     * Disconnect from the remote host
     *
     * @return the result of the native call
     */
    public boolean disconnect() {
        return disconnectNative();
    }

    /**
     * Report error to the remote host
     *
     * @param error error byte
     * @return the result of the native call
     */
    public boolean reportError(byte error) {
        return reportErrorNative(error);
    }

    private synchronized void onApplicationStateChanged(byte[] address, boolean registered) {
        HidDeviceService service = HidDeviceService.getHidDeviceService();
        if (service != null) {
            service.onApplicationStateChangedFromNative(getDevice(address), registered);
        } else {
            Log.wtfStack(TAG, "FATAL: onApplicationStateChanged() "
                    + "is called from the stack while service is not available.");
        }
    }

    private synchronized void onConnectStateChanged(byte[] address, int state) {
        HidDeviceService service = HidDeviceService.getHidDeviceService();
        if (service != null) {
            service.onConnectStateChangedFromNative(getDevice(address), state);
        } else {
            Log.wtfStack(TAG, "FATAL: onConnectStateChanged() "
                    + "is called from the stack while service is not available.");
        }
    }

    private synchronized void onGetReport(byte type, byte id, short bufferSize) {
        HidDeviceService service = HidDeviceService.getHidDeviceService();
        if (service != null) {
            service.onGetReportFromNative(type, id, bufferSize);
        } else {
            Log.wtfStack(TAG, "FATAL: onGetReport() "
                    + "is called from the stack while service is not available.");
        }
    }

    private synchronized void onSetReport(byte reportType, byte reportId, byte[] data) {
        HidDeviceService service = HidDeviceService.getHidDeviceService();
        if (service != null) {
            service.onSetReportFromNative(reportType, reportId, data);
        } else {
            Log.wtfStack(TAG, "FATAL: onSetReport() "
                    + "is called from the stack while service is not available.");
        }
    }

    private synchronized void onSetProtocol(byte protocol) {
        HidDeviceService service = HidDeviceService.getHidDeviceService();
        if (service != null) {
            service.onSetProtocolFromNative(protocol);
        } else {
            Log.wtfStack(TAG, "FATAL: onSetProtocol() "
                    + "is called from the stack while service is not available.");
        }
    }

    private synchronized void onInterruptData(byte reportId, byte[] data) {
        HidDeviceService service = HidDeviceService.getHidDeviceService();
        if (service != null) {
            service.onInterruptDataFromNative(reportId, data);
        } else {
            Log.wtfStack(TAG, "FATAL: onInterruptData() "
                    + "is called from the stack while service is not available.");
        }
    }

    private synchronized void onVirtualCableUnplug() {
        HidDeviceService service = HidDeviceService.getHidDeviceService();
        if (service != null) {
            service.onVirtualCableUnplugFromNative();
        } else {
            Log.wtfStack(TAG, "FATAL: onVirtualCableUnplug() "
                    + "is called from the stack while service is not available.");
        }
    }

    private BluetoothDevice getDevice(byte[] address) {
        if (address == null) {
            return null;
        }
        return mAdapter.getRemoteDevice(address);
    }

    private byte[] getByteAddress(BluetoothDevice device) {
        return Utils.getBytesFromAddress(device.getAddress());
    }

    private static native void classInitNative();

    private native void initNative();

    private native void cleanupNative();

    private native boolean registerAppNative(String name, String description, String provider,
            byte subclass, byte[] descriptors, int[] inQos, int[] outQos);

    private native boolean unregisterAppNative();

    private native boolean sendReportNative(int id, byte[] data);

    private native boolean replyReportNative(byte type, byte id, byte[] data);

    private native boolean unplugNative();

    private native boolean connectNative(byte[] btAddress);

    private native boolean disconnectNative();

    private native boolean reportErrorNative(byte error);
}
