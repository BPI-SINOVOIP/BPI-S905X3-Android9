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

package com.android.bluetooth.hid;

import android.app.ActivityManager;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHidDevice;
import android.bluetooth.BluetoothHidDeviceAppQosSettings;
import android.bluetooth.BluetoothHidDeviceAppSdpSettings;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.IBluetoothHidDevice;
import android.bluetooth.IBluetoothHidDeviceCallback;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Process;
import android.os.RemoteException;
import android.util.Log;

import com.android.bluetooth.BluetoothMetricsProto;
import com.android.bluetooth.Utils;
import com.android.bluetooth.btservice.MetricsLogger;
import com.android.bluetooth.btservice.ProfileService;
import com.android.internal.annotations.VisibleForTesting;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.NoSuchElementException;

/** @hide */
public class HidDeviceService extends ProfileService {
    private static final boolean DBG = false;
    private static final String TAG = HidDeviceService.class.getSimpleName();

    private static final int MESSAGE_APPLICATION_STATE_CHANGED = 1;
    private static final int MESSAGE_CONNECT_STATE_CHANGED = 2;
    private static final int MESSAGE_GET_REPORT = 3;
    private static final int MESSAGE_SET_REPORT = 4;
    private static final int MESSAGE_SET_PROTOCOL = 5;
    private static final int MESSAGE_INTR_DATA = 6;
    private static final int MESSAGE_VC_UNPLUG = 7;
    private static final int MESSAGE_IMPORTANCE_CHANGE = 8;

    private static final int FOREGROUND_IMPORTANCE_CUTOFF =
            ActivityManager.RunningAppProcessInfo.IMPORTANCE_VISIBLE;

    private static HidDeviceService sHidDeviceService;

    private HidDeviceNativeInterface mHidDeviceNativeInterface;

    private boolean mNativeAvailable = false;
    private BluetoothDevice mHidDevice;
    private int mHidDeviceState = BluetoothHidDevice.STATE_DISCONNECTED;
    private int mUserUid = 0;
    private IBluetoothHidDeviceCallback mCallback;
    private BluetoothHidDeviceDeathRecipient mDeathRcpt;
    private ActivityManager mActivityManager;

    private HidDeviceServiceHandler mHandler;

    private class HidDeviceServiceHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            if (DBG) {
                Log.d(TAG, "handleMessage(): msg.what=" + msg.what);
            }

            switch (msg.what) {
                case MESSAGE_APPLICATION_STATE_CHANGED: {
                    BluetoothDevice device = msg.obj != null ? (BluetoothDevice) msg.obj : null;
                    boolean success = (msg.arg1 != 0);

                    if (success) {
                        Log.d(TAG, "App registered, set device to: " + device);
                        mHidDevice = device;
                    } else {
                        mHidDevice = null;
                    }

                    try {
                        if (mCallback != null) {
                            mCallback.onAppStatusChanged(device, success);
                        } else {
                            break;
                        }
                    } catch (RemoteException e) {
                        Log.e(TAG, "e=" + e.toString());
                        e.printStackTrace();
                    }

                    if (success) {
                        mDeathRcpt = new BluetoothHidDeviceDeathRecipient(HidDeviceService.this);
                        if (mCallback != null) {
                            IBinder binder = mCallback.asBinder();
                            try {
                                binder.linkToDeath(mDeathRcpt, 0);
                                Log.i(TAG, "IBinder.linkToDeath() ok");
                            } catch (RemoteException e) {
                                e.printStackTrace();
                            }
                        }
                    } else if (mDeathRcpt != null) {
                        if (mCallback != null) {
                            IBinder binder = mCallback.asBinder();
                            try {
                                binder.unlinkToDeath(mDeathRcpt, 0);
                                Log.i(TAG, "IBinder.unlinkToDeath() ok");
                            } catch (NoSuchElementException e) {
                                e.printStackTrace();
                            }
                            mDeathRcpt.cleanup();
                            mDeathRcpt = null;
                        }
                    }

                    if (!success) {
                        mCallback = null;
                    }

                    break;
                }

                case MESSAGE_CONNECT_STATE_CHANGED: {
                    BluetoothDevice device = (BluetoothDevice) msg.obj;
                    int halState = msg.arg1;
                    int state = convertHalState(halState);

                    if (state != BluetoothHidDevice.STATE_DISCONNECTED) {
                        mHidDevice = device;
                    }

                    setAndBroadcastConnectionState(device, state);

                    try {
                        if (mCallback != null) {
                            mCallback.onConnectionStateChanged(device, state);
                        }
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                    break;
                }

                case MESSAGE_GET_REPORT:
                    byte type = (byte) msg.arg1;
                    byte id = (byte) msg.arg2;
                    int bufferSize = msg.obj == null ? 0 : ((Integer) msg.obj).intValue();

                    try {
                        if (mCallback != null) {
                            mCallback.onGetReport(mHidDevice, type, id, bufferSize);
                        }
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                    break;

                case MESSAGE_SET_REPORT: {
                    byte reportType = (byte) msg.arg1;
                    byte reportId = (byte) msg.arg2;
                    byte[] data = ((ByteBuffer) msg.obj).array();

                    try {
                        if (mCallback != null) {
                            mCallback.onSetReport(mHidDevice, reportType, reportId, data);
                        }
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                    break;
                }

                case MESSAGE_SET_PROTOCOL:
                    byte protocol = (byte) msg.arg1;

                    try {
                        if (mCallback != null) {
                            mCallback.onSetProtocol(mHidDevice, protocol);
                        }
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                    break;

                case MESSAGE_INTR_DATA:
                    byte reportId = (byte) msg.arg1;
                    byte[] data = ((ByteBuffer) msg.obj).array();

                    try {
                        if (mCallback != null) {
                            mCallback.onInterruptData(mHidDevice, reportId, data);
                        }
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                    break;

                case MESSAGE_VC_UNPLUG:
                    try {
                        if (mCallback != null) {
                            mCallback.onVirtualCableUnplug(mHidDevice);
                        }
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                    mHidDevice = null;
                    break;

                case MESSAGE_IMPORTANCE_CHANGE:
                    int importance = msg.arg1;
                    int uid = msg.arg2;
                    if (importance > FOREGROUND_IMPORTANCE_CUTOFF
                            && uid >= Process.FIRST_APPLICATION_UID) {
                        unregisterAppUid(uid);
                    }
                    break;
            }
        }
    }

    private static class BluetoothHidDeviceDeathRecipient implements IBinder.DeathRecipient {
        private HidDeviceService mService;

        BluetoothHidDeviceDeathRecipient(HidDeviceService service) {
            mService = service;
        }

        @Override
        public void binderDied() {
            Log.w(TAG, "Binder died, need to unregister app :(");
            mService.unregisterApp();
        }

        public void cleanup() {
            mService = null;
        }
    }

    private ActivityManager.OnUidImportanceListener mUidImportanceListener =
            new ActivityManager.OnUidImportanceListener() {
                @Override
                public void onUidImportance(final int uid, final int importance) {
                    Message message = mHandler.obtainMessage(MESSAGE_IMPORTANCE_CHANGE);
                    message.arg1 = importance;
                    message.arg2 = uid;
                    mHandler.sendMessage(message);
                }
            };

    @VisibleForTesting
    static class BluetoothHidDeviceBinder extends IBluetoothHidDevice.Stub
            implements IProfileServiceBinder {

        private static final String TAG = BluetoothHidDeviceBinder.class.getSimpleName();

        private HidDeviceService mService;

        BluetoothHidDeviceBinder(HidDeviceService service) {
            mService = service;
        }

        @VisibleForTesting
        HidDeviceService getServiceForTesting() {
            if (mService != null && mService.isAvailable()) {
                return mService;
            }
            return null;
        }

        @Override
        public void cleanup() {
            mService = null;
        }

        private HidDeviceService getService() {
            if (!Utils.checkCaller()) {
                Log.w(TAG, "HidDevice call not allowed for non-active user");
                return null;
            }

            if (mService != null && mService.isAvailable()) {
                return mService;
            }

            return null;
        }

        @Override
        public boolean registerApp(BluetoothHidDeviceAppSdpSettings sdp,
                BluetoothHidDeviceAppQosSettings inQos, BluetoothHidDeviceAppQosSettings outQos,
                IBluetoothHidDeviceCallback callback) {
            if (DBG) {
                Log.d(TAG, "registerApp()");
            }

            HidDeviceService service = getService();
            if (service == null) {
                return false;
            }

            return service.registerApp(sdp, inQos, outQos, callback);
        }

        @Override
        public boolean unregisterApp() {
            if (DBG) {
                Log.d(TAG, "unregisterApp()");
            }

            HidDeviceService service = getService();
            if (service == null) {
                return false;
            }

            return service.unregisterApp();
        }

        @Override
        public boolean sendReport(BluetoothDevice device, int id, byte[] data) {
            if (DBG) {
                Log.d(TAG, "sendReport(): device=" + device + "  id=" + id);
            }

            HidDeviceService service = getService();
            if (service == null) {
                return false;
            }

            return service.sendReport(device, id, data);
        }

        @Override
        public boolean replyReport(BluetoothDevice device, byte type, byte id, byte[] data) {
            if (DBG) {
                Log.d(TAG, "replyReport(): device=" + device + " type=" + type + " id=" + id);
            }

            HidDeviceService service = getService();
            if (service == null) {
                return false;
            }

            return service.replyReport(device, type, id, data);
        }

        @Override
        public boolean unplug(BluetoothDevice device) {
            if (DBG) {
                Log.d(TAG, "unplug(): device=" + device);
            }

            HidDeviceService service = getService();
            if (service == null) {
                return false;
            }

            return service.unplug(device);
        }

        @Override
        public boolean connect(BluetoothDevice device) {
            if (DBG) {
                Log.d(TAG, "connect(): device=" + device);
            }

            HidDeviceService service = getService();
            if (service == null) {
                return false;
            }

            return service.connect(device);
        }

        @Override
        public boolean disconnect(BluetoothDevice device) {
            if (DBG) {
                Log.d(TAG, "disconnect(): device=" + device);
            }

            HidDeviceService service = getService();
            if (service == null) {
                return false;
            }

            return service.disconnect(device);
        }

        @Override
        public boolean reportError(BluetoothDevice device, byte error) {
            if (DBG) {
                Log.d(TAG, "reportError(): device=" + device + " error=" + error);
            }

            HidDeviceService service = getService();
            if (service == null) {
                return false;
            }

            return service.reportError(device, error);
        }

        @Override
        public int getConnectionState(BluetoothDevice device) {
            if (DBG) {
                Log.d(TAG, "getConnectionState(): device=" + device);
            }

            HidDeviceService service = getService();
            if (service == null) {
                return BluetoothHidDevice.STATE_DISCONNECTED;
            }

            return service.getConnectionState(device);
        }

        @Override
        public List<BluetoothDevice> getConnectedDevices() {
            if (DBG) {
                Log.d(TAG, "getConnectedDevices()");
            }

            return getDevicesMatchingConnectionStates(new int[]{BluetoothProfile.STATE_CONNECTED});
        }

        @Override
        public List<BluetoothDevice> getDevicesMatchingConnectionStates(int[] states) {
            if (DBG) {
                Log.d(TAG,
                        "getDevicesMatchingConnectionStates(): states=" + Arrays.toString(states));
            }

            HidDeviceService service = getService();
            if (service == null) {
                return new ArrayList<BluetoothDevice>(0);
            }

            return service.getDevicesMatchingConnectionStates(states);
        }

        @Override
        public String getUserAppName() {
            HidDeviceService service = getService();
            if (service == null) {
                return "";
            }
            return service.getUserAppName();
        }
    }

    @Override
    protected IProfileServiceBinder initBinder() {
        return new BluetoothHidDeviceBinder(this);
    }

    private boolean checkDevice(BluetoothDevice device) {
        if (mHidDevice == null || !mHidDevice.equals(device)) {
            Log.w(TAG, "Unknown device: " + device);
            return false;
        }
        return true;
    }

    private boolean checkCallingUid() {
        int callingUid = Binder.getCallingUid();
        if (callingUid != mUserUid) {
            Log.w(TAG, "checkCallingUid(): caller UID doesn't match registered user UID");
            return false;
        }
        return true;
    }

    synchronized boolean registerApp(BluetoothHidDeviceAppSdpSettings sdp,
            BluetoothHidDeviceAppQosSettings inQos, BluetoothHidDeviceAppQosSettings outQos,
            IBluetoothHidDeviceCallback callback) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");
        if (mUserUid != 0) {
            Log.w(TAG, "registerApp(): failed because another app is registered");
            return false;
        }

        int callingUid = Binder.getCallingUid();
        if (DBG) {
            Log.d(TAG, "registerApp(): calling uid=" + callingUid);
        }
        if (callingUid >= Process.FIRST_APPLICATION_UID
                && mActivityManager.getUidImportance(callingUid) > FOREGROUND_IMPORTANCE_CUTOFF) {
            Log.w(TAG, "registerApp(): failed because the app is not foreground");
            return false;
        }
        mUserUid = callingUid;
        mCallback = callback;

        return mHidDeviceNativeInterface.registerApp(
                sdp.getName(),
                sdp.getDescription(),
                sdp.getProvider(),
                sdp.getSubclass(),
                sdp.getDescriptors(),
                inQos == null
                        ? null
                        : new int[] {
                            inQos.getServiceType(),
                            inQos.getTokenRate(),
                            inQos.getTokenBucketSize(),
                            inQos.getPeakBandwidth(),
                            inQos.getLatency(),
                            inQos.getDelayVariation()
                        },
                outQos == null
                        ? null
                        : new int[] {
                            outQos.getServiceType(),
                            outQos.getTokenRate(),
                            outQos.getTokenBucketSize(),
                            outQos.getPeakBandwidth(),
                            outQos.getLatency(),
                            outQos.getDelayVariation()
                        });
    }

    synchronized boolean unregisterApp() {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");
        if (DBG) {
            Log.d(TAG, "unregisterApp()");
        }

        int callingUid = Binder.getCallingUid();
        return unregisterAppUid(callingUid);
    }

    private synchronized boolean unregisterAppUid(int uid) {
        if (DBG) {
            Log.d(TAG, "unregisterAppUid(): uid=" + uid);
        }

        if (mUserUid != 0 && (uid == mUserUid || uid < Process.FIRST_APPLICATION_UID)) {
            mUserUid = 0;
            return mHidDeviceNativeInterface.unregisterApp();
        }
        if (DBG) {
            Log.d(TAG, "unregisterAppUid(): caller UID doesn't match user UID");
        }
        return false;
    }

    synchronized boolean sendReport(BluetoothDevice device, int id, byte[] data) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");
        if (DBG) {
            Log.d(TAG, "sendReport(): device=" + device + " id=" + id);
        }

        return checkDevice(device) && checkCallingUid()
                && mHidDeviceNativeInterface.sendReport(id, data);
    }

    synchronized boolean replyReport(BluetoothDevice device, byte type, byte id, byte[] data) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");
        if (DBG) {
            Log.d(TAG, "replyReport(): device=" + device + " type=" + type + " id=" + id);
        }

        return checkDevice(device) && checkCallingUid()
                && mHidDeviceNativeInterface.replyReport(type, id, data);
    }

    synchronized boolean unplug(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");
        if (DBG) {
            Log.d(TAG, "unplug(): device=" + device);
        }

        return checkDevice(device) && checkCallingUid()
                && mHidDeviceNativeInterface.unplug();
    }

    synchronized boolean connect(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");
        if (DBG) {
            Log.d(TAG, "connect(): device=" + device);
        }

        return checkCallingUid() && mHidDeviceNativeInterface.connect(device);
    }

    synchronized boolean disconnect(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");
        if (DBG) {
            Log.d(TAG, "disconnect(): device=" + device);
        }

        int callingUid = Binder.getCallingUid();
        if (callingUid != mUserUid && callingUid >= Process.FIRST_APPLICATION_UID) {
            Log.w(TAG, "disconnect(): caller UID doesn't match user UID");
            return false;
        }
        return checkDevice(device) && mHidDeviceNativeInterface.disconnect();
    }

    synchronized boolean reportError(BluetoothDevice device, byte error) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");
        if (DBG) {
            Log.d(TAG, "reportError(): device=" + device + " error=" + error);
        }

        return checkDevice(device) && checkCallingUid()
                && mHidDeviceNativeInterface.reportError(error);
    }

    synchronized String getUserAppName() {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        if (mUserUid < Process.FIRST_APPLICATION_UID) {
            return "";
        }
        String appName = getPackageManager().getNameForUid(mUserUid);
        return appName != null ? appName : "";
    }

    @Override
    protected boolean start() {
        if (DBG) {
            Log.d(TAG, "start()");
        }

        mHandler = new HidDeviceServiceHandler();
        mHidDeviceNativeInterface = HidDeviceNativeInterface.getInstance();
        mHidDeviceNativeInterface.init();
        mNativeAvailable = true;
        mActivityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        mActivityManager.addOnUidImportanceListener(mUidImportanceListener,
                FOREGROUND_IMPORTANCE_CUTOFF);
        setHidDeviceService(this);
        return true;
    }

    @Override
    protected boolean stop() {
        if (DBG) {
            Log.d(TAG, "stop()");
        }

        if (sHidDeviceService == null) {
            Log.w(TAG, "stop() called before start()");
            return true;
        }

        setHidDeviceService(null);
        if (mNativeAvailable) {
            mHidDeviceNativeInterface.cleanup();
            mNativeAvailable = false;
        }
        mActivityManager.removeOnUidImportanceListener(mUidImportanceListener);
        return true;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        Log.d(TAG, "Need to unregister app");
        unregisterApp();
        return super.onUnbind(intent);
    }

    /**
     * Get the HID Device Service instance
     * @return HID Device Service instance
     */
    public static synchronized HidDeviceService getHidDeviceService() {
        if (sHidDeviceService == null) {
            Log.d(TAG, "getHidDeviceService(): service is NULL");
            return null;
        }
        if (!sHidDeviceService.isAvailable()) {
            Log.d(TAG, "getHidDeviceService(): service is not available");
            return null;
        }
        return sHidDeviceService;
    }

    private static synchronized void setHidDeviceService(HidDeviceService instance) {
        if (DBG) {
            Log.d(TAG, "setHidDeviceService(): set to: " + instance);
        }
        sHidDeviceService = instance;
    }

    int getConnectionState(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        if (mHidDevice != null && mHidDevice.equals(device)) {
            return mHidDeviceState;
        }
        return BluetoothHidDevice.STATE_DISCONNECTED;
    }

    List<BluetoothDevice> getDevicesMatchingConnectionStates(int[] states) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        List<BluetoothDevice> inputDevices = new ArrayList<BluetoothDevice>();

        if (mHidDevice != null) {
            for (int state : states) {
                if (state == mHidDeviceState) {
                    inputDevices.add(mHidDevice);
                    break;
                }
            }
        }
        return inputDevices;
    }

    synchronized void onApplicationStateChangedFromNative(BluetoothDevice device,
            boolean registered) {
        if (DBG) {
            Log.d(TAG, "onApplicationStateChanged(): registered=" + registered);
        }

        Message msg = mHandler.obtainMessage(MESSAGE_APPLICATION_STATE_CHANGED);
        msg.obj = device;
        msg.arg1 = registered ? 1 : 0;
        mHandler.sendMessage(msg);
    }

    synchronized void onConnectStateChangedFromNative(BluetoothDevice device, int state) {
        if (DBG) {
            Log.d(TAG, "onConnectStateChanged(): device="
                    + device + " state=" + state);
        }

        Message msg = mHandler.obtainMessage(MESSAGE_CONNECT_STATE_CHANGED);
        msg.obj = device;
        msg.arg1 = state;
        mHandler.sendMessage(msg);
    }

    synchronized void onGetReportFromNative(byte type, byte id, short bufferSize) {
        if (DBG) {
            Log.d(TAG, "onGetReport(): type=" + type + " id=" + id + " bufferSize=" + bufferSize);
        }

        Message msg = mHandler.obtainMessage(MESSAGE_GET_REPORT);
        msg.obj = bufferSize > 0 ? new Integer(bufferSize) : null;
        msg.arg1 = type;
        msg.arg2 = id;
        mHandler.sendMessage(msg);
    }

    synchronized void onSetReportFromNative(byte reportType, byte reportId, byte[] data) {
        if (DBG) {
            Log.d(TAG, "onSetReport(): reportType=" + reportType + " reportId=" + reportId);
        }

        ByteBuffer bb = ByteBuffer.wrap(data);

        Message msg = mHandler.obtainMessage(MESSAGE_SET_REPORT);
        msg.arg1 = reportType;
        msg.arg2 = reportId;
        msg.obj = bb;
        mHandler.sendMessage(msg);
    }

    synchronized void onSetProtocolFromNative(byte protocol) {
        if (DBG) {
            Log.d(TAG, "onSetProtocol(): protocol=" + protocol);
        }

        Message msg = mHandler.obtainMessage(MESSAGE_SET_PROTOCOL);
        msg.arg1 = protocol;
        mHandler.sendMessage(msg);
    }

    synchronized void onInterruptDataFromNative(byte reportId, byte[] data) {
        if (DBG) {
            Log.d(TAG, "onInterruptData(): reportId=" + reportId);
        }

        ByteBuffer bb = ByteBuffer.wrap(data);

        Message msg = mHandler.obtainMessage(MESSAGE_INTR_DATA);
        msg.arg1 = reportId;
        msg.obj = bb;
        mHandler.sendMessage(msg);
    }

    synchronized void onVirtualCableUnplugFromNative() {
        if (DBG) {
            Log.d(TAG, "onVirtualCableUnplug()");
        }

        Message msg = mHandler.obtainMessage(MESSAGE_VC_UNPLUG);
        mHandler.sendMessage(msg);
    }

    private void setAndBroadcastConnectionState(BluetoothDevice device, int newState) {
        if (DBG) {
            Log.d(TAG, "setAndBroadcastConnectionState(): device=" + device.getAddress()
                    + " oldState=" + mHidDeviceState + " newState=" + newState);
        }

        if (mHidDevice != null && !mHidDevice.equals(device)) {
            Log.w(TAG, "Connection state changed for unknown device, ignoring");
            return;
        }

        int prevState = mHidDeviceState;
        mHidDeviceState = newState;

        if (prevState == newState) {
            Log.w(TAG, "Connection state is unchanged, ignoring");
            return;
        }

        if (newState == BluetoothProfile.STATE_CONNECTED) {
            MetricsLogger.logProfileConnectionEvent(BluetoothMetricsProto.ProfileId.HID_DEVICE);
        }

        Intent intent = new Intent(BluetoothHidDevice.ACTION_CONNECTION_STATE_CHANGED);
        intent.putExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, prevState);
        intent.putExtra(BluetoothProfile.EXTRA_STATE, newState);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        sendBroadcast(intent, BLUETOOTH_PERM);
    }

    private static int convertHalState(int halState) {
        switch (halState) {
            case HAL_CONN_STATE_CONNECTED:
                return BluetoothProfile.STATE_CONNECTED;
            case HAL_CONN_STATE_CONNECTING:
                return BluetoothProfile.STATE_CONNECTING;
            case HAL_CONN_STATE_DISCONNECTED:
                return BluetoothProfile.STATE_DISCONNECTED;
            case HAL_CONN_STATE_DISCONNECTING:
                return BluetoothProfile.STATE_DISCONNECTING;
            default:
                return BluetoothProfile.STATE_DISCONNECTED;
        }
    }

    static final int HAL_CONN_STATE_CONNECTED = 0;
    static final int HAL_CONN_STATE_CONNECTING = 1;
    static final int HAL_CONN_STATE_DISCONNECTED = 2;
    static final int HAL_CONN_STATE_DISCONNECTING = 3;
}
