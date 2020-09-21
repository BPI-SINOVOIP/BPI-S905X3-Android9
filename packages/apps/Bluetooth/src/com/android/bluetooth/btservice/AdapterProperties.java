/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.bluetooth.btservice;

import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothA2dpSink;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothAvrcpController;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothHeadsetClient;
import android.bluetooth.BluetoothHidDevice;
import android.bluetooth.BluetoothHidHost;
import android.bluetooth.BluetoothMap;
import android.bluetooth.BluetoothMapClient;
import android.bluetooth.BluetoothPan;
import android.bluetooth.BluetoothPbap;
import android.bluetooth.BluetoothPbapClient;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothSap;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.ParcelUuid;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.provider.Settings.Secure;
import android.util.Log;
import android.util.Pair;
import android.util.StatsLog;

import com.android.bluetooth.Utils;
import com.android.bluetooth.btservice.RemoteDevices.DeviceProperties;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.concurrent.CopyOnWriteArrayList;

class AdapterProperties {
    private static final boolean DBG = true;
    private static final boolean VDBG = false;
    private static final String TAG = "AdapterProperties";

    private static final String MAX_CONNECTED_AUDIO_DEVICES_PROPERTY =
            "persist.bluetooth.maxconnectedaudiodevices";
    static final int MAX_CONNECTED_AUDIO_DEVICES_LOWER_BOND = 1;
    private static final int MAX_CONNECTED_AUDIO_DEVICES_UPPER_BOUND = 5;
    private static final String A2DP_OFFLOAD_SUPPORTED_PROPERTY =
            "ro.bluetooth.a2dp_offload.supported";
    private static final String A2DP_OFFLOAD_DISABLED_PROPERTY =
            "persist.bluetooth.a2dp_offload.disabled";

    private static final long DEFAULT_DISCOVERY_TIMEOUT_MS = 12800;
    private static final int BD_ADDR_LEN = 6; // in bytes

    private volatile String mName;
    private volatile byte[] mAddress;
    private volatile BluetoothClass mBluetoothClass;
    private volatile int mScanMode;
    private volatile int mDiscoverableTimeout;
    private volatile ParcelUuid[] mUuids;
    private CopyOnWriteArrayList<BluetoothDevice> mBondedDevices =
            new CopyOnWriteArrayList<BluetoothDevice>();

    private int mProfilesConnecting, mProfilesConnected, mProfilesDisconnecting;
    private final HashMap<Integer, Pair<Integer, Integer>> mProfileConnectionState =
            new HashMap<>();

    private volatile int mConnectionState = BluetoothAdapter.STATE_DISCONNECTED;
    private volatile int mState = BluetoothAdapter.STATE_OFF;
    private int mMaxConnectedAudioDevices = 1;
    private boolean mA2dpOffloadEnabled = false;

    private AdapterService mService;
    private boolean mDiscovering;
    private long mDiscoveryEndMs; //< Time (ms since epoch) that discovery ended or will end.
    private RemoteDevices mRemoteDevices;
    private BluetoothAdapter mAdapter;
    //TODO - all hw capabilities to be exposed as a class
    private int mNumOfAdvertisementInstancesSupported;
    private boolean mRpaOffloadSupported;
    private int mNumOfOffloadedIrkSupported;
    private int mNumOfOffloadedScanFilterSupported;
    private int mOffloadedScanResultStorageBytes;
    private int mVersSupported;
    private int mTotNumOfTrackableAdv;
    private boolean mIsExtendedScanSupported;
    private boolean mIsDebugLogSupported;
    private boolean mIsActivityAndEnergyReporting;
    private boolean mIsLe2MPhySupported;
    private boolean mIsLeCodedPhySupported;
    private boolean mIsLeExtendedAdvertisingSupported;
    private boolean mIsLePeriodicAdvertisingSupported;
    private int mLeMaximumAdvertisingDataLength;

    private boolean mReceiverRegistered;
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action == null) {
                Log.w(TAG, "Received intent with null action");
                return;
            }
            switch (action) {
                case BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.HEADSET, intent);
                    break;
                case BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.A2DP, intent);
                    break;
                case BluetoothHeadsetClient.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.HEADSET_CLIENT, intent);
                    break;
                case BluetoothA2dpSink.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.A2DP_SINK, intent);
                    break;
                case BluetoothHidDevice.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.HID_DEVICE, intent);
                    break;
                case BluetoothHidHost.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.HID_HOST, intent);
                    break;
                case BluetoothAvrcpController.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.AVRCP_CONTROLLER, intent);
                    break;
                case BluetoothPan.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.PAN, intent);
                    break;
                case BluetoothMap.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.MAP, intent);
                    break;
                case BluetoothMapClient.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.MAP_CLIENT, intent);
                    break;
                case BluetoothSap.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.SAP, intent);
                    break;
                case BluetoothPbapClient.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.PBAP_CLIENT, intent);
                    break;
                case BluetoothPbap.ACTION_CONNECTION_STATE_CHANGED:
                    sendConnectionStateChange(BluetoothProfile.PBAP, intent);
                    break;
                default:
                    Log.w(TAG, "Received unknown intent " + intent);
                    break;
            }
        }
    };

    // Lock for all getters and setters.
    // If finer grained locking is needer, more locks
    // can be added here.
    private final Object mObject = new Object();

    AdapterProperties(AdapterService service) {
        mService = service;
        mAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    public void init(RemoteDevices remoteDevices) {
        mProfileConnectionState.clear();
        mRemoteDevices = remoteDevices;

        // Get default max connected audio devices from config.xml in frameworks/base/core
        int configDefaultMaxConnectedAudioDevices = mService.getResources().getInteger(
                com.android.internal.R.integer.config_bluetooth_max_connected_audio_devices);
        // Override max connected audio devices if MAX_CONNECTED_AUDIO_DEVICES_PROPERTY is set
        int propertyOverlayedMaxConnectedAudioDevices =
                SystemProperties.getInt(MAX_CONNECTED_AUDIO_DEVICES_PROPERTY,
                        configDefaultMaxConnectedAudioDevices);
        // Make sure the final value of max connected audio devices is within allowed range
        mMaxConnectedAudioDevices = Math.min(Math.max(propertyOverlayedMaxConnectedAudioDevices,
                MAX_CONNECTED_AUDIO_DEVICES_LOWER_BOND), MAX_CONNECTED_AUDIO_DEVICES_UPPER_BOUND);
        Log.i(TAG, "init(), maxConnectedAudioDevices, default="
                + configDefaultMaxConnectedAudioDevices + ", propertyOverlayed="
                + propertyOverlayedMaxConnectedAudioDevices + ", finalValue="
                + mMaxConnectedAudioDevices);

        mA2dpOffloadEnabled =
                SystemProperties.getBoolean(A2DP_OFFLOAD_SUPPORTED_PROPERTY, false)
                && !SystemProperties.getBoolean(A2DP_OFFLOAD_DISABLED_PROPERTY, false);

        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothHeadsetClient.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothA2dpSink.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothHidDevice.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothHidHost.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothAvrcpController.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothPan.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothMap.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothMapClient.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothSap.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothPbapClient.ACTION_CONNECTION_STATE_CHANGED);
        mService.registerReceiver(mReceiver, filter);
        mReceiverRegistered = true;
    }

    public void cleanup() {
        mRemoteDevices = null;
        mProfileConnectionState.clear();
        if (mReceiverRegistered) {
            mService.unregisterReceiver(mReceiver);
            mReceiverRegistered = false;
        }
        mService = null;
        mBondedDevices.clear();
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        throw new CloneNotSupportedException();
    }

    /**
     * @return the mName
     */
    String getName() {
        return mName;
    }

    /**
     * Set the local adapter property - name
     * @param name the name to set
     */
    boolean setName(String name) {
        synchronized (mObject) {
            return mService.setAdapterPropertyNative(AbstractionLayer.BT_PROPERTY_BDNAME,
                    name.getBytes());
        }
    }

    /**
     * Set the Bluetooth Class of Device (CoD) of the adapter.
     *
     * <p>Bluetooth stack stores some adapter properties in native BT stack storage and some in the
     * Java Android stack. Bluetooth CoD is stored in the Android layer through
     * {@link android.provider.Settings.Global#BLUETOOTH_CLASS_OF_DEVICE}.
     *
     * <p>Due to this, the getAdapterPropertyNative and adapterPropertyChangedCallback methods don't
     * actually update mBluetoothClass. Hence, we update the field mBluetoothClass every time we
     * successfully update BluetoothClass.
     *
     * @param bluetoothClass BluetoothClass of the device
     */
    boolean setBluetoothClass(BluetoothClass bluetoothClass) {
        synchronized (mObject) {
            boolean result =
                    mService.setAdapterPropertyNative(AbstractionLayer.BT_PROPERTY_CLASS_OF_DEVICE,
                            bluetoothClass.getClassOfDeviceBytes());

            if (result) {
                mBluetoothClass = bluetoothClass;
            }

            return result;
        }
    }

    /**
     * @return the BluetoothClass of the Bluetooth adapter.
     */
    BluetoothClass getBluetoothClass() {
        synchronized (mObject) {
            return mBluetoothClass;
        }
    }

    /**
     * @return the mScanMode
     */
    int getScanMode() {
        return mScanMode;
    }

    /**
     * Set the local adapter property - scanMode
     *
     * @param scanMode the ScanMode to set
     */
    boolean setScanMode(int scanMode) {
        synchronized (mObject) {
            return mService.setAdapterPropertyNative(AbstractionLayer.BT_PROPERTY_ADAPTER_SCAN_MODE,
                    Utils.intToByteArray(scanMode));
        }
    }

    /**
     * @return the mUuids
     */
    ParcelUuid[] getUuids() {
        return mUuids;
    }

    /**
     * @return the mAddress
     */
    byte[] getAddress() {
        return mAddress;
    }

    /**
     * @param connectionState the mConnectionState to set
     */
    void setConnectionState(int connectionState) {
        mConnectionState = connectionState;
    }

    /**
     * @return the mConnectionState
     */
    int getConnectionState() {
        return mConnectionState;
    }

    /**
     * @param mState the mState to set
     */
    void setState(int state) {
        debugLog("Setting state to " + BluetoothAdapter.nameForState(state));
        mState = state;
    }

    /**
     * @return the mState
     */
    int getState() {
        return mState;
    }

    /**
     * @return the mNumOfAdvertisementInstancesSupported
     */
    int getNumOfAdvertisementInstancesSupported() {
        return mNumOfAdvertisementInstancesSupported;
    }

    /**
     * @return the mRpaOffloadSupported
     */
    boolean isRpaOffloadSupported() {
        return mRpaOffloadSupported;
    }

    /**
     * @return the mNumOfOffloadedIrkSupported
     */
    int getNumOfOffloadedIrkSupported() {
        return mNumOfOffloadedIrkSupported;
    }

    /**
     * @return the mNumOfOffloadedScanFilterSupported
     */
    int getNumOfOffloadedScanFilterSupported() {
        return mNumOfOffloadedScanFilterSupported;
    }

    /**
     * @return the mOffloadedScanResultStorageBytes
     */
    int getOffloadedScanResultStorage() {
        return mOffloadedScanResultStorageBytes;
    }

    /**
     * @return tx/rx/idle activity and energy info
     */
    boolean isActivityAndEnergyReportingSupported() {
        return mIsActivityAndEnergyReporting;
    }

    /**
     * @return the mIsLe2MPhySupported
     */
    boolean isLe2MPhySupported() {
        return mIsLe2MPhySupported;
    }

    /**
     * @return the mIsLeCodedPhySupported
     */
    boolean isLeCodedPhySupported() {
        return mIsLeCodedPhySupported;
    }

    /**
     * @return the mIsLeExtendedAdvertisingSupported
     */
    boolean isLeExtendedAdvertisingSupported() {
        return mIsLeExtendedAdvertisingSupported;
    }

    /**
     * @return the mIsLePeriodicAdvertisingSupported
     */
    boolean isLePeriodicAdvertisingSupported() {
        return mIsLePeriodicAdvertisingSupported;
    }

    /**
     * @return the getLeMaximumAdvertisingDataLength
     */
    int getLeMaximumAdvertisingDataLength() {
        return mLeMaximumAdvertisingDataLength;
    }

    /**
     * @return total number of trackable advertisements
     */
    int getTotalNumOfTrackableAdvertisements() {
        return mTotNumOfTrackableAdv;
    }

    /**
     * @return the maximum number of connected audio devices
     */
    int getMaxConnectedAudioDevices() {
        return mMaxConnectedAudioDevices;
    }

    /**
     * @return A2DP offload support
     */
    boolean isA2dpOffloadEnabled() {
        return mA2dpOffloadEnabled;
    }

    /**
     * @return the mBondedDevices
     */
    BluetoothDevice[] getBondedDevices() {
        BluetoothDevice[] bondedDeviceList = new BluetoothDevice[0];
        try {
            bondedDeviceList = mBondedDevices.toArray(bondedDeviceList);
        } catch (ArrayStoreException ee) {
            errorLog("Error retrieving bonded device array");
        }
        infoLog("getBondedDevices: length=" + bondedDeviceList.length);
        return bondedDeviceList;
    }

    // This function shall be invoked from BondStateMachine whenever the bond
    // state changes.
    void onBondStateChanged(BluetoothDevice device, int state) {
        if (device == null) {
            Log.w(TAG, "onBondStateChanged, device is null");
            return;
        }
        try {
            byte[] addrByte = Utils.getByteAddress(device);
            DeviceProperties prop = mRemoteDevices.getDeviceProperties(device);
            if (prop == null) {
                prop = mRemoteDevices.addDeviceProperties(addrByte);
            }
            prop.setBondState(state);

            if (state == BluetoothDevice.BOND_BONDED) {
                // add if not already in list
                if (!mBondedDevices.contains(device)) {
                    debugLog("Adding bonded device:" + device);
                    mBondedDevices.add(device);
                }
            } else if (state == BluetoothDevice.BOND_NONE) {
                // remove device from list
                if (mBondedDevices.remove(device)) {
                    debugLog("Removing bonded device:" + device);
                } else {
                    debugLog("Failed to remove device: " + device);
                }
            }
        } catch (Exception ee) {
            Log.w(TAG, "onBondStateChanged: Exception ", ee);
        }
    }

    int getDiscoverableTimeout() {
        return mDiscoverableTimeout;
    }

    boolean setDiscoverableTimeout(int timeout) {
        synchronized (mObject) {
            return mService.setAdapterPropertyNative(
                    AbstractionLayer.BT_PROPERTY_ADAPTER_DISCOVERABLE_TIMEOUT,
                    Utils.intToByteArray(timeout));
        }
    }

    int getProfileConnectionState(int profile) {
        synchronized (mObject) {
            Pair<Integer, Integer> p = mProfileConnectionState.get(profile);
            if (p != null) {
                return p.first;
            }
            return BluetoothProfile.STATE_DISCONNECTED;
        }
    }

    long discoveryEndMillis() {
        return mDiscoveryEndMs;
    }

    boolean isDiscovering() {
        return mDiscovering;
    }

    private void sendConnectionStateChange(int profile, Intent connIntent) {
        BluetoothDevice device = connIntent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        int prevState = connIntent.getIntExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, -1);
        int state = connIntent.getIntExtra(BluetoothProfile.EXTRA_STATE, -1);
        Log.d(TAG,
                "PROFILE_CONNECTION_STATE_CHANGE: profile=" + profile + ", device=" + device + ", "
                        + prevState + " -> " + state);
        String ssaid = Secure.getString(mService.getContentResolver(), Secure.ANDROID_ID);
        String combined = ssaid + device.getAddress();
        int obfuscated_id = combined.hashCode() & 0xFFFF; // Last two bytes only
        StatsLog.write(StatsLog.BLUETOOTH_CONNECTION_STATE_CHANGED,
                state, obfuscated_id, profile);
        if (!isNormalStateTransition(prevState, state)) {
            Log.w(TAG,
                    "PROFILE_CONNECTION_STATE_CHANGE: unexpected transition for profile=" + profile
                            + ", device=" + device + ", " + prevState + " -> " + state);
        }
        sendConnectionStateChange(device, profile, state, prevState);
    }

    void sendConnectionStateChange(BluetoothDevice device, int profile, int state, int prevState) {
        if (!validateProfileConnectionState(state) || !validateProfileConnectionState(prevState)) {
            // Previously, an invalid state was broadcast anyway,
            // with the invalid state converted to -1 in the intent.
            // Better to log an error and not send an intent with
            // invalid contents or set mAdapterConnectionState to -1.
            errorLog("sendConnectionStateChange: invalid state transition " + prevState + " -> "
                    + state);
            return;
        }

        synchronized (mObject) {
            updateProfileConnectionState(profile, state, prevState);

            if (updateCountersAndCheckForConnectionStateChange(state, prevState)) {
                int newAdapterState = convertToAdapterState(state);
                int prevAdapterState = convertToAdapterState(prevState);
                setConnectionState(newAdapterState);

                Intent intent = new Intent(BluetoothAdapter.ACTION_CONNECTION_STATE_CHANGED);
                intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
                intent.putExtra(BluetoothAdapter.EXTRA_CONNECTION_STATE, newAdapterState);
                intent.putExtra(BluetoothAdapter.EXTRA_PREVIOUS_CONNECTION_STATE, prevAdapterState);
                intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
                Log.d(TAG, "ADAPTER_CONNECTION_STATE_CHANGE: " + device + ": " + prevAdapterState
                        + " -> " + newAdapterState);
                if (!isNormalStateTransition(prevState, state)) {
                    Log.w(TAG, "ADAPTER_CONNECTION_STATE_CHANGE: unexpected transition for profile="
                            + profile + ", device=" + device + ", " + prevState + " -> " + state);
                }
                mService.sendBroadcastAsUser(intent, UserHandle.ALL, AdapterService.BLUETOOTH_PERM);
            }
        }
    }

    private boolean validateProfileConnectionState(int state) {
        return (state == BluetoothProfile.STATE_DISCONNECTED
                || state == BluetoothProfile.STATE_CONNECTING
                || state == BluetoothProfile.STATE_CONNECTED
                || state == BluetoothProfile.STATE_DISCONNECTING);
    }

    private static int convertToAdapterState(int state) {
        switch (state) {
            case BluetoothProfile.STATE_DISCONNECTED:
                return BluetoothAdapter.STATE_DISCONNECTED;
            case BluetoothProfile.STATE_DISCONNECTING:
                return BluetoothAdapter.STATE_DISCONNECTING;
            case BluetoothProfile.STATE_CONNECTED:
                return BluetoothAdapter.STATE_CONNECTED;
            case BluetoothProfile.STATE_CONNECTING:
                return BluetoothAdapter.STATE_CONNECTING;
        }
        Log.e(TAG, "convertToAdapterState, unknow state " + state);
        return -1;
    }

    private static boolean isNormalStateTransition(int prevState, int nextState) {
        switch (prevState) {
            case BluetoothProfile.STATE_DISCONNECTED:
                return nextState == BluetoothProfile.STATE_CONNECTING;
            case BluetoothProfile.STATE_CONNECTED:
                return nextState == BluetoothProfile.STATE_DISCONNECTING;
            case BluetoothProfile.STATE_DISCONNECTING:
            case BluetoothProfile.STATE_CONNECTING:
                return (nextState == BluetoothProfile.STATE_DISCONNECTED) || (nextState
                        == BluetoothProfile.STATE_CONNECTED);
            default:
                return false;
        }
    }

    private boolean updateCountersAndCheckForConnectionStateChange(int state, int prevState) {
        switch (prevState) {
            case BluetoothProfile.STATE_CONNECTING:
                if (mProfilesConnecting > 0) {
                    mProfilesConnecting--;
                } else {
                    Log.e(TAG, "mProfilesConnecting " + mProfilesConnecting);
                    throw new IllegalStateException(
                            "Invalid state transition, " + prevState + " -> " + state);
                }
                break;

            case BluetoothProfile.STATE_CONNECTED:
                if (mProfilesConnected > 0) {
                    mProfilesConnected--;
                } else {
                    Log.e(TAG, "mProfilesConnected " + mProfilesConnected);
                    throw new IllegalStateException(
                            "Invalid state transition, " + prevState + " -> " + state);
                }
                break;

            case BluetoothProfile.STATE_DISCONNECTING:
                if (mProfilesDisconnecting > 0) {
                    mProfilesDisconnecting--;
                } else {
                    Log.e(TAG, "mProfilesDisconnecting " + mProfilesDisconnecting);
                    throw new IllegalStateException(
                            "Invalid state transition, " + prevState + " -> " + state);
                }
                break;
        }

        switch (state) {
            case BluetoothProfile.STATE_CONNECTING:
                mProfilesConnecting++;
                return (mProfilesConnected == 0 && mProfilesConnecting == 1);

            case BluetoothProfile.STATE_CONNECTED:
                mProfilesConnected++;
                return (mProfilesConnected == 1);

            case BluetoothProfile.STATE_DISCONNECTING:
                mProfilesDisconnecting++;
                return (mProfilesConnected == 0 && mProfilesDisconnecting == 1);

            case BluetoothProfile.STATE_DISCONNECTED:
                return (mProfilesConnected == 0 && mProfilesConnecting == 0);

            default:
                return true;
        }
    }

    private void updateProfileConnectionState(int profile, int newState, int oldState) {
        // mProfileConnectionState is a hashmap -
        // <Integer, Pair<Integer, Integer>>
        // The key is the profile, the value is a pair. first element
        // is the state and the second element is the number of devices
        // in that state.
        int numDev = 1;
        int newHashState = newState;
        boolean update = true;

        // The following conditions are considered in this function:
        // 1. If there is no record of profile and state - update
        // 2. If a new device's state is current hash state - increment
        //    number of devices in the state.
        // 3. If a state change has happened to Connected or Connecting
        //    (if current state is not connected), update.
        // 4. If numDevices is 1 and that device state is being updated, update
        // 5. If numDevices is > 1 and one of the devices is changing state,
        //    decrement numDevices but maintain oldState if it is Connected or
        //    Connecting
        Pair<Integer, Integer> stateNumDev = mProfileConnectionState.get(profile);
        if (stateNumDev != null) {
            int currHashState = stateNumDev.first;
            numDev = stateNumDev.second;

            if (newState == currHashState) {
                numDev++;
            } else if (newState == BluetoothProfile.STATE_CONNECTED || (
                    newState == BluetoothProfile.STATE_CONNECTING
                            && currHashState != BluetoothProfile.STATE_CONNECTED)) {
                numDev = 1;
            } else if (numDev == 1 && oldState == currHashState) {
                update = true;
            } else if (numDev > 1 && oldState == currHashState) {
                numDev--;

                if (currHashState == BluetoothProfile.STATE_CONNECTED
                        || currHashState == BluetoothProfile.STATE_CONNECTING) {
                    newHashState = currHashState;
                }
            } else {
                update = false;
            }
        }

        if (update) {
            mProfileConnectionState.put(profile, new Pair<Integer, Integer>(newHashState, numDev));
        }
    }

    void adapterPropertyChangedCallback(int[] types, byte[][] values) {
        Intent intent;
        int type;
        byte[] val;
        for (int i = 0; i < types.length; i++) {
            val = values[i];
            type = types[i];
            infoLog("adapterPropertyChangedCallback with type:" + type + " len:" + val.length);
            synchronized (mObject) {
                switch (type) {
                    case AbstractionLayer.BT_PROPERTY_BDNAME:
                        mName = new String(val);
                        intent = new Intent(BluetoothAdapter.ACTION_LOCAL_NAME_CHANGED);
                        intent.putExtra(BluetoothAdapter.EXTRA_LOCAL_NAME, mName);
                        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
                        mService.sendBroadcastAsUser(intent, UserHandle.ALL,
                                AdapterService.BLUETOOTH_PERM);
                        debugLog("Name is: " + mName);
                        break;
                    case AbstractionLayer.BT_PROPERTY_BDADDR:
                        mAddress = val;
                        String address = Utils.getAddressStringFromByte(mAddress);
                        debugLog("Address is:" + address);
                        intent = new Intent(BluetoothAdapter.ACTION_BLUETOOTH_ADDRESS_CHANGED);
                        intent.putExtra(BluetoothAdapter.EXTRA_BLUETOOTH_ADDRESS, address);
                        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
                        mService.sendBroadcastAsUser(intent, UserHandle.ALL,
                                AdapterService.BLUETOOTH_PERM);
                        break;
                    case AbstractionLayer.BT_PROPERTY_CLASS_OF_DEVICE:
                        if (val == null || val.length != 3) {
                            debugLog("Invalid BT CoD value from stack.");
                            return;
                        }
                        int bluetoothClass =
                                ((int) val[0] << 16) + ((int) val[1] << 8) + (int) val[2];
                        if (bluetoothClass != 0) {
                            mBluetoothClass = new BluetoothClass(bluetoothClass);
                        }
                        debugLog("BT Class:" + mBluetoothClass);
                        break;
                    case AbstractionLayer.BT_PROPERTY_ADAPTER_SCAN_MODE:
                        int mode = Utils.byteArrayToInt(val, 0);
                        mScanMode = AdapterService.convertScanModeFromHal(mode);
                        intent = new Intent(BluetoothAdapter.ACTION_SCAN_MODE_CHANGED);
                        intent.putExtra(BluetoothAdapter.EXTRA_SCAN_MODE, mScanMode);
                        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
                        mService.sendBroadcast(intent, AdapterService.BLUETOOTH_PERM);
                        debugLog("Scan Mode:" + mScanMode);
                        break;
                    case AbstractionLayer.BT_PROPERTY_UUIDS:
                        mUuids = Utils.byteArrayToUuid(val);
                        break;
                    case AbstractionLayer.BT_PROPERTY_ADAPTER_BONDED_DEVICES:
                        int number = val.length / BD_ADDR_LEN;
                        byte[] addrByte = new byte[BD_ADDR_LEN];
                        for (int j = 0; j < number; j++) {
                            System.arraycopy(val, j * BD_ADDR_LEN, addrByte, 0, BD_ADDR_LEN);
                            onBondStateChanged(mAdapter.getRemoteDevice(
                                    Utils.getAddressStringFromByte(addrByte)),
                                    BluetoothDevice.BOND_BONDED);
                        }
                        break;
                    case AbstractionLayer.BT_PROPERTY_ADAPTER_DISCOVERABLE_TIMEOUT:
                        mDiscoverableTimeout = Utils.byteArrayToInt(val, 0);
                        debugLog("Discoverable Timeout:" + mDiscoverableTimeout);
                        break;

                    case AbstractionLayer.BT_PROPERTY_LOCAL_LE_FEATURES:
                        updateFeatureSupport(val);
                        break;

                    default:
                        errorLog("Property change not handled in Java land:" + type);
                }
            }
        }
    }

    private void updateFeatureSupport(byte[] val) {
        mVersSupported = ((0xFF & ((int) val[1])) << 8) + (0xFF & ((int) val[0]));
        mNumOfAdvertisementInstancesSupported = (0xFF & ((int) val[3]));
        mRpaOffloadSupported = ((0xFF & ((int) val[4])) != 0);
        mNumOfOffloadedIrkSupported = (0xFF & ((int) val[5]));
        mNumOfOffloadedScanFilterSupported = (0xFF & ((int) val[6]));
        mIsActivityAndEnergyReporting = ((0xFF & ((int) val[7])) != 0);
        mOffloadedScanResultStorageBytes = ((0xFF & ((int) val[9])) << 8) + (0xFF & ((int) val[8]));
        mTotNumOfTrackableAdv = ((0xFF & ((int) val[11])) << 8) + (0xFF & ((int) val[10]));
        mIsExtendedScanSupported = ((0xFF & ((int) val[12])) != 0);
        mIsDebugLogSupported = ((0xFF & ((int) val[13])) != 0);
        mIsLe2MPhySupported = ((0xFF & ((int) val[14])) != 0);
        mIsLeCodedPhySupported = ((0xFF & ((int) val[15])) != 0);
        mIsLeExtendedAdvertisingSupported = ((0xFF & ((int) val[16])) != 0);
        mIsLePeriodicAdvertisingSupported = ((0xFF & ((int) val[17])) != 0);
        mLeMaximumAdvertisingDataLength =
                (0xFF & ((int) val[18])) + ((0xFF & ((int) val[19])) << 8);

        Log.d(TAG, "BT_PROPERTY_LOCAL_LE_FEATURES: update from BT controller"
                + " mNumOfAdvertisementInstancesSupported = "
                + mNumOfAdvertisementInstancesSupported + " mRpaOffloadSupported = "
                + mRpaOffloadSupported + " mNumOfOffloadedIrkSupported = "
                + mNumOfOffloadedIrkSupported + " mNumOfOffloadedScanFilterSupported = "
                + mNumOfOffloadedScanFilterSupported + " mOffloadedScanResultStorageBytes= "
                + mOffloadedScanResultStorageBytes + " mIsActivityAndEnergyReporting = "
                + mIsActivityAndEnergyReporting + " mVersSupported = " + mVersSupported
                + " mTotNumOfTrackableAdv = " + mTotNumOfTrackableAdv
                + " mIsExtendedScanSupported = " + mIsExtendedScanSupported
                + " mIsDebugLogSupported = " + mIsDebugLogSupported + " mIsLe2MPhySupported = "
                + mIsLe2MPhySupported + " mIsLeCodedPhySupported = " + mIsLeCodedPhySupported
                + " mIsLeExtendedAdvertisingSupported = " + mIsLeExtendedAdvertisingSupported
                + " mIsLePeriodicAdvertisingSupported = " + mIsLePeriodicAdvertisingSupported
                + " mLeMaximumAdvertisingDataLength = " + mLeMaximumAdvertisingDataLength);
    }

    void onBluetoothReady() {
        debugLog("onBluetoothReady, state=" + BluetoothAdapter.nameForState(getState())
                + ", ScanMode=" + mScanMode);

        synchronized (mObject) {
            // Reset adapter and profile connection states
            setConnectionState(BluetoothAdapter.STATE_DISCONNECTED);
            mProfileConnectionState.clear();
            mProfilesConnected = 0;
            mProfilesConnecting = 0;
            mProfilesDisconnecting = 0;
            // adapterPropertyChangedCallback has already been received.  Set the scan mode.
            setScanMode(AbstractionLayer.BT_SCAN_MODE_CONNECTABLE);
            // This keeps NV up-to date on first-boot after flash.
            setDiscoverableTimeout(mDiscoverableTimeout);
        }
    }

    void onBleDisable() {
        // Sequence BLE_ON to STATE_OFF - that is _complete_ OFF state.
        debugLog("onBleDisable");
        // Set the scan_mode to NONE (no incoming connections).
        setScanMode(AbstractionLayer.BT_SCAN_MODE_NONE);
    }

    void onBluetoothDisable() {
        // From STATE_ON to BLE_ON
        debugLog("onBluetoothDisable()");
        // Turn off any Device Search/Inquiry
        mService.cancelDiscovery();
        // Set the scan_mode to NONE (no incoming connections).
        setScanMode(AbstractionLayer.BT_SCAN_MODE_NONE);
    }

    void discoveryStateChangeCallback(int state) {
        infoLog("Callback:discoveryStateChangeCallback with state:" + state);
        synchronized (mObject) {
            Intent intent;
            if (state == AbstractionLayer.BT_DISCOVERY_STOPPED) {
                mDiscovering = false;
                mDiscoveryEndMs = System.currentTimeMillis();
                intent = new Intent(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
                mService.sendBroadcast(intent, AdapterService.BLUETOOTH_PERM);
            } else if (state == AbstractionLayer.BT_DISCOVERY_STARTED) {
                mDiscovering = true;
                mDiscoveryEndMs = System.currentTimeMillis() + DEFAULT_DISCOVERY_TIMEOUT_MS;
                intent = new Intent(BluetoothAdapter.ACTION_DISCOVERY_STARTED);
                mService.sendBroadcast(intent, AdapterService.BLUETOOTH_PERM);
            }
        }
    }

    protected void dump(FileDescriptor fd, PrintWriter writer, String[] args) {
        writer.println(TAG);
        writer.println("  " + "Name: " + getName());
        writer.println("  " + "Address: " + Utils.getAddressStringFromByte(mAddress));
        writer.println("  " + "BluetoothClass: " + getBluetoothClass());
        writer.println("  " + "ScanMode: " + dumpScanMode(getScanMode()));
        writer.println("  " + "ConnectionState: " + dumpConnectionState(getConnectionState()));
        writer.println("  " + "State: " + BluetoothAdapter.nameForState(getState()));
        writer.println("  " + "MaxConnectedAudioDevices: " + getMaxConnectedAudioDevices());
        writer.println("  " + "A2dpOffloadEnabled: " + mA2dpOffloadEnabled);
        writer.println("  " + "Discovering: " + mDiscovering);
        writer.println("  " + "DiscoveryEndMs: " + mDiscoveryEndMs);

        writer.println("  " + "Bonded devices:");
        for (BluetoothDevice device : mBondedDevices) {
            writer.println(
                    "    " + device.getAddress() + " [" + dumpDeviceType(device.getType()) + "] "
                            + device.getName());
        }
    }

    private String dumpDeviceType(int deviceType) {
        switch (deviceType) {
            case BluetoothDevice.DEVICE_TYPE_UNKNOWN:
                return " ???? ";
            case BluetoothDevice.DEVICE_TYPE_CLASSIC:
                return "BR/EDR";
            case BluetoothDevice.DEVICE_TYPE_LE:
                return "  LE  ";
            case BluetoothDevice.DEVICE_TYPE_DUAL:
                return " DUAL ";
            default:
                return "Invalid device type: " + deviceType;
        }
    }

    private String dumpConnectionState(int state) {
        switch (state) {
            case BluetoothAdapter.STATE_DISCONNECTED:
                return "STATE_DISCONNECTED";
            case BluetoothAdapter.STATE_DISCONNECTING:
                return "STATE_DISCONNECTING";
            case BluetoothAdapter.STATE_CONNECTING:
                return "STATE_CONNECTING";
            case BluetoothAdapter.STATE_CONNECTED:
                return "STATE_CONNECTED";
            default:
                return "Unknown Connection State " + state;
        }
    }

    private String dumpScanMode(int scanMode) {
        switch (scanMode) {
            case BluetoothAdapter.SCAN_MODE_NONE:
                return "SCAN_MODE_NONE";
            case BluetoothAdapter.SCAN_MODE_CONNECTABLE:
                return "SCAN_MODE_CONNECTABLE";
            case BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE:
                return "SCAN_MODE_CONNECTABLE_DISCOVERABLE";
            default:
                return "Unknown Scan Mode " + scanMode;
        }
    }

    private static void infoLog(String msg) {
        if (VDBG) {
            Log.i(TAG, msg);
        }
    }

    private static void debugLog(String msg) {
        if (DBG) {
            Log.d(TAG, msg);
        }
    }

    private static void errorLog(String msg) {
        Log.e(TAG, msg);
    }
}
