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
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothHealth;
import android.bluetooth.BluetoothHealthCallback;
import android.bluetooth.BluetoothHealthAppConfiguration;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Messenger;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;

import com.googlecode.android_scripting.Log;
import com.googlecode.android_scripting.MainThread;
import com.googlecode.android_scripting.facade.EventFacade;
import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcDeprecated;
import com.googlecode.android_scripting.rpc.RpcParameter;
import com.googlecode.android_scripting.rpc.RpcStopEvent;

import java.lang.reflect.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.Callable;

public class BluetoothHealthFacade extends RpcReceiver {
    private final EventFacade mEventFacade;
    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothManager mBluetoothManager;
    private final Service mService;
    private final Context mContext;
    private final HashMap<Integer, BluetoothHealthAppConfiguration> mConfigList;
    private static int ConfigCount;
    private static boolean sIsHealthReady = false;
    private static BluetoothHealth sHealthProfile = null;

    public BluetoothHealthFacade(FacadeManager manager) {
        super(manager);
        mService = manager.getService();
        mContext = mService.getApplicationContext();
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mBluetoothManager = (BluetoothManager) mContext.getSystemService(Service.BLUETOOTH_SERVICE);
        mBluetoothAdapter.getProfileProxy(mService, new HealthServiceListener(),
        BluetoothProfile.HEALTH);
        mEventFacade = manager.getReceiver(EventFacade.class);
        mConfigList = new HashMap<Integer, BluetoothHealthAppConfiguration>();
    }


    class HealthServiceListener implements BluetoothProfile.ServiceListener {
        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
          sHealthProfile = (BluetoothHealth) proxy;
          sIsHealthReady = true;
        }

        @Override
        public void onServiceDisconnected(int profile) {
          sIsHealthReady = false;
        }
    }

    @Rpc(description = "Is Health profile ready.")
    public Boolean bluetoothHealthIsReady() {
        return sIsHealthReady;
    }

    @Rpc(description = "Connect to channel source")
    public boolean bluetoothHealthConnectChannelToSource(
            @RpcParameter(name = "configIndex") Integer configIndex,
            @RpcParameter(name = "macAddress") String macAddress)
            throws Exception {
        BluetoothDevice mDevice = mBluetoothAdapter.getRemoteDevice(macAddress);
        if (mConfigList.get(configIndex) != null) {
            return sHealthProfile.connectChannelToSource(mDevice, mConfigList.get(configIndex));
        } else {
            throw new Exception("Invalid configIndex input:" + Integer.toString(configIndex));
        }
    }

    @Rpc(description = "Connect to channel source")
    public boolean bluetoothHealthConnectChannelToSink(
            @RpcParameter(name = "configIndex") Integer configIndex,
            @RpcParameter(name = "macAddress") String macAddress,
            @RpcParameter(name = "channelType") Integer channelType)
            throws Exception {
        BluetoothDevice mDevice = mBluetoothAdapter.getRemoteDevice(macAddress);
        if (mConfigList.get(configIndex) != null) {
            return sHealthProfile.connectChannelToSink(mDevice, mConfigList.get(configIndex), channelType);
        } else {
            throw new Exception("Invalid configIndex input:" + Integer.toString(configIndex));
        }
    }

    @Rpc(description = "Create BluetoothHealthAppConfiguration config.")
    public int bluetoothHealthCreateConfig(
            @RpcParameter(name = "name") String name,
            @RpcParameter(name = "dataType") Integer dataType) throws Exception{
        Class btHealthAppConfigClass = Class.forName(
            "android.bluetooth.BluetoothHealthAppConfiguration");
        Constructor mConfigConstructor = btHealthAppConfigClass.getDeclaredConstructor(
            new Class[]{String.class, int.class});
        mConfigConstructor.setAccessible(true);
        BluetoothHealthAppConfiguration mConfig = (BluetoothHealthAppConfiguration) mConfigConstructor.newInstance(
            name, dataType);
        ConfigCount += 1;
        mConfigList.put(ConfigCount, mConfig);
        return ConfigCount;
    }

    @Rpc(description = "Register BluetoothHealthAppConfiguration config.")
    public int bluetoothHealthRegisterAppConfiguration(
            @RpcParameter(name = "name") String name,
            @RpcParameter(name = "dataType") Integer dataType,
            @RpcParameter(name = "role") Integer role) throws Exception{
        Class btHealthAppConfigClass = Class.forName(
            "android.bluetooth.BluetoothHealthAppConfiguration");
        Constructor[] cons = btHealthAppConfigClass.getConstructors();
        for (int i = 0; i < cons.length; i++) {
            System.out.println("constuctor: " + cons[i]);
        }
        Constructor[] cons2 = btHealthAppConfigClass.getDeclaredConstructors();
        for (int i = 0; i < cons2.length; i++) {
            System.out.println("constuctor: " + cons2[i]);
        }
        System.out.println(Integer.toString(cons.length));
        System.out.println(Integer.toString(cons2.length));
        Constructor mConfigConstructor = btHealthAppConfigClass.getDeclaredConstructor(
            new Class[]{String.class, int.class});
        mConfigConstructor.setAccessible(true);
        BluetoothHealthAppConfiguration mConfig = (BluetoothHealthAppConfiguration) mConfigConstructor.newInstance(
            name, dataType);
        ConfigCount += 1;
        mConfigList.put(ConfigCount, mConfig);
        return ConfigCount;
    }

    @Rpc(description = "BluetoothRegisterSinkAppConfiguration.")
    public void bluetoothHealthRegisterSinkAppConfiguration(
            @RpcParameter(name = "name") String name,
            @RpcParameter(name = "dataType") Integer dataType) throws Exception{

        sHealthProfile.registerSinkAppConfiguration(name, dataType, new MyBluetoothHealthCallback());
    }

    private class MyBluetoothHealthCallback extends BluetoothHealthCallback {

        @Override
        public void onHealthAppConfigurationStatusChange(BluetoothHealthAppConfiguration config,
                int status) {
        }

        @Override
        public void onHealthChannelStateChange(BluetoothHealthAppConfiguration config,
                BluetoothDevice device,
                int prevState,
                int newState,
                ParcelFileDescriptor fd,
                int channelId) {
        }
    }

    @Override
    public void shutdown() {
        mConfigList.clear();
    }
}
