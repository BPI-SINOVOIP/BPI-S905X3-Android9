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
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.os.Bundle;

import com.googlecode.android_scripting.Log;
import com.googlecode.android_scripting.MainThread;
import com.googlecode.android_scripting.facade.EventFacade;
import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcParameter;
import com.googlecode.android_scripting.rpc.RpcStopEvent;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.Callable;

public class GattClientFacade extends RpcReceiver {
    private final EventFacade mEventFacade;
    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothManager mBluetoothManager;
    private final Service mService;
    private final Context mContext;
    private final HashMap<Integer, myBluetoothGattCallback> mGattCallbackList;
    private final HashMap<Integer, BluetoothGatt> mBluetoothGattList;
    private final HashMap<Integer, BluetoothGattCharacteristic> mCharacteristicList;
    private final HashMap<Integer, BluetoothGattDescriptor> mDescriptorList;
    private final HashMap<Integer, BluetoothGattService> mGattServiceList;
    private final HashMap<Integer, List<BluetoothGattService>> mBluetoothGattDiscoveredServicesList;
    private final HashMap<Integer, List<BluetoothDevice>> mGattServerDiscoveredDevicesList;
    private static int GattCallbackCount;
    private static int BluetoothGattDiscoveredServicesCount;
    private static int BluetoothGattCount;
    private static int CharacteristicCount;
    private static int DescriptorCount;
    private static int GattServerCallbackCount;
    private static int GattServerCount;
    private static int GattServiceCount;

    public GattClientFacade(FacadeManager manager) {
        super(manager);
        mService = manager.getService();
        mContext = mService.getApplicationContext();
        mBluetoothAdapter =
                MainThread.run(
                        mService,
                        new Callable<BluetoothAdapter>() {
                            @Override
                            public BluetoothAdapter call() throws Exception {
                                return BluetoothAdapter.getDefaultAdapter();
                            }
                        });
        mBluetoothManager = (BluetoothManager) mContext.getSystemService(Service.BLUETOOTH_SERVICE);
        mEventFacade = manager.getReceiver(EventFacade.class);
        mGattCallbackList = new HashMap<Integer, myBluetoothGattCallback>();
        mCharacteristicList = new HashMap<Integer, BluetoothGattCharacteristic>();
        mBluetoothGattList = new HashMap<Integer, BluetoothGatt>();
        mDescriptorList = new HashMap<Integer, BluetoothGattDescriptor>();
        mGattServiceList = new HashMap<Integer, BluetoothGattService>();
        mBluetoothGattDiscoveredServicesList = new HashMap<Integer, List<BluetoothGattService>>();
        mGattServerDiscoveredDevicesList = new HashMap<Integer, List<BluetoothDevice>>();
    }

    /**
     * Create a BluetoothGatt connection
     *
     * @param index of the callback to start a connection on
     * @param macAddress the mac address of the ble device
     * @param autoConnect Whether to directly connect to the remote device (false) or to
     *       automatically connect as soon as the remote device becomes available (true)
     * @param opportunistic Whether this GATT client is opportunistic. An opportunistic GATT client
     *                      does not hold a GATT connection. It automatically disconnects when no
     *                      other GATT connections are active for the remote device.
     * @param transport preferred transport for GATT connections to remote dual-mode devices
     *       TRANSPORT_AUTO or TRANSPORT_BREDR or TRANSPORT_LE
     * @return the index of the BluetoothGatt object
     * @throws Exception
     */
    @Rpc(description = "Create a gatt connection")
    public int gattClientConnectGatt(
            @RpcParameter(name = "index") Integer index,
            @RpcParameter(name = "macAddress") String macAddress,
            @RpcParameter(name = "autoConnect") Boolean autoConnect,
            @RpcParameter(name = "transport") Integer transport,
            @RpcParameter(name = "opportunistic") Boolean opportunistic,
            @RpcParameter(name = "phy") Integer phy)
            throws Exception {
        if (mGattCallbackList.get(index) != null) {
            BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(macAddress);
            if (phy == null) phy = BluetoothDevice.PHY_LE_1M;

            BluetoothGatt mBluetoothGatt = device.connectGatt(mService.getApplicationContext(),
                    autoConnect, mGattCallbackList.get(index), transport, opportunistic, phy, null);
            BluetoothGattCount += 1;
            mBluetoothGattList.put(BluetoothGattCount, mBluetoothGatt);
            return BluetoothGattCount;
        } else {
            throw new Exception("Invalid index input:" + Integer.toString(index));
        }
    }

    /**
     * Trigger discovering of services on the BluetoothGatt object
     *
     * @param index The BluetoothGatt object index
     * @return true, if the remote service discovery has been started
     * @throws Exception
     */
    @Rpc(description = "Trigger discovering of services on the BluetoothGatt object")
    public boolean gattClientDiscoverServices(@RpcParameter(name = "index") Integer index)
            throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            return mBluetoothGattList.get(index).discoverServices();
        } else {
            throw new Exception("Invalid index input:" + Integer.toString(index));
        }
    }

    /**
     * Trigger discovering of services by UUID on the BluetoothGatt object
     *
     * @param index The BluetoothGatt object index
     * @param uuid service UUID
     * @return true, if the remote service discovery has been started
     * @throws Exception
     */
    @Rpc(description = "Trigger discovering of services on the BluetoothGatt object")
    public boolean gattClientDiscoverServiceByUuid(@RpcParameter(name = "index") Integer index,
            @RpcParameter(name = "uuid") String uuid) throws Exception {
        BluetoothGatt gatt = mBluetoothGattList.get(index);
        if (gatt != null) {
            Object ret = gatt.getClass().getMethod("discoverServiceByUuid", UUID.class)
                            .invoke(gatt, UUID.fromString(uuid));
            return (Boolean) ret;
        } else {
            throw new Exception("Invalid index input:" + Integer.toString(index));
        }
    }


    /**
     * Get the services from the BluetoothGatt object
     *
     * @param index The BluetoothGatt object index
     * @return a list of BluetoothGattServices
     * @throws Exception
     */
    @Rpc(description = "Get the services from the BluetoothGatt object")
    public List<BluetoothGattService> gattClientGetServices(
            @RpcParameter(name = "index") Integer index) throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            return mBluetoothGattList.get(index).getServices();
        } else {
            throw new Exception("Invalid index input:" + Integer.toString(index));
        }
    }

    /**
     * Abort reliable write of a bluetooth gatt
     *
     * @param index the bluetooth gatt index
     * @throws Exception
     */
    @Rpc(description = "Abort reliable write of a bluetooth gatt")
    public void gattClientAbortReliableWrite(@RpcParameter(name = "index") Integer index)
            throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            mBluetoothGattList.get(index).abortReliableWrite();
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Begin reliable write of a bluetooth gatt
     *
     * @param index the bluetooth gatt index
     * @return
     * @throws Exception
     */
    @Rpc(description = "Begin reliable write of a bluetooth gatt")
    public boolean gattClientBeginReliableWrite(@RpcParameter(name = "index") Integer index)
            throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            return mBluetoothGattList.get(index).beginReliableWrite();
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Configure a bluetooth gatt's MTU
     *
     * @param index the bluetooth gatt index
     * @param mtu the MTU to set
     * @return
     * @throws Exception
     */
    @Rpc(description = "true, if the new MTU value has been requested successfully")
    public boolean gattClientRequestMtu(
            @RpcParameter(name = "index") Integer index, @RpcParameter(name = "mtu") Integer mtu)
            throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            return mBluetoothGattList.get(index).requestMtu(mtu);
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Read the current transmitter PHY and receiver PHY of the connection.
     *
     * @param index the bluetooth gatt index
     * @throws Exception
     */
    @Rpc(description = "Read PHY")
    public void gattClientReadPhy(@RpcParameter(name = "index") Integer index) throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            mBluetoothGattList.get(index).readPhy();
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Set the preferred connection PHY.
     *
     * @param index the bluetooth gatt index
     * @throws Exception
     */
    @Rpc(description = "Set the preferred connection PHY")
    public void gattClientSetPreferredPhy(@RpcParameter(name = "index") Integer index,
            @RpcParameter(name = "txPhy") Integer txPhy,
            @RpcParameter(name = "rxPhy") Integer rxPhy,
            @RpcParameter(name = "txPhy") Integer phyOptions) throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            mBluetoothGattList.get(index).setPreferredPhy(txPhy, rxPhy, phyOptions);
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Disconnect a bluetooth gatt
     *
     * @param index the bluetooth gatt index
     * @throws Exception
     */
    @Rpc(description = "Disconnect a bluetooth gatt")
    @RpcStopEvent("GattConnect")
    public void gattClientDisconnect(@RpcParameter(name = "index") Integer index) throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            mBluetoothGattList.get(index).disconnect();
        } else {
            throw new Exception("Invalid index input: " + index);
        }
    }

    /**
     * Close a bluetooth gatt object
     *
     * @param index the bluetooth gatt index
     * @throws Exception
     */
    @Rpc(description = "Close a Bluetooth GATT object")
    public void gattClientClose(@RpcParameter(name = "index") Integer index) throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            mBluetoothGattList.get(index).close();
        } else {
            throw new Exception("Invalid index input: " + index);
        }
    }

    /**
     * Execute reliable write on a bluetooth gatt
     *
     * @param index the bluetooth gatt index
     * @return true, if the request to execute the transaction has been sent
     * @throws Exception
     */
    @Rpc(description = "Execute reliable write on a bluetooth gatt")
    public boolean gattExecuteReliableWrite(@RpcParameter(name = "index") Integer index)
            throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            return mBluetoothGattList.get(index).executeReliableWrite();
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Get a list of Bluetooth Devices connnected to the bluetooth gatt
     *
     * @param index the bluetooth gatt index
     * @return List of BluetoothDevice Objects
     * @throws Exception
     */
    @Rpc(description = "Get a list of Bluetooth Devices connnected to the bluetooth gatt")
    public List<BluetoothDevice> gattClientGetConnectedDevices(
            @RpcParameter(name = "index") Integer index) throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            return mBluetoothGattList.get(index).getConnectedDevices();
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Get the remote bluetooth device this GATT client targets to
     *
     * @param index the bluetooth gatt index
     * @return the remote bluetooth device this gatt client targets to
     * @throws Exception
     */
    @Rpc(description = "Get the remote bluetooth device this GATT client targets to")
    public BluetoothDevice gattGetDevice(@RpcParameter(name = "index") Integer index)
            throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            return mBluetoothGattList.get(index).getDevice();
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Get the bluetooth devices matching input connection states
     *
     * @param index the bluetooth gatt index
     * @param states the list of states to match
     * @return The list of BluetoothDevice objects that match the states
     * @throws Exception
     */
    @Rpc(description = "Get the bluetooth devices matching input connection states")
    public List<BluetoothDevice> gattClientGetDevicesMatchingConnectionStates(
            @RpcParameter(name = "index") Integer index, @RpcParameter(
                name = "states") int[] states)
            throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            return mBluetoothGattList.get(index).getDevicesMatchingConnectionStates(states);
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Get the service from an input UUID
     *
     * @param index the bluetooth gatt index
     * @return BluetoothGattService related to the bluetooth gatt
     * @throws Exception
     */
    @Rpc(description = "Get the service from an input UUID")
    public ArrayList<String> gattClientGetServiceUuidList(
            @RpcParameter(name = "index") Integer index)
            throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            ArrayList<String> serviceUuidList = new ArrayList<String>();
            for (BluetoothGattService service : mBluetoothGattList.get(index).getServices()) {
                serviceUuidList.add(service.getUuid().toString());
            }
            return serviceUuidList;
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Reads the requested characteristic from the associated remote device.
     *
     * @deprecated Use {@link #gattClientReadCharacteristicByIndex(gattIndex,
     *     discoveredServiceListIndex, serviceIndex, characteristicIndex)} instead.
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param serviceIndex the service index of the discovered services
     * @param characteristicUuid the characteristic uuid to read
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Reads the requested characteristic from the associated remote device.")
    @Deprecated
    public boolean gattClientReadCharacteristic(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicUuid") String characteristicUuid) throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = gattServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        UUID cUuid = UUID.fromString(characteristicUuid);
        BluetoothGattCharacteristic gattCharacteristic = gattService.getCharacteristic(cUuid);
        if (gattCharacteristic == null) {
            throw new Exception("Invalid characteristic uuid: " + characteristicUuid);
        }
        return bluetoothGatt.readCharacteristic(gattCharacteristic);
    }

    /**
     * Reads the characteristic from the associated remote device.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param uuid the characteristic uuid to read
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Reads the characteristic from the associated remote device.")
    public boolean gattClientReadUsingCharacteristicUuid(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "uuid") String uuid,
            @RpcParameter(name = "startHandle") Integer startHandle,
            @RpcParameter(name = "endHandle") Integer endHandle) throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        UUID cUuid = UUID.fromString(uuid);
        return bluetoothGatt.readUsingCharacteristicUuid(cUuid, startHandle, endHandle);
    }

    /**
     * Reads the requested characteristic from the associated remote device.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param serviceIndex the service index of the discovered services
     * @param characteristicIndex the characteristic index
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Reads the requested characteristic from the associated remote device.")
    public boolean gattClientReadCharacteristicByIndex(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicIndex") Integer characteristicIndex)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = gattServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        List<BluetoothGattCharacteristic> charList = gattService.getCharacteristics();
        if (charList.get(characteristicIndex) == null) {
            throw new Exception("Invalid characteristicIndex " + characteristicIndex);
        }
        return bluetoothGatt.readCharacteristic(charList.get(characteristicIndex));
    }

    /**
     * Reads the requested characteristic from the associated remote device by instance id.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Reads the requested characteristic from the associated remote "
            + "device by instance id.")
    public boolean gattClientReadCharacteristicByInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "characteristicInstanceId") Integer characteristicInstanceId)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            List<BluetoothGattCharacteristic> charList = mGattService.getCharacteristics();
            for (BluetoothGattCharacteristic mGattChar : charList) {
                if (mGattChar.getInstanceId() == characteristicInstanceId) {
                    Log.i("Found Characteristic to read. instanceId: "
                        + Integer.toString(characteristicInstanceId)
                        + " UUID: " + mGattChar.getUuid().toString());
                    return bluetoothGatt.readCharacteristic(mGattChar);
                }
            }
        }
        Log.e("Failed to find Characteristic with instanceId: " + Integer.toString(
            characteristicInstanceId));
        return false;
    }

    /**
     * Writes the requested characteristic from the associated remote device by instance id.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param characteristicInstanceId the integer instance id of the Characteristic to write to
     * @param value the value to write to the characteristic
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Writes the requested characteristic from the associated remote "
            + "device by instance id.")
    public boolean gattClientWriteDescriptorByInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "descriptorInstanceId") Integer descriptorInstanceId,
            @RpcParameter(name = "value") byte[] value)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            List<BluetoothGattCharacteristic> charList = mGattService.getCharacteristics();
            for (BluetoothGattCharacteristic mGattChar : charList) {
                List<BluetoothGattDescriptor> descList = mGattChar.getDescriptors();
                for (BluetoothGattDescriptor mGattDesc : descList) {
                    if (mGattDesc.getInstanceId() == descriptorInstanceId) {
                        mGattDesc.setValue(value);
                        Log.i("Found Descriptor to write. instanceId: "
                            + Integer.toString(descriptorInstanceId)
                            + " UUID: " + mGattDesc.getUuid().toString());
                        return bluetoothGatt.writeDescriptor(mGattDesc);
                    }
                }
            }
        }
        Log.e("Failed to find Descriptor with instanceId: " + Integer.toString(
            descriptorInstanceId));
        return false;
    }

    /**
     * Writes the requested characteristic from the associated remote device by instance id.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param descriptorInstanceId the integer instance id of the Characteristic to write to
     * @param value the value to write to the characteristic
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Writes the requested characteristic from the associated remote "
            + "device by instance id.")
    public boolean gattClientWriteCharacteristicByInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "characteristicInstanceId") Integer characteristicInstanceId,
            @RpcParameter(name = "value") byte[] value)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            List<BluetoothGattCharacteristic> charList = mGattService.getCharacteristics();
            for (BluetoothGattCharacteristic mGattChar : charList) {
                if (mGattChar.getInstanceId() == characteristicInstanceId) {
                    mGattChar.setValue(value);
                    Log.i("Found Characteristic to write. instanceId: "
                        + Integer.toString(characteristicInstanceId)
                        + " UUID: " + mGattChar.getUuid().toString());
                    return bluetoothGatt.writeCharacteristic(mGattChar);
                }
            }
        }
        Log.e("Failed to find Characteristic with instanceId: " + Integer.toString(
            characteristicInstanceId));
        return false;
    }

    /**
     * Writes the requested characteristic in which write is not permitted. For conformance tests
     * only.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param descriptorInstanceId the integer instance id of the Characteristic to write to
     * @param value the value to write to the characteristic
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Writes the requested characteristic from the associated remote "
            + "device by instance id.")
    public boolean gattClientModifyAccessAndWriteCharacteristicByInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "characteristicInstanceId") Integer characteristicInstanceId,
            @RpcParameter(name = "value") byte[] value)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            List<BluetoothGattCharacteristic> charList = mGattService.getCharacteristics();
            for (BluetoothGattCharacteristic mGattChar : charList) {
                if (mGattChar.getInstanceId() == characteristicInstanceId) {
                    Log.i("Found Characteristic to write. instanceId: "
                        + Integer.toString(characteristicInstanceId)
                        + " UUID: " + mGattChar.getUuid().toString());
                    BluetoothGattCharacteristic modChar = new BluetoothGattCharacteristic(
                        mGattChar.getUuid(), 0x08, 0x10);
                    modChar.setInstanceId(mGattChar.getInstanceId());
                    mGattService.addCharacteristic(modChar);
                    modChar.setValue(value);
                    return bluetoothGatt.writeCharacteristic(modChar);
                }
            }
        }
        Log.e("Failed to find Characteristic with instanceId: " + Integer.toString(
            characteristicInstanceId));
        return false;
    }

    /**
     * Writes the requested descriptor in which write is not permitted. For conformance tests only.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param descriptorInstanceId the integer instance id of the Characteristic to write to
     * @param value the value to write to the characteristic
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Writes a Characteristic with an invalid instanceId to each service.")
    public boolean gattClientWriteInvalidCharacteristicByInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "instanceId") Integer instanceId,
            @RpcParameter(name = "value") byte[] value)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            BluetoothGattCharacteristic invalidHandleChar = new BluetoothGattCharacteristic(
                UUID.fromString("aa7edd5a-4d1d-4f0e-883a-d145616a1630"), 0x08, 0x10);
            invalidHandleChar.setInstanceId(instanceId);
            mGattService.addCharacteristic(invalidHandleChar);
            invalidHandleChar.setValue(value);
            //todo: this used to be return bluetoothGatt. Retest with and without return
            bluetoothGatt.writeCharacteristic(invalidHandleChar);
        }
        return true;
    }

    /**
     * Writes the requested characteristic in which write is not permitted. For conformance tests
     * only.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param descriptorInstanceId the integer instance id of the Characteristic to write to
     * @param value the value to write to the characteristic
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Read the requested characteristic from the associated remote "
            + "device by instance id.")
    public boolean gattClientModifyAccessAndReadCharacteristicByInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "characteristicInstanceId") Integer characteristicInstanceId)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            List<BluetoothGattCharacteristic> charList = mGattService.getCharacteristics();
            for (BluetoothGattCharacteristic mGattChar : charList) {
                if (mGattChar.getInstanceId() == characteristicInstanceId) {
                    Log.i("Found Characteristic to read. instanceId: "
                        + Integer.toString(characteristicInstanceId)
                        + " UUID: " + mGattChar.getUuid().toString());
                    BluetoothGattCharacteristic modChar = new BluetoothGattCharacteristic(
                        mGattChar.getUuid(), 0x02, 0x01);
                    modChar.setInstanceId(mGattChar.getInstanceId());
                    mGattService.addCharacteristic(modChar);
                    return bluetoothGatt.readCharacteristic(modChar);
                }
            }
        }
        Log.e("Failed to find Characteristic with instanceId: " + Integer.toString(
            characteristicInstanceId));
        return false;
    }

    /**
     * Writes the requested descriptor in which write is not permitted. For conformance tests only.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param descriptorInstanceId the integer instance id of the Characteristic to write to
     * @param value the value to write to the characteristic
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Read a Characteristic with an invalid instanceId to each service.")
    public boolean gattClientReadInvalidCharacteristicByInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "characteristicInstanceId") Integer characteristicInstanceId)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            BluetoothGattCharacteristic invalidHandleChar = new BluetoothGattCharacteristic(
                UUID.fromString("aa7edd5a-4d1d-4f0e-883a-d145616a1630"), 0x02, 0x01);
            invalidHandleChar.setInstanceId(characteristicInstanceId);
            mGattService.addCharacteristic(invalidHandleChar);
            bluetoothGatt.readCharacteristic(invalidHandleChar);
        }
        return true;
    }

    /**
     * Writes the requested descriptor in which write is not permitted. For conformance tests
     * only.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param descriptorInstanceId the integer instance id of the Characteristic to write to
     * @param value the value to write to the characteristic
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Writes the requested descriptor from the associated remote "
            + "device by instance id.")
    public boolean gattClientModifyAccessAndWriteDescriptorByInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "descriptorInstanceId") Integer descriptorInstanceId,
            @RpcParameter(name = "value") byte[] value)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            List<BluetoothGattCharacteristic> charList = mGattService.getCharacteristics();
            for (BluetoothGattCharacteristic mGattChar : charList) {
                for (BluetoothGattDescriptor mGattDesc : mGattChar.getDescriptors()) {
                    if (mGattDesc.getInstanceId() == descriptorInstanceId) {
                        Log.i("Found Descriptor to write. instanceId: "
                            + Integer.toString(descriptorInstanceId)
                            + " UUID: " + mGattChar.getUuid().toString());
                        BluetoothGattDescriptor modDesc = new BluetoothGattDescriptor(
                            mGattDesc.getUuid(), 0x10);
                        modDesc.setInstanceId(descriptorInstanceId);
                        mGattChar.addDescriptor(modDesc);
                        modDesc.setValue(value);
                        return bluetoothGatt.writeDescriptor(modDesc);
                    }
                }
            }
        }
        Log.e("Failed to find Descriptor with instanceId: " + Integer.toString(
            descriptorInstanceId));
        return false;
    }

    /**
     * Writes the requested descriptor in which write is not permitted. For conformance tests only.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param descriptorInstanceId the integer instance id of the Characteristic to write to
     * @param value the value to write to the characteristic
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Writes a Characteristic with an invalid instanceId to each service.")
    public boolean gattClientWriteInvalidDescriptorByInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "instanceId") Integer instanceId,
            @RpcParameter(name = "value") byte[] value)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            for (BluetoothGattCharacteristic mGattChar : mGattService.getCharacteristics()) {
                BluetoothGattDescriptor invalidHandleDesc = new BluetoothGattDescriptor(
                    UUID.fromString("aa7edd5a-4d1d-4f0e-883a-d145616a1630"), 0x10);
                invalidHandleDesc.setInstanceId(instanceId);
                mGattChar.addDescriptor(invalidHandleDesc);
                invalidHandleDesc.setValue(value);
                bluetoothGatt.writeDescriptor(invalidHandleDesc);
            }
        }
        return true;
    }

    /**
     * Writes the requested descriptor in which write is not permitted. For conformance tests
     * only.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param descriptorInstanceId the integer instance id of the Characteristic to write to
     * @param value the value to write to the characteristic
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Read the requested descriptor from the associated remote "
            + "device by instance id.")
    public boolean gattClientModifyAccessAndReadDescriptorByInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "descriptorInstanceId") Integer descriptorInstanceId)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            List<BluetoothGattCharacteristic> charList = mGattService.getCharacteristics();
            for (BluetoothGattCharacteristic mGattChar : charList) {
                for (BluetoothGattDescriptor mGattDesc : mGattChar.getDescriptors()) {
                    if (mGattDesc.getInstanceId() == descriptorInstanceId) {
                        Log.i("Found Descriptor to read. instanceId: "
                            + Integer.toString(descriptorInstanceId)
                            + " UUID: " + mGattDesc.getUuid().toString());
                        BluetoothGattDescriptor modDesc = new BluetoothGattDescriptor(
                            mGattDesc.getUuid(), 0x01);
                        modDesc.setInstanceId(descriptorInstanceId);
                        mGattChar.addDescriptor(modDesc);
                        return bluetoothGatt.readDescriptor(modDesc);
                    }
                }
            }
        }
        Log.e("Failed to find Descriptor with instanceId: " + Integer.toString(
            descriptorInstanceId));
        return false;
    }

    /**
     * Writes the requested descriptor in which write is not permitted. For conformance tests only.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param descriptorInstanceId the integer instance id of the Characteristic to write to
     * @param value the value to write to the characteristic
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Read a Characteristic with an invalid instanceId to each service.")
    public boolean gattClientReadInvalidDescriptorByInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "descriptorInstanceId") Integer descriptorInstanceId)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            for (BluetoothGattCharacteristic mGattChar : mGattService.getCharacteristics()) {
                BluetoothGattDescriptor invalidHandleDesc = new BluetoothGattDescriptor(
                    UUID.fromString("aa7edd5a-4d1d-4f0e-883a-d145616a1630"), 0x01);
                invalidHandleDesc.setInstanceId(descriptorInstanceId);
                mGattChar.addDescriptor(invalidHandleDesc);
                bluetoothGatt.readDescriptor(invalidHandleDesc);
            }
        }
        return true;
    }

    /**
     * Writes the requested characteristic in which write is not permitted. For conformance tests
     * only.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param descriptorInstanceId the integer instance id of the Characteristic to write to
     * @param value the value to write to the characteristic
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Read the requested characteristic from the associated remote "
            + "device by uuid.")
    public boolean gattClientModifyAccessAndReadCharacteristicByUuidAndInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "characteristicInstanceId") Integer characteristicInstanceId,
            @RpcParameter(name = "characteristicUuid") String characteristicUuid)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            List<BluetoothGattCharacteristic> charList = mGattService.getCharacteristics();
            for (BluetoothGattCharacteristic mGattChar : charList) {
                if (mGattChar.getUuid().toString().equalsIgnoreCase(characteristicUuid) &&
                        mGattChar.getInstanceId() == characteristicInstanceId) {
                    Log.i("Found Characteristic to read. UUID: " + mGattChar.getUuid().toString());
                    BluetoothGattCharacteristic modChar = new BluetoothGattCharacteristic(
                        mGattChar.getUuid(), 0x02, 0x01);
                    modChar.setInstanceId(characteristicInstanceId);
                    mGattService.addCharacteristic(modChar);
                    bluetoothGatt.readCharacteristic(modChar);
                }
            }
        }
        return true;
    }

    /**
     * Writes the requested descriptor in which write is not permitted. For conformance tests only.
     *
     * @param gattIndex the BluetoothGatt server accociated with the device
     * @param discoveredServiceListIndex the index returned from the discovered services callback
     * @param descriptorInstanceId the integer instance id of the Characteristic to write to
     * @param value the value to write to the characteristic
     * @return true, if the read operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Read a Characteristic with an invalid Uuid to each service.")
    public boolean gattClientReadInvalidCharacteristicByUuidAndInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "characteristicUuid") String characteristicUuid)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        for (BluetoothGattService mGattService : gattServiceList) {
            BluetoothGattCharacteristic invalidHandleChar = new BluetoothGattCharacteristic(
                UUID.fromString(characteristicUuid), 0x02, 0x01);
            mGattService.addCharacteristic(invalidHandleChar);
            bluetoothGatt.readCharacteristic(invalidHandleChar);
        }
        return true;
    }

    /**
     * /** Reads the value for a given descriptor from the associated remote device
     *
     * @deprecated Use {@link #gattClientReadDescriptorByIndex(
     * gattIndex, discoveredServiceListIndex, serviceIndex, characteristicIndex, descriptorIndex)}
     * instead.
     * @param gattIndex - the gatt index to use
     * @param discoveredServiceListIndex - the discvered serivice list index
     * @param serviceIndex - the servce index of the discoveredServiceListIndex
     * @param characteristicUuid - the characteristic uuid in which the descriptor is
     * @param descriptorUuid - the descriptor uuid to read
     * @return
     * @throws Exception
     */
    @Deprecated
    @Rpc(description = "Reads the value for a given descriptor from the associated remote device")
    public boolean gattClientReadDescriptor(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicUuid") String characteristicUuid,
            @RpcParameter(name = "descriptorUuid") String descriptorUuid) throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList = mBluetoothGattDiscoveredServicesList.get(
                      discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = gattServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        UUID cUuid = UUID.fromString(characteristicUuid);
        BluetoothGattCharacteristic gattCharacteristic = gattService.getCharacteristic(cUuid);
        if (gattCharacteristic == null) {
            throw new Exception("Invalid characteristic uuid: " + characteristicUuid);
        }
        UUID dUuid = UUID.fromString(descriptorUuid);
        BluetoothGattDescriptor gattDescriptor = gattCharacteristic.getDescriptor(dUuid);
        if (gattDescriptor == null) {
            throw new Exception("Invalid descriptor uuid: " + descriptorUuid);
        }
        return bluetoothGatt.readDescriptor(gattDescriptor);
    }


    /**
     * /** Reads the value for a given descriptor from the associated remote device
     *
     * @param gattIndex - the gatt index to use
     * @param discoveredServiceListIndex - the discvered serivice list index
     * @param serviceIndex - the servce index of the discoveredServiceListIndex
     * @param characteristicIndex - the characteristic index
     * @param descriptorIndex - the descriptor index to read
     * @return
     * @throws Exception
     */
    @Rpc(description = "Reads the value for a given descriptor from the associated remote device")
    public boolean gattClientReadDescriptorByIndex(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicIndex") Integer characteristicIndex,
            @RpcParameter(name = "descriptorIndex") Integer descriptorIndex)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = gattServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        List<BluetoothGattCharacteristic> charList = gattService.getCharacteristics();
        if (charList.get(characteristicIndex) == null) {
            throw new Exception("Invalid characteristicIndex " + characteristicIndex);
        }
        BluetoothGattCharacteristic gattCharacteristic = charList.get(characteristicIndex);
        List<BluetoothGattDescriptor> descList = gattCharacteristic.getDescriptors();
        if (descList.get(descriptorIndex) == null) {
            throw new Exception("Invalid descriptorIndex " + descriptorIndex);
        }
        return bluetoothGatt.readDescriptor(descList.get(descriptorIndex));
    }

    /**
     * Write the value of a given descriptor to the associated remote device
     *
     * @deprecated Use {@link #gattClientWriteDescriptorByIndex(
     * gattIndex, discoveredServiceListIndex, serviceIndex, characteristicIndex, descriptorIndex)}
     * instead.
     * @param index the bluetooth gatt index
     * @param serviceIndex the service index to write to
     * @param characteristicUuid the uuid where the descriptor lives
     * @param descriptorIndex the descriptor index
     * @return true, if the write operation was initiated successfully
     * @throws Exception
     */
    @Deprecated
    @Rpc(description = "Write the value of a given descriptor to the associated remote device")
    public boolean gattClientWriteDescriptor(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicUuid") String characteristicUuid,
            @RpcParameter(name = "descriptorUuid") String descriptorUuid) throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> discoveredServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (discoveredServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = discoveredServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        UUID cUuid = UUID.fromString(characteristicUuid);
        BluetoothGattCharacteristic gattCharacteristic = gattService.getCharacteristic(cUuid);
        if (gattCharacteristic == null) {
            throw new Exception("Invalid characteristic uuid: " + characteristicUuid);
        }
        UUID dUuid = UUID.fromString(descriptorUuid);
        BluetoothGattDescriptor gattDescriptor = gattCharacteristic.getDescriptor(dUuid);
        if (gattDescriptor == null) {
            throw new Exception("Invalid descriptor uuid: " + descriptorUuid);
        }
        return bluetoothGatt.writeDescriptor(gattDescriptor);
    }

    /**
     * Write the value of a given descriptor to the associated remote device
     *
     * @param index the bluetooth gatt index
     * @param serviceIndex the service index to write to
     * @param characteristicIndex the characteristic index to write to
     * @param descriptorIndex the descriptor index to write to
     * @return true, if the write operation was initiated successfully
     * @throws Exception
     */
    @Rpc(description = "Write the value of a given descriptor to the associated remote device")
    public boolean gattClientWriteDescriptorByIndex(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicIndex") Integer characteristicIndex,
            @RpcParameter(name = "descriptorIndex") Integer descriptorIndex)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = gattServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        List<BluetoothGattCharacteristic> charList = gattService.getCharacteristics();
        if (charList.get(characteristicIndex) == null) {
            throw new Exception("Invalid characteristicIndex " + characteristicIndex);
        }
        BluetoothGattCharacteristic gattCharacteristic = charList.get(characteristicIndex);
        List<BluetoothGattDescriptor> descList = gattCharacteristic.getDescriptors();
        if (descList.get(descriptorIndex) == null) {
            throw new Exception("Invalid descriptorIndex " + descriptorIndex);
        }
        return bluetoothGatt.writeDescriptor(descList.get(descriptorIndex));
    }

    /**
     * Write the value to a discovered descriptor.
     *
     * @deprecated Use {@link #gattClientDescriptorSetValueByIndex(
     *  gattIndex, discoveredServiceListIndex, serviceIndex, characteristicIndex, descriptorIndex,
     *  value)} instead.
     * @param gattIndex - the gatt index to use
     * @param discoveredServiceListIndex - the discovered service list index
     * @param serviceIndex - the service index of the discoveredServiceListIndex
     * @param characteristicUuid - the characteristic uuid in which the descriptor is
     * @param descriptorUuid - the descriptor uuid to read
     * @param value - the value to set the descriptor to
     * @return true is the value was set to the descriptor
     * @throws Exception
     */
    @Deprecated
    @Rpc(description = "Write the value of a given descriptor to the associated remote device")
    public boolean gattClientDescriptorSetValue(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicUuid") String characteristicUuid,
            @RpcParameter(name = "descriptorUuid") String descriptorUuid,
            @RpcParameter(name = "value") byte[] value) throws Exception {
        if (mBluetoothGattList.get(gattIndex) == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> discoveredServiceList =
                  mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (discoveredServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = discoveredServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        UUID cUuid = UUID.fromString(characteristicUuid);
        BluetoothGattCharacteristic gattCharacteristic = gattService.getCharacteristic(cUuid);
        if (gattCharacteristic == null) {
            throw new Exception("Invalid characteristic uuid: " + characteristicUuid);
        }
        UUID dUuid = UUID.fromString(descriptorUuid);
        BluetoothGattDescriptor gattDescriptor = gattCharacteristic.getDescriptor(dUuid);
        if (gattDescriptor == null) {
            throw new Exception("Invalid descriptor uuid: " + descriptorUuid);
        }
        return gattDescriptor.setValue(value);
    }

    /**
     * Write the value to a discovered descriptor.
     *
     * @param gattIndex - the gatt index to use
     * @param discoveredServiceListIndex - the discovered service list index
     * @param serviceIndex - the service index of the discoveredServiceListIndex
     * @param characteristicIndex - the characteristic index
     * @param descriptorIndex - the descriptor index to set
     * @param value - the value to set the descriptor to
     * @return true is the value was set to the descriptor
     * @throws Exception
     */
    @Rpc(description = "Write the value of a given descriptor to the associated remote device")
    public boolean gattClientDescriptorSetValueByIndex(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicIndex") Integer characteristicIndex,
            @RpcParameter(name = "descriptorIndex") Integer descriptorIndex,
            @RpcParameter(name = "value") byte[] value)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = gattServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        List<BluetoothGattCharacteristic> charList = gattService.getCharacteristics();
        if (charList.get(characteristicIndex) == null) {
            throw new Exception("Invalid characteristicIndex " + characteristicIndex);
        }
        BluetoothGattCharacteristic gattCharacteristic = charList.get(characteristicIndex);
        List<BluetoothGattDescriptor> descList = gattCharacteristic.getDescriptors();
        if (descList.get(descriptorIndex) == null) {
            throw new Exception("Invalid descriptorIndex " + descriptorIndex);
        }
        return descList.get(descriptorIndex).setValue(value);
    }


    /**
     * Write the value of a given characteristic to the associated remote device
     *
     * @deprecated Use {@link #gattClientWriteCharacteristicByIndex(
     *  gattIndex, discoveredServiceListIndex, serviceIndex, characteristicIndex)} instead.
     * @param index the bluetooth gatt index
     * @param serviceIndex the service where the characteristic lives
     * @param characteristicUuid the characteristic uuid to write to
     * @return true, if the write operation was successful
     * @throws Exception
     */
    @Deprecated
    @Rpc(description = "Write the value of a given characteristic to the associated remote device")
    public boolean gattClientWriteCharacteristic(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicUuid") String characteristicUuid) throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> discoveredServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (discoveredServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = discoveredServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        UUID cUuid = UUID.fromString(characteristicUuid);
        BluetoothGattCharacteristic gattCharacteristic = gattService.getCharacteristic(cUuid);
        if (gattCharacteristic == null) {
            throw new Exception("Invalid characteristic uuid: " + characteristicUuid);
        }
        return bluetoothGatt.writeCharacteristic(gattCharacteristic);
    }

    /**
     * Write the value of a given characteristic to the associated remote device
     *
     * @param index the bluetooth gatt index
     * @param serviceIndex the service where the characteristic lives
     * @param characteristicIndex the characteristic index
     * @return true, if the write operation was successful
     * @throws Exception
     */
    @Rpc(description = "Write the value of a given characteristic to the associated remote device")
    public boolean gattClientWriteCharacteristicByIndex(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicIndex") Integer characteristicIndex)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = gattServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        List<BluetoothGattCharacteristic> charList = gattService.getCharacteristics();
        if (charList.get(characteristicIndex) == null) {
            throw new Exception("Invalid characteristicIndex " + characteristicIndex);
        }
        return bluetoothGatt.writeCharacteristic(charList.get(characteristicIndex));
    }

    /**
     * PTS HELPER... Write the value of a given characteristic to the associated remote device
     *
     * @param index the bluetooth gatt index
     * @param serviceIndex the service where the characteristic lives
     * @param characteristicIndex the characteristic index
     * @return true, if the write operation was successful
     * @throws Exception
     */
    @Rpc(description = "Write the value of a given characteristic to the associated remote device")
    public boolean gattClientReadInvalidCharacteristicInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "instanceId") Integer instanceId)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = gattServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        List<BluetoothGattCharacteristic> charList = gattService.getCharacteristics();
        charList.get(0).setInstanceId(instanceId);
        return bluetoothGatt.readCharacteristic(charList.get(0));
    }

    /**
     * Get the input Characteristic's instance ID.
     *
     * @param index the bluetooth gatt index
     * @param serviceIndex the service where the characteristic lives
     * @param characteristicIndex the characteristic index
     * @return true, if the write operation was successful
     * @throws Exception
     */
    @Rpc(description = "Write the value of a given characteristic to the associated remote device")
    public Integer gattClientGetCharacteristicInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicIndex") Integer characteristicIndex)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = gattServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        List<BluetoothGattCharacteristic> charList = gattService.getCharacteristics();
        if (charList.get(characteristicIndex) == null) {
            throw new Exception("Invalid characteristicIndex " + characteristicIndex);
        }
        return charList.get(characteristicIndex).getInstanceId();
    }

    /**
     * Get the input Descriptor's instance ID.
     *
     * @param index the bluetooth gatt index
     * @param serviceIndex the service where the characteristic lives
     * @param characteristicIndex the characteristic index
     * @param descriptorIndex the descriptor index
     * @return true, if the write operation was successful
     * @throws Exception
     */
    @Rpc(description = "Write the value of a given characteristic to the associated remote device")
    public Integer gattClientGetDescriptorInstanceId(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicIndex") Integer characteristicIndex,
            @RpcParameter(name = "descriptorIndex") Integer descriptorIndex)
            throws Exception {
        BluetoothGatt bluetoothGatt = mBluetoothGattList.get(gattIndex);
        if (bluetoothGatt == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> gattServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (gattServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = gattServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        List<BluetoothGattCharacteristic> charList = gattService.getCharacteristics();
        if (charList.get(characteristicIndex) == null) {
            throw new Exception("Invalid characteristicIndex " + characteristicIndex);
        }
        BluetoothGattCharacteristic gattCharacteristic = charList.get(characteristicIndex);
        if (gattCharacteristic == null) {
            throw new Exception("Invalid characteristicIndex " + serviceIndex);
        }
        List<BluetoothGattDescriptor> descList = gattCharacteristic.getDescriptors();
        if (descList.get(descriptorIndex) == null) {
            throw new Exception("Invalid descriptorIndex " + descriptorIndex);
        }
        return descList.get(descriptorIndex).getInstanceId();
    }

    /**
     * Write the value to a discovered characteristic.
     *
     * @deprecated Use {@link #gattClientCharacteristicSetValueByIndex(
     *  gattIndex, discoveredServiceListIndex, serviceIndex, characteristicIndex, value)} instead.
     * @param gattIndex - the gatt index to use
     * @param discoveredServiceListIndex - the discovered service list index
     * @param serviceIndex - the service index of the discoveredServiceListIndex
     * @param characteristicUuid - the characteristic uuid in which the descriptor is
     * @param value - the value to set the characteristic to
     * @return true, if the value was set to the characteristic
     * @throws Exception
     */
    @Deprecated
    @Rpc(description = "Write the value of a given characteristic to the associated remote device")
    public boolean gattClientCharacteristicSetValue(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicUuid") String characteristicUuid,
            @RpcParameter(name = "value") byte[] value) throws Exception {
        if (mBluetoothGattList.get(gattIndex) == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> discoveredServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (discoveredServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = discoveredServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        UUID cUuid = UUID.fromString(characteristicUuid);
        BluetoothGattCharacteristic gattCharacteristic = gattService.getCharacteristic(cUuid);
        if (gattCharacteristic == null) {
            throw new Exception("Invalid characteristic uuid: " + characteristicUuid);
        }
        return gattCharacteristic.setValue(value);
    }

    /**
     * Write the value to a discovered characteristic.
     *
     * @param gattIndex - the gatt index to use
     * @param discoveredServiceListIndex - the discovered service list index
     * @param serviceIndex - the service index of the discoveredServiceListIndex
     * @param characteristicIndex - the characteristic index
     * @param value - the value to set the characteristic to
     * @return true, if the value was set to the characteristic
     * @throws Exception
     */
    @Rpc(description = "Write the value of a given characteristic to the associated remote device")
    public boolean gattClientCharacteristicSetValueByIndex(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicIndex") Integer characteristicIndex,
            @RpcParameter(name = "value") byte[] value)
            throws Exception {
        if (mBluetoothGattList.get(gattIndex) == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> discoveredServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (discoveredServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = discoveredServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        List<BluetoothGattCharacteristic> charList = gattService.getCharacteristics();
        if (charList.get(characteristicIndex) == null) {
            throw new Exception("Invalid characteristicIndex: " + characteristicIndex);
        }
        return charList.get(characteristicIndex).setValue(value);
    }

    /**
     * Set write type to a discovered characteristic.
     *
     * @deprecated Use {@link #gattClientCharacteristicSetWriteTypeByIndex(
     *  gattIndex, discoveredServiceListIndex, serviceIndex, ccharacteristicUuid, writeType)}
     * instead.
     * @param gattIndex - the gatt index to use
     * @param discoveredServiceListIndex - the discovered service list index
     * @param serviceIndex - the service index of the discoveredServiceListIndex
     * @param characteristicUuid - the characteristic uuid in which the descriptor is
     * @param writeType - the write type for characteristic
     * @return true, if the value was set to the characteristic
     * @throws Exception
     */
    @Deprecated
    @Rpc(description = "Set write type of a given characteristic to the associated remote device")
    public boolean gattClientCharacteristicSetWriteType(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicUuid") String characteristicUuid,
            @RpcParameter(name = "writeType") Integer writeType) throws Exception {
        if (mBluetoothGattList.get(gattIndex) == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> discoveredServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (discoveredServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = discoveredServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        UUID cUuid = UUID.fromString(characteristicUuid);
        BluetoothGattCharacteristic gattCharacteristic = gattService.getCharacteristic(cUuid);
        if (gattCharacteristic == null) {
            throw new Exception("Invalid characteristic uuid: " + characteristicUuid);
        }
        gattCharacteristic.setWriteType(writeType);
        return true;
    }

    /**
     * Set write type to a discovered characteristic.
     *
     * @param gattIndex - the gatt index to use
     * @param discoveredServiceListIndex - the discovered service list index
     * @param serviceIndex - the service index of the discoveredServiceListIndex
     * @param characteristicIndex - the characteristic index
     * @param writeType - the write type for characteristic
     * @return true, if the value was set to the characteristic
     * @throws Exception
     */
    @Rpc(description = "Set write type of a given characteristic to the associated remote device")
    public void gattClientCharacteristicSetWriteTypeByIndex(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicIndex") Integer characteristicIndex,
            @RpcParameter(name = "writeType") Integer writeType)
            throws Exception {
        if (mBluetoothGattList.get(gattIndex) == null) {
            throw new Exception("Invalid gattIndex " + gattIndex);
        }
        List<BluetoothGattService> discoveredServiceList =
                mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
        if (discoveredServiceList == null) {
            throw new Exception("Invalid discoveredServiceListIndex " + discoveredServiceListIndex);
        }
        BluetoothGattService gattService = discoveredServiceList.get(serviceIndex);
        if (gattService == null) {
            throw new Exception("Invalid serviceIndex " + serviceIndex);
        }
        List<BluetoothGattCharacteristic> charList = gattService.getCharacteristics();
        if (charList.get(characteristicIndex) == null) {
            throw new Exception("Invalid characteristicIndex: " + characteristicIndex);
        }
        charList.get(characteristicIndex).setWriteType(writeType);
    }

    /**
     * Read the RSSI for a connected remote device
     *
     * @param index the bluetooth gatt index
     * @return true, if the RSSI value has been requested successfully
     * @throws Exception
     */
    @Rpc(description = "Read the RSSI for a connected remote device")
    public boolean gattClientReadRSSI(
            @RpcParameter(name = "index") Integer index) throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            return mBluetoothGattList.get(index).readRemoteRssi();
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Clears the internal cache and forces a refresh of the services from the remote device
     *
     * @param index the bluetooth gatt index
     * @return Clears the internal cache and forces a refresh of the services from the remote
     *         device.
     * @throws Exception
     */
    @Rpc(description = "Clears the internal cache and forces a refresh of the services from the "
            + "remote device")
    public boolean gattClientRefresh(@RpcParameter(name = "index") Integer index) throws Exception {
        if (mBluetoothGattList.get(index) != null) {
            return mBluetoothGattList.get(index).refresh();
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Request a connection parameter update.
     *
     * @param index the bluetooth gatt index
     * @param connectionPriority connection priority
     * @return boolean True if successful False otherwise.
     * @throws Exception
     */
    @Rpc(description = "Request a connection parameter update. from the Bluetooth Gatt")
    public boolean gattClientRequestConnectionPriority(
            @RpcParameter(name = "index") Integer index,
            @RpcParameter(name = "connectionPriority") Integer connectionPriority)
            throws Exception {
        boolean result = false;
        if (mBluetoothGattList.get(index) != null) {
            result = mBluetoothGattList.get(index).requestConnectionPriority(connectionPriority);
        } else {
            throw new Exception("Invalid index input:" + index);
        }
        return result;
    }

    /**
     * Request a connection parameter update for Connection Interval.
     *
     * @param index the bluetooth gatt index
     * @param minConnectionInterval minimum connection interval
     * @param maxConnectionInterval maximum connection interval
     * @param slaveLatency maximum slave latency
     * @param supervisionTimeout supervision timeout
     * @return boolean True if successful False otherwise.
     * @throws Exception
     */
    @Rpc(description = "Request an LE connection parameters update.")
    public boolean gattClientRequestLeConnectionParameters(
            @RpcParameter(name = "index") Integer index,
            @RpcParameter(name = "minConnectionInterval") Integer minConnectionInterval,
            @RpcParameter(name = "maxConnectionInterval") Integer maxConnectionInterval,
            @RpcParameter(name = "slaveLatency") Integer slaveLatency,
            @RpcParameter(name = "supervisionTimeout") Integer supervisionTimeout,
            @RpcParameter(name = "minConnectionEventLen") Integer minConnectionEventLen,
            @RpcParameter(name = "maxConnectionEventLen") Integer maxConnectionEventLen)
            throws Exception {
        boolean result = false;
        if (mBluetoothGattList.get(index) != null) {
            result = mBluetoothGattList.get(index).requestLeConnectionUpdate(
                minConnectionInterval, maxConnectionInterval, slaveLatency, supervisionTimeout,
                minConnectionEventLen, maxConnectionEventLen);
        } else {
            throw new Exception("Invalid index input:" + index);
        }
        return result;
    }

    /**
     * Sets the characteristic notification of a bluetooth gatt
     *
     * @deprecated Use {@link #gattClientSetCharacteristicNotificationByIndex(
     *  gattIndex, discoveredServiceListIndex, serviceIndex, characteristicIndex, enable)} instead.
     * @param index the bluetooth gatt index
     * @param characteristicIndex the characteristic index
     * @param enable Enable or disable notifications/indications for a given characteristic
     * @return true, if the requested notification status was set successfully
     * @throws Exception
     */
    @Deprecated
    @Rpc(description = "Sets the characteristic notification of a bluetooth gatt")
    public boolean gattClientSetCharacteristicNotification(
            @RpcParameter(name = "gattIndex")
            Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex")
            Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex")
            Integer serviceIndex,
            @RpcParameter(name = "characteristicUuid")
            String characteristicUuid,
            @RpcParameter(name = "enable")
            Boolean enable
            ) throws Exception {
        if (mBluetoothGattList.get(gattIndex) != null) {
            if(mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex) != null) {
                List<BluetoothGattService> discoveredServiceList =
                    mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
                if (discoveredServiceList.get(serviceIndex) != null) {
                    UUID cUuid = UUID.fromString(characteristicUuid);
                    if (discoveredServiceList.get(serviceIndex).getCharacteristic(cUuid) != null) {
                        return mBluetoothGattList.get(gattIndex).setCharacteristicNotification(
                                discoveredServiceList.get(serviceIndex).getCharacteristic(cUuid), enable);
                    } else {
                        throw new Exception ("Invalid characteristic uuid: " + characteristicUuid);
                    }
                } else {
                    throw new Exception ("Invalid serviceIndex " + serviceIndex);
                }
            } else {
                throw new Exception("Invalid discoveredServiceListIndex: " + discoveredServiceListIndex);
            }
        } else {
            throw new Exception("Invalid gattIndex input: " + gattIndex);
        }
    }

    /**
     * Sets the characteristic notification of a bluetooth gatt
     *
     * @param gattIndex the bluetooth gatt index
     * @param characteristicIndex the characteristic index
     * @param enable Enable or disable notifications/indications for a given characteristic
     * @return true, if the requested notification status was set successfully
     * @throws Exception
     */
    @Rpc(description = "Sets the characteristic notification of a bluetooth gatt")
    public boolean gattClientSetCharacteristicNotificationByIndex(
            @RpcParameter(name = "gattIndex") Integer gattIndex,
            @RpcParameter(name = "discoveredServiceListIndex") Integer discoveredServiceListIndex,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicIndex") Integer characteristicIndex,
            @RpcParameter(name = "enable") Boolean enable)
            throws Exception {
        if (mBluetoothGattList.get(gattIndex) != null) {
            if (mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex) != null) {
                List<BluetoothGattService> discoveredServiceList =
                        mBluetoothGattDiscoveredServicesList.get(discoveredServiceListIndex);
                if (discoveredServiceList.get(serviceIndex) != null) {
                    List<BluetoothGattCharacteristic> charList =
                            discoveredServiceList.get(serviceIndex).getCharacteristics();
                    if (charList.get(characteristicIndex) != null) {
                        return mBluetoothGattList
                                .get(gattIndex)
                                .setCharacteristicNotification(charList.get(characteristicIndex),
                                    enable);
                    } else {
                        throw new Exception("Invalid characteristicIndex: " + characteristicIndex);
                    }
                } else {
                    throw new Exception("Invalid serviceIndex " + serviceIndex);
                }
            } else {
                throw new Exception(
                    "Invalid discoveredServiceListIndex: " + discoveredServiceListIndex);
            }
        } else {
            throw new Exception("Invalid gattIndex input: " + gattIndex);
        }
    }

    /**
     * Create a new GattCallback object
     *
     * @return the index of the callback object
     */
    @Rpc(description = "Create a new GattCallback object")
    public Integer gattCreateGattCallback() {
        GattCallbackCount += 1;
        int index = GattCallbackCount;
        mGattCallbackList.put(index, new myBluetoothGattCallback(index));
        return index;
    }

    /**
     * Returns the list of discovered Bluetooth Gatt Services.
     *
     * @throws Exception
     */
    @Rpc(description = "Get Bluetooth Gatt Services")
    public int gattClientGetDiscoveredServicesCount(@RpcParameter(name = "index") Integer index)
            throws Exception {
        if (mBluetoothGattDiscoveredServicesList.get(index) != null) {
            return mBluetoothGattDiscoveredServicesList.get(index).size();
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Returns the discovered Bluetooth Gatt Service Uuid.
     *
     * @throws Exception
     */
    @Rpc(description = "Get Bluetooth Gatt Service Uuid")
    public String gattClientGetDiscoveredServiceUuid(
            @RpcParameter(name = "index") Integer index,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex)
            throws Exception {
        List<BluetoothGattService> mBluetoothServiceList =
                mBluetoothGattDiscoveredServicesList.get(index);
        if (mBluetoothServiceList != null) {
            return mBluetoothServiceList.get(serviceIndex).getUuid().toString();
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Get discovered characteristic uuids from the pheripheral device.
     *
     * @param index the index of the bluetooth gatt discovered services list
     * @param serviceIndex the service to get
     * @return the list of characteristic uuids
     * @throws Exception
     */
    @Rpc(description = "Get Bluetooth Gatt Services")
    public ArrayList<String> gattClientGetDiscoveredCharacteristicUuids(
            @RpcParameter(name = "index") Integer index,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex)
            throws Exception {
        if (mBluetoothGattDiscoveredServicesList.get(index) != null) {
            if (mBluetoothGattDiscoveredServicesList.get(index).get(serviceIndex) != null) {
                ArrayList<String> uuidList = new ArrayList<String>();
                List<BluetoothGattCharacteristic> charList =
                        mBluetoothGattDiscoveredServicesList.get(index).get(
                            serviceIndex).getCharacteristics();
                for (BluetoothGattCharacteristic mChar : charList) {
                    uuidList.add(mChar.getUuid().toString());
                }
                return uuidList;
            } else {
                throw new Exception("Invalid serviceIndex input:" + index);
            }
        } else {
            throw new Exception("Invalid index input:" + index);
        }
    }

    /**
     * Get discovered descriptor uuids from the pheripheral device.
     *
     * @deprecated Use {@link #gattClientGetDiscoveredDescriptorUuidsByIndex(
     *  index, serviceIndex, characteristicIndex)} instead.
     * @param index the discovered services list index
     * @param serviceIndex the service index of the discovered services list
     * @param characteristicUuid the characteristicUuid to select from the
     * discovered service which contains the list of descriptors.
     * @return the list of descriptor uuids
     * @throws Exception
     */
    @Deprecated
    @Rpc(description = "Get Bluetooth Gatt Services")
    public ArrayList<String> gattClientGetDiscoveredDescriptorUuids (
            @RpcParameter(name = "index")
            Integer index,
            @RpcParameter(name = "serviceIndex")
            Integer serviceIndex,
            @RpcParameter(name = "characteristicUuid")
            String characteristicUuid
            ) throws Exception {
        if (mBluetoothGattDiscoveredServicesList.get(index) != null) {
            if (mBluetoothGattDiscoveredServicesList.get(index).get(serviceIndex) != null) {
                BluetoothGattService service = mBluetoothGattDiscoveredServicesList.get(
                        index).get(serviceIndex);
                UUID cUuid = UUID.fromString(characteristicUuid);
                if (service.getCharacteristic(cUuid) != null) {
                    ArrayList<String> uuidList = new ArrayList<String>();
                    for (BluetoothGattDescriptor mDesc : service.getCharacteristic(
                            cUuid).getDescriptors()) {
                        uuidList.add(mDesc.getUuid().toString());
                    }
                    return uuidList;
                } else {
                    throw new Exception("Invalid characeristicUuid : "
                            + characteristicUuid);
                }
            } else {
                throw new Exception("Invalid serviceIndex input:"
                        + index);
            }
        } else {
            throw new Exception("Invalid index input:"
                    + index);
        }
    }

    /**
     * Get discovered descriptor uuids from the pheripheral device.
     *
     * @param index the discovered services list index
     * @param serviceIndex the service index of the discovered services list
     * @param characteristicIndex the characteristicIndex to select from the discovered service
     *       which contains the list of descriptors.
     * @return the list of descriptor uuids
     * @throws Exception
     */
    @Rpc(description = "Get Bluetooth Gatt Services")
    public ArrayList<String> gattClientGetDiscoveredDescriptorUuidsByIndex(
            @RpcParameter(name = "index") Integer index,
            @RpcParameter(name = "serviceIndex") Integer serviceIndex,
            @RpcParameter(name = "characteristicIndex") Integer characteristicIndex)
            throws Exception {
        if (mBluetoothGattDiscoveredServicesList.get(index) == null) {
            throw new Exception("Invalid index: " + index);
        }
        if (mBluetoothGattDiscoveredServicesList.get(index).get(serviceIndex) == null) {
            throw new Exception("Invalid serviceIndex: " + serviceIndex);
        }
        List<BluetoothGattCharacteristic> charList =
                mBluetoothGattDiscoveredServicesList.get(index).get(
                    serviceIndex).getCharacteristics();
        if (charList.get(characteristicIndex) == null) {
            throw new Exception("Invalid characteristicIndex: " + characteristicIndex);
        }
        List<BluetoothGattDescriptor> descList = charList.get(characteristicIndex).getDescriptors();
        ArrayList<String> uuidList = new ArrayList<String>();
        for (BluetoothGattDescriptor mDesc : descList) {
            uuidList.add(mDesc.getUuid().toString());
        }
        return uuidList;
    }

    private class myBluetoothGattCallback extends BluetoothGattCallback {
        private final Bundle mResults;
        private final int index;
        private final String mEventType;

        public myBluetoothGattCallback(int idx) {
            mResults = new Bundle();
            mEventType = "GattConnect";
            index = idx;
        }

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            Log.d("gatt_connect change onConnectionStateChange " + mEventType + " " + index);
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                Log.d(
                        "State Connected to mac address "
                                + gatt.getDevice().getAddress()
                                + " status "
                                + status);
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                Log.d(
                        "State Disconnected from mac address "
                                + gatt.getDevice().getAddress()
                                + " status "
                                + status);
            } else if (newState == BluetoothProfile.STATE_CONNECTING) {
                Log.d(
                        "State Connecting to mac address "
                                + gatt.getDevice().getAddress()
                                + " status "
                                + status);
            } else if (newState == BluetoothProfile.STATE_DISCONNECTING) {
                Log.d(
                        "State Disconnecting from mac address "
                                + gatt.getDevice().getAddress()
                                + " status "
                                + status);
            }
            mResults.putInt("Status", status);
            mResults.putInt("State", newState);
            mEventFacade.postEvent(
                    mEventType + index + "onConnectionStateChange", mResults.clone());
            mResults.clear();
        }

        @Override
        public void onPhyRead(BluetoothGatt gatt, int txPhy, int rxPhy, int status) {
            Log.d("gatt_connect change onPhyRead " + mEventType + " " + index);
            mResults.putInt("TxPhy", txPhy);
            mResults.putInt("RxPhy", rxPhy);
            mResults.putInt("Status", status);
            mEventFacade.postEvent(mEventType + index + "onPhyRead", mResults.clone());
            mResults.clear();
        }

        @Override
        public void onPhyUpdate(BluetoothGatt gatt, int txPhy, int rxPhy, int status) {
            Log.d("gatt_connect change onPhyUpdate " + mEventType + " " + index);
            mResults.putInt("TxPhy", txPhy);
            mResults.putInt("RxPhy", rxPhy);
            mResults.putInt("Status", status);
            mEventFacade.postEvent(mEventType + index + "onPhyUpdate", mResults.clone());
            mResults.clear();
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            Log.d("gatt_connect change onServicesDiscovered " + mEventType + " " + index);
            int idx = BluetoothGattDiscoveredServicesCount++;
            mBluetoothGattDiscoveredServicesList.put(idx, gatt.getServices());
            mResults.putInt("ServicesIndex", idx);
            mResults.putInt("Status", status);
            mEventFacade.postEvent(mEventType + index + "onServicesDiscovered", mResults.clone());
            mResults.clear();
        }

        @Override
        public void onCharacteristicRead(
                BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            Log.d("gatt_connect change onCharacteristicRead " + mEventType + " " + index);
            mResults.putInt("Status", status);
            mResults.putString("CharacteristicUuid", characteristic.getUuid().toString());
            mResults.putByteArray("CharacteristicValue", characteristic.getValue());
            mEventFacade.postEvent(mEventType + index + "onCharacteristicRead", mResults.clone());
            mResults.clear();
        }

        @Override
        public void onCharacteristicWrite(
                BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            Log.d("gatt_connect change onCharacteristicWrite " + mEventType + " " + index);
            mResults.putInt("Status", status);
            mResults.putString("CharacteristicUuid", characteristic.getUuid().toString());
            mResults.putByteArray("CharacteristicValue", characteristic.getValue());
            mEventFacade.postEvent(mEventType + index + "onCharacteristicWrite", mResults.clone());
            mResults.clear();
        }

        @Override
        public void onCharacteristicChanged(
                BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            Log.d("gatt_connect change onCharacteristicChanged " + mEventType + " " + index);
            mResults.putInt("ID", index);
            mResults.putString("CharacteristicUuid", characteristic.getUuid().toString());
            mResults.putByteArray("CharacteristicValue", characteristic.getValue());
            mEventFacade.postEvent(
                    mEventType + index + "onCharacteristicChanged", mResults.clone());
            mResults.clear();
        }

        @Override
        public void onDescriptorRead(
                BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
            Log.d("gatt_connect change onServicesDiscovered " + mEventType + " " + index);
            mResults.putInt("Status", status);
            mResults.putString("DescriptorUuid", descriptor.getUuid().toString());
            mEventFacade.postEvent(mEventType + index + "onDescriptorRead", mResults.clone());
            mResults.clear();
        }

        @Override
        public void onDescriptorWrite(
                BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
            Log.d("gatt_connect change onDescriptorWrite " + mEventType + " " + index);
            mResults.putInt("ID", index);
            mResults.putInt("Status", status);
            mResults.putString("DescriptorUuid", descriptor.getUuid().toString());
            mEventFacade.postEvent(mEventType + index + "onDescriptorWrite", mResults.clone());
            mResults.clear();
        }

        @Override
        public void onReliableWriteCompleted(BluetoothGatt gatt, int status) {
            Log.d("gatt_connect change onReliableWriteCompleted " + mEventType + " " + index);
            mResults.putInt("Status", status);
            mEventFacade.postEvent(
                    mEventType + index + "onReliableWriteCompleted", mResults.clone());
            mResults.clear();
        }

        @Override
        public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {
            Log.d("gatt_connect change onReadRemoteRssi " + mEventType + " " + index);
            mResults.putInt("Status", status);
            mResults.putInt("Rssi", rssi);
            mEventFacade.postEvent(mEventType + index + "onReadRemoteRssi", mResults.clone());
            mResults.clear();
        }

        @Override
        public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
            Log.d("gatt_connect change onMtuChanged " + mEventType + " " + index);
            mResults.putInt("Status", status);
            mResults.putInt("MTU", mtu);
            mEventFacade.postEvent(mEventType + index + "onMtuChanged", mResults.clone());
            mResults.clear();
        }

        public void onConnectionUpdated(BluetoothGatt gatt, int interval, int latency,
                                            int timeout, int status) {
            Log.d("gatt_connect change onConnectionUpdated " + mEventType + " " + index
                    + ", interval: " + interval + ", latency: " + latency
                    + ", timeout: " + timeout + ", status: " + status);

            mResults.putInt("Status", status);
            mResults.putInt("Interval", interval);
            mResults.putInt("Latency", latency);
            mResults.putInt("Timeout", timeout);
            mEventFacade.postEvent(mEventType + index + "onConnectionUpdated", mResults.clone());
            mResults.clear();
        }
    }

    @Override
    public void shutdown() {
        if (!mBluetoothGattList.isEmpty()) {
            if (mBluetoothGattList.values() != null) {
                for (BluetoothGatt mBluetoothGatt : mBluetoothGattList.values()) {
                    mBluetoothGatt.close();
                }
            }
        }
        mGattCallbackList.clear();
    }
}
