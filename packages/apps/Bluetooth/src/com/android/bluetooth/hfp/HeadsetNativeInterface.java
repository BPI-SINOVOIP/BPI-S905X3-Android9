/*
 * Copyright 2017 The Android Open Source Project
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

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import com.android.bluetooth.Utils;

/**
 * Defines native calls that are used by state machine/service to either send or receive
 * messages to/from the native stack. This file is registered for the native methods in
 * corresponding CPP file.
 */
public class HeadsetNativeInterface {
    private static final String TAG = "HeadsetNativeInterface";

    private final BluetoothAdapter mAdapter = BluetoothAdapter.getDefaultAdapter();

    static {
        classInitNative();
    }

    private static HeadsetNativeInterface sInterface;
    private static final Object INSTANCE_LOCK = new Object();

    private HeadsetNativeInterface() {}

    /**
     * This class is a singleton because native library should only be loaded once
     *
     * @return default instance
     */
    public static HeadsetNativeInterface getInstance() {
        synchronized (INSTANCE_LOCK) {
            if (sInterface == null) {
                sInterface = new HeadsetNativeInterface();
            }
        }
        return sInterface;
    }

    private void sendMessageToService(HeadsetStackEvent event) {
        HeadsetService service = HeadsetService.getHeadsetService();
        if (service != null) {
            service.messageFromNative(event);
        } else {
            // Service must call cleanup() when quiting and native stack shouldn't send any event
            // after cleanup() -> cleanupNative() is called.
            Log.wtfStack(TAG, "FATAL: Stack sent event while service is not available: " + event);
        }
    }

    private BluetoothDevice getDevice(byte[] address) {
        return mAdapter.getRemoteDevice(Utils.getAddressStringFromByte(address));
    }

    void onConnectionStateChanged(int state, byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED, state,
                        getDevice(address));
        sendMessageToService(event);
    }

    // Callbacks for native code

    private void onAudioStateChanged(int state, byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_AUDIO_STATE_CHANGED, state,
                        getDevice(address));
        sendMessageToService(event);
    }

    private void onVrStateChanged(int state, byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_VR_STATE_CHANGED, state,
                        getDevice(address));
        sendMessageToService(event);
    }

    private void onAnswerCall(byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_ANSWER_CALL, getDevice(address));
        sendMessageToService(event);
    }

    private void onHangupCall(byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_HANGUP_CALL, getDevice(address));
        sendMessageToService(event);
    }

    private void onVolumeChanged(int type, int volume, byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_VOLUME_CHANGED, type, volume,
                        getDevice(address));
        sendMessageToService(event);
    }

    private void onDialCall(String number, byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_DIAL_CALL, number,
                        getDevice(address));
        sendMessageToService(event);
    }

    private void onSendDtmf(int dtmf, byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_SEND_DTMF, dtmf,
                        getDevice(address));
        sendMessageToService(event);
    }

    private void onNoiceReductionEnable(boolean enable, byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_NOICE_REDUCTION, enable ? 1 : 0,
                        getDevice(address));
        sendMessageToService(event);
    }

    private void onWBS(int codec, byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_WBS, codec, getDevice(address));
        sendMessageToService(event);
    }

    private void onAtChld(int chld, byte[] address) {
        HeadsetStackEvent event = new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_AT_CHLD, chld,
                getDevice(address));
        sendMessageToService(event);
    }

    private void onAtCnum(byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_SUBSCRIBER_NUMBER_REQUEST,
                        getDevice(address));
        sendMessageToService(event);
    }

    private void onAtCind(byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_AT_CIND, getDevice(address));
        sendMessageToService(event);
    }

    private void onAtCops(byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_AT_COPS, getDevice(address));
        sendMessageToService(event);
    }

    private void onAtClcc(byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_AT_CLCC, getDevice(address));
        sendMessageToService(event);
    }

    private void onUnknownAt(String atString, byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_UNKNOWN_AT, atString,
                        getDevice(address));
        sendMessageToService(event);
    }

    private void onKeyPressed(byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_KEY_PRESSED, getDevice(address));
        sendMessageToService(event);
    }

    private void onATBind(String atString, byte[] address) {
        HeadsetStackEvent event = new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_BIND, atString,
                getDevice(address));
        sendMessageToService(event);
    }

    private void onATBiev(int indId, int indValue, byte[] address) {
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_BIEV, indId, indValue,
                        getDevice(address));
        sendMessageToService(event);
    }

    private void onAtBia(boolean service, boolean roam, boolean signal, boolean battery,
            byte[] address) {
        HeadsetAgIndicatorEnableState agIndicatorEnableState =
                new HeadsetAgIndicatorEnableState(service, roam, signal, battery);
        HeadsetStackEvent event =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_BIA, agIndicatorEnableState,
                        getDevice(address));
        sendMessageToService(event);
    }

    // Native wrappers to help unit testing

    /**
     * Initialize native stack
     *
     * @param maxHfClients maximum number of headset clients that can be connected simultaneously
     * @param inbandRingingEnabled whether in-band ringing is enabled on this AG
     */
    @VisibleForTesting
    public void init(int maxHfClients, boolean inbandRingingEnabled) {
        initializeNative(maxHfClients, inbandRingingEnabled);
    }

    /**
     * Closes the interface
     */
    @VisibleForTesting
    public void cleanup() {
        cleanupNative();
    }

    /**
     * ok/error response
     *
     * @param device target device
     * @param responseCode 0 - ERROR, 1 - OK
     * @param errorCode error code in case of ERROR
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean atResponseCode(BluetoothDevice device, int responseCode, int errorCode) {
        return atResponseCodeNative(responseCode, errorCode, Utils.getByteAddress(device));
    }

    /**
     * Pre-formatted AT response, typically in response to unknown AT cmd
     *
     * @param device target device
     * @param responseString formatted AT response string
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean atResponseString(BluetoothDevice device, String responseString) {
        return atResponseStringNative(responseString, Utils.getByteAddress(device));
    }

    /**
     * Connect to headset
     *
     * @param device target headset
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean connectHfp(BluetoothDevice device) {
        return connectHfpNative(Utils.getByteAddress(device));
    }

    /**
     * Disconnect from headset
     *
     * @param device target headset
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean disconnectHfp(BluetoothDevice device) {
        return disconnectHfpNative(Utils.getByteAddress(device));
    }

    /**
     * Connect HFP audio (SCO) to headset
     *
     * @param device target headset
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean connectAudio(BluetoothDevice device) {
        return connectAudioNative(Utils.getByteAddress(device));
    }

    /**
     * Disconnect HFP audio (SCO) from to headset
     *
     * @param device target headset
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean disconnectAudio(BluetoothDevice device) {
        return disconnectAudioNative(Utils.getByteAddress(device));
    }

    /**
     * Start voice recognition
     *
     * @param device target headset
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean startVoiceRecognition(BluetoothDevice device) {
        return startVoiceRecognitionNative(Utils.getByteAddress(device));
    }


    /**
     * Stop voice recognition
     *
     * @param device target headset
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean stopVoiceRecognition(BluetoothDevice device) {
        return stopVoiceRecognitionNative(Utils.getByteAddress(device));
    }

    /**
     * Set HFP audio (SCO) volume
     *
     * @param device target headset
     * @param volumeType type of volume
     * @param volume value value
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean setVolume(BluetoothDevice device, int volumeType, int volume) {
        return setVolumeNative(volumeType, volume, Utils.getByteAddress(device));
    }

    /**
     * Response for CIND command
     *
     * @param device target device
     * @param service service availability, 0 - no service, 1 - presence of service
     * @param numActive number of active calls
     * @param numHeld number of held calls
     * @param callState overall call state [0-6]
     * @param signal signal quality [0-5]
     * @param roam roaming indicator, 0 - not roaming, 1 - roaming
     * @param batteryCharge battery charge level [0-5]
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean cindResponse(BluetoothDevice device, int service, int numActive, int numHeld,
            int callState, int signal, int roam, int batteryCharge) {
        return cindResponseNative(service, numActive, numHeld, callState, signal, roam,
                batteryCharge, Utils.getByteAddress(device));
    }

    /**
     * Combined device status change notification
     *
     * @param device target device
     * @param deviceState device status object
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean notifyDeviceStatus(BluetoothDevice device, HeadsetDeviceState deviceState) {
        return notifyDeviceStatusNative(deviceState.mService, deviceState.mRoam,
                deviceState.mSignal, deviceState.mBatteryCharge, Utils.getByteAddress(device));
    }

    /**
     * Response for CLCC command. Can be iteratively called for each call index. Call index of 0
     * will be treated as NULL termination (Completes response)
     *
     * @param device target device
     * @param index index of the call given by the sequence of setting up or receiving the calls
     * as seen by the served subscriber. Calls hold their number until they are released. New
     * calls take the lowest available number.
     * @param dir direction of the call, 0 (outgoing), 1 (incoming)
     * @param status 0 = Active, 1 = Held, 2 = Dialing (outgoing calls only), 3 = Alerting
     * (outgoing calls only), 4 = Incoming (incoming calls only), 5 = Waiting (incoming calls
     * only), 6 = Call held by Response and Hold
     * @param mode 0 (Voice), 1 (Data), 2 (FAX)
     * @param mpty 0 - this call is NOT a member of a multi-party (conference) call, 1 - this
     * call IS a member of a multi-party (conference) call
     * @param number optional
     * @param type optional
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean clccResponse(BluetoothDevice device, int index, int dir, int status, int mode,
            boolean mpty, String number, int type) {
        return clccResponseNative(index, dir, status, mode, mpty, number, type,
                Utils.getByteAddress(device));
    }

    /**
     * Response for COPS command
     *
     * @param device target device
     * @param operatorName operator name
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean copsResponse(BluetoothDevice device, String operatorName) {
        return copsResponseNative(operatorName, Utils.getByteAddress(device));
    }

    /**
     *  Notify of a call state change
     *  Each update notifies
     *    1. Number of active/held/ringing calls
     *    2. call_state: This denotes the state change that triggered this msg
     *                   This will take one of the values from BtHfCallState
     *    3. number & type: valid only for incoming & waiting call
     *
     * @param device target device for this update
     * @param callState callstate structure
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean phoneStateChange(BluetoothDevice device, HeadsetCallState callState) {
        return phoneStateChangeNative(callState.mNumActive, callState.mNumHeld,
                callState.mCallState, callState.mNumber, callState.mType,
                Utils.getByteAddress(device));
    }

    /**
     * Set whether we will initiate SCO or not
     *
     * @param value True to enable, False to disable
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean setScoAllowed(boolean value) {
        return setScoAllowedNative(value);
    }

    /**
     * Enable or disable in-band ringing for the current service level connection through sending
     * +BSIR AT command
     *
     * @param value True to enable, False to disable
     * @return True on success, False on failure
     */
    @VisibleForTesting
    public boolean sendBsir(BluetoothDevice device, boolean value) {
        return sendBsirNative(value, Utils.getByteAddress(device));
    }

    /**
     * Set the current active headset device for SCO audio
     * @param device current active SCO device
     * @return true on success
     */
    @VisibleForTesting
    public boolean setActiveDevice(BluetoothDevice device) {
        return setActiveDeviceNative(Utils.getByteAddress(device));
    }

    /* Native methods */
    private static native void classInitNative();

    private native boolean atResponseCodeNative(int responseCode, int errorCode, byte[] address);

    private native boolean atResponseStringNative(String responseString, byte[] address);

    private native void initializeNative(int maxHfClients, boolean inbandRingingEnabled);

    private native void cleanupNative();

    private native boolean connectHfpNative(byte[] address);

    private native boolean disconnectHfpNative(byte[] address);

    private native boolean connectAudioNative(byte[] address);

    private native boolean disconnectAudioNative(byte[] address);

    private native boolean startVoiceRecognitionNative(byte[] address);

    private native boolean stopVoiceRecognitionNative(byte[] address);

    private native boolean setVolumeNative(int volumeType, int volume, byte[] address);

    private native boolean cindResponseNative(int service, int numActive, int numHeld,
            int callState, int signal, int roam, int batteryCharge, byte[] address);

    private native boolean notifyDeviceStatusNative(int networkState, int serviceType, int signal,
            int batteryCharge, byte[] address);

    private native boolean clccResponseNative(int index, int dir, int status, int mode,
            boolean mpty, String number, int type, byte[] address);

    private native boolean copsResponseNative(String operatorName, byte[] address);

    private native boolean phoneStateChangeNative(int numActive, int numHeld, int callState,
            String number, int type, byte[] address);

    private native boolean setScoAllowedNative(boolean value);

    private native boolean sendBsirNative(boolean value, byte[] address);

    private native boolean setActiveDeviceNative(byte[] address);
}
