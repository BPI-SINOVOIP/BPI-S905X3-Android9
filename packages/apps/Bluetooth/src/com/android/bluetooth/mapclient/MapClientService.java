/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.bluetooth.mapclient;

import android.Manifest;
import android.app.PendingIntent;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothUuid;
import android.bluetooth.IBluetoothMapClient;
import android.bluetooth.SdpMasRecord;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.ParcelUuid;
import android.provider.Settings;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import com.android.bluetooth.Utils;
import com.android.bluetooth.btservice.ProfileService;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

public class MapClientService extends ProfileService {
    private static final String TAG = "MapClientService";

    static final boolean DBG = false;
    static final boolean VDBG = false;

    static final int MAXIMUM_CONNECTED_DEVICES = 4;

    private static final String BLUETOOTH_PERM = android.Manifest.permission.BLUETOOTH;

    private Map<BluetoothDevice, MceStateMachine> mMapInstanceMap = new ConcurrentHashMap<>(1);
    private MnsService mMnsServer;
    private BluetoothAdapter mAdapter;
    private static MapClientService sMapClientService;
    private MapBroadcastReceiver mMapReceiver = new MapBroadcastReceiver();

    public static synchronized MapClientService getMapClientService() {
        if (sMapClientService == null) {
            Log.w(TAG, "getMapClientService(): service is null");
            return null;
        }
        if (!sMapClientService.isAvailable()) {
            Log.w(TAG, "getMapClientService(): service is not available ");
            return null;
        }
        return sMapClientService;
    }

    private static synchronized void setMapClientService(MapClientService instance) {
        if (DBG) {
            Log.d(TAG, "setMapClientService(): set to: " + instance);
        }
        sMapClientService = instance;
    }

    @VisibleForTesting
    Map<BluetoothDevice, MceStateMachine> getInstanceMap() {
        return mMapInstanceMap;
    }

    /**
     * Connect the given Bluetooth device.
     *
     * @param device
     * @return true if connection is successful, false otherwise.
     */
    public synchronized boolean connect(BluetoothDevice device) {
        if (device == null) {
            throw new IllegalArgumentException("Null device");
        }
        if (DBG) {
            StringBuilder sb = new StringBuilder();
            dump(sb);
            Log.d(TAG, "MAP connect device: " + device
                    + ", InstanceMap start state: " + sb.toString());
        }
        MceStateMachine mapStateMachine = mMapInstanceMap.get(device);
        if (mapStateMachine == null) {
            // a map state machine instance doesn't exist yet, create a new one if we can.
            if (mMapInstanceMap.size() < MAXIMUM_CONNECTED_DEVICES) {
                addDeviceToMapAndConnect(device);
                return true;
            } else {
                // Maxed out on the number of allowed connections.
                // see if some of the current connections can be cleaned-up, to make room.
                removeUncleanAccounts();
                if (mMapInstanceMap.size() < MAXIMUM_CONNECTED_DEVICES) {
                    addDeviceToMapAndConnect(device);
                    return true;
                } else {
                    Log.e(TAG, "Maxed out on the number of allowed MAP connections. "
                            + "Connect request rejected on " + device);
                    return false;
                }
            }
        }

        // statemachine already exists in the map.
        int state = getConnectionState(device);
        if (state == BluetoothProfile.STATE_CONNECTED
                || state == BluetoothProfile.STATE_CONNECTING) {
            Log.w(TAG, "Received connect request while already connecting/connected.");
            return true;
        }

        // Statemachine exists but not in connecting or connected state! it should
        // have been removed form the map. lets get rid of it and add a new one.
        if (DBG) {
            Log.d(TAG, "Statemachine exists for a device in unexpected state: " + state);
        }
        mMapInstanceMap.remove(device);
        addDeviceToMapAndConnect(device);
        if (DBG) {
            StringBuilder sb = new StringBuilder();
            dump(sb);
            Log.d(TAG, "MAP connect device: " + device
                    + ", InstanceMap end state: " + sb.toString());
        }
        return true;
    }

    private synchronized void addDeviceToMapAndConnect(BluetoothDevice device) {
        // When creating a new statemachine, its state is set to CONNECTING - which will trigger
        // connect.
        MceStateMachine mapStateMachine = new MceStateMachine(this, device);
        mMapInstanceMap.put(device, mapStateMachine);
    }

    public synchronized boolean disconnect(BluetoothDevice device) {
        if (DBG) {
            StringBuilder sb = new StringBuilder();
            dump(sb);
            Log.d(TAG, "MAP disconnect device: " + device
                    + ", InstanceMap start state: " + sb.toString());
        }
        MceStateMachine mapStateMachine = mMapInstanceMap.get(device);
        // a map state machine instance doesn't exist. maybe it is already gone?
        if (mapStateMachine == null) {
            return false;
        }
        int connectionState = mapStateMachine.getState();
        if (connectionState != BluetoothProfile.STATE_CONNECTED
                && connectionState != BluetoothProfile.STATE_CONNECTING) {
            return false;
        }
        mapStateMachine.disconnect();
        if (DBG) {
            StringBuilder sb = new StringBuilder();
            dump(sb);
            Log.d(TAG, "MAP disconnect device: " + device
                    + ", InstanceMap start state: " + sb.toString());
        }
        return true;
    }

    public List<BluetoothDevice> getConnectedDevices() {
        return getDevicesMatchingConnectionStates(new int[]{BluetoothAdapter.STATE_CONNECTED});
    }

    MceStateMachine getMceStateMachineForDevice(BluetoothDevice device) {
        return mMapInstanceMap.get(device);
    }

    public synchronized List<BluetoothDevice> getDevicesMatchingConnectionStates(int[] states) {
        Log.d(TAG, "getDevicesMatchingConnectionStates" + Arrays.toString(states));
        List<BluetoothDevice> deviceList = new ArrayList<>();
        Set<BluetoothDevice> bondedDevices = mAdapter.getBondedDevices();
        int connectionState;
        for (BluetoothDevice device : bondedDevices) {
            connectionState = getConnectionState(device);
            Log.d(TAG, "Device: " + device + "State: " + connectionState);
            for (int i = 0; i < states.length; i++) {
                if (connectionState == states[i]) {
                    deviceList.add(device);
                }
            }
        }
        Log.d(TAG, deviceList.toString());
        return deviceList;
    }

    public synchronized int getConnectionState(BluetoothDevice device) {
        MceStateMachine mapStateMachine = mMapInstanceMap.get(device);
        // a map state machine instance doesn't exist yet, create a new one if we can.
        return (mapStateMachine == null) ? BluetoothProfile.STATE_DISCONNECTED
                : mapStateMachine.getState();
    }

    public boolean setPriority(BluetoothDevice device, int priority) {
        Settings.Global.putInt(getContentResolver(),
                Settings.Global.getBluetoothMapClientPriorityKey(device.getAddress()), priority);
        if (VDBG) {
            Log.v(TAG, "Saved priority " + device + " = " + priority);
        }
        return true;
    }

    public int getPriority(BluetoothDevice device) {
        int priority = Settings.Global.getInt(getContentResolver(),
                Settings.Global.getBluetoothMapClientPriorityKey(device.getAddress()),
                BluetoothProfile.PRIORITY_UNDEFINED);
        return priority;
    }

    public synchronized boolean sendMessage(BluetoothDevice device, Uri[] contacts, String message,
            PendingIntent sentIntent, PendingIntent deliveredIntent) {
        MceStateMachine mapStateMachine = mMapInstanceMap.get(device);
        return mapStateMachine != null
                && mapStateMachine.sendMapMessage(contacts, message, sentIntent, deliveredIntent);
    }

    @Override
    protected IProfileServiceBinder initBinder() {
        return new Binder(this);
    }

    @Override
    protected boolean start() {
        Log.e(TAG, "start()");

        if (mMnsServer == null) {
            mMnsServer = MapUtils.newMnsServiceInstance(this);
            if (mMnsServer == null) {
                // this can't happen
                Log.w(TAG, "MnsService is *not* created!");
                return false;
            }
        }

        mAdapter = BluetoothAdapter.getDefaultAdapter();

        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_SDP_RECORD);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        registerReceiver(mMapReceiver, filter);
        removeUncleanAccounts();
        setMapClientService(this);
        return true;
    }

    @Override
    protected synchronized boolean stop() {
        if (DBG) {
            Log.d(TAG, "stop()");
        }
        unregisterReceiver(mMapReceiver);
        if (mMnsServer != null) {
            mMnsServer.stop();
        }
        for (MceStateMachine stateMachine : mMapInstanceMap.values()) {
            if (stateMachine.getState() == BluetoothAdapter.STATE_CONNECTED) {
                stateMachine.disconnect();
            }
            stateMachine.doQuit();
        }
        return true;
    }

    @Override
    protected void cleanup() {
        if (DBG) {
            Log.d(TAG, "in Cleanup");
        }
        removeUncleanAccounts();
        // TODO(b/72948646): should be moved to stop()
        setMapClientService(null);
    }

    void cleanupDevice(BluetoothDevice device) {
        if (DBG) {
            StringBuilder sb = new StringBuilder();
            dump(sb);
            Log.d(TAG, "Cleanup device: " + device + ", InstanceMap start state: "
                    + sb.toString());
        }
        synchronized (mMapInstanceMap) {
            MceStateMachine stateMachine = mMapInstanceMap.get(device);
            if (stateMachine != null) {
                mMapInstanceMap.remove(device);
            }
        }
        if (DBG) {
            StringBuilder sb = new StringBuilder();
            dump(sb);
            Log.d(TAG, "Cleanup device: " + device + ", InstanceMap end state: "
                    + sb.toString());
        }
    }

    @VisibleForTesting
    void removeUncleanAccounts() {
        if (DBG) {
            StringBuilder sb = new StringBuilder();
            dump(sb);
            Log.d(TAG, "removeUncleanAccounts:InstanceMap end state: "
                    + sb.toString());
        }
        Iterator iterator = mMapInstanceMap.entrySet().iterator();
        while (iterator.hasNext()) {
            Map.Entry<BluetoothDevice, MceStateMachine> profileConnection =
                    (Map.Entry) iterator.next();
            if (profileConnection.getValue().getState() == BluetoothProfile.STATE_DISCONNECTED) {
                iterator.remove();
            }
        }
        if (DBG) {
            StringBuilder sb = new StringBuilder();
            dump(sb);
            Log.d(TAG, "removeUncleanAccounts:InstanceMap end state: "
                    + sb.toString());
        }
    }

    public synchronized boolean getUnreadMessages(BluetoothDevice device) {
        MceStateMachine mapStateMachine = mMapInstanceMap.get(device);
        if (mapStateMachine == null) {
            return false;
        }
        return mapStateMachine.getUnreadMessages();
    }

    @Override
    public void dump(StringBuilder sb) {
        super.dump(sb);
        ProfileService.println(sb, "# Services Connected: " + mMapInstanceMap.size());
        for (MceStateMachine stateMachine : mMapInstanceMap.values()) {
            stateMachine.dump(sb);
        }
    }

    //Binder object: Must be static class or memory leak may occur

    /**
     * This class implements the IClient interface - or actually it validates the
     * preconditions for calling the actual functionality in the MapClientService, and calls it.
     */
    private static class Binder extends IBluetoothMapClient.Stub implements IProfileServiceBinder {
        private MapClientService mService;

        Binder(MapClientService service) {
            if (VDBG) {
                Log.v(TAG, "Binder()");
            }
            mService = service;
        }

        private MapClientService getService() {
            if (!Utils.checkCaller()) {
                Log.w(TAG, "MAP call not allowed for non-active user");
                return null;
            }

            if (mService != null && mService.isAvailable()) {
                mService.enforceCallingOrSelfPermission(BLUETOOTH_PERM,
                        "Need BLUETOOTH permission");
                return mService;
            }
            return null;
        }

        @Override
        public void cleanup() {
            mService = null;
        }

        @Override
        public boolean isConnected(BluetoothDevice device) {
            if (VDBG) {
                Log.v(TAG, "isConnected()");
            }
            MapClientService service = getService();
            if (service == null) {
                return false;
            }
            return service.getConnectionState(device) == BluetoothProfile.STATE_CONNECTED;
        }

        @Override
        public boolean connect(BluetoothDevice device) {
            if (VDBG) {
                Log.v(TAG, "connect()");
            }
            MapClientService service = getService();
            if (service == null) {
                return false;
            }
            return service.connect(device);
        }

        @Override
        public boolean disconnect(BluetoothDevice device) {
            if (VDBG) {
                Log.v(TAG, "disconnect()");
            }
            MapClientService service = getService();
            if (service == null) {
                return false;
            }
            return service.disconnect(device);
        }

        @Override
        public List<BluetoothDevice> getConnectedDevices() {
            if (VDBG) {
                Log.v(TAG, "getConnectedDevices()");
            }
            MapClientService service = getService();
            if (service == null) {
                return new ArrayList<BluetoothDevice>(0);
            }
            return service.getConnectedDevices();
        }

        @Override
        public List<BluetoothDevice> getDevicesMatchingConnectionStates(int[] states) {
            if (VDBG) {
                Log.v(TAG, "getDevicesMatchingConnectionStates()");
            }
            MapClientService service = getService();
            if (service == null) {
                return new ArrayList<BluetoothDevice>(0);
            }
            return service.getDevicesMatchingConnectionStates(states);
        }

        @Override
        public int getConnectionState(BluetoothDevice device) {
            if (VDBG) {
                Log.v(TAG, "getConnectionState()");
            }
            MapClientService service = getService();
            if (service == null) {
                return BluetoothProfile.STATE_DISCONNECTED;
            }
            return service.getConnectionState(device);
        }

        @Override
        public boolean setPriority(BluetoothDevice device, int priority) {
            MapClientService service = getService();
            if (service == null) {
                return false;
            }
            return service.setPriority(device, priority);
        }

        @Override
        public int getPriority(BluetoothDevice device) {
            MapClientService service = getService();
            if (service == null) {
                return BluetoothProfile.PRIORITY_UNDEFINED;
            }
            return service.getPriority(device);
        }

        @Override
        public boolean sendMessage(BluetoothDevice device, Uri[] contacts, String message,
                PendingIntent sentIntent, PendingIntent deliveredIntent) {
            MapClientService service = getService();
            if (service == null) {
                return false;
            }
            Log.d(TAG, "Checking Permission of sendMessage");
            mService.enforceCallingOrSelfPermission(Manifest.permission.SEND_SMS,
                    "Need SEND_SMS permission");

            return service.sendMessage(device, contacts, message, sentIntent, deliveredIntent);
        }

        @Override
        public boolean getUnreadMessages(BluetoothDevice device) {
            MapClientService service = getService();
            if (service == null) {
                return false;
            }
            mService.enforceCallingOrSelfPermission(Manifest.permission.READ_SMS,
                    "Need READ_SMS permission");
            return service.getUnreadMessages(device);
        }
    }

    private class MapBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (DBG) {
                Log.d(TAG, "onReceive: " + action);
            }
            if (!action.equals(BluetoothDevice.ACTION_ACL_DISCONNECTED)
                    && !action.equals(BluetoothDevice.ACTION_SDP_RECORD)) {
                // we don't care about this intent
                return;
            }
            BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            if (device == null) {
                Log.e(TAG, "broadcast has NO device param!");
                return;
            }
            if (DBG) {
                Log.d(TAG, "broadcast has device: (" + device.getAddress() + ", "
                        + device.getName() + ")");
            }
            MceStateMachine stateMachine = mMapInstanceMap.get(device);
            if (stateMachine == null) {
                Log.e(TAG, "No Statemachine found for the device from broadcast");
                return;
            }

            if (action.equals(BluetoothDevice.ACTION_ACL_DISCONNECTED)) {
                if (stateMachine.getState() == BluetoothProfile.STATE_CONNECTED) {
                    stateMachine.disconnect();
                }
            }

            if (action.equals(BluetoothDevice.ACTION_SDP_RECORD)) {
                ParcelUuid uuid = intent.getParcelableExtra(BluetoothDevice.EXTRA_UUID);
                if (DBG) {
                    Log.d(TAG, "UUID of SDP: " + uuid);
                }

                if (uuid.equals(BluetoothUuid.MAS)) {
                    // Check if we have a valid SDP record.
                    SdpMasRecord masRecord =
                            intent.getParcelableExtra(BluetoothDevice.EXTRA_SDP_RECORD);
                    if (DBG) {
                        Log.d(TAG, "SDP = " + masRecord);
                    }
                    int status = intent.getIntExtra(BluetoothDevice.EXTRA_SDP_SEARCH_STATUS, -1);
                    if (masRecord == null) {
                        Log.w(TAG, "SDP search ended with no MAS record. Status: " + status);
                        return;
                    }
                    stateMachine.obtainMessage(MceStateMachine.MSG_MAS_SDP_DONE,
                            masRecord).sendToTarget();
                }
            }
        }
    }
}
