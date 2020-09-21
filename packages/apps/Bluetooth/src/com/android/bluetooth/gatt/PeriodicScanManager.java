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

package com.android.bluetooth.gatt;

import android.bluetooth.le.IPeriodicAdvertisingCallback;
import android.bluetooth.le.PeriodicAdvertisingReport;
import android.bluetooth.le.ScanRecord;
import android.bluetooth.le.ScanResult;
import android.os.IBinder;
import android.os.IInterface;
import android.os.RemoteException;
import android.util.Log;

import com.android.bluetooth.btservice.AdapterService;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Manages Bluetooth LE Periodic scans
 *
 * @hide
 */
class PeriodicScanManager {
    private static final boolean DBG = GattServiceConfig.DBG;
    private static final String TAG = GattServiceConfig.TAG_PREFIX + "SyncManager";

    private final AdapterService mAdapterService;
    Map<IBinder, SyncInfo> mSyncs = Collections.synchronizedMap(new HashMap<>());
    static int sTempRegistrationId = -1;

    /**
     * Constructor of {@link SyncManager}.
     */
    PeriodicScanManager(AdapterService adapterService) {
        if (DBG) {
            Log.d(TAG, "advertise manager created");
        }
        mAdapterService = adapterService;
    }

    void start() {
        initializeNative();
    }

    void cleanup() {
        if (DBG) {
            Log.d(TAG, "cleanup()");
        }
        cleanupNative();
        mSyncs.clear();
        sTempRegistrationId = -1;
    }

    class SyncInfo {
        /* When id is negative, the registration is ongoing. When the registration finishes, id
         * becomes equal to sync_handle */
        public Integer id;
        public SyncDeathRecipient deathRecipient;
        public IPeriodicAdvertisingCallback callback;

        SyncInfo(Integer id, SyncDeathRecipient deathRecipient,
                IPeriodicAdvertisingCallback callback) {
            this.id = id;
            this.deathRecipient = deathRecipient;
            this.callback = callback;
        }
    }

    IBinder toBinder(IPeriodicAdvertisingCallback e) {
        return ((IInterface) e).asBinder();
    }

    class SyncDeathRecipient implements IBinder.DeathRecipient {
        public IPeriodicAdvertisingCallback callback;

        SyncDeathRecipient(IPeriodicAdvertisingCallback callback) {
            this.callback = callback;
        }

        @Override
        public void binderDied() {
            if (DBG) {
                Log.d(TAG, "Binder is dead - unregistering advertising set");
            }
            stopSync(callback);
        }
    }

    Map.Entry<IBinder, SyncInfo> findSync(int syncHandle) {
        Map.Entry<IBinder, SyncInfo> entry = null;
        for (Map.Entry<IBinder, SyncInfo> e : mSyncs.entrySet()) {
            if (e.getValue().id == syncHandle) {
                entry = e;
                break;
            }
        }
        return entry;
    }

    void onSyncStarted(int regId, int syncHandle, int sid, int addressType, String address, int phy,
            int interval, int status) throws Exception {
        if (DBG) {
            Log.d(TAG,
                    "onSyncStarted() - regId=" + regId + ", syncHandle=" + syncHandle + ", status="
                            + status);
        }

        Map.Entry<IBinder, SyncInfo> entry = findSync(regId);
        if (entry == null) {
            Log.i(TAG, "onSyncStarted() - no callback found for regId " + regId);
            // Sync was stopped before it was properly registered.
            stopSyncNative(syncHandle);
            return;
        }

        IPeriodicAdvertisingCallback callback = entry.getValue().callback;
        if (status == 0) {
            entry.setValue(new SyncInfo(syncHandle, entry.getValue().deathRecipient, callback));
        } else {
            IBinder binder = entry.getKey();
            binder.unlinkToDeath(entry.getValue().deathRecipient, 0);
            mSyncs.remove(binder);
        }

        // TODO: fix callback arguments
        // callback.onSyncStarted(syncHandle, tx_power, status);
    }

    void onSyncReport(int syncHandle, int txPower, int rssi, int dataStatus, byte[] data)
            throws Exception {
        if (DBG) {
            Log.d(TAG, "onSyncReport() - syncHandle=" + syncHandle);
        }

        Map.Entry<IBinder, SyncInfo> entry = findSync(syncHandle);
        if (entry == null) {
            Log.i(TAG, "onSyncReport() - no callback found for syncHandle " + syncHandle);
            return;
        }

        IPeriodicAdvertisingCallback callback = entry.getValue().callback;
        PeriodicAdvertisingReport report =
                new PeriodicAdvertisingReport(syncHandle, txPower, rssi, dataStatus,
                        ScanRecord.parseFromBytes(data));
        callback.onPeriodicAdvertisingReport(report);
    }

    void onSyncLost(int syncHandle) throws Exception {
        if (DBG) {
            Log.d(TAG, "onSyncLost() - syncHandle=" + syncHandle);
        }

        Map.Entry<IBinder, SyncInfo> entry = findSync(syncHandle);
        if (entry == null) {
            Log.i(TAG, "onSyncLost() - no callback found for syncHandle " + syncHandle);
            return;
        }

        IPeriodicAdvertisingCallback callback = entry.getValue().callback;
        mSyncs.remove(entry);
        callback.onSyncLost(syncHandle);
    }

    void startSync(ScanResult scanResult, int skip, int timeout,
            IPeriodicAdvertisingCallback callback) {
        SyncDeathRecipient deathRecipient = new SyncDeathRecipient(callback);
        IBinder binder = toBinder(callback);
        try {
            binder.linkToDeath(deathRecipient, 0);
        } catch (RemoteException e) {
            throw new IllegalArgumentException("Can't link to periodic scanner death");
        }

        String address = scanResult.getDevice().getAddress();
        int sid = scanResult.getAdvertisingSid();

        int cbId = --sTempRegistrationId;
        mSyncs.put(binder, new SyncInfo(cbId, deathRecipient, callback));

        if (DBG) {
            Log.d(TAG, "startSync() - reg_id=" + cbId + ", callback: " + binder);
        }
        startSyncNative(sid, address, skip, timeout, cbId);
    }

    void stopSync(IPeriodicAdvertisingCallback callback) {
        IBinder binder = toBinder(callback);
        if (DBG) {
            Log.d(TAG, "stopSync() " + binder);
        }

        SyncInfo sync = mSyncs.remove(binder);
        if (sync == null) {
            Log.e(TAG, "stopSync() - no client found for callback");
            return;
        }

        Integer syncHandle = sync.id;
        binder.unlinkToDeath(sync.deathRecipient, 0);

        if (syncHandle < 0) {
            Log.i(TAG, "stopSync() - not finished registration yet");
            // Sync will be freed once initiated in onSyncStarted()
            return;
        }

        stopSyncNative(syncHandle);
    }

    static {
        classInitNative();
    }

    private static native void classInitNative();

    private native void initializeNative();

    private native void cleanupNative();

    private native void startSyncNative(int sid, String address, int skip, int timeout, int regId);

    private native void stopSyncNative(int syncHandle);
}
