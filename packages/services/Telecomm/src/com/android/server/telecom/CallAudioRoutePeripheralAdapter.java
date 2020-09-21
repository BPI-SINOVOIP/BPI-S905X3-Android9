/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.server.telecom;

import com.android.server.telecom.bluetooth.BluetoothRouteManager;

/**
 * A class that acts as a listener to things that could change call audio routing, namely
 * bluetooth status, wired headset status, and dock status.
 */
public class CallAudioRoutePeripheralAdapter implements WiredHeadsetManager.Listener,
        DockManager.Listener, BluetoothRouteManager.BluetoothStateListener {

    private final CallAudioRouteStateMachine mCallAudioRouteStateMachine;
    private final BluetoothRouteManager mBluetoothRouteManager;

    public CallAudioRoutePeripheralAdapter(
            CallAudioRouteStateMachine callAudioRouteStateMachine,
            BluetoothRouteManager bluetoothManager,
            WiredHeadsetManager wiredHeadsetManager,
            DockManager dockManager) {
        mCallAudioRouteStateMachine = callAudioRouteStateMachine;
        mBluetoothRouteManager = bluetoothManager;

        mBluetoothRouteManager.setListener(this);
        wiredHeadsetManager.addListener(this);
        dockManager.addListener(this);
    }

    public boolean isBluetoothAudioOn() {
        return mBluetoothRouteManager.isBluetoothAudioConnectedOrPending();
    }

    @Override
    public void onBluetoothDeviceListChanged() {
        mCallAudioRouteStateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.BLUETOOTH_DEVICE_LIST_CHANGED);
    }

    @Override
    public void onBluetoothActiveDevicePresent() {
        mCallAudioRouteStateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.BT_ACTIVE_DEVICE_PRESENT);
    }

    @Override
    public void onBluetoothActiveDeviceGone() {
        mCallAudioRouteStateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.BT_ACTIVE_DEVICE_GONE);
    }

    @Override
    public void onBluetoothAudioConnected() {
        mCallAudioRouteStateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.BT_AUDIO_CONNECTED);
    }

    @Override
    public void onBluetoothAudioDisconnected() {
        mCallAudioRouteStateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.BT_AUDIO_DISCONNECTED);
    }

    /**
      * Updates the audio route when the headset plugged in state changes. For example, if audio is
      * being routed over speakerphone and a headset is plugged in then switch to wired headset.
      */
    @Override
    public void onWiredHeadsetPluggedInChanged(boolean oldIsPluggedIn, boolean newIsPluggedIn) {
        if (!oldIsPluggedIn && newIsPluggedIn) {
            mCallAudioRouteStateMachine.sendMessageWithSessionInfo(
                    CallAudioRouteStateMachine.CONNECT_WIRED_HEADSET);
        } else if (oldIsPluggedIn && !newIsPluggedIn){
            mCallAudioRouteStateMachine.sendMessageWithSessionInfo(
                    CallAudioRouteStateMachine.DISCONNECT_WIRED_HEADSET);
        }
    }

    @Override
    public void onDockChanged(boolean isDocked) {
        mCallAudioRouteStateMachine.sendMessageWithSessionInfo(
                isDocked ? CallAudioRouteStateMachine.CONNECT_DOCK
                        : CallAudioRouteStateMachine.DISCONNECT_DOCK
        );
    }
}
