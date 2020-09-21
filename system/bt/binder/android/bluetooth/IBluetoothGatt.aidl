/*
 * Copyright 2013 The Android Open Source Project
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

package android.bluetooth;

import android.app.PendingIntent;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertisingSetParameters;
import android.bluetooth.le.PeriodicAdvertisingParameters;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.bluetooth.le.ResultStorageDescriptor;
import android.os.ParcelUuid;
import android.os.WorkSource;

import android.bluetooth.IBluetoothGattCallback;
import android.bluetooth.IBluetoothGattServerCallback;
import android.bluetooth.le.IAdvertisingSetCallback;
import android.bluetooth.le.IPeriodicAdvertisingCallback;
import android.bluetooth.le.IScannerCallback;

/**
 * API for interacting with BLE / GATT
 * @hide
 */
interface IBluetoothGatt {
    List<BluetoothDevice> getDevicesMatchingConnectionStates(in int[] states);

    void registerScanner(in IScannerCallback callback, in WorkSource workSource);
    void unregisterScanner(in int scannerId);
    void startScan(in int scannerId, in ScanSettings settings, in List<ScanFilter> filters,
                   in List scanStorages, in String callingPackage);
    void startScanForIntent(in PendingIntent intent, in ScanSettings settings, in List<ScanFilter> filters,
                            in String callingPackage);
    void stopScanForIntent(in PendingIntent intent, in String callingPackage);
    void stopScan(in int scannerId);
    void flushPendingBatchResults(in int scannerId);

    void startAdvertisingSet(in AdvertisingSetParameters parameters, in AdvertiseData advertiseData,
                                in AdvertiseData scanResponse, in PeriodicAdvertisingParameters periodicParameters,
                                in AdvertiseData periodicData, in int duration, in int maxExtAdvEvents,
                                in IAdvertisingSetCallback callback);
    void stopAdvertisingSet(in IAdvertisingSetCallback callback);

    void getOwnAddress(in int advertiserId);
    void enableAdvertisingSet(in int advertiserId, in boolean enable, in int duration, in int maxExtAdvEvents);
    void setAdvertisingData(in int advertiserId, in AdvertiseData data);
    void setScanResponseData(in int advertiserId, in AdvertiseData data);
    void setAdvertisingParameters(in int advertiserId, in AdvertisingSetParameters parameters);
    void setPeriodicAdvertisingParameters(in int advertiserId, in PeriodicAdvertisingParameters parameters);
    void setPeriodicAdvertisingData(in int advertiserId, in AdvertiseData data);
    void setPeriodicAdvertisingEnable(in int advertiserId, in boolean enable);

    void registerSync(in ScanResult scanResult, in int skip, in int timeout, in IPeriodicAdvertisingCallback callback);
    void unregisterSync(in IPeriodicAdvertisingCallback callback);

    void registerClient(in ParcelUuid appId, in IBluetoothGattCallback callback);

    void unregisterClient(in int clientIf);
    void clientConnect(in int clientIf, in String address, in boolean isDirect, in int transport, in boolean opportunistic, in int phy);
    void clientDisconnect(in int clientIf, in String address);
    void clientSetPreferredPhy(in int clientIf, in String address, in int txPhy, in int rxPhy, in int phyOptions);
    void clientReadPhy(in int clientIf, in String address);
    void refreshDevice(in int clientIf, in String address);
    void discoverServices(in int clientIf, in String address);
    void discoverServiceByUuid(in int clientIf, in String address, in ParcelUuid uuid);
    void readCharacteristic(in int clientIf, in String address, in int handle, in int authReq);
    void readUsingCharacteristicUuid(in int clientIf, in String address, in ParcelUuid uuid,
                           in int startHandle, in int endHandle, in int authReq);
    void writeCharacteristic(in int clientIf, in String address, in int handle,
                            in int writeType, in int authReq, in byte[] value);
    void readDescriptor(in int clientIf, in String address, in int handle, in int authReq);
    void writeDescriptor(in int clientIf, in String address, in int handle,
                            in int authReq, in byte[] value);
    void registerForNotification(in int clientIf, in String address, in int handle, in boolean enable);
    void beginReliableWrite(in int clientIf, in String address);
    void endReliableWrite(in int clientIf, in String address, in boolean execute);
    void readRemoteRssi(in int clientIf, in String address);
    void configureMTU(in int clientIf, in String address, in int mtu);
    void connectionParameterUpdate(in int clientIf, in String address, in int connectionPriority);
    void leConnectionUpdate(int clientIf, String address, int minInterval,
                            int maxInterval, int slaveLatency, int supervisionTimeout,
                            int minConnectionEventLen, int maxConnectionEventLen);

    void registerServer(in ParcelUuid appId, in IBluetoothGattServerCallback callback);
    void unregisterServer(in int serverIf);
    void serverConnect(in int serverIf, in String address, in boolean isDirect, in int transport);
    void serverDisconnect(in int serverIf, in String address);
    void serverSetPreferredPhy(in int clientIf, in String address, in int txPhy, in int rxPhy, in int phyOptions);
    void serverReadPhy(in int clientIf, in String address);
    void addService(in int serverIf, in BluetoothGattService service);
    void removeService(in int serverIf, in int handle);
    void clearServices(in int serverIf);
    void sendResponse(in int serverIf, in String address, in int requestId,
                            in int status, in int offset, in byte[] value);
    void sendNotification(in int serverIf, in String address, in int handle,
                            in boolean confirm, in byte[] value);
    void disconnectAll();
    void unregAll();
    int numHwTrackFiltersAvailable();
}
