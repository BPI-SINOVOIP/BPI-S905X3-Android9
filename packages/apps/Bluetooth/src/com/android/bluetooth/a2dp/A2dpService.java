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

package com.android.bluetooth.a2dp;

import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothCodecConfig;
import android.bluetooth.BluetoothCodecStatus;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothUuid;
import android.bluetooth.IBluetoothA2dp;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.os.HandlerThread;
import android.provider.Settings;
import android.support.annotation.GuardedBy;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import com.android.bluetooth.BluetoothMetricsProto;
import com.android.bluetooth.Utils;
import com.android.bluetooth.avrcp.AvrcpTargetService;
import com.android.bluetooth.btservice.AdapterService;
import com.android.bluetooth.btservice.MetricsLogger;
import com.android.bluetooth.btservice.ProfileService;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

/**
 * Provides Bluetooth A2DP profile, as a service in the Bluetooth application.
 * @hide
 */
public class A2dpService extends ProfileService {
    private static final boolean DBG = true;
    private static final String TAG = "A2dpService";

    private static A2dpService sA2dpService;

    private BluetoothAdapter mAdapter;
    private AdapterService mAdapterService;
    private HandlerThread mStateMachinesThread;

    @VisibleForTesting
    A2dpNativeInterface mA2dpNativeInterface;
    private AudioManager mAudioManager;
    private A2dpCodecConfig mA2dpCodecConfig;

    @GuardedBy("mStateMachines")
    private BluetoothDevice mActiveDevice;
    private final ConcurrentMap<BluetoothDevice, A2dpStateMachine> mStateMachines =
            new ConcurrentHashMap<>();

    // Upper limit of all A2DP devices: Bonded or Connected
    private static final int MAX_A2DP_STATE_MACHINES = 50;
    // Upper limit of all A2DP devices that are Connected or Connecting
    private int mMaxConnectedAudioDevices = 1;
    // A2DP Offload Enabled in platform
    boolean mA2dpOffloadEnabled = false;

    private BroadcastReceiver mBondStateChangedReceiver;
    private BroadcastReceiver mConnectionStateChangedReceiver;

    @Override
    protected IProfileServiceBinder initBinder() {
        return new BluetoothA2dpBinder(this);
    }

    @Override
    protected void create() {
        Log.i(TAG, "create()");
    }

    @Override
    protected boolean start() {
        Log.i(TAG, "start()");
        if (sA2dpService != null) {
            throw new IllegalStateException("start() called twice");
        }

        // Step 1: Get BluetoothAdapter, AdapterService, A2dpNativeInterface, AudioManager.
        // None of them can be null.
        mAdapter = Objects.requireNonNull(BluetoothAdapter.getDefaultAdapter(),
                "BluetoothAdapter cannot be null when A2dpService starts");
        mAdapterService = Objects.requireNonNull(AdapterService.getAdapterService(),
                "AdapterService cannot be null when A2dpService starts");
        mA2dpNativeInterface = Objects.requireNonNull(A2dpNativeInterface.getInstance(),
                "A2dpNativeInterface cannot be null when A2dpService starts");
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        Objects.requireNonNull(mAudioManager,
                               "AudioManager cannot be null when A2dpService starts");

        // Step 2: Get maximum number of connected audio devices
        mMaxConnectedAudioDevices = mAdapterService.getMaxConnectedAudioDevices();
        Log.i(TAG, "Max connected audio devices set to " + mMaxConnectedAudioDevices);

        // Step 3: Start handler thread for state machines
        mStateMachines.clear();
        mStateMachinesThread = new HandlerThread("A2dpService.StateMachines");
        mStateMachinesThread.start();

        // Step 4: Setup codec config
        mA2dpCodecConfig = new A2dpCodecConfig(this, mA2dpNativeInterface);

        // Step 5: Initialize native interface
        mA2dpNativeInterface.init(mMaxConnectedAudioDevices,
                                  mA2dpCodecConfig.codecConfigPriorities());

        // Step 6: Check if A2DP is in offload mode
        mA2dpOffloadEnabled = mAdapterService.isA2dpOffloadEnabled();
        if (DBG) {
            Log.d(TAG, "A2DP offload flag set to " + mA2dpOffloadEnabled);
        }

        // Step 7: Setup broadcast receivers
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        mBondStateChangedReceiver = new BondStateChangedReceiver();
        registerReceiver(mBondStateChangedReceiver, filter);
        filter = new IntentFilter();
        filter.addAction(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED);
        mConnectionStateChangedReceiver = new ConnectionStateChangedReceiver();
        registerReceiver(mConnectionStateChangedReceiver, filter);

        // Step 8: Mark service as started
        setA2dpService(this);

        // Step 9: Clear active device
        setActiveDevice(null);

        return true;
    }

    @Override
    protected boolean stop() {
        Log.i(TAG, "stop()");
        if (sA2dpService == null) {
            Log.w(TAG, "stop() called before start()");
            return true;
        }

        // Step 10: Store volume if there is an active device
        if (mActiveDevice != null && AvrcpTargetService.get() != null) {
            AvrcpTargetService.get().storeVolumeForDevice(mActiveDevice);
        }

        // Step 9: Clear active device and stop playing audio
        removeActiveDevice(true);

        // Step 8: Mark service as stopped
        setA2dpService(null);

        // Step 7: Unregister broadcast receivers
        unregisterReceiver(mConnectionStateChangedReceiver);
        mConnectionStateChangedReceiver = null;
        unregisterReceiver(mBondStateChangedReceiver);
        mBondStateChangedReceiver = null;

        // Step 6: Cleanup native interface
        mA2dpNativeInterface.cleanup();
        mA2dpNativeInterface = null;

        // Step 5: Clear codec config
        mA2dpCodecConfig = null;

        // Step 4: Destroy state machines and stop handler thread
        synchronized (mStateMachines) {
            for (A2dpStateMachine sm : mStateMachines.values()) {
                sm.doQuit();
                sm.cleanup();
            }
            mStateMachines.clear();
        }
        mStateMachinesThread.quitSafely();
        mStateMachinesThread = null;

        // Step 2: Reset maximum number of connected audio devices
        mMaxConnectedAudioDevices = 1;

        // Step 1: Clear BluetoothAdapter, AdapterService, A2dpNativeInterface, AudioManager
        mAudioManager = null;
        mA2dpNativeInterface = null;
        mAdapterService = null;
        mAdapter = null;

        return true;
    }

    @Override
    protected void cleanup() {
        Log.i(TAG, "cleanup()");
    }

    public static synchronized A2dpService getA2dpService() {
        if (sA2dpService == null) {
            Log.w(TAG, "getA2dpService(): service is null");
            return null;
        }
        if (!sA2dpService.isAvailable()) {
            Log.w(TAG, "getA2dpService(): service is not available");
            return null;
        }
        return sA2dpService;
    }

    private static synchronized void setA2dpService(A2dpService instance) {
        if (DBG) {
            Log.d(TAG, "setA2dpService(): set to: " + instance);
        }
        sA2dpService = instance;
    }

    public boolean connect(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");
        if (DBG) {
            Log.d(TAG, "connect(): " + device);
        }

        if (getPriority(device) == BluetoothProfile.PRIORITY_OFF) {
            Log.e(TAG, "Cannot connect to " + device + " : PRIORITY_OFF");
            return false;
        }
        if (!BluetoothUuid.isUuidPresent(mAdapterService.getRemoteUuids(device),
                                         BluetoothUuid.AudioSink)) {
            Log.e(TAG, "Cannot connect to " + device + " : Remote does not have A2DP Sink UUID");
            return false;
        }

        synchronized (mStateMachines) {
            if (!connectionAllowedCheckMaxDevices(device)) {
                Log.e(TAG, "Cannot connect to " + device + " : too many connected devices");
                return false;
            }
            A2dpStateMachine smConnect = getOrCreateStateMachine(device);
            if (smConnect == null) {
                Log.e(TAG, "Cannot connect to " + device + " : no state machine");
                return false;
            }
            smConnect.sendMessage(A2dpStateMachine.CONNECT);
            return true;
        }
    }

    boolean disconnect(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");
        if (DBG) {
            Log.d(TAG, "disconnect(): " + device);
        }

        synchronized (mStateMachines) {
            A2dpStateMachine sm = mStateMachines.get(device);
            if (sm == null) {
                Log.e(TAG, "Ignored disconnect request for " + device + " : no state machine");
                return false;
            }
            sm.sendMessage(A2dpStateMachine.DISCONNECT);
            return true;
        }
    }

    public List<BluetoothDevice> getConnectedDevices() {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        synchronized (mStateMachines) {
            List<BluetoothDevice> devices = new ArrayList<>();
            for (A2dpStateMachine sm : mStateMachines.values()) {
                if (sm.isConnected()) {
                    devices.add(sm.getDevice());
                }
            }
            return devices;
        }
    }

    /**
     * Check whether can connect to a peer device.
     * The check considers the maximum number of connected peers.
     *
     * @param device the peer device to connect to
     * @return true if connection is allowed, otherwise false
     */
    private boolean connectionAllowedCheckMaxDevices(BluetoothDevice device) {
        int connected = 0;
        // Count devices that are in the process of connecting or already connected
        synchronized (mStateMachines) {
            for (A2dpStateMachine sm : mStateMachines.values()) {
                switch (sm.getConnectionState()) {
                    case BluetoothProfile.STATE_CONNECTING:
                    case BluetoothProfile.STATE_CONNECTED:
                        if (Objects.equals(device, sm.getDevice())) {
                            return true;    // Already connected or accounted for
                        }
                        connected++;
                        break;
                    default:
                        break;
                }
            }
        }
        return (connected < mMaxConnectedAudioDevices);
    }

    /**
     * Check whether can connect to a peer device.
     * The check considers a number of factors during the evaluation.
     *
     * @param device the peer device to connect to
     * @param isOutgoingRequest if true, the check is for outgoing connection
     * request, otherwise is for incoming connection request
     * @return true if connection is allowed, otherwise false
     */
    @VisibleForTesting(otherwise = VisibleForTesting.PACKAGE_PRIVATE)
    public boolean okToConnect(BluetoothDevice device, boolean isOutgoingRequest) {
        Log.i(TAG, "okToConnect: device " + device + " isOutgoingRequest: " + isOutgoingRequest);
        // Check if this is an incoming connection in Quiet mode.
        if (mAdapterService.isQuietModeEnabled() && !isOutgoingRequest) {
            Log.e(TAG, "okToConnect: cannot connect to " + device + " : quiet mode enabled");
            return false;
        }
        // Check if too many devices
        if (!connectionAllowedCheckMaxDevices(device)) {
            Log.e(TAG, "okToConnect: cannot connect to " + device
                    + " : too many connected devices");
            return false;
        }
        // Check priority and accept or reject the connection.
        // Note: Logic can be simplified, but keeping it this way for readability
        int priority = getPriority(device);
        int bondState = mAdapterService.getBondState(device);
        // If priority is undefined, it is likely that service discovery has not completed and peer
        // initiated the connection. Allow this connection only if the device is bonded or bonding
        boolean serviceDiscoveryPending = (priority == BluetoothProfile.PRIORITY_UNDEFINED)
                && (bondState == BluetoothDevice.BOND_BONDING
                || bondState == BluetoothDevice.BOND_BONDED);
        // Also allow connection when device is bonded/bonding and priority is ON/AUTO_CONNECT.
        boolean isEnabled = (priority == BluetoothProfile.PRIORITY_ON
                || priority == BluetoothProfile.PRIORITY_AUTO_CONNECT)
                && (bondState == BluetoothDevice.BOND_BONDED
                || bondState == BluetoothDevice.BOND_BONDING);
        if (!serviceDiscoveryPending && !isEnabled) {
            // Otherwise, reject the connection if no service discovery is pending and priority is
            // neither PRIORITY_ON nor PRIORITY_AUTO_CONNECT
            Log.w(TAG, "okToConnect: return false, priority=" + priority + ", bondState="
                    + bondState);
            return false;
        }
        return true;
    }

    List<BluetoothDevice> getDevicesMatchingConnectionStates(int[] states) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        List<BluetoothDevice> devices = new ArrayList<>();
        Set<BluetoothDevice> bondedDevices = mAdapter.getBondedDevices();
        synchronized (mStateMachines) {
            for (BluetoothDevice device : bondedDevices) {
                if (!BluetoothUuid.isUuidPresent(mAdapterService.getRemoteUuids(device),
                                                 BluetoothUuid.AudioSink)) {
                    continue;
                }
                int connectionState = BluetoothProfile.STATE_DISCONNECTED;
                A2dpStateMachine sm = mStateMachines.get(device);
                if (sm != null) {
                    connectionState = sm.getConnectionState();
                }
                for (int i = 0; i < states.length; i++) {
                    if (connectionState == states[i]) {
                        devices.add(device);
                    }
                }
            }
            return devices;
        }
    }

    /**
     * Get the list of devices that have state machines.
     *
     * @return the list of devices that have state machines
     */
    @VisibleForTesting
    List<BluetoothDevice> getDevices() {
        List<BluetoothDevice> devices = new ArrayList<>();
        synchronized (mStateMachines) {
            for (A2dpStateMachine sm : mStateMachines.values()) {
                devices.add(sm.getDevice());
            }
            return devices;
        }
    }

    public int getConnectionState(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        synchronized (mStateMachines) {
            A2dpStateMachine sm = mStateMachines.get(device);
            if (sm == null) {
                return BluetoothProfile.STATE_DISCONNECTED;
            }
            return sm.getConnectionState();
        }
    }

    private void removeActiveDevice(boolean forceStopPlayingAudio) {
        BluetoothDevice previousActiveDevice = mActiveDevice;
        synchronized (mStateMachines) {
            // Clear the active device
            mActiveDevice = null;
            // This needs to happen before we inform the audio manager that the device
            // disconnected. Please see comment in broadcastActiveDevice() for why.
            broadcastActiveDevice(null);

            if (previousActiveDevice == null) {
                return;
            }

            // Make sure the Audio Manager knows the previous Active device is disconnected.
            // However, if A2DP is still connected and not forcing stop audio for that remote
            // device, the user has explicitly switched the output to the local device and music
            // should continue playing. Otherwise, the remote device has been indeed disconnected
            // and audio should be suspended before switching the output to the local device.
            boolean suppressNoisyIntent = !forceStopPlayingAudio
                    && (getConnectionState(previousActiveDevice)
                    == BluetoothProfile.STATE_CONNECTED);
            Log.i(TAG, "removeActiveDevice: suppressNoisyIntent=" + suppressNoisyIntent);
            mAudioManager.setBluetoothA2dpDeviceConnectionStateSuppressNoisyIntent(
                    previousActiveDevice, BluetoothProfile.STATE_DISCONNECTED,
                    BluetoothProfile.A2DP, suppressNoisyIntent, -1);
            // Make sure the Active device in native layer is set to null and audio is off
            if (!mA2dpNativeInterface.setActiveDevice(null)) {
                Log.w(TAG, "setActiveDevice(null): Cannot remove active device in native "
                        + "layer");
            }
        }
    }

    /**
     * Set the active device.
     *
     * @param device the active device
     * @return true on success, otherwise false
     */
    public boolean setActiveDevice(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");
        synchronized (mStateMachines) {
            BluetoothDevice previousActiveDevice = mActiveDevice;
            if (DBG) {
                Log.d(TAG, "setActiveDevice(" + device + "): previous is " + previousActiveDevice);
            }

            if (previousActiveDevice != null && AvrcpTargetService.get() != null) {
                AvrcpTargetService.get().storeVolumeForDevice(previousActiveDevice);
            }

            if (device == null) {
                // Remove active device and continue playing audio only if necessary.
                removeActiveDevice(false);
                return true;
            }

            BluetoothCodecStatus codecStatus = null;
            A2dpStateMachine sm = mStateMachines.get(device);
            if (sm == null) {
                Log.e(TAG, "setActiveDevice(" + device + "): Cannot set as active: "
                          + "no state machine");
                return false;
            }
            if (sm.getConnectionState() != BluetoothProfile.STATE_CONNECTED) {
                Log.e(TAG, "setActiveDevice(" + device + "): Cannot set as active: "
                          + "device is not connected");
                return false;
            }
            if (!mA2dpNativeInterface.setActiveDevice(device)) {
                Log.e(TAG, "setActiveDevice(" + device + "): Cannot set as active in native layer");
                return false;
            }
            codecStatus = sm.getCodecStatus();

            boolean deviceChanged = !Objects.equals(device, mActiveDevice);
            mActiveDevice = device;
            // This needs to happen before we inform the audio manager that the device
            // disconnected. Please see comment in broadcastActiveDevice() for why.
            broadcastActiveDevice(mActiveDevice);
            if (deviceChanged) {
                // Send an intent with the active device codec config
                if (codecStatus != null) {
                    broadcastCodecConfig(mActiveDevice, codecStatus);
                }
                // Make sure the Audio Manager knows the previous Active device is disconnected,
                // and the new Active device is connected.
                if (previousActiveDevice != null) {
                    mAudioManager.setBluetoothA2dpDeviceConnectionStateSuppressNoisyIntent(
                            previousActiveDevice, BluetoothProfile.STATE_DISCONNECTED,
                            BluetoothProfile.A2DP, true, -1);
                }

                int rememberedVolume = -1;
                if (AvrcpTargetService.get() != null) {
                    AvrcpTargetService.get().volumeDeviceSwitched(mActiveDevice);

                    rememberedVolume = AvrcpTargetService.get()
                            .getRememberedVolumeForDevice(mActiveDevice);
                }

                mAudioManager.setBluetoothA2dpDeviceConnectionStateSuppressNoisyIntent(
                        mActiveDevice, BluetoothProfile.STATE_CONNECTED, BluetoothProfile.A2DP,
                        true, rememberedVolume);

                // Inform the Audio Service about the codec configuration
                // change, so the Audio Service can reset accordingly the audio
                // feeding parameters in the Audio HAL to the Bluetooth stack.
                mAudioManager.handleBluetoothA2dpDeviceConfigChange(mActiveDevice);
            }
        }
        return true;
    }

    /**
     * Get the active device.
     *
     * @return the active device or null if no device is active
     */
    public BluetoothDevice getActiveDevice() {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        synchronized (mStateMachines) {
            return mActiveDevice;
        }
    }

    private boolean isActiveDevice(BluetoothDevice device) {
        synchronized (mStateMachines) {
            return (device != null) && Objects.equals(device, mActiveDevice);
        }
    }

    public boolean setPriority(BluetoothDevice device, int priority) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH_ADMIN permission");
        Settings.Global.putInt(getContentResolver(),
                Settings.Global.getBluetoothA2dpSinkPriorityKey(device.getAddress()), priority);
        if (DBG) {
            Log.d(TAG, "Saved priority " + device + " = " + priority);
        }
        return true;
    }

    public int getPriority(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH_ADMIN permission");
        int priority = Settings.Global.getInt(getContentResolver(),
                Settings.Global.getBluetoothA2dpSinkPriorityKey(device.getAddress()),
                BluetoothProfile.PRIORITY_UNDEFINED);
        return priority;
    }

    public boolean isAvrcpAbsoluteVolumeSupported() {
        // TODO (apanicke): Add a hook here for the AvrcpTargetService.
        return false;
    }


    public void setAvrcpAbsoluteVolume(int volume) {
        // TODO (apanicke): Instead of using A2DP as a middleman for volume changes, add a binder
        // service to the new AVRCP Profile and have the audio manager use that instead.
        if (AvrcpTargetService.get() != null) {
            AvrcpTargetService.get().sendVolumeChanged(volume);
            return;
        }
    }

    boolean isA2dpPlaying(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        if (DBG) {
            Log.d(TAG, "isA2dpPlaying(" + device + ")");
        }
        synchronized (mStateMachines) {
            A2dpStateMachine sm = mStateMachines.get(device);
            if (sm == null) {
                return false;
            }
            return sm.isPlaying();
        }
    }

    /**
     * Gets the current codec status (configuration and capability).
     *
     * @param device the remote Bluetooth device. If null, use the currect
     * active A2DP Bluetooth device.
     * @return the current codec status
     * @hide
     */
    public BluetoothCodecStatus getCodecStatus(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        if (DBG) {
            Log.d(TAG, "getCodecStatus(" + device + ")");
        }
        synchronized (mStateMachines) {
            if (device == null) {
                device = mActiveDevice;
            }
            if (device == null) {
                return null;
            }
            A2dpStateMachine sm = mStateMachines.get(device);
            if (sm != null) {
                return sm.getCodecStatus();
            }
            return null;
        }
    }

    /**
     * Sets the codec configuration preference.
     *
     * @param device the remote Bluetooth device. If null, use the currect
     * active A2DP Bluetooth device.
     * @param codecConfig the codec configuration preference
     * @hide
     */
    public void setCodecConfigPreference(BluetoothDevice device,
                                         BluetoothCodecConfig codecConfig) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        if (DBG) {
            Log.d(TAG, "setCodecConfigPreference(" + device + "): "
                    + Objects.toString(codecConfig));
        }
        if (device == null) {
            device = mActiveDevice;
        }
        if (device == null) {
            Log.e(TAG, "Cannot set codec config preference: no active A2DP device");
            return;
        }
        mA2dpCodecConfig.setCodecConfigPreference(device, codecConfig);
    }

    /**
     * Enables the optional codecs.
     *
     * @param device the remote Bluetooth device. If null, use the currect
     * active A2DP Bluetooth device.
     * @hide
     */
    public void enableOptionalCodecs(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        if (DBG) {
            Log.d(TAG, "enableOptionalCodecs(" + device + ")");
        }
        if (device == null) {
            device = mActiveDevice;
        }
        if (device == null) {
            Log.e(TAG, "Cannot enable optional codecs: no active A2DP device");
            return;
        }
        mA2dpCodecConfig.enableOptionalCodecs(device);
    }

    /**
     * Disables the optional codecs.
     *
     * @param device the remote Bluetooth device. If null, use the currect
     * active A2DP Bluetooth device.
     * @hide
     */
    public void disableOptionalCodecs(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        if (DBG) {
            Log.d(TAG, "disableOptionalCodecs(" + device + ")");
        }
        if (device == null) {
            device = mActiveDevice;
        }
        if (device == null) {
            Log.e(TAG, "Cannot disable optional codecs: no active A2DP device");
            return;
        }
        mA2dpCodecConfig.disableOptionalCodecs(device);
    }

    public int getSupportsOptionalCodecs(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH_ADMIN permission");
        int support = Settings.Global.getInt(getContentResolver(),
                Settings.Global.getBluetoothA2dpSupportsOptionalCodecsKey(device.getAddress()),
                BluetoothA2dp.OPTIONAL_CODECS_SUPPORT_UNKNOWN);
        return support;
    }

    public void setSupportsOptionalCodecs(BluetoothDevice device, boolean doesSupport) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH_ADMIN permission");
        int value = doesSupport ? BluetoothA2dp.OPTIONAL_CODECS_SUPPORTED
                : BluetoothA2dp.OPTIONAL_CODECS_NOT_SUPPORTED;
        Settings.Global.putInt(getContentResolver(),
                Settings.Global.getBluetoothA2dpSupportsOptionalCodecsKey(device.getAddress()),
                value);
    }

    public int getOptionalCodecsEnabled(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH_ADMIN permission");
        return Settings.Global.getInt(getContentResolver(),
                Settings.Global.getBluetoothA2dpOptionalCodecsEnabledKey(device.getAddress()),
                BluetoothA2dp.OPTIONAL_CODECS_PREF_UNKNOWN);
    }

    public void setOptionalCodecsEnabled(BluetoothDevice device, int value) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH_ADMIN permission");
        if (value != BluetoothA2dp.OPTIONAL_CODECS_PREF_UNKNOWN
                && value != BluetoothA2dp.OPTIONAL_CODECS_PREF_DISABLED
                && value != BluetoothA2dp.OPTIONAL_CODECS_PREF_ENABLED) {
            Log.w(TAG, "Unexpected value passed to setOptionalCodecsEnabled:" + value);
            return;
        }
        Settings.Global.putInt(getContentResolver(),
                Settings.Global.getBluetoothA2dpOptionalCodecsEnabledKey(device.getAddress()),
                value);
    }

    // Handle messages from native (JNI) to Java
    void messageFromNative(A2dpStackEvent stackEvent) {
        Objects.requireNonNull(stackEvent.device,
                               "Device should never be null, event: " + stackEvent);
        synchronized (mStateMachines) {
            BluetoothDevice device = stackEvent.device;
            A2dpStateMachine sm = mStateMachines.get(device);
            if (sm == null) {
                if (stackEvent.type == A2dpStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED) {
                    switch (stackEvent.valueInt) {
                        case A2dpStackEvent.CONNECTION_STATE_CONNECTED:
                        case A2dpStackEvent.CONNECTION_STATE_CONNECTING:
                            // Create a new state machine only when connecting to a device
                            if (!connectionAllowedCheckMaxDevices(device)) {
                                Log.e(TAG, "Cannot connect to " + device
                                        + " : too many connected devices");
                                return;
                            }
                            sm = getOrCreateStateMachine(device);
                            break;
                        default:
                            break;
                    }
                }
            }
            if (sm == null) {
                Log.e(TAG, "Cannot process stack event: no state machine: " + stackEvent);
                return;
            }
            sm.sendMessage(A2dpStateMachine.STACK_EVENT, stackEvent);
        }
    }

    /**
     * The codec configuration for a device has been updated.
     *
     * @param device the remote device
     * @param codecStatus the new codec status
     * @param sameAudioFeedingParameters if true the audio feeding parameters
     * haven't been changed
     */
    void codecConfigUpdated(BluetoothDevice device, BluetoothCodecStatus codecStatus,
                            boolean sameAudioFeedingParameters) {
        broadcastCodecConfig(device, codecStatus);

        // Inform the Audio Service about the codec configuration change,
        // so the Audio Service can reset accordingly the audio feeding
        // parameters in the Audio HAL to the Bluetooth stack.
        if (isActiveDevice(device) && !sameAudioFeedingParameters) {
            mAudioManager.handleBluetoothA2dpDeviceConfigChange(device);
        }
    }

    private A2dpStateMachine getOrCreateStateMachine(BluetoothDevice device) {
        if (device == null) {
            Log.e(TAG, "getOrCreateStateMachine failed: device cannot be null");
            return null;
        }
        synchronized (mStateMachines) {
            A2dpStateMachine sm = mStateMachines.get(device);
            if (sm != null) {
                return sm;
            }
            // Limit the maximum number of state machines to avoid DoS attack
            if (mStateMachines.size() >= MAX_A2DP_STATE_MACHINES) {
                Log.e(TAG, "Maximum number of A2DP state machines reached: "
                        + MAX_A2DP_STATE_MACHINES);
                return null;
            }
            if (DBG) {
                Log.d(TAG, "Creating a new state machine for " + device);
            }
            sm = A2dpStateMachine.make(device, this, mA2dpNativeInterface,
                                       mStateMachinesThread.getLooper());
            mStateMachines.put(device, sm);
            return sm;
        }
    }

    private void broadcastActiveDevice(BluetoothDevice device) {
        if (DBG) {
            Log.d(TAG, "broadcastActiveDevice(" + device + ")");
        }

        Intent intent = new Intent(BluetoothA2dp.ACTION_ACTIVE_DEVICE_CHANGED);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT
                        | Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND);
        sendBroadcast(intent, ProfileService.BLUETOOTH_PERM);
    }

    private void broadcastCodecConfig(BluetoothDevice device, BluetoothCodecStatus codecStatus) {
        if (DBG) {
            Log.d(TAG, "broadcastCodecConfig(" + device + "): " + codecStatus);
        }
        Intent intent = new Intent(BluetoothA2dp.ACTION_CODEC_CONFIG_CHANGED);
        intent.putExtra(BluetoothCodecStatus.EXTRA_CODEC_STATUS, codecStatus);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT
                        | Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND);
        sendBroadcast(intent, A2dpService.BLUETOOTH_PERM);
    }

    private class BondStateChangedReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (!BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(intent.getAction())) {
                return;
            }
            int state = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE,
                                           BluetoothDevice.ERROR);
            BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            Objects.requireNonNull(device, "ACTION_BOND_STATE_CHANGED with no EXTRA_DEVICE");
            bondStateChanged(device, state);
        }
    }

    /**
     * Process a change in the bonding state for a device.
     *
     * @param device the device whose bonding state has changed
     * @param bondState the new bond state for the device. Possible values are:
     * {@link BluetoothDevice#BOND_NONE},
     * {@link BluetoothDevice#BOND_BONDING},
     * {@link BluetoothDevice#BOND_BONDED}.
     */
    @VisibleForTesting
    void bondStateChanged(BluetoothDevice device, int bondState) {
        if (DBG) {
            Log.d(TAG, "Bond state changed for device: " + device + " state: " + bondState);
        }
        // Remove state machine if the bonding for a device is removed
        if (bondState != BluetoothDevice.BOND_NONE) {
            return;
        }
        synchronized (mStateMachines) {
            A2dpStateMachine sm = mStateMachines.get(device);
            if (sm == null) {
                return;
            }
            if (sm.getConnectionState() != BluetoothProfile.STATE_DISCONNECTED) {
                return;
            }
            removeStateMachine(device);
        }
    }

    private void removeStateMachine(BluetoothDevice device) {
        synchronized (mStateMachines) {
            A2dpStateMachine sm = mStateMachines.get(device);
            if (sm == null) {
                Log.w(TAG, "removeStateMachine: device " + device
                        + " does not have a state machine");
                return;
            }
            Log.i(TAG, "removeStateMachine: removing state machine for device: " + device);
            sm.doQuit();
            sm.cleanup();
            mStateMachines.remove(device);
        }
    }

    private void updateOptionalCodecsSupport(BluetoothDevice device) {
        int previousSupport = getSupportsOptionalCodecs(device);
        boolean supportsOptional = false;

        synchronized (mStateMachines) {
            A2dpStateMachine sm = mStateMachines.get(device);
            if (sm == null) {
                return;
            }
            BluetoothCodecStatus codecStatus = sm.getCodecStatus();
            if (codecStatus != null) {
                for (BluetoothCodecConfig config : codecStatus.getCodecsSelectableCapabilities()) {
                    if (!config.isMandatoryCodec()) {
                        supportsOptional = true;
                        break;
                    }
                }
            }
        }
        if (previousSupport == BluetoothA2dp.OPTIONAL_CODECS_SUPPORT_UNKNOWN
                || supportsOptional != (previousSupport
                                    == BluetoothA2dp.OPTIONAL_CODECS_SUPPORTED)) {
            setSupportsOptionalCodecs(device, supportsOptional);
        }
        if (supportsOptional) {
            int enabled = getOptionalCodecsEnabled(device);
            if (enabled == BluetoothA2dp.OPTIONAL_CODECS_PREF_ENABLED) {
                enableOptionalCodecs(device);
            } else if (enabled == BluetoothA2dp.OPTIONAL_CODECS_PREF_DISABLED) {
                disableOptionalCodecs(device);
            }
        }
    }

    private void connectionStateChanged(BluetoothDevice device, int fromState, int toState) {
        if ((device == null) || (fromState == toState)) {
            return;
        }
        synchronized (mStateMachines) {
            if (toState == BluetoothProfile.STATE_CONNECTED) {
                // Each time a device connects, we want to re-check if it supports optional
                // codecs (perhaps it's had a firmware update, etc.) and save that state if
                // it differs from what we had saved before.
                updateOptionalCodecsSupport(device);
                MetricsLogger.logProfileConnectionEvent(BluetoothMetricsProto.ProfileId.A2DP);
            }
            // Set the active device if only one connected device is supported and it was connected
            if (toState == BluetoothProfile.STATE_CONNECTED && (mMaxConnectedAudioDevices == 1)) {
                setActiveDevice(device);
            }
            // Check if the active device is not connected anymore
            if (isActiveDevice(device) && (fromState == BluetoothProfile.STATE_CONNECTED)) {
                setActiveDevice(null);
            }
            // Check if the device is disconnected - if unbond, remove the state machine
            if (toState == BluetoothProfile.STATE_DISCONNECTED) {
                int bondState = mAdapterService.getBondState(device);
                if (bondState == BluetoothDevice.BOND_NONE) {
                    removeStateMachine(device);
                }
            }
        }
    }

    /**
     * Receiver for processing device connection state changes.
     *
     * <ul>
     * <li> Update codec support per device when device is (re)connected
     * <li> Delete the state machine instance if the device is disconnected and unbond
     * </ul>
     */
    private class ConnectionStateChangedReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (!BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED.equals(intent.getAction())) {
                return;
            }
            BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            int toState = intent.getIntExtra(BluetoothProfile.EXTRA_STATE, -1);
            int fromState = intent.getIntExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, -1);
            connectionStateChanged(device, fromState, toState);
        }
    }

    /**
     * Binder object: must be a static class or memory leak may occur.
     */
    @VisibleForTesting
    static class BluetoothA2dpBinder extends IBluetoothA2dp.Stub
            implements IProfileServiceBinder {
        private A2dpService mService;

        private A2dpService getService() {
            if (!Utils.checkCaller()) {
                Log.w(TAG, "A2DP call not allowed for non-active user");
                return null;
            }

            if (mService != null && mService.isAvailable()) {
                return mService;
            }
            return null;
        }

        BluetoothA2dpBinder(A2dpService svc) {
            mService = svc;
        }

        @Override
        public void cleanup() {
            mService = null;
        }

        @Override
        public boolean connect(BluetoothDevice device) {
            A2dpService service = getService();
            if (service == null) {
                return false;
            }
            return service.connect(device);
        }

        @Override
        public boolean disconnect(BluetoothDevice device) {
            A2dpService service = getService();
            if (service == null) {
                return false;
            }
            return service.disconnect(device);
        }

        @Override
        public List<BluetoothDevice> getConnectedDevices() {
            A2dpService service = getService();
            if (service == null) {
                return new ArrayList<>(0);
            }
            return service.getConnectedDevices();
        }

        @Override
        public List<BluetoothDevice> getDevicesMatchingConnectionStates(int[] states) {
            A2dpService service = getService();
            if (service == null) {
                return new ArrayList<>(0);
            }
            return service.getDevicesMatchingConnectionStates(states);
        }

        @Override
        public int getConnectionState(BluetoothDevice device) {
            A2dpService service = getService();
            if (service == null) {
                return BluetoothProfile.STATE_DISCONNECTED;
            }
            return service.getConnectionState(device);
        }

        @Override
        public boolean setActiveDevice(BluetoothDevice device) {
            A2dpService service = getService();
            if (service == null) {
                return false;
            }
            return service.setActiveDevice(device);
        }

        @Override
        public BluetoothDevice getActiveDevice() {
            A2dpService service = getService();
            if (service == null) {
                return null;
            }
            return service.getActiveDevice();
        }

        @Override
        public boolean setPriority(BluetoothDevice device, int priority) {
            A2dpService service = getService();
            if (service == null) {
                return false;
            }
            return service.setPriority(device, priority);
        }

        @Override
        public int getPriority(BluetoothDevice device) {
            A2dpService service = getService();
            if (service == null) {
                return BluetoothProfile.PRIORITY_UNDEFINED;
            }
            return service.getPriority(device);
        }

        @Override
        public boolean isAvrcpAbsoluteVolumeSupported() {
            A2dpService service = getService();
            if (service == null) {
                return false;
            }
            return service.isAvrcpAbsoluteVolumeSupported();
        }

        @Override
        public void setAvrcpAbsoluteVolume(int volume) {
            A2dpService service = getService();
            if (service == null) {
                return;
            }
            service.setAvrcpAbsoluteVolume(volume);
        }

        @Override
        public boolean isA2dpPlaying(BluetoothDevice device) {
            A2dpService service = getService();
            if (service == null) {
                return false;
            }
            return service.isA2dpPlaying(device);
        }

        @Override
        public BluetoothCodecStatus getCodecStatus(BluetoothDevice device) {
            A2dpService service = getService();
            if (service == null) {
                return null;
            }
            return service.getCodecStatus(device);
        }

        @Override
        public void setCodecConfigPreference(BluetoothDevice device,
                                             BluetoothCodecConfig codecConfig) {
            A2dpService service = getService();
            if (service == null) {
                return;
            }
            service.setCodecConfigPreference(device, codecConfig);
        }

        @Override
        public void enableOptionalCodecs(BluetoothDevice device) {
            A2dpService service = getService();
            if (service == null) {
                return;
            }
            service.enableOptionalCodecs(device);
        }

        @Override
        public void disableOptionalCodecs(BluetoothDevice device) {
            A2dpService service = getService();
            if (service == null) {
                return;
            }
            service.disableOptionalCodecs(device);
        }

        public int supportsOptionalCodecs(BluetoothDevice device) {
            A2dpService service = getService();
            if (service == null) {
                return BluetoothA2dp.OPTIONAL_CODECS_SUPPORT_UNKNOWN;
            }
            return service.getSupportsOptionalCodecs(device);
        }

        public int getOptionalCodecsEnabled(BluetoothDevice device) {
            A2dpService service = getService();
            if (service == null) {
                return BluetoothA2dp.OPTIONAL_CODECS_PREF_UNKNOWN;
            }
            return service.getOptionalCodecsEnabled(device);
        }

        public void setOptionalCodecsEnabled(BluetoothDevice device, int value) {
            A2dpService service = getService();
            if (service == null) {
                return;
            }
            service.setOptionalCodecsEnabled(device, value);
        }
    }

    @Override
    public void dump(StringBuilder sb) {
        super.dump(sb);
        ProfileService.println(sb, "mActiveDevice: " + mActiveDevice);
        for (A2dpStateMachine sm : mStateMachines.values()) {
            sm.dump(sb);
        }
    }
}
