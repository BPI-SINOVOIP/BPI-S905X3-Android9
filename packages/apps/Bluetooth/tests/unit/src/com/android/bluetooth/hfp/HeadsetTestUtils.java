/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.bluetooth.hfp;

import static org.mockito.Mockito.*;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothProfile;
import android.content.Intent;

import org.junit.Assert;

/**
 * Helper functions for HFP related tests
 */
public class HeadsetTestUtils {

    /**
     * Verify the content of a {@link BluetoothHeadset#ACTION_AUDIO_STATE_CHANGED} intent
     *
     * @param device Bluetooth device
     * @param toState value of {@link BluetoothProfile#EXTRA_STATE}
     * @param fromState value of {@link BluetoothProfile#EXTRA_PREVIOUS_STATE}
     * @param intent a {@link BluetoothHeadset#ACTION_AUDIO_STATE_CHANGED} intent
     */
    public static void verifyAudioStateBroadcast(BluetoothDevice device, int toState, int fromState,
            Intent intent) {
        Assert.assertNotNull(intent);
        Assert.assertEquals(BluetoothHeadset.ACTION_AUDIO_STATE_CHANGED, intent.getAction());
        Assert.assertEquals(device, intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE));
        Assert.assertEquals(toState, intent.getIntExtra(BluetoothProfile.EXTRA_STATE, -1));
        Assert.assertEquals(fromState,
                intent.getIntExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, -1));
    }

    /**
     * Verify the content of a {@link BluetoothHeadset#ACTION_CONNECTION_STATE_CHANGED} intent
     *
     * @param device Bluetooth device
     * @param toState value of {@link BluetoothProfile#EXTRA_STATE}
     * @param fromState value of {@link BluetoothProfile#EXTRA_PREVIOUS_STATE}
     * @param intent a {@link BluetoothHeadset#ACTION_CONNECTION_STATE_CHANGED} intent
     * @param checkFlag whether intent flag should be verified, normally this can only be done at
     *                  the sender end
     */
    public static void verifyConnectionStateBroadcast(BluetoothDevice device, int toState,
            int fromState, Intent intent, boolean checkFlag) {
        Assert.assertNotNull(intent);
        Assert.assertEquals(BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED, intent.getAction());
        if (checkFlag) {
            Assert.assertEquals(Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND, intent.getFlags());
        }
        Assert.assertEquals(device, intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE));
        Assert.assertEquals(toState, intent.getIntExtra(BluetoothProfile.EXTRA_STATE, -1));
        Assert.assertEquals(fromState,
                intent.getIntExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, -1));
    }

    /**
     * Verify the content of a {@link BluetoothHeadset#ACTION_CONNECTION_STATE_CHANGED} intent
     * and its flag, normally used at sender end
     *
     * @param device Bluetooth device
     * @param toState value of {@link BluetoothProfile#EXTRA_STATE}
     * @param fromState value of {@link BluetoothProfile#EXTRA_PREVIOUS_STATE}
     * @param intent a {@link BluetoothHeadset#ACTION_CONNECTION_STATE_CHANGED} intent
     */
    public static void verifyConnectionStateBroadcast(BluetoothDevice device, int toState,
            int fromState, Intent intent) {
        verifyConnectionStateBroadcast(device, toState, fromState, intent, true);
    }

    /**
     * Verify the content of a {@link BluetoothHeadset#ACTION_ACTIVE_DEVICE_CHANGED} intent
     * and its flag, normally used at sender end
     *
     * @param device intended active Bluetooth device
     * @param intent a {@link BluetoothHeadset#ACTION_ACTIVE_DEVICE_CHANGED} intent
     * @param checkFlag whether intent flag should be verified, normally this can only be done at
     *                  the sender end
     */
    public static void verifyActiveDeviceChangedBroadcast(BluetoothDevice device, Intent intent,
            boolean checkFlag) {
        Assert.assertNotNull(intent);
        Assert.assertEquals(BluetoothHeadset.ACTION_ACTIVE_DEVICE_CHANGED, intent.getAction());
        Assert.assertEquals(device, intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE));
        if (checkFlag) {
            Assert.assertEquals(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT
                    | Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND, intent.getFlags());
        }
    }

    /**
     * Helper function to check if {@link HeadsetPhoneState} is set to correct values indicated in
     * {@param headsetCallState}
     *
     * @param headsetPhoneState a mocked {@link HeadsetPhoneState}
     * @param headsetCallState intended headset call state
     * @param timeoutMs timeout for this check in asynchronous test conditions
     */
    public static void verifyPhoneStateChangeSetters(HeadsetPhoneState headsetPhoneState,
            HeadsetCallState headsetCallState, int timeoutMs) {
        verify(headsetPhoneState, timeout(timeoutMs).times(1)).setNumActiveCall(
                headsetCallState.mNumActive);
        verify(headsetPhoneState, timeout(timeoutMs).times(1)).setNumHeldCall(
                headsetCallState.mNumHeld);
        verify(headsetPhoneState, timeout(timeoutMs).times(1)).setCallState(
                headsetCallState.mCallState);
    }
}
