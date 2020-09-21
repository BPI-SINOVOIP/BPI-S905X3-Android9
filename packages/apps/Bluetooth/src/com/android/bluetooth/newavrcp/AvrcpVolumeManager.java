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

package com.android.bluetooth.avrcp;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.SharedPreferences;
import android.media.AudioDeviceCallback;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

class AvrcpVolumeManager extends AudioDeviceCallback {
    public static final String TAG = "NewAvrcpVolumeManager";
    public static final boolean DEBUG = true;

    // All volumes are stored at system volume values, not AVRCP values
    public static final String VOLUME_MAP = "bluetooth_volume_map";
    public static final String VOLUME_BLACKLIST = "absolute_volume_blacklist";
    public static final int AVRCP_MAX_VOL = 127;
    public static int sDeviceMaxVolume = 0;
    public static final int STREAM_MUSIC = AudioManager.STREAM_MUSIC;

    Context mContext;
    AudioManager mAudioManager;
    AvrcpNativeInterface mNativeInterface;

    HashMap<BluetoothDevice, Boolean> mDeviceMap = new HashMap();
    HashMap<BluetoothDevice, Integer> mVolumeMap = new HashMap();
    BluetoothDevice mCurrentDevice = null;
    boolean mAbsoluteVolumeSupported = false;

    static int avrcpToSystemVolume(int avrcpVolume) {
        return (int) Math.floor((double) avrcpVolume * sDeviceMaxVolume / AVRCP_MAX_VOL);
    }

    static int systemToAvrcpVolume(int deviceVolume) {
        int avrcpVolume = (int) Math.floor((double) deviceVolume
                * AVRCP_MAX_VOL / sDeviceMaxVolume);
        if (avrcpVolume > 127) avrcpVolume = 127;
        return avrcpVolume;
    }

    private SharedPreferences getVolumeMap() {
        return mContext.getSharedPreferences(VOLUME_MAP, Context.MODE_PRIVATE);
    }

    private void switchVolumeDevice(@NonNull BluetoothDevice device) {
        // Inform the audio manager that the device has changed
        d("switchVolumeDevice: Set Absolute volume support to " + mDeviceMap.get(device));
        mAudioManager.avrcpSupportsAbsoluteVolume(device.getAddress(), mDeviceMap.get(device));

        // Get the current system volume and try to get the preference volume
        int currVolume = mAudioManager.getStreamVolume(STREAM_MUSIC);
        int savedVolume = getVolume(device, currVolume);

        d("switchVolumeDevice: currVolume=" + currVolume + " savedVolume=" + savedVolume);

        // If absolute volume for the device is supported, set the volume for the device
        if (mDeviceMap.get(device)) {
            int avrcpVolume = systemToAvrcpVolume(savedVolume);
            Log.i(TAG, "switchVolumeDevice: Updating device volume: avrcpVolume=" + avrcpVolume);
            mNativeInterface.sendVolumeChanged(avrcpVolume);
        }
    }

    AvrcpVolumeManager(Context context, AudioManager audioManager,
            AvrcpNativeInterface nativeInterface) {
        mContext = context;
        mAudioManager = audioManager;
        mNativeInterface = nativeInterface;
        sDeviceMaxVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);

        mAudioManager.registerAudioDeviceCallback(this, null);

        // Load the stored volume preferences into a hash map since shared preferences are slow
        // to poll and update. If the device has been unbonded since last start remove it from
        // the map.
        Map<String, ?> allKeys = getVolumeMap().getAll();
        SharedPreferences.Editor volumeMapEditor = getVolumeMap().edit();
        for (Map.Entry<String, ?> entry : allKeys.entrySet()) {
            String key = entry.getKey();
            Object value = entry.getValue();
            BluetoothDevice d = BluetoothAdapter.getDefaultAdapter().getRemoteDevice(key);

            if (value instanceof Integer && d.getBondState() == BluetoothDevice.BOND_BONDED) {
                mVolumeMap.put(d, (Integer) value);
            } else {
                d("Removing " + key + " from the volume map");
                volumeMapEditor.remove(key);
            }
        }
        volumeMapEditor.apply();
    }

    void storeVolumeForDevice(BluetoothDevice device) {
        SharedPreferences.Editor pref = getVolumeMap().edit();
        int storeVolume =  mAudioManager.getStreamVolume(STREAM_MUSIC);
        Log.i(TAG, "storeVolume: Storing stream volume level for device " + device
                + " : " + storeVolume);
        mVolumeMap.put(device, storeVolume);
        pref.putInt(device.getAddress(), storeVolume);

        // Always use apply() since it is asynchronous, otherwise the call can hang waiting for
        // storage to be written.
        pref.apply();
    }

    int getVolume(@NonNull BluetoothDevice device, int defaultValue) {
        if (!mVolumeMap.containsKey(device)) {
            Log.w(TAG, "getVolume: Couldn't find volume preference for device: " + device);
            return defaultValue;
        }

        d("getVolume: Returning volume " + mVolumeMap.get(device));
        return mVolumeMap.get(device);
    }

    @Override
    public synchronized void onAudioDevicesAdded(AudioDeviceInfo[] addedDevices) {
        if (mCurrentDevice == null) {
            d("onAudioDevicesAdded: Not expecting device changed");
            return;
        }

        boolean foundDevice = false;
        d("onAudioDevicesAdded: size: " + addedDevices.length);
        for (int i = 0; i < addedDevices.length; i++) {
            d("onAudioDevicesAdded: address=" + addedDevices[i].getAddress());
            if (addedDevices[i].getType() == AudioDeviceInfo.TYPE_BLUETOOTH_A2DP
                    && Objects.equals(addedDevices[i].getAddress(), mCurrentDevice.getAddress())) {
                foundDevice = true;
                break;
            }
        }

        if (!foundDevice) {
            d("Didn't find deferred device in list: device=" + mCurrentDevice);
            return;
        }

        // A2DP can sometimes connect and set a device to active before AVRCP has determined if the
        // device supports absolute volume. Defer switching the device until AVRCP returns the
        // info.
        if (!mDeviceMap.containsKey(mCurrentDevice)) {
            Log.w(TAG, "volumeDeviceSwitched: Device isn't connected: " + mCurrentDevice);
            return;
        }

        switchVolumeDevice(mCurrentDevice);
    }

    synchronized void deviceConnected(@NonNull BluetoothDevice device, boolean absoluteVolume) {
        d("deviceConnected: device=" + device + " absoluteVolume=" + absoluteVolume);

        mDeviceMap.put(device, absoluteVolume);

        // AVRCP features lookup has completed after the device became active. Switch to the new
        // device now.
        if (device.equals(mCurrentDevice)) {
            switchVolumeDevice(device);
        }
    }

    synchronized void volumeDeviceSwitched(@Nullable BluetoothDevice device) {
        d("volumeDeviceSwitched: mCurrentDevice=" + mCurrentDevice + " device=" + device);

        if (Objects.equals(device, mCurrentDevice)) {
            return;
        }

        // Wait until AudioManager informs us that the new device is connected
        mCurrentDevice = device;
    }

    void deviceDisconnected(@NonNull BluetoothDevice device) {
        d("deviceDisconnected: device=" + device);
        mDeviceMap.remove(device);
    }

    public void dump(StringBuilder sb) {
        sb.append("AvrcpVolumeManager:\n");
        sb.append("  mCurrentDevice: " + mCurrentDevice + "\n");
        sb.append("  Current System Volume: " + mAudioManager.getStreamVolume(STREAM_MUSIC) + "\n");
        sb.append("  Device Volume Memory Map:\n");
        sb.append(String.format("    %-17s : %-14s : %3s : %s\n",
                "Device Address", "Device Name", "Vol", "AbsVol"));
        Map<String, ?> allKeys = getVolumeMap().getAll();
        for (Map.Entry<String, ?> entry : allKeys.entrySet()) {
            Object value = entry.getValue();
            BluetoothDevice d = BluetoothAdapter.getDefaultAdapter()
                    .getRemoteDevice(entry.getKey());

            String deviceName = d.getName();
            if (deviceName == null) {
                deviceName = "";
            } else if (deviceName.length() > 14) {
                deviceName = deviceName.substring(0, 11).concat("...");
            }

            String absoluteVolume = "NotConnected";
            if (mDeviceMap.containsKey(d)) {
                absoluteVolume = mDeviceMap.get(d).toString();
            }

            if (value instanceof Integer) {
                sb.append(String.format("    %-17s : %-14s : %3d : %s\n",
                        d.getAddress(), deviceName, (Integer) value, absoluteVolume));
            }
        }
    }

    static void d(String msg) {
        if (DEBUG) {
            Log.d(TAG, msg);
        }
    }
}
