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

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertisingSet;
import android.bluetooth.le.AdvertisingSetCallback;
import android.bluetooth.le.AdvertisingSetParameters;
import android.bluetooth.le.PeriodicAdvertisingParameters;
import android.os.Bundle;
import android.os.ParcelUuid;

import com.googlecode.android_scripting.Log;
import com.googlecode.android_scripting.MainThread;
import com.googlecode.android_scripting.facade.EventFacade;
import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcParameter;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.concurrent.Callable;

import org.json.JSONArray;
import org.json.JSONObject;

/**
 * BluetoothLe AdvertisingSet functions.
 */
public class BluetoothLeAdvertisingSetFacade extends RpcReceiver {

    private static int sAdvertisingSetCount;
    private static int sAdvertisingSetCallbackCount;
    private final EventFacade mEventFacade;
    private final HashMap<Integer, MyAdvertisingSetCallback> mAdvertisingSetCallbacks;
    private final HashMap<Integer, AdvertisingSet> mAdvertisingSets;
    private BluetoothAdapter mBluetoothAdapter;

    private static final Map<String, Integer> ADV_PHYS = new HashMap<>();
    static {
        ADV_PHYS.put("PHY_LE_1M", BluetoothDevice.PHY_LE_1M);
        ADV_PHYS.put("PHY_LE_2M", BluetoothDevice.PHY_LE_2M);
        ADV_PHYS.put("PHY_LE_CODED", BluetoothDevice.PHY_LE_CODED);
    }

    public BluetoothLeAdvertisingSetFacade(FacadeManager manager) {
        super(manager);
        mBluetoothAdapter = MainThread.run(manager.getService(),
                new Callable<BluetoothAdapter>() {
                    @Override
                    public BluetoothAdapter call() throws Exception {
                        return BluetoothAdapter.getDefaultAdapter();
                    }
                });
        mEventFacade = manager.getReceiver(EventFacade.class);
        mAdvertisingSetCallbacks = new HashMap<>();
        mAdvertisingSets = new HashMap<>();
    }

    /**
     * Constructs a MyAdvertisingSetCallback obj and returns its index
     *
     * @return MyAdvertisingSetCallback.index
     */
    @Rpc(description = "Generate a new MyAdvertisingSetCallback Object")
    public Integer bleAdvSetGenCallback() {
        int index = ++sAdvertisingSetCallbackCount;
        MyAdvertisingSetCallback callback = new MyAdvertisingSetCallback(index);
        mAdvertisingSetCallbacks.put(callback.index, callback);
        return callback.index;
    }

    /**
     * Converts String or JSONArray representation of byte array into raw byte array
     */
    public byte[] somethingToByteArray(Object something) throws Exception {
        if (something instanceof String) {
            String s = (String) something;
            int len = s.length();
            byte[] data = new byte[len / 2];
            for (int i = 0; i < len; i += 2) {
                data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                                     + Character.digit(s.charAt(i + 1), 16));
            }
            return data;
        } else if (something instanceof JSONArray) {
            JSONArray arr = (JSONArray) something;
            int len = arr.length();
            byte[] data = new byte[len];
            for (int i = 0; i < len; i++) {
                data[i] = (byte) arr.getInt(i);
            }
            return data;
        } else {
            throw new IllegalArgumentException("Don't know how to convert "
                + something.getClass().getName() + " to byte array!");
        }
    }

    /**
     * Converts JSONObject representation of AdvertiseData into actual object.
     */
    public AdvertiseData buildAdvData(JSONObject params) throws Exception {
        AdvertiseData.Builder builder = new AdvertiseData.Builder();

        Iterator<String> keys = params.keys();
        while (keys.hasNext()) {
            String key = keys.next();

            /** Python doesn't have multi map, if advertise data should repeat use
              * serviceUuid, serviceUuid2, serviceUuid3... . For that use "startsWith"
              */
            if (key.startsWith("manufacturerData")) {
                JSONArray manuf = params.getJSONArray(key);
                if (manuf.length() != 2) {
                    throw new IllegalArgumentException(
                        "manufacturerData should contain exactly two elements");
                }
                int manufId = manuf.getInt(0);
                byte[] data = somethingToByteArray(manuf.get(1));
                builder.addManufacturerData(manufId, data);
            } else if (key.startsWith("serviceData")) {
                JSONArray serDat = params.getJSONArray(key);
                ParcelUuid uuid = ParcelUuid.fromString(serDat.getString(0));
                byte[] data = somethingToByteArray(serDat.get(1));
                builder.addServiceData(uuid, data);
            } else if (key.startsWith("serviceUuid")) {
                builder.addServiceUuid(ParcelUuid.fromString(params.getString(key)));
            } else if (key.startsWith("includeDeviceName")) {
                builder.setIncludeDeviceName(params.getBoolean(key));
            } else if (key.startsWith("includeTxPowerLevel")) {
                builder.setIncludeTxPowerLevel(params.getBoolean(key));
            } else {
                throw new IllegalArgumentException("Unknown AdvertiseData field " + key);
            }
        }

        return builder.build();
    }

    /**
     * Converts JSONObject representation of AdvertisingSetParameters into actual object.
     */
    public AdvertisingSetParameters buildParameters(JSONObject params) throws Exception {
        AdvertisingSetParameters.Builder builder = new AdvertisingSetParameters.Builder();

        Iterator<String> keys = params.keys();
        while (keys.hasNext()) {
            String key = keys.next();

            if (key.equals("connectable")) {
                builder.setConnectable(params.getBoolean(key));
            } else if (key.equals("scannable")) {
                builder.setScannable(params.getBoolean(key));
            } else if (key.equals("legacyMode")) {
                builder.setLegacyMode(params.getBoolean(key));
            } else if (key.equals("anonymous")) {
                builder.setAnonymous(params.getBoolean(key));
            } else if (key.equals("includeTxPower")) {
                builder.setIncludeTxPower(params.getBoolean(key));
            } else if (key.equals("primaryPhy")) {
                builder.setPrimaryPhy(ADV_PHYS.get(params.getString(key)));
            } else if (key.equals("secondaryPhy")) {
                builder.setSecondaryPhy(ADV_PHYS.get(params.getString(key)));
            } else if (key.equals("interval")) {
                builder.setInterval(params.getInt(key));
            } else if (key.equals("txPowerLevel")) {
                builder.setTxPowerLevel(params.getInt(key));
            } else {
                throw new IllegalArgumentException("Unknown AdvertisingSetParameters field " + key);
            }
        }

        return builder.build();
    }

    /**
     * Converts JSONObject representation of PeriodicAdvertisingParameters into actual object.
     */
    public PeriodicAdvertisingParameters buildPeriodicParameters(JSONObject params)
            throws Exception {
        PeriodicAdvertisingParameters.Builder builder = new PeriodicAdvertisingParameters.Builder();

        Iterator<String> keys = params.keys();
        while (keys.hasNext()) {
            String key = keys.next();

            if (key.equals("includeTxPower")) {
                builder.setIncludeTxPower(params.getBoolean(key));
            } else if (key.equals("interval")) {
                builder.setInterval(params.getInt(key));
            } else {
                throw new IllegalArgumentException(
                        "Unknown PeriodicAdvertisingParameters field " + key);
            }
        }

        return builder.build();
    }

    /**
     * Starts ble advertising
     *
     * @throws Exception
     */
    @Rpc(description = "Starts ble advertisement")
    public void bleAdvSetStartAdvertisingSet(
            @RpcParameter(name = "params") JSONObject parametersJson,
            @RpcParameter(name = "data") JSONObject dataJson,
            @RpcParameter(name = "scanResponse") JSONObject scanResponseJson,
            @RpcParameter(name = "periodicParameters") JSONObject periodicParametersJson,
            @RpcParameter(name = "periodicDataIndex") JSONObject periodicDataJson,
            @RpcParameter(name = "duration") Integer duration,
            @RpcParameter(name = "maxExtAdvEvents") Integer maxExtAdvEvents,
            @RpcParameter(name = "callbackIndex") Integer callbackIndex) throws Exception {

        AdvertisingSetParameters parameters = null;
        if (parametersJson != null) {
            parameters = buildParameters(parametersJson);
        }

        AdvertiseData data = null;
        if (dataJson != null) {
            data = buildAdvData(dataJson);
        }

        AdvertiseData scanResponse = null;
        if (scanResponseJson != null) {
            scanResponse = buildAdvData(scanResponseJson);
        }

        PeriodicAdvertisingParameters periodicParameters = null;
        if (periodicParametersJson != null) {
            periodicParameters = buildPeriodicParameters(periodicParametersJson);
        }

        AdvertiseData periodicData = null;
        if (periodicDataJson != null) {
            periodicData = buildAdvData(periodicDataJson);
        }

        MyAdvertisingSetCallback callback = mAdvertisingSetCallbacks.get(callbackIndex);
        if (callback != null) {
            Log.d("starting le advertising set on callback index: " + callbackIndex);
            mBluetoothAdapter.getBluetoothLeAdvertiser().startAdvertisingSet(
                    parameters, data, scanResponse, periodicParameters, periodicData, callback);
        } else {
            throw new Exception("Invalid callbackIndex input" + callbackIndex);
        }
    }

    /**
     * Get the address associated with this Advertising set. This method returns immediately,
     * the operation result is delivered through callback.onOwnAddressRead().
     * This is for PTS only.
     */
    @Rpc(description = "Get own address")
    public void bleAdvSetGetOwnAddress(
            @RpcParameter(name = "setIndex") Integer setIndex) throws Exception {
        mAdvertisingSets.get(setIndex).getOwnAddress();
    }

    /**
     * Enables Advertising. This method returns immediately, the operation status is
     * delivered through callback.onAdvertisingEnabled().
     *
     * @param enable whether the advertising should be enabled (true), or disabled (false)
     * @param duration advertising duration, in 10ms unit. Valid range is from 1 (10ms) to
     *                     65535 (655,350 ms)
     * @param maxExtAdvEvents maximum number of extended advertising events the
     *                     controller shall attempt to send prior to terminating the extended
     *                     advertising, even if the duration has not expired. Valid range is
     *                     from 1 to 255.
     */
    @Rpc(description = "Enable/disable advertising")
    public void bleAdvSetEnableAdvertising(
            @RpcParameter(name = "setIndex") Integer setIndex,
            @RpcParameter(name = "enable") Boolean enable,
            @RpcParameter(name = "duration") Integer duration,
            @RpcParameter(name = "maxExtAdvEvents") Integer maxExtAdvEvents) throws Exception {
        mAdvertisingSets.get(setIndex).enableAdvertising(enable, duration, maxExtAdvEvents);
    }

    /**
     * Set/update data being Advertised. Make sure that data doesn't exceed the size limit for
     * specified AdvertisingSetParameters. This method returns immediately, the operation status is
     * delivered through callback.onAdvertisingDataSet().
     *
     * Advertising data must be empty if non-legacy scannable advertising is used.
     *
     * @param dataJson Advertisement data to be broadcasted. Size must not exceed
     *                     {@link BluetoothAdapter#getLeMaximumAdvertisingDataLength}. If the
     *                     advertisement is connectable, three bytes will be added for flags. If the
     *                     update takes place when the advertising set is enabled, the data can be
     *                     maximum 251 bytes long.
     */
    @Rpc(description = "Set advertise data")
    public void bleAdvSetSetAdvertisingData(
            @RpcParameter(name = "setIndex") Integer setIndex,
            @RpcParameter(name = "data") JSONObject dataJson) throws Exception {
        AdvertiseData data = null;
        if (dataJson != null) {
            data = buildAdvData(dataJson);
        }

        Log.i("setAdvertisingData()");
        mAdvertisingSets.get(setIndex).setAdvertisingData(data);
    }

    /**
     * Stops a ble advertising set
     *
     * @param index the id of the advertising set to stop
     * @throws Exception
     */
    @Rpc(description = "Stops an ongoing ble advertising set")
    public void bleAdvSetStopAdvertisingSet(
            @RpcParameter(name = "index")
            Integer index) throws Exception {
        MyAdvertisingSetCallback callback = mAdvertisingSetCallbacks.remove(index);
        if (callback == null) {
            throw new Exception("Invalid index input:" + index);
        }

        Log.d("stopping le advertising set " + index);
        mBluetoothAdapter.getBluetoothLeAdvertiser().stopAdvertisingSet(callback);
    }

    private class MyAdvertisingSetCallback extends AdvertisingSetCallback {
        public Integer index;
        public Integer setIndex = -1;
        String mEventType;

        MyAdvertisingSetCallback(int idx) {
            index = idx;
            mEventType = "AdvertisingSet";
        }

        @Override
        public void onAdvertisingSetStarted(AdvertisingSet advertisingSet, int txPower,
                    int status) {
            Log.d("onAdvertisingSetStarted" + mEventType + " " + index);
            Bundle results = new Bundle();
            results.putString("Type", "onAdvertisingSetStarted");
            results.putInt("status", status);
            if (advertisingSet != null) {
                setIndex = ++sAdvertisingSetCount;
                mAdvertisingSets.put(setIndex, advertisingSet);
                results.putInt("setId", setIndex);
            } else {
                mAdvertisingSetCallbacks.remove(index);
            }
            mEventFacade.postEvent(mEventType + index + "onAdvertisingSetStarted", results);
        }

        @Override
        public void onAdvertisingSetStopped(AdvertisingSet advertisingSet) {
            Log.d("onAdvertisingSetStopped" + mEventType + " " + index);
            Bundle results = new Bundle();
            results.putString("Type", "onAdvertisingSetStopped");
            mEventFacade.postEvent(mEventType + index + "onAdvertisingSetStopped", results);
        }

        @Override
        public void onAdvertisingEnabled(AdvertisingSet advertisingSet, boolean enable,
                int status) {
            sendGeneric("onAdvertisingEnabled", setIndex, status, enable);
        }

        @Override
        public void onAdvertisingDataSet(AdvertisingSet advertisingSet, int status) {
            sendGeneric("onAdvertisingDataSet", setIndex, status);
        }

        @Override
        public void onScanResponseDataSet(AdvertisingSet advertisingSet, int status) {
            sendGeneric("onScanResponseDataSet", setIndex, status);
        }

        @Override
        public void onAdvertisingParametersUpdated(AdvertisingSet advertisingSet, int txPower,
                int status) {
            sendGeneric("onAdvertisingParametersUpdated", setIndex, status);
        }

        @Override
        public void onPeriodicAdvertisingParametersUpdated(AdvertisingSet advertisingSet,
                int status) {
            sendGeneric("onPeriodicAdvertisingParametersUpdated", setIndex, status);
        }

        @Override
        public void onPeriodicAdvertisingDataSet(AdvertisingSet advertisingSet, int status) {
            sendGeneric("onPeriodicAdvertisingDataSet", setIndex, status);
        }

        @Override
        public void onPeriodicAdvertisingEnabled(AdvertisingSet advertisingSet, boolean enable,
                int status) {
            sendGeneric("onPeriodicAdvertisingEnabled", setIndex, status, enable);
        }

        @Override
        public void onOwnAddressRead(AdvertisingSet advertisingSet, int addressType,
                String address) {
            Log.d("onOwnAddressRead" + mEventType + " " + setIndex);
            Bundle results = new Bundle();
            results.putInt("setId", setIndex);
            results.putInt("addressType", addressType);
            results.putString("address", address);
            mEventFacade.postEvent(mEventType + setIndex + "onOwnAddressRead", results);
        }

        public void sendGeneric(String cb, int setIndex, int status) {
            sendGeneric(cb, setIndex, status, null);
        }

        public void sendGeneric(String cb, int setIndex, int status, Boolean enable) {
            Log.d(cb + mEventType + " " + index);
            Bundle results = new Bundle();
            results.putInt("setId", setIndex);
            results.putInt("status", status);
            if (enable != null) results.putBoolean("enable", enable);
            mEventFacade.postEvent(mEventType + index + cb, results);
        }
    }

    @Override
    public void shutdown() {
        if (mBluetoothAdapter.getState() == BluetoothAdapter.STATE_ON) {
            Iterator<Map.Entry<Integer, MyAdvertisingSetCallback>> it =
                    mAdvertisingSetCallbacks.entrySet().iterator();
            while (it.hasNext()) {
                Map.Entry<Integer, MyAdvertisingSetCallback> entry = it.next();
                MyAdvertisingSetCallback advertisingSetCb = entry.getValue();
                it.remove();

                if (advertisingSetCb == null) continue;

                Log.d("shutdown() stopping le advertising set " + advertisingSetCb.index);
                try {
                    mBluetoothAdapter.getBluetoothLeAdvertiser()
                        .stopAdvertisingSet(advertisingSetCb);
                } catch (NullPointerException e) {
                    Log.e("Failed to stop ble advertising.", e);
                }
            }
        }
        mAdvertisingSetCallbacks.clear();
        mAdvertisingSets.clear();
    }
}
