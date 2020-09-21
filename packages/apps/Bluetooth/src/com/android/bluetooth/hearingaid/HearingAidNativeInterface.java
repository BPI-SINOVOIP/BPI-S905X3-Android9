/*
 * Copyright 2018 The Android Open Source Project
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
 * Defines the native interface that is used by state machine/service to
 * send or receive messages from the native stack. This file is registered
 * for the native methods in the corresponding JNI C++ file.
 */
package com.android.bluetooth.hearingaid;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import com.android.bluetooth.Utils;
import com.android.internal.annotations.GuardedBy;

/**
 * HearingAid Native Interface to/from JNI.
 */
public class HearingAidNativeInterface {
    private static final String TAG = "HearingAidNativeInterface";
    private static final boolean DBG = true;
    private BluetoothAdapter mAdapter;

    @GuardedBy("INSTANCE_LOCK")
    private static HearingAidNativeInterface sInstance;
    private static final Object INSTANCE_LOCK = new Object();

    static {
        classInitNative();
    }

    private HearingAidNativeInterface() {
        mAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mAdapter == null) {
            Log.wtfStack(TAG, "No Bluetooth Adapter Available");
        }
    }

    /**
     * Get singleton instance.
     */
    public static HearingAidNativeInterface getInstance() {
        synchronized (INSTANCE_LOCK) {
            if (sInstance == null) {
                sInstance = new HearingAidNativeInterface();
            }
            return sInstance;
        }
    }

    /**
     * Initializes the native interface.
     *
     * priorities to configure.
     */
    @VisibleForTesting (otherwise = VisibleForTesting.PACKAGE_PRIVATE)
    public void init() {
        initNative();
    }

    /**
     * Cleanup the native interface.
     */
    @VisibleForTesting (otherwise = VisibleForTesting.PACKAGE_PRIVATE)
    public void cleanup() {
        cleanupNative();
    }

    /**
     * Initiates HearingAid connection to a remote device.
     *
     * @param device the remote device
     * @return true on success, otherwise false.
     */
    @VisibleForTesting (otherwise = VisibleForTesting.PACKAGE_PRIVATE)
    public boolean connectHearingAid(BluetoothDevice device) {
        return connectHearingAidNative(getByteAddress(device));
    }

    /**
     * Disconnects HearingAid from a remote device.
     *
     * @param device the remote device
     * @return true on success, otherwise false.
     */
    @VisibleForTesting (otherwise = VisibleForTesting.PACKAGE_PRIVATE)
    public boolean disconnectHearingAid(BluetoothDevice device) {
        return disconnectHearingAidNative(getByteAddress(device));
    }

    /**
     * Sets the HearingAid volume
     * @param volume
     */
    @VisibleForTesting (otherwise = VisibleForTesting.PACKAGE_PRIVATE)
    public void setVolume(int volume) {
        setVolumeNative(volume);
    }

    private BluetoothDevice getDevice(byte[] address) {
        return mAdapter.getRemoteDevice(address);
    }

    private byte[] getByteAddress(BluetoothDevice device) {
        if (device == null) {
            return Utils.getBytesFromAddress("00:00:00:00:00:00");
        }
        return Utils.getBytesFromAddress(device.getAddress());
    }

    private void sendMessageToService(HearingAidStackEvent event) {
        HearingAidService service = HearingAidService.getHearingAidService();
        if (service != null) {
            service.messageFromNative(event);
        } else {
            Log.e(TAG, "Event ignored, service not available: " + event);
        }
    }

    // Callbacks from the native stack back into the Java framework.
    // All callbacks are routed via the Service which will disambiguate which
    // state machine the message should be routed to.

    private void onConnectionStateChanged(int state, byte[] address) {
        HearingAidStackEvent event =
                new HearingAidStackEvent(HearingAidStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED);
        event.device = getDevice(address);
        event.valueInt1 = state;

        if (DBG) {
            Log.d(TAG, "onConnectionStateChanged: " + event);
        }
        sendMessageToService(event);
    }

    private void onDeviceAvailable(byte capabilities, long hiSyncId, byte[] address) {
        HearingAidStackEvent event = new HearingAidStackEvent(
                HearingAidStackEvent.EVENT_TYPE_DEVICE_AVAILABLE);
        event.device = getDevice(address);
        event.valueInt1 = capabilities;
        event.valueLong2 = hiSyncId;

        if (DBG) {
            Log.d(TAG, "onDeviceAvailable: " + event);
        }
        sendMessageToService(event);
    }

    // Native methods that call into the JNI interface
    private static native void classInitNative();
    private native void initNative();
    private native void cleanupNative();
    private native boolean connectHearingAidNative(byte[] address);
    private native boolean disconnectHearingAidNative(byte[] address);
    private native void setVolumeNative(int volume);
}
