/*
 * Copyright (C) 2012 The Android Open Source Project
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

import android.bluetooth.BluetoothAssignedNumbers;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothProfile;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.os.UserHandle;
import android.support.annotation.VisibleForTesting;
import android.telephony.PhoneNumberUtils;
import android.telephony.PhoneStateListener;
import android.util.Log;

import com.android.bluetooth.btservice.AdapterService;
import com.android.bluetooth.btservice.ProfileService;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import java.util.Scanner;

/**
 * A Bluetooth Handset StateMachine
 *                        (Disconnected)
 *                           |      ^
 *                   CONNECT |      | DISCONNECTED
 *                           V      |
 *                  (Connecting)   (Disconnecting)
 *                           |      ^
 *                 CONNECTED |      | DISCONNECT
 *                           V      |
 *                          (Connected)
 *                           |      ^
 *             CONNECT_AUDIO |      | AUDIO_DISCONNECTED
 *                           V      |
 *             (AudioConnecting)   (AudioDiconnecting)
 *                           |      ^
 *           AUDIO_CONNECTED |      | DISCONNECT_AUDIO
 *                           V      |
 *                           (AudioOn)
 */
@VisibleForTesting
public class HeadsetStateMachine extends StateMachine {
    private static final String TAG = "HeadsetStateMachine";
    private static final boolean DBG = false;

    private static final String HEADSET_NAME = "bt_headset_name";
    private static final String HEADSET_NREC = "bt_headset_nrec";
    private static final String HEADSET_WBS = "bt_wbs";
    private static final String HEADSET_AUDIO_FEATURE_ON = "on";
    private static final String HEADSET_AUDIO_FEATURE_OFF = "off";

    static final int CONNECT = 1;
    static final int DISCONNECT = 2;
    static final int CONNECT_AUDIO = 3;
    static final int DISCONNECT_AUDIO = 4;
    static final int VOICE_RECOGNITION_START = 5;
    static final int VOICE_RECOGNITION_STOP = 6;

    // message.obj is an intent AudioManager.VOLUME_CHANGED_ACTION
    // EXTRA_VOLUME_STREAM_TYPE is STREAM_BLUETOOTH_SCO
    static final int INTENT_SCO_VOLUME_CHANGED = 7;
    static final int INTENT_CONNECTION_ACCESS_REPLY = 8;
    static final int CALL_STATE_CHANGED = 9;
    static final int DEVICE_STATE_CHANGED = 10;
    static final int SEND_CCLC_RESPONSE = 11;
    static final int SEND_VENDOR_SPECIFIC_RESULT_CODE = 12;
    static final int SEND_BSIR = 13;
    static final int DIALING_OUT_RESULT = 14;
    static final int VOICE_RECOGNITION_RESULT = 15;

    static final int STACK_EVENT = 101;
    private static final int CLCC_RSP_TIMEOUT = 104;

    private static final int CONNECT_TIMEOUT = 201;

    private static final int CLCC_RSP_TIMEOUT_MS = 5000;
    // NOTE: the value is not "final" - it is modified in the unit tests
    @VisibleForTesting static int sConnectTimeoutMs = 30000;

    private static final HeadsetAgIndicatorEnableState DEFAULT_AG_INDICATOR_ENABLE_STATE =
            new HeadsetAgIndicatorEnableState(true, true, true, true);

    private final BluetoothDevice mDevice;

    // State machine states
    private final Disconnected mDisconnected = new Disconnected();
    private final Connecting mConnecting = new Connecting();
    private final Disconnecting mDisconnecting = new Disconnecting();
    private final Connected mConnected = new Connected();
    private final AudioOn mAudioOn = new AudioOn();
    private final AudioConnecting mAudioConnecting = new AudioConnecting();
    private final AudioDisconnecting mAudioDisconnecting = new AudioDisconnecting();
    private HeadsetStateBase mPrevState;

    // Run time dependencies
    private final HeadsetService mHeadsetService;
    private final AdapterService mAdapterService;
    private final HeadsetNativeInterface mNativeInterface;
    private final HeadsetSystemInterface mSystemInterface;

    // Runtime states
    private int mSpeakerVolume;
    private int mMicVolume;
    private HeadsetAgIndicatorEnableState mAgIndicatorEnableState;
    // The timestamp when the device entered connecting/connected state
    private long mConnectingTimestampMs = Long.MIN_VALUE;
    // Audio Parameters like NREC
    private final HashMap<String, String> mAudioParams = new HashMap<>();
    // AT Phone book keeps a group of states used by AT+CPBR commands
    private final AtPhonebook mPhonebook;
    // HSP specific
    private boolean mNeedDialingOutReply;

    // Keys are AT commands, and values are the company IDs.
    private static final Map<String, Integer> VENDOR_SPECIFIC_AT_COMMAND_COMPANY_ID;

    static {
        VENDOR_SPECIFIC_AT_COMMAND_COMPANY_ID = new HashMap<>();
        VENDOR_SPECIFIC_AT_COMMAND_COMPANY_ID.put(
                BluetoothHeadset.VENDOR_SPECIFIC_HEADSET_EVENT_XEVENT,
                BluetoothAssignedNumbers.PLANTRONICS);
        VENDOR_SPECIFIC_AT_COMMAND_COMPANY_ID.put(
                BluetoothHeadset.VENDOR_RESULT_CODE_COMMAND_ANDROID,
                BluetoothAssignedNumbers.GOOGLE);
        VENDOR_SPECIFIC_AT_COMMAND_COMPANY_ID.put(
                BluetoothHeadset.VENDOR_SPECIFIC_HEADSET_EVENT_XAPL,
                BluetoothAssignedNumbers.APPLE);
        VENDOR_SPECIFIC_AT_COMMAND_COMPANY_ID.put(
                BluetoothHeadset.VENDOR_SPECIFIC_HEADSET_EVENT_IPHONEACCEV,
                BluetoothAssignedNumbers.APPLE);
    }

    private HeadsetStateMachine(BluetoothDevice device, Looper looper,
            HeadsetService headsetService, AdapterService adapterService,
            HeadsetNativeInterface nativeInterface, HeadsetSystemInterface systemInterface) {
        super(TAG, Objects.requireNonNull(looper, "looper cannot be null"));
        // Enable/Disable StateMachine debug logs
        setDbg(DBG);
        mDevice = Objects.requireNonNull(device, "device cannot be null");
        mHeadsetService = Objects.requireNonNull(headsetService, "headsetService cannot be null");
        mNativeInterface =
                Objects.requireNonNull(nativeInterface, "nativeInterface cannot be null");
        mSystemInterface =
                Objects.requireNonNull(systemInterface, "systemInterface cannot be null");
        mAdapterService = Objects.requireNonNull(adapterService, "AdapterService cannot be null");
        // Create phonebook helper
        mPhonebook = new AtPhonebook(mHeadsetService, mNativeInterface);
        // Initialize state machine
        addState(mDisconnected);
        addState(mConnecting);
        addState(mDisconnecting);
        addState(mConnected);
        addState(mAudioOn);
        addState(mAudioConnecting);
        addState(mAudioDisconnecting);
        setInitialState(mDisconnected);
    }

    static HeadsetStateMachine make(BluetoothDevice device, Looper looper,
            HeadsetService headsetService, AdapterService adapterService,
            HeadsetNativeInterface nativeInterface, HeadsetSystemInterface systemInterface) {
        HeadsetStateMachine stateMachine =
                new HeadsetStateMachine(device, looper, headsetService, adapterService,
                        nativeInterface, systemInterface);
        stateMachine.start();
        Log.i(TAG, "Created state machine " + stateMachine + " for " + device);
        return stateMachine;
    }

    static void destroy(HeadsetStateMachine stateMachine) {
        Log.i(TAG, "destroy");
        if (stateMachine == null) {
            Log.w(TAG, "destroy(), stateMachine is null");
            return;
        }
        stateMachine.quitNow();
        stateMachine.cleanup();
    }

    public void cleanup() {
        if (mPhonebook != null) {
            mPhonebook.cleanup();
        }
        mAudioParams.clear();
    }

    public void dump(StringBuilder sb) {
        ProfileService.println(sb, "  mCurrentDevice: " + mDevice);
        ProfileService.println(sb, "  mCurrentState: " + getCurrentState());
        ProfileService.println(sb, "  mPrevState: " + mPrevState);
        ProfileService.println(sb, "  mConnectionState: " + getConnectionState());
        ProfileService.println(sb, "  mAudioState: " + getAudioState());
        ProfileService.println(sb, "  mNeedDialingOutReply: " + mNeedDialingOutReply);
        ProfileService.println(sb, "  mSpeakerVolume: " + mSpeakerVolume);
        ProfileService.println(sb, "  mMicVolume: " + mMicVolume);
        ProfileService.println(sb,
                "  mConnectingTimestampMs(uptimeMillis): " + mConnectingTimestampMs);
        ProfileService.println(sb, "  StateMachine: " + this);
        // Dump the state machine logs
        StringWriter stringWriter = new StringWriter();
        PrintWriter printWriter = new PrintWriter(stringWriter);
        super.dump(new FileDescriptor(), printWriter, new String[]{});
        printWriter.flush();
        stringWriter.flush();
        ProfileService.println(sb, "  StateMachineLog:");
        Scanner scanner = new Scanner(stringWriter.toString());
        while (scanner.hasNextLine()) {
            String line = scanner.nextLine();
            ProfileService.println(sb, "    " + line);
        }
        scanner.close();
    }

    /**
     * Base class for states used in this state machine to share common infrastructures
     */
    private abstract class HeadsetStateBase extends State {
        @Override
        public void enter() {
            // Crash if mPrevState is null and state is not Disconnected
            if (!(this instanceof Disconnected) && mPrevState == null) {
                throw new IllegalStateException("mPrevState is null on enter()");
            }
            enforceValidConnectionStateTransition();
        }

        @Override
        public void exit() {
            mPrevState = this;
        }

        @Override
        public String toString() {
            return getName();
        }

        /**
         * Broadcast audio and connection state changes to the system. This should be called at the
         * end of enter() method after all the setup is done
         */
        void broadcastStateTransitions() {
            if (mPrevState == null) {
                return;
            }
            // TODO: Add STATE_AUDIO_DISCONNECTING constant to get rid of the 2nd part of this logic
            if (getAudioStateInt() != mPrevState.getAudioStateInt() || (
                    mPrevState instanceof AudioDisconnecting && this instanceof AudioOn)) {
                stateLogD("audio state changed: " + mDevice + ": " + mPrevState + " -> " + this);
                broadcastAudioState(mDevice, mPrevState.getAudioStateInt(), getAudioStateInt());
            }
            if (getConnectionStateInt() != mPrevState.getConnectionStateInt()) {
                stateLogD(
                        "connection state changed: " + mDevice + ": " + mPrevState + " -> " + this);
                broadcastConnectionState(mDevice, mPrevState.getConnectionStateInt(),
                        getConnectionStateInt());
            }
        }

        // Should not be called from enter() method
        void broadcastConnectionState(BluetoothDevice device, int fromState, int toState) {
            stateLogD("broadcastConnectionState " + device + ": " + fromState + "->" + toState);
            mHeadsetService.onConnectionStateChangedFromStateMachine(device, fromState, toState);
            Intent intent = new Intent(BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED);
            intent.putExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, fromState);
            intent.putExtra(BluetoothProfile.EXTRA_STATE, toState);
            intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
            intent.addFlags(Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND);
            mHeadsetService.sendBroadcastAsUser(intent, UserHandle.ALL,
                    HeadsetService.BLUETOOTH_PERM);
        }

        // Should not be called from enter() method
        void broadcastAudioState(BluetoothDevice device, int fromState, int toState) {
            stateLogD("broadcastAudioState: " + device + ": " + fromState + "->" + toState);
            mHeadsetService.onAudioStateChangedFromStateMachine(device, fromState, toState);
            Intent intent = new Intent(BluetoothHeadset.ACTION_AUDIO_STATE_CHANGED);
            intent.putExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, fromState);
            intent.putExtra(BluetoothProfile.EXTRA_STATE, toState);
            intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
            mHeadsetService.sendBroadcastAsUser(intent, UserHandle.ALL,
                    HeadsetService.BLUETOOTH_PERM);
        }

        /**
         * Verify if the current state transition is legal. This is supposed to be called from
         * enter() method and crash if the state transition is out of the specification
         *
         * Note:
         * This method uses state objects to verify transition because these objects should be final
         * and any other instances are invalid
         */
        void enforceValidConnectionStateTransition() {
            boolean result = false;
            if (this == mDisconnected) {
                result = mPrevState == null || mPrevState == mConnecting
                        || mPrevState == mDisconnecting
                        // TODO: edges to be removed after native stack refactoring
                        // all transitions to disconnected state should go through a pending state
                        // also, states should not go directly from an active audio state to
                        // disconnected state
                        || mPrevState == mConnected || mPrevState == mAudioOn
                        || mPrevState == mAudioConnecting || mPrevState == mAudioDisconnecting;
            } else if (this == mConnecting) {
                result = mPrevState == mDisconnected;
            } else if (this == mDisconnecting) {
                result = mPrevState == mConnected
                        // TODO: edges to be removed after native stack refactoring
                        // all transitions to disconnecting state should go through connected state
                        || mPrevState == mAudioConnecting || mPrevState == mAudioOn
                        || mPrevState == mAudioDisconnecting;
            } else if (this == mConnected) {
                result = mPrevState == mConnecting || mPrevState == mAudioDisconnecting
                        || mPrevState == mDisconnecting || mPrevState == mAudioConnecting
                        // TODO: edges to be removed after native stack refactoring
                        // all transitions to connected state should go through a pending state
                        || mPrevState == mAudioOn || mPrevState == mDisconnected;
            } else if (this == mAudioConnecting) {
                result = mPrevState == mConnected;
            } else if (this == mAudioDisconnecting) {
                result = mPrevState == mAudioOn;
            } else if (this == mAudioOn) {
                result = mPrevState == mAudioConnecting || mPrevState == mAudioDisconnecting
                        // TODO: edges to be removed after native stack refactoring
                        // all transitions to audio connected state should go through a pending
                        // state
                        || mPrevState == mConnected;
            }
            if (!result) {
                throw new IllegalStateException(
                        "Invalid state transition from " + mPrevState + " to " + this
                                + " for device " + mDevice);
            }
        }

        void stateLogD(String msg) {
            log(getName() + ": currentDevice=" + mDevice + ", msg=" + msg);
        }

        void stateLogW(String msg) {
            logw(getName() + ": currentDevice=" + mDevice + ", msg=" + msg);
        }

        void stateLogE(String msg) {
            loge(getName() + ": currentDevice=" + mDevice + ", msg=" + msg);
        }

        void stateLogV(String msg) {
            logv(getName() + ": currentDevice=" + mDevice + ", msg=" + msg);
        }

        void stateLogI(String msg) {
            logi(getName() + ": currentDevice=" + mDevice + ", msg=" + msg);
        }

        void stateLogWtfStack(String msg) {
            Log.wtfStack(TAG, getName() + ": " + msg);
        }

        /**
         * Process connection event
         *
         * @param message the current message for the event
         * @param state connection state to transition to
         */
        public abstract void processConnectionEvent(Message message, int state);

        /**
         * Get a state value from {@link BluetoothProfile} that represents the connection state of
         * this headset state
         *
         * @return a value in {@link BluetoothProfile#STATE_DISCONNECTED},
         * {@link BluetoothProfile#STATE_CONNECTING}, {@link BluetoothProfile#STATE_CONNECTED}, or
         * {@link BluetoothProfile#STATE_DISCONNECTING}
         */
        abstract int getConnectionStateInt();

        /**
         * Get an audio state value from {@link BluetoothHeadset}
         * @return a value in {@link BluetoothHeadset#STATE_AUDIO_DISCONNECTED},
         * {@link BluetoothHeadset#STATE_AUDIO_CONNECTING}, or
         * {@link BluetoothHeadset#STATE_AUDIO_CONNECTED}
         */
        abstract int getAudioStateInt();

    }

    class Disconnected extends HeadsetStateBase {
        @Override
        int getConnectionStateInt() {
            return BluetoothProfile.STATE_DISCONNECTED;
        }

        @Override
        int getAudioStateInt() {
            return BluetoothHeadset.STATE_AUDIO_DISCONNECTED;
        }

        @Override
        public void enter() {
            super.enter();
            mConnectingTimestampMs = Long.MIN_VALUE;
            mPhonebook.resetAtState();
            updateAgIndicatorEnableState(null);
            mNeedDialingOutReply = false;
            mAudioParams.clear();
            broadcastStateTransitions();
            // Remove the state machine for unbonded devices
            if (mPrevState != null
                    && mAdapterService.getBondState(mDevice) == BluetoothDevice.BOND_NONE) {
                getHandler().post(() -> mHeadsetService.removeStateMachine(mDevice));
            }
        }

        @Override
        public boolean processMessage(Message message) {
            switch (message.what) {
                case CONNECT:
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    stateLogD("Connecting to " + device);
                    if (!mDevice.equals(device)) {
                        stateLogE(
                                "CONNECT failed, device=" + device + ", currentDevice=" + mDevice);
                        break;
                    }
                    if (!mNativeInterface.connectHfp(device)) {
                        stateLogE("CONNECT failed for connectHfp(" + device + ")");
                        // No state transition is involved, fire broadcast immediately
                        broadcastConnectionState(device, BluetoothProfile.STATE_DISCONNECTED,
                                BluetoothProfile.STATE_DISCONNECTED);
                        break;
                    }
                    transitionTo(mConnecting);
                    break;
                case DISCONNECT:
                    // ignore
                    break;
                case CALL_STATE_CHANGED:
                    stateLogD("Ignoring CALL_STATE_CHANGED event");
                    break;
                case DEVICE_STATE_CHANGED:
                    stateLogD("Ignoring DEVICE_STATE_CHANGED event");
                    break;
                case STACK_EVENT:
                    HeadsetStackEvent event = (HeadsetStackEvent) message.obj;
                    stateLogD("STACK_EVENT: " + event);
                    if (!mDevice.equals(event.device)) {
                        stateLogE("Event device does not match currentDevice[" + mDevice
                                + "], event: " + event);
                        break;
                    }
                    switch (event.type) {
                        case HeadsetStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED:
                            processConnectionEvent(message, event.valueInt);
                            break;
                        default:
                            stateLogE("Unexpected stack event: " + event);
                            break;
                    }
                    break;
                default:
                    stateLogE("Unexpected msg " + getMessageName(message.what) + ": " + message);
                    return NOT_HANDLED;
            }
            return HANDLED;
        }

        @Override
        public void processConnectionEvent(Message message, int state) {
            stateLogD("processConnectionEvent, state=" + state);
            switch (state) {
                case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED:
                    stateLogW("ignore DISCONNECTED event");
                    break;
                // Both events result in Connecting state as SLC establishment is still required
                case HeadsetHalConstants.CONNECTION_STATE_CONNECTED:
                case HeadsetHalConstants.CONNECTION_STATE_CONNECTING:
                    if (mHeadsetService.okToAcceptConnection(mDevice)) {
                        stateLogI("accept incoming connection");
                        transitionTo(mConnecting);
                    } else {
                        stateLogI("rejected incoming HF, priority=" + mHeadsetService.getPriority(
                                mDevice) + " bondState=" + mAdapterService.getBondState(mDevice));
                        // Reject the connection and stay in Disconnected state itself
                        if (!mNativeInterface.disconnectHfp(mDevice)) {
                            stateLogE("failed to disconnect");
                        }
                        // Indicate rejection to other components.
                        broadcastConnectionState(mDevice, BluetoothProfile.STATE_DISCONNECTED,
                                BluetoothProfile.STATE_DISCONNECTED);
                    }
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTING:
                    stateLogW("Ignore DISCONNECTING event");
                    break;
                default:
                    stateLogE("Incorrect state: " + state);
                    break;
            }
        }
    }

    // Per HFP 1.7.1 spec page 23/144, Pending state needs to handle
    //      AT+BRSF, AT+CIND, AT+CMER, AT+BIND, +CHLD
    // commands during SLC establishment
    class Connecting extends HeadsetStateBase {
        @Override
        int getConnectionStateInt() {
            return BluetoothProfile.STATE_CONNECTING;
        }

        @Override
        int getAudioStateInt() {
            return BluetoothHeadset.STATE_AUDIO_DISCONNECTED;
        }

        @Override
        public void enter() {
            super.enter();
            mConnectingTimestampMs = SystemClock.uptimeMillis();
            sendMessageDelayed(CONNECT_TIMEOUT, mDevice, sConnectTimeoutMs);
            broadcastStateTransitions();
        }

        @Override
        public boolean processMessage(Message message) {
            switch (message.what) {
                case CONNECT:
                case CONNECT_AUDIO:
                case DISCONNECT:
                    deferMessage(message);
                    break;
                case CONNECT_TIMEOUT: {
                    // We timed out trying to connect, transition to Disconnected state
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (!mDevice.equals(device)) {
                        stateLogE("Unknown device timeout " + device);
                        break;
                    }
                    stateLogW("CONNECT_TIMEOUT");
                    transitionTo(mDisconnected);
                    break;
                }
                case CALL_STATE_CHANGED:
                    stateLogD("ignoring CALL_STATE_CHANGED event");
                    break;
                case DEVICE_STATE_CHANGED:
                    stateLogD("ignoring DEVICE_STATE_CHANGED event");
                    break;
                case STACK_EVENT:
                    HeadsetStackEvent event = (HeadsetStackEvent) message.obj;
                    stateLogD("STACK_EVENT: " + event);
                    if (!mDevice.equals(event.device)) {
                        stateLogE("Event device does not match currentDevice[" + mDevice
                                + "], event: " + event);
                        break;
                    }
                    switch (event.type) {
                        case HeadsetStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED:
                            processConnectionEvent(message, event.valueInt);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_AT_CHLD:
                            processAtChld(event.valueInt, event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_AT_CIND:
                            processAtCind(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_WBS:
                            processWBSEvent(event.valueInt);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_BIND:
                            processAtBind(event.valueString, event.device);
                            break;
                        // Unexpected AT commands, we only handle them for comparability reasons
                        case HeadsetStackEvent.EVENT_TYPE_VR_STATE_CHANGED:
                            stateLogW("Unexpected VR event, device=" + event.device + ", state="
                                    + event.valueInt);
                            processVrEvent(event.valueInt);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_DIAL_CALL:
                            stateLogW("Unexpected dial event, device=" + event.device);
                            processDialCall(event.valueString);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_SUBSCRIBER_NUMBER_REQUEST:
                            stateLogW("Unexpected subscriber number event for" + event.device
                                    + ", state=" + event.valueInt);
                            processSubscriberNumberRequest(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_AT_COPS:
                            stateLogW("Unexpected COPS event for " + event.device);
                            processAtCops(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_AT_CLCC:
                            Log.w(TAG, "Connecting: Unexpected CLCC event for" + event.device);
                            processAtClcc(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_UNKNOWN_AT:
                            stateLogW("Unexpected unknown AT event for" + event.device + ", cmd="
                                    + event.valueString);
                            processUnknownAt(event.valueString, event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_KEY_PRESSED:
                            stateLogW("Unexpected key-press event for " + event.device);
                            processKeyPressed(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_BIEV:
                            stateLogW("Unexpected BIEV event for " + event.device + ", indId="
                                    + event.valueInt + ", indVal=" + event.valueInt2);
                            processAtBiev(event.valueInt, event.valueInt2, event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_VOLUME_CHANGED:
                            stateLogW("Unexpected volume event for " + event.device);
                            processVolumeEvent(event.valueInt, event.valueInt2);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_ANSWER_CALL:
                            stateLogW("Unexpected answer event for " + event.device);
                            mSystemInterface.answerCall(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_HANGUP_CALL:
                            stateLogW("Unexpected hangup event for " + event.device);
                            mSystemInterface.hangupCall(event.device);
                            break;
                        default:
                            stateLogE("Unexpected event: " + event);
                            break;
                    }
                    break;
                default:
                    stateLogE("Unexpected msg " + getMessageName(message.what) + ": " + message);
                    return NOT_HANDLED;
            }
            return HANDLED;
        }

        @Override
        public void processConnectionEvent(Message message, int state) {
            stateLogD("processConnectionEvent, state=" + state);
            switch (state) {
                case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED:
                    stateLogW("Disconnected");
                    transitionTo(mDisconnected);
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_CONNECTED:
                    stateLogD("RFCOMM connected");
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_SLC_CONNECTED:
                    stateLogD("SLC connected");
                    transitionTo(mConnected);
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_CONNECTING:
                    // Ignored
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTING:
                    stateLogW("Disconnecting");
                    break;
                default:
                    stateLogE("Incorrect state " + state);
                    break;
            }
        }

        @Override
        public void exit() {
            removeMessages(CONNECT_TIMEOUT);
            super.exit();
        }
    }

    class Disconnecting extends HeadsetStateBase {
        @Override
        int getConnectionStateInt() {
            return BluetoothProfile.STATE_DISCONNECTING;
        }

        @Override
        int getAudioStateInt() {
            return BluetoothHeadset.STATE_AUDIO_DISCONNECTED;
        }

        @Override
        public void enter() {
            super.enter();
            sendMessageDelayed(CONNECT_TIMEOUT, mDevice, sConnectTimeoutMs);
            broadcastStateTransitions();
        }

        @Override
        public boolean processMessage(Message message) {
            switch (message.what) {
                case CONNECT:
                case CONNECT_AUDIO:
                case DISCONNECT:
                    deferMessage(message);
                    break;
                case CONNECT_TIMEOUT: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (!mDevice.equals(device)) {
                        stateLogE("Unknown device timeout " + device);
                        break;
                    }
                    stateLogE("timeout");
                    transitionTo(mDisconnected);
                    break;
                }
                case STACK_EVENT:
                    HeadsetStackEvent event = (HeadsetStackEvent) message.obj;
                    stateLogD("STACK_EVENT: " + event);
                    if (!mDevice.equals(event.device)) {
                        stateLogE("Event device does not match currentDevice[" + mDevice
                                + "], event: " + event);
                        break;
                    }
                    switch (event.type) {
                        case HeadsetStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED:
                            processConnectionEvent(message, event.valueInt);
                            break;
                        default:
                            stateLogE("Unexpected event: " + event);
                            break;
                    }
                    break;
                default:
                    stateLogE("Unexpected msg " + getMessageName(message.what) + ": " + message);
                    return NOT_HANDLED;
            }
            return HANDLED;
        }

        // in Disconnecting state
        @Override
        public void processConnectionEvent(Message message, int state) {
            switch (state) {
                case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED:
                    stateLogD("processConnectionEvent: Disconnected");
                    transitionTo(mDisconnected);
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_SLC_CONNECTED:
                    stateLogD("processConnectionEvent: Connected");
                    transitionTo(mConnected);
                    break;
                default:
                    stateLogE("processConnectionEvent: Bad state: " + state);
                    break;
            }
        }

        @Override
        public void exit() {
            removeMessages(CONNECT_TIMEOUT);
            super.exit();
        }
    }

    /**
     * Base class for Connected, AudioConnecting, AudioOn, AudioDisconnecting states
     */
    private abstract class ConnectedBase extends HeadsetStateBase {
        @Override
        int getConnectionStateInt() {
            return BluetoothProfile.STATE_CONNECTED;
        }

        /**
         * Handle common messages in connected states. However, state specific messages must be
         * handled individually.
         *
         * @param message Incoming message to handle
         * @return True if handled successfully, False otherwise
         */
        @Override
        public boolean processMessage(Message message) {
            switch (message.what) {
                case CONNECT:
                case DISCONNECT:
                case CONNECT_AUDIO:
                case DISCONNECT_AUDIO:
                case CONNECT_TIMEOUT:
                    throw new IllegalStateException(
                            "Illegal message in generic handler: " + message);
                case VOICE_RECOGNITION_START: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (!mDevice.equals(device)) {
                        stateLogW("VOICE_RECOGNITION_START failed " + device
                                + " is not currentDevice");
                        break;
                    }
                    if (!mNativeInterface.startVoiceRecognition(mDevice)) {
                        stateLogW("Failed to start voice recognition");
                        break;
                    }
                    break;
                }
                case VOICE_RECOGNITION_STOP: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (!mDevice.equals(device)) {
                        stateLogW("VOICE_RECOGNITION_STOP failed " + device
                                + " is not currentDevice");
                        break;
                    }
                    if (!mNativeInterface.stopVoiceRecognition(mDevice)) {
                        stateLogW("Failed to stop voice recognition");
                        break;
                    }
                    break;
                }
                case CALL_STATE_CHANGED: {
                    HeadsetCallState callState = (HeadsetCallState) message.obj;
                    if (!mNativeInterface.phoneStateChange(mDevice, callState)) {
                        stateLogW("processCallState: failed to update call state " + callState);
                        break;
                    }
                    break;
                }
                case DEVICE_STATE_CHANGED:
                    mNativeInterface.notifyDeviceStatus(mDevice, (HeadsetDeviceState) message.obj);
                    break;
                case SEND_CCLC_RESPONSE:
                    processSendClccResponse((HeadsetClccResponse) message.obj);
                    break;
                case CLCC_RSP_TIMEOUT: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (!mDevice.equals(device)) {
                        stateLogW("CLCC_RSP_TIMEOUT failed " + device + " is not currentDevice");
                        break;
                    }
                    mNativeInterface.clccResponse(device, 0, 0, 0, 0, false, "", 0);
                }
                break;
                case SEND_VENDOR_SPECIFIC_RESULT_CODE:
                    processSendVendorSpecificResultCode(
                            (HeadsetVendorSpecificResultCode) message.obj);
                    break;
                case SEND_BSIR:
                    mNativeInterface.sendBsir(mDevice, message.arg1 == 1);
                    break;
                case VOICE_RECOGNITION_RESULT: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (!mDevice.equals(device)) {
                        stateLogW("VOICE_RECOGNITION_RESULT failed " + device
                                + " is not currentDevice");
                        break;
                    }
                    mNativeInterface.atResponseCode(mDevice,
                            message.arg1 == 1 ? HeadsetHalConstants.AT_RESPONSE_OK
                                    : HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
                    break;
                }
                case DIALING_OUT_RESULT: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (!mDevice.equals(device)) {
                        stateLogW("DIALING_OUT_RESULT failed " + device + " is not currentDevice");
                        break;
                    }
                    if (mNeedDialingOutReply) {
                        mNeedDialingOutReply = false;
                        mNativeInterface.atResponseCode(mDevice,
                                message.arg1 == 1 ? HeadsetHalConstants.AT_RESPONSE_OK
                                        : HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
                    }
                }
                break;
                case INTENT_CONNECTION_ACCESS_REPLY:
                    handleAccessPermissionResult((Intent) message.obj);
                    break;
                case STACK_EVENT:
                    HeadsetStackEvent event = (HeadsetStackEvent) message.obj;
                    stateLogD("STACK_EVENT: " + event);
                    if (!mDevice.equals(event.device)) {
                        stateLogE("Event device does not match currentDevice[" + mDevice
                                + "], event: " + event);
                        break;
                    }
                    switch (event.type) {
                        case HeadsetStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED:
                            processConnectionEvent(message, event.valueInt);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_AUDIO_STATE_CHANGED:
                            processAudioEvent(event.valueInt);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_VR_STATE_CHANGED:
                            processVrEvent(event.valueInt);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_ANSWER_CALL:
                            mSystemInterface.answerCall(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_HANGUP_CALL:
                            mSystemInterface.hangupCall(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_VOLUME_CHANGED:
                            processVolumeEvent(event.valueInt, event.valueInt2);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_DIAL_CALL:
                            processDialCall(event.valueString);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_SEND_DTMF:
                            mSystemInterface.sendDtmf(event.valueInt, event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_NOICE_REDUCTION:
                            processNoiseReductionEvent(event.valueInt == 1);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_WBS:
                            processWBSEvent(event.valueInt);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_AT_CHLD:
                            processAtChld(event.valueInt, event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_SUBSCRIBER_NUMBER_REQUEST:
                            processSubscriberNumberRequest(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_AT_CIND:
                            processAtCind(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_AT_COPS:
                            processAtCops(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_AT_CLCC:
                            processAtClcc(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_UNKNOWN_AT:
                            processUnknownAt(event.valueString, event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_KEY_PRESSED:
                            processKeyPressed(event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_BIND:
                            processAtBind(event.valueString, event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_BIEV:
                            processAtBiev(event.valueInt, event.valueInt2, event.device);
                            break;
                        case HeadsetStackEvent.EVENT_TYPE_BIA:
                            updateAgIndicatorEnableState(
                                    (HeadsetAgIndicatorEnableState) event.valueObject);
                            break;
                        default:
                            stateLogE("Unknown stack event: " + event);
                            break;
                    }
                    break;
                default:
                    stateLogE("Unexpected msg " + getMessageName(message.what) + ": " + message);
                    return NOT_HANDLED;
            }
            return HANDLED;
        }

        @Override
        public void processConnectionEvent(Message message, int state) {
            stateLogD("processConnectionEvent, state=" + state);
            switch (state) {
                case HeadsetHalConstants.CONNECTION_STATE_CONNECTED:
                    stateLogE("processConnectionEvent: RFCOMM connected again, shouldn't happen");
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_SLC_CONNECTED:
                    stateLogE("processConnectionEvent: SLC connected again, shouldn't happen");
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTING:
                    stateLogI("processConnectionEvent: Disconnecting");
                    transitionTo(mDisconnecting);
                    break;
                case HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED:
                    stateLogI("processConnectionEvent: Disconnected");
                    transitionTo(mDisconnected);
                    break;
                default:
                    stateLogE("processConnectionEvent: bad state: " + state);
                    break;
            }
        }

        /**
         * Each state should handle audio events differently
         *
         * @param state audio state
         */
        public abstract void processAudioEvent(int state);
    }

    class Connected extends ConnectedBase {
        @Override
        int getAudioStateInt() {
            return BluetoothHeadset.STATE_AUDIO_DISCONNECTED;
        }

        @Override
        public void enter() {
            super.enter();
            if (mConnectingTimestampMs == Long.MIN_VALUE) {
                mConnectingTimestampMs = SystemClock.uptimeMillis();
            }
            if (mPrevState == mConnecting) {
                // Reset AG indicator subscriptions, HF can set this later using AT+BIA command
                updateAgIndicatorEnableState(DEFAULT_AG_INDICATOR_ENABLE_STATE);
                // Reset NREC on connect event. Headset will override later
                processNoiseReductionEvent(true);
                // Query phone state for initial setup
                mSystemInterface.queryPhoneState();
                // Remove pending connection attempts that were deferred during the pending
                // state. This is to prevent auto connect attempts from disconnecting
                // devices that previously successfully connected.
                removeDeferredMessages(CONNECT);
            }
            broadcastStateTransitions();
        }

        @Override
        public boolean processMessage(Message message) {
            switch (message.what) {
                case CONNECT: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    stateLogW("CONNECT, ignored, device=" + device + ", currentDevice" + mDevice);
                    break;
                }
                case DISCONNECT: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    stateLogD("DISCONNECT from device=" + device);
                    if (!mDevice.equals(device)) {
                        stateLogW("DISCONNECT, device " + device + " not connected");
                        break;
                    }
                    if (!mNativeInterface.disconnectHfp(device)) {
                        // broadcast immediately as no state transition is involved
                        stateLogE("DISCONNECT from " + device + " failed");
                        broadcastConnectionState(device, BluetoothProfile.STATE_CONNECTED,
                                BluetoothProfile.STATE_CONNECTED);
                        break;
                    }
                    transitionTo(mDisconnecting);
                }
                break;
                case CONNECT_AUDIO:
                    stateLogD("CONNECT_AUDIO, device=" + mDevice);
                    mSystemInterface.getAudioManager().setParameters("A2dpSuspended=true");
                    if (!mNativeInterface.connectAudio(mDevice)) {
                        mSystemInterface.getAudioManager().setParameters("A2dpSuspended=false");
                        stateLogE("Failed to connect SCO audio for " + mDevice);
                        // No state change involved, fire broadcast immediately
                        broadcastAudioState(mDevice, BluetoothHeadset.STATE_AUDIO_DISCONNECTED,
                                BluetoothHeadset.STATE_AUDIO_DISCONNECTED);
                        break;
                    }
                    transitionTo(mAudioConnecting);
                    break;
                case DISCONNECT_AUDIO:
                    stateLogD("ignore DISCONNECT_AUDIO, device=" + mDevice);
                    // ignore
                    break;
                default:
                    return super.processMessage(message);
            }
            return HANDLED;
        }

        @Override
        public void processAudioEvent(int state) {
            stateLogD("processAudioEvent, state=" + state);
            switch (state) {
                case HeadsetHalConstants.AUDIO_STATE_CONNECTED:
                    if (!mHeadsetService.isScoAcceptable(mDevice)) {
                        stateLogW("processAudioEvent: reject incoming audio connection");
                        if (!mNativeInterface.disconnectAudio(mDevice)) {
                            stateLogE("processAudioEvent: failed to disconnect audio");
                        }
                        // Indicate rejection to other components.
                        broadcastAudioState(mDevice, BluetoothHeadset.STATE_AUDIO_DISCONNECTED,
                                BluetoothHeadset.STATE_AUDIO_DISCONNECTED);
                        break;
                    }
                    stateLogI("processAudioEvent: audio connected");
                    transitionTo(mAudioOn);
                    break;
                case HeadsetHalConstants.AUDIO_STATE_CONNECTING:
                    if (!mHeadsetService.isScoAcceptable(mDevice)) {
                        stateLogW("processAudioEvent: reject incoming pending audio connection");
                        if (!mNativeInterface.disconnectAudio(mDevice)) {
                            stateLogE("processAudioEvent: failed to disconnect pending audio");
                        }
                        // Indicate rejection to other components.
                        broadcastAudioState(mDevice, BluetoothHeadset.STATE_AUDIO_DISCONNECTED,
                                BluetoothHeadset.STATE_AUDIO_DISCONNECTED);
                        break;
                    }
                    stateLogI("processAudioEvent: audio connecting");
                    transitionTo(mAudioConnecting);
                    break;
                case HeadsetHalConstants.AUDIO_STATE_DISCONNECTED:
                case HeadsetHalConstants.AUDIO_STATE_DISCONNECTING:
                    // ignore
                    break;
                default:
                    stateLogE("processAudioEvent: bad state: " + state);
                    break;
            }
        }
    }

    class AudioConnecting extends ConnectedBase {
        @Override
        int getAudioStateInt() {
            return BluetoothHeadset.STATE_AUDIO_CONNECTING;
        }

        @Override
        public void enter() {
            super.enter();
            sendMessageDelayed(CONNECT_TIMEOUT, mDevice, sConnectTimeoutMs);
            broadcastStateTransitions();
        }

        @Override
        public boolean processMessage(Message message) {
            switch (message.what) {
                case CONNECT:
                case DISCONNECT:
                case CONNECT_AUDIO:
                case DISCONNECT_AUDIO:
                    deferMessage(message);
                    break;
                case CONNECT_TIMEOUT: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (!mDevice.equals(device)) {
                        stateLogW("CONNECT_TIMEOUT for unknown device " + device);
                        break;
                    }
                    stateLogW("CONNECT_TIMEOUT");
                    transitionTo(mConnected);
                    break;
                }
                default:
                    return super.processMessage(message);
            }
            return HANDLED;
        }

        @Override
        public void processAudioEvent(int state) {
            switch (state) {
                case HeadsetHalConstants.AUDIO_STATE_DISCONNECTED:
                    stateLogW("processAudioEvent: audio connection failed");
                    transitionTo(mConnected);
                    break;
                case HeadsetHalConstants.AUDIO_STATE_CONNECTING:
                    // ignore, already in audio connecting state
                    break;
                case HeadsetHalConstants.AUDIO_STATE_DISCONNECTING:
                    // ignore, there is no BluetoothHeadset.STATE_AUDIO_DISCONNECTING
                    break;
                case HeadsetHalConstants.AUDIO_STATE_CONNECTED:
                    stateLogI("processAudioEvent: audio connected");
                    transitionTo(mAudioOn);
                    break;
                default:
                    stateLogE("processAudioEvent: bad state: " + state);
                    break;
            }
        }

        @Override
        public void exit() {
            removeMessages(CONNECT_TIMEOUT);
            super.exit();
        }
    }

    class AudioOn extends ConnectedBase {
        @Override
        int getAudioStateInt() {
            return BluetoothHeadset.STATE_AUDIO_CONNECTED;
        }

        @Override
        public void enter() {
            super.enter();
            removeDeferredMessages(CONNECT_AUDIO);
            // Set active device to current active SCO device when the current active device
            // is different from mCurrentDevice. This is to accommodate active device state
            // mis-match between native and Java.
            if (!mDevice.equals(mHeadsetService.getActiveDevice())) {
                mHeadsetService.setActiveDevice(mDevice);
            }
            setAudioParameters();
            broadcastStateTransitions();
        }

        @Override
        public boolean processMessage(Message message) {
            switch (message.what) {
                case CONNECT: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    stateLogW("CONNECT, ignored, device=" + device + ", currentDevice" + mDevice);
                    break;
                }
                case DISCONNECT: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    stateLogD("DISCONNECT, device=" + device);
                    if (!mDevice.equals(device)) {
                        stateLogW("DISCONNECT, device " + device + " not connected");
                        break;
                    }
                    // Disconnect BT SCO first
                    if (!mNativeInterface.disconnectAudio(mDevice)) {
                        stateLogW("DISCONNECT failed, device=" + mDevice);
                        // if disconnect BT SCO failed, transition to mConnected state to force
                        // disconnect device
                    }
                    deferMessage(obtainMessage(DISCONNECT, mDevice));
                    transitionTo(mAudioDisconnecting);
                    break;
                }
                case CONNECT_AUDIO: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (!mDevice.equals(device)) {
                        stateLogW("CONNECT_AUDIO device is not connected " + device);
                        break;
                    }
                    stateLogW("CONNECT_AUDIO device auido is already connected " + device);
                    break;
                }
                case DISCONNECT_AUDIO: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (!mDevice.equals(device)) {
                        stateLogW("DISCONNECT_AUDIO, failed, device=" + device + ", currentDevice="
                                + mDevice);
                        break;
                    }
                    if (mNativeInterface.disconnectAudio(mDevice)) {
                        stateLogD("DISCONNECT_AUDIO, device=" + mDevice);
                        transitionTo(mAudioDisconnecting);
                    } else {
                        stateLogW("DISCONNECT_AUDIO failed, device=" + mDevice);
                        broadcastAudioState(mDevice, BluetoothHeadset.STATE_AUDIO_CONNECTED,
                                BluetoothHeadset.STATE_AUDIO_CONNECTED);
                    }
                    break;
                }
                case INTENT_SCO_VOLUME_CHANGED:
                    processIntentScoVolume((Intent) message.obj, mDevice);
                    break;
                case STACK_EVENT:
                    HeadsetStackEvent event = (HeadsetStackEvent) message.obj;
                    stateLogD("STACK_EVENT: " + event);
                    if (!mDevice.equals(event.device)) {
                        stateLogE("Event device does not match currentDevice[" + mDevice
                                + "], event: " + event);
                        break;
                    }
                    switch (event.type) {
                        case HeadsetStackEvent.EVENT_TYPE_WBS:
                            stateLogE("Cannot change WBS state when audio is connected: " + event);
                            break;
                        default:
                            super.processMessage(message);
                            break;
                    }
                    break;
                default:
                    return super.processMessage(message);
            }
            return HANDLED;
        }

        @Override
        public void processAudioEvent(int state) {
            switch (state) {
                case HeadsetHalConstants.AUDIO_STATE_DISCONNECTED:
                    stateLogI("processAudioEvent: audio disconnected by remote");
                    transitionTo(mConnected);
                    break;
                case HeadsetHalConstants.AUDIO_STATE_DISCONNECTING:
                    stateLogI("processAudioEvent: audio being disconnected by remote");
                    transitionTo(mAudioDisconnecting);
                    break;
                default:
                    stateLogE("processAudioEvent: bad state: " + state);
                    break;
            }
        }

        private void processIntentScoVolume(Intent intent, BluetoothDevice device) {
            int volumeValue = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE, 0);
            if (mSpeakerVolume != volumeValue) {
                mSpeakerVolume = volumeValue;
                mNativeInterface.setVolume(device, HeadsetHalConstants.VOLUME_TYPE_SPK,
                        mSpeakerVolume);
            }
        }
    }

    class AudioDisconnecting extends ConnectedBase {
        @Override
        int getAudioStateInt() {
            // TODO: need BluetoothHeadset.STATE_AUDIO_DISCONNECTING
            return BluetoothHeadset.STATE_AUDIO_CONNECTED;
        }

        @Override
        public void enter() {
            super.enter();
            sendMessageDelayed(CONNECT_TIMEOUT, mDevice, sConnectTimeoutMs);
            broadcastStateTransitions();
        }

        @Override
        public boolean processMessage(Message message) {
            switch (message.what) {
                case CONNECT:
                case DISCONNECT:
                case CONNECT_AUDIO:
                case DISCONNECT_AUDIO:
                    deferMessage(message);
                    break;
                case CONNECT_TIMEOUT: {
                    BluetoothDevice device = (BluetoothDevice) message.obj;
                    if (!mDevice.equals(device)) {
                        stateLogW("CONNECT_TIMEOUT for unknown device " + device);
                        break;
                    }
                    stateLogW("CONNECT_TIMEOUT");
                    transitionTo(mConnected);
                    break;
                }
                default:
                    return super.processMessage(message);
            }
            return HANDLED;
        }

        @Override
        public void processAudioEvent(int state) {
            switch (state) {
                case HeadsetHalConstants.AUDIO_STATE_DISCONNECTED:
                    stateLogI("processAudioEvent: audio disconnected");
                    transitionTo(mConnected);
                    break;
                case HeadsetHalConstants.AUDIO_STATE_DISCONNECTING:
                    // ignore
                    break;
                case HeadsetHalConstants.AUDIO_STATE_CONNECTED:
                    stateLogW("processAudioEvent: audio disconnection failed");
                    transitionTo(mAudioOn);
                    break;
                case HeadsetHalConstants.AUDIO_STATE_CONNECTING:
                    // ignore, see if it goes into connected state, otherwise, timeout
                    break;
                default:
                    stateLogE("processAudioEvent: bad state: " + state);
                    break;
            }
        }

        @Override
        public void exit() {
            removeMessages(CONNECT_TIMEOUT);
            super.exit();
        }
    }

    /**
     * Get the underlying device tracked by this state machine
     *
     * @return device in focus
     */
    @VisibleForTesting
    public synchronized BluetoothDevice getDevice() {
        return mDevice;
    }

    /**
     * Get the current connection state of this state machine
     *
     * @return current connection state, one of {@link BluetoothProfile#STATE_DISCONNECTED},
     * {@link BluetoothProfile#STATE_CONNECTING}, {@link BluetoothProfile#STATE_CONNECTED}, or
     * {@link BluetoothProfile#STATE_DISCONNECTING}
     */
    @VisibleForTesting
    public synchronized int getConnectionState() {
        HeadsetStateBase state = (HeadsetStateBase) getCurrentState();
        if (state == null) {
            return BluetoothHeadset.STATE_DISCONNECTED;
        }
        return state.getConnectionStateInt();
    }

    /**
     * Get the current audio state of this state machine
     *
     * @return current audio state, one of {@link BluetoothHeadset#STATE_AUDIO_DISCONNECTED},
     * {@link BluetoothHeadset#STATE_AUDIO_CONNECTING}, or
     * {@link BluetoothHeadset#STATE_AUDIO_CONNECTED}
     */
    public synchronized int getAudioState() {
        HeadsetStateBase state = (HeadsetStateBase) getCurrentState();
        if (state == null) {
            return BluetoothHeadset.STATE_AUDIO_DISCONNECTED;
        }
        return state.getAudioStateInt();
    }

    public long getConnectingTimestampMs() {
        return mConnectingTimestampMs;
    }

    /*
     * Put the AT command, company ID, arguments, and device in an Intent and broadcast it.
     */
    private void broadcastVendorSpecificEventIntent(String command, int companyId, int commandType,
            Object[] arguments, BluetoothDevice device) {
        log("broadcastVendorSpecificEventIntent(" + command + ")");
        Intent intent = new Intent(BluetoothHeadset.ACTION_VENDOR_SPECIFIC_HEADSET_EVENT);
        intent.putExtra(BluetoothHeadset.EXTRA_VENDOR_SPECIFIC_HEADSET_EVENT_CMD, command);
        intent.putExtra(BluetoothHeadset.EXTRA_VENDOR_SPECIFIC_HEADSET_EVENT_CMD_TYPE, commandType);
        // assert: all elements of args are Serializable
        intent.putExtra(BluetoothHeadset.EXTRA_VENDOR_SPECIFIC_HEADSET_EVENT_ARGS, arguments);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        intent.addCategory(BluetoothHeadset.VENDOR_SPECIFIC_HEADSET_EVENT_COMPANY_ID_CATEGORY + "."
                + Integer.toString(companyId));
        mHeadsetService.sendBroadcastAsUser(intent, UserHandle.ALL, HeadsetService.BLUETOOTH_PERM);
    }

    private void setAudioParameters() {
        String keyValuePairs = String.join(";", new String[]{
                HEADSET_NAME + "=" + getCurrentDeviceName(),
                HEADSET_NREC + "=" + mAudioParams.getOrDefault(HEADSET_NREC,
                        HEADSET_AUDIO_FEATURE_OFF),
                HEADSET_WBS + "=" + mAudioParams.getOrDefault(HEADSET_WBS,
                        HEADSET_AUDIO_FEATURE_OFF)
        });
        Log.i(TAG, "setAudioParameters for " + mDevice + ": " + keyValuePairs);
        mSystemInterface.getAudioManager().setParameters(keyValuePairs);
    }

    private String parseUnknownAt(String atString) {
        StringBuilder atCommand = new StringBuilder(atString.length());

        for (int i = 0; i < atString.length(); i++) {
            char c = atString.charAt(i);
            if (c == '"') {
                int j = atString.indexOf('"', i + 1); // search for closing "
                if (j == -1) { // unmatched ", insert one.
                    atCommand.append(atString.substring(i, atString.length()));
                    atCommand.append('"');
                    break;
                }
                atCommand.append(atString.substring(i, j + 1));
                i = j;
            } else if (c != ' ') {
                atCommand.append(Character.toUpperCase(c));
            }
        }
        return atCommand.toString();
    }

    private int getAtCommandType(String atCommand) {
        int commandType = AtPhonebook.TYPE_UNKNOWN;
        String atString = null;
        atCommand = atCommand.trim();
        if (atCommand.length() > 5) {
            atString = atCommand.substring(5);
            if (atString.startsWith("?")) { // Read
                commandType = AtPhonebook.TYPE_READ;
            } else if (atString.startsWith("=?")) { // Test
                commandType = AtPhonebook.TYPE_TEST;
            } else if (atString.startsWith("=")) { // Set
                commandType = AtPhonebook.TYPE_SET;
            } else {
                commandType = AtPhonebook.TYPE_UNKNOWN;
            }
        }
        return commandType;
    }

    private void processDialCall(String number) {
        String dialNumber;
        if (mHeadsetService.hasDeviceInitiatedDialingOut()) {
            Log.w(TAG, "processDialCall, already dialling");
            mNativeInterface.atResponseCode(mDevice, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
            return;
        }
        if ((number == null) || (number.length() == 0)) {
            dialNumber = mPhonebook.getLastDialledNumber();
            if (dialNumber == null) {
                Log.w(TAG, "processDialCall, last dial number null");
                mNativeInterface.atResponseCode(mDevice, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
                return;
            }
        } else if (number.charAt(0) == '>') {
            // Yuck - memory dialling requested.
            // Just dial last number for now
            if (number.startsWith(">9999")) { // for PTS test
                Log.w(TAG, "Number is too big");
                mNativeInterface.atResponseCode(mDevice, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
                return;
            }
            log("processDialCall, memory dial do last dial for now");
            dialNumber = mPhonebook.getLastDialledNumber();
            if (dialNumber == null) {
                Log.w(TAG, "processDialCall, last dial number null");
                mNativeInterface.atResponseCode(mDevice, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
                return;
            }
        } else {
            // Remove trailing ';'
            if (number.charAt(number.length() - 1) == ';') {
                number = number.substring(0, number.length() - 1);
            }
            dialNumber = PhoneNumberUtils.convertPreDial(number);
        }
        if (!mHeadsetService.dialOutgoingCall(mDevice, dialNumber)) {
            Log.w(TAG, "processDialCall, failed to dial in service");
            mNativeInterface.atResponseCode(mDevice, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
            return;
        }
        mNeedDialingOutReply = true;
    }

    private void processVrEvent(int state) {
        if (state == HeadsetHalConstants.VR_STATE_STARTED) {
            if (!mHeadsetService.startVoiceRecognitionByHeadset(mDevice)) {
                mNativeInterface.atResponseCode(mDevice, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
            }
        } else if (state == HeadsetHalConstants.VR_STATE_STOPPED) {
            if (mHeadsetService.stopVoiceRecognitionByHeadset(mDevice)) {
                mNativeInterface.atResponseCode(mDevice, HeadsetHalConstants.AT_RESPONSE_OK, 0);
            } else {
                mNativeInterface.atResponseCode(mDevice, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
            }
        } else {
            mNativeInterface.atResponseCode(mDevice, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
        }
    }

    private void processVolumeEvent(int volumeType, int volume) {
        // Only current active device can change SCO volume
        if (!mDevice.equals(mHeadsetService.getActiveDevice())) {
            Log.w(TAG, "processVolumeEvent, ignored because " + mDevice + " is not active");
            return;
        }
        if (volumeType == HeadsetHalConstants.VOLUME_TYPE_SPK) {
            mSpeakerVolume = volume;
            int flag = (getCurrentState() == mAudioOn) ? AudioManager.FLAG_SHOW_UI : 0;
            mSystemInterface.getAudioManager()
                    .setStreamVolume(AudioManager.STREAM_BLUETOOTH_SCO, volume, flag);
        } else if (volumeType == HeadsetHalConstants.VOLUME_TYPE_MIC) {
            // Not used currently
            mMicVolume = volume;
        } else {
            Log.e(TAG, "Bad volume type: " + volumeType);
        }
    }

    private void processNoiseReductionEvent(boolean enable) {
        String prevNrec = mAudioParams.getOrDefault(HEADSET_NREC, HEADSET_AUDIO_FEATURE_OFF);
        String newNrec = enable ? HEADSET_AUDIO_FEATURE_ON : HEADSET_AUDIO_FEATURE_OFF;
        mAudioParams.put(HEADSET_NREC, newNrec);
        log("processNoiseReductionEvent: " + HEADSET_NREC + " change " + prevNrec + " -> "
                + newNrec);
        if (getAudioState() == BluetoothHeadset.STATE_AUDIO_CONNECTED) {
            setAudioParameters();
        }
    }

    private void processWBSEvent(int wbsConfig) {
        String prevWbs = mAudioParams.getOrDefault(HEADSET_WBS, HEADSET_AUDIO_FEATURE_OFF);
        switch (wbsConfig) {
            case HeadsetHalConstants.BTHF_WBS_YES:
                mAudioParams.put(HEADSET_WBS, HEADSET_AUDIO_FEATURE_ON);
                break;
            case HeadsetHalConstants.BTHF_WBS_NO:
            case HeadsetHalConstants.BTHF_WBS_NONE:
                mAudioParams.put(HEADSET_WBS, HEADSET_AUDIO_FEATURE_OFF);
                break;
            default:
                Log.e(TAG, "processWBSEvent: unknown wbsConfig " + wbsConfig);
                return;
        }
        log("processWBSEvent: " + HEADSET_NREC + " change " + prevWbs + " -> " + mAudioParams.get(
                HEADSET_WBS));
    }

    private void processAtChld(int chld, BluetoothDevice device) {
        if (mSystemInterface.processChld(chld)) {
            mNativeInterface.atResponseCode(device, HeadsetHalConstants.AT_RESPONSE_OK, 0);
        } else {
            mNativeInterface.atResponseCode(device, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
        }
    }

    private void processSubscriberNumberRequest(BluetoothDevice device) {
        String number = mSystemInterface.getSubscriberNumber();
        if (number != null) {
            mNativeInterface.atResponseString(device,
                    "+CNUM: ,\"" + number + "\"," + PhoneNumberUtils.toaFromString(number) + ",,4");
            mNativeInterface.atResponseCode(device, HeadsetHalConstants.AT_RESPONSE_OK, 0);
        } else {
            Log.e(TAG, "getSubscriberNumber returns null");
            mNativeInterface.atResponseCode(device, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
        }
    }

    private void processAtCind(BluetoothDevice device) {
        int call, callSetup;
        final HeadsetPhoneState phoneState = mSystemInterface.getHeadsetPhoneState();

        /* Handsfree carkits expect that +CIND is properly responded to
         Hence we ensure that a proper response is sent
         for the virtual call too.*/
        if (mHeadsetService.isVirtualCallStarted()) {
            call = 1;
            callSetup = 0;
        } else {
            // regular phone call
            call = phoneState.getNumActiveCall();
            callSetup = phoneState.getNumHeldCall();
        }

        mNativeInterface.cindResponse(device, phoneState.getCindService(), call, callSetup,
                phoneState.getCallState(), phoneState.getCindSignal(), phoneState.getCindRoam(),
                phoneState.getCindBatteryCharge());
    }

    private void processAtCops(BluetoothDevice device) {
        String operatorName = mSystemInterface.getNetworkOperator();
        if (operatorName == null) {
            operatorName = "";
        }
        mNativeInterface.copsResponse(device, operatorName);
    }

    private void processAtClcc(BluetoothDevice device) {
        if (mHeadsetService.isVirtualCallStarted()) {
            // In virtual call, send our phone number instead of remote phone number
            String phoneNumber = mSystemInterface.getSubscriberNumber();
            if (phoneNumber == null) {
                phoneNumber = "";
            }
            int type = PhoneNumberUtils.toaFromString(phoneNumber);
            mNativeInterface.clccResponse(device, 1, 0, 0, 0, false, phoneNumber, type);
            mNativeInterface.clccResponse(device, 0, 0, 0, 0, false, "", 0);
        } else {
            // In Telecom call, ask Telecom to send send remote phone number
            if (!mSystemInterface.listCurrentCalls()) {
                Log.e(TAG, "processAtClcc: failed to list current calls for " + device);
                mNativeInterface.clccResponse(device, 0, 0, 0, 0, false, "", 0);
            } else {
                sendMessageDelayed(CLCC_RSP_TIMEOUT, device, CLCC_RSP_TIMEOUT_MS);
            }
        }
    }

    private void processAtCscs(String atString, int type, BluetoothDevice device) {
        log("processAtCscs - atString = " + atString);
        if (mPhonebook != null) {
            mPhonebook.handleCscsCommand(atString, type, device);
        } else {
            Log.e(TAG, "Phonebook handle null for At+CSCS");
            mNativeInterface.atResponseCode(device, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
        }
    }

    private void processAtCpbs(String atString, int type, BluetoothDevice device) {
        log("processAtCpbs - atString = " + atString);
        if (mPhonebook != null) {
            mPhonebook.handleCpbsCommand(atString, type, device);
        } else {
            Log.e(TAG, "Phonebook handle null for At+CPBS");
            mNativeInterface.atResponseCode(device, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
        }
    }

    private void processAtCpbr(String atString, int type, BluetoothDevice device) {
        log("processAtCpbr - atString = " + atString);
        if (mPhonebook != null) {
            mPhonebook.handleCpbrCommand(atString, type, device);
        } else {
            Log.e(TAG, "Phonebook handle null for At+CPBR");
            mNativeInterface.atResponseCode(device, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
        }
    }

    /**
     * Find a character ch, ignoring quoted sections.
     * Return input.length() if not found.
     */
    private static int findChar(char ch, String input, int fromIndex) {
        for (int i = fromIndex; i < input.length(); i++) {
            char c = input.charAt(i);
            if (c == '"') {
                i = input.indexOf('"', i + 1);
                if (i == -1) {
                    return input.length();
                }
            } else if (c == ch) {
                return i;
            }
        }
        return input.length();
    }

    /**
     * Break an argument string into individual arguments (comma delimited).
     * Integer arguments are turned into Integer objects. Otherwise a String
     * object is used.
     */
    private static Object[] generateArgs(String input) {
        int i = 0;
        int j;
        ArrayList<Object> out = new ArrayList<Object>();
        while (i <= input.length()) {
            j = findChar(',', input, i);

            String arg = input.substring(i, j);
            try {
                out.add(new Integer(arg));
            } catch (NumberFormatException e) {
                out.add(arg);
            }

            i = j + 1; // move past comma
        }
        return out.toArray();
    }

    /**
     * Process vendor specific AT commands
     *
     * @param atString AT command after the "AT+" prefix
     * @param device Remote device that has sent this command
     */
    private void processVendorSpecificAt(String atString, BluetoothDevice device) {
        log("processVendorSpecificAt - atString = " + atString);

        // Currently we accept only SET type commands.
        int indexOfEqual = atString.indexOf("=");
        if (indexOfEqual == -1) {
            Log.w(TAG, "processVendorSpecificAt: command type error in " + atString);
            mNativeInterface.atResponseCode(device, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
            return;
        }

        String command = atString.substring(0, indexOfEqual);
        Integer companyId = VENDOR_SPECIFIC_AT_COMMAND_COMPANY_ID.get(command);
        if (companyId == null) {
            Log.i(TAG, "processVendorSpecificAt: unsupported command: " + atString);
            mNativeInterface.atResponseCode(device, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
            return;
        }

        String arg = atString.substring(indexOfEqual + 1);
        if (arg.startsWith("?")) {
            Log.w(TAG, "processVendorSpecificAt: command type error in " + atString);
            mNativeInterface.atResponseCode(device, HeadsetHalConstants.AT_RESPONSE_ERROR, 0);
            return;
        }

        Object[] args = generateArgs(arg);
        if (command.equals(BluetoothHeadset.VENDOR_SPECIFIC_HEADSET_EVENT_XAPL)) {
            processAtXapl(args, device);
        }
        broadcastVendorSpecificEventIntent(command, companyId, BluetoothHeadset.AT_CMD_TYPE_SET,
                args, device);
        mNativeInterface.atResponseCode(device, HeadsetHalConstants.AT_RESPONSE_OK, 0);
    }

    /**
     * Process AT+XAPL AT command
     *
     * @param args command arguments after the equal sign
     * @param device Remote device that has sent this command
     */
    private void processAtXapl(Object[] args, BluetoothDevice device) {
        if (args.length != 2) {
            Log.w(TAG, "processAtXapl() args length must be 2: " + String.valueOf(args.length));
            return;
        }
        if (!(args[0] instanceof String) || !(args[1] instanceof Integer)) {
            Log.w(TAG, "processAtXapl() argument types not match");
            return;
        }
        // feature = 2 indicates that we support battery level reporting only
        mNativeInterface.atResponseString(device, "+XAPL=iPhone," + String.valueOf(2));
    }

    private void processUnknownAt(String atString, BluetoothDevice device) {
        if (device == null) {
            Log.w(TAG, "processUnknownAt device is null");
            return;
        }
        log("processUnknownAt - atString = " + atString);
        String atCommand = parseUnknownAt(atString);
        int commandType = getAtCommandType(atCommand);
        if (atCommand.startsWith("+CSCS")) {
            processAtCscs(atCommand.substring(5), commandType, device);
        } else if (atCommand.startsWith("+CPBS")) {
            processAtCpbs(atCommand.substring(5), commandType, device);
        } else if (atCommand.startsWith("+CPBR")) {
            processAtCpbr(atCommand.substring(5), commandType, device);
        } else {
            processVendorSpecificAt(atCommand, device);
        }
    }

    // HSP +CKPD command
    private void processKeyPressed(BluetoothDevice device) {
        if (mSystemInterface.isRinging()) {
            mSystemInterface.answerCall(device);
        } else if (mSystemInterface.isInCall()) {
            if (getAudioState() == BluetoothHeadset.STATE_AUDIO_DISCONNECTED) {
                // Should connect audio as well
                if (!mHeadsetService.setActiveDevice(mDevice)) {
                    Log.w(TAG, "processKeyPressed, failed to set active device to " + mDevice);
                }
            } else {
                mSystemInterface.hangupCall(device);
            }
        } else if (getAudioState() != BluetoothHeadset.STATE_AUDIO_DISCONNECTED) {
            if (!mNativeInterface.disconnectAudio(mDevice)) {
                Log.w(TAG, "processKeyPressed, failed to disconnect audio from " + mDevice);
            }
        } else {
            // We have already replied OK to this HSP command, no feedback is needed
            if (mHeadsetService.hasDeviceInitiatedDialingOut()) {
                Log.w(TAG, "processKeyPressed, already dialling");
                return;
            }
            String dialNumber = mPhonebook.getLastDialledNumber();
            if (dialNumber == null) {
                Log.w(TAG, "processKeyPressed, last dial number null");
                return;
            }
            if (!mHeadsetService.dialOutgoingCall(mDevice, dialNumber)) {
                Log.w(TAG, "processKeyPressed, failed to call in service");
                return;
            }
        }
    }

    /**
     * Send HF indicator value changed intent
     *
     * @param device Device whose HF indicator value has changed
     * @param indId Indicator ID [0-65535]
     * @param indValue Indicator Value [0-65535], -1 means invalid but indId is supported
     */
    private void sendIndicatorIntent(BluetoothDevice device, int indId, int indValue) {
        Intent intent = new Intent(BluetoothHeadset.ACTION_HF_INDICATORS_VALUE_CHANGED);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        intent.putExtra(BluetoothHeadset.EXTRA_HF_INDICATORS_IND_ID, indId);
        intent.putExtra(BluetoothHeadset.EXTRA_HF_INDICATORS_IND_VALUE, indValue);

        mHeadsetService.sendBroadcast(intent, HeadsetService.BLUETOOTH_PERM);
    }

    private void processAtBind(String atString, BluetoothDevice device) {
        log("processAtBind: " + atString);

        // Parse the AT String to find the Indicator Ids that are supported
        int indId = 0;
        int iter = 0;
        int iter1 = 0;

        while (iter < atString.length()) {
            iter1 = findChar(',', atString, iter);
            String id = atString.substring(iter, iter1);

            try {
                indId = Integer.valueOf(id);
            } catch (NumberFormatException e) {
                Log.e(TAG, Log.getStackTraceString(new Throwable()));
            }

            switch (indId) {
                case HeadsetHalConstants.HF_INDICATOR_ENHANCED_DRIVER_SAFETY:
                    log("Send Broadcast intent for the Enhanced Driver Safety indicator.");
                    sendIndicatorIntent(device, indId, -1);
                    break;
                case HeadsetHalConstants.HF_INDICATOR_BATTERY_LEVEL_STATUS:
                    log("Send Broadcast intent for the Battery Level indicator.");
                    sendIndicatorIntent(device, indId, -1);
                    break;
                default:
                    log("Invalid HF Indicator Received");
                    break;
            }

            iter = iter1 + 1; // move past comma
        }
    }

    private void processAtBiev(int indId, int indValue, BluetoothDevice device) {
        log("processAtBiev: ind_id=" + indId + ", ind_value=" + indValue);
        sendIndicatorIntent(device, indId, indValue);
    }

    private void processSendClccResponse(HeadsetClccResponse clcc) {
        if (!hasMessages(CLCC_RSP_TIMEOUT)) {
            return;
        }
        if (clcc.mIndex == 0) {
            removeMessages(CLCC_RSP_TIMEOUT);
        }
        mNativeInterface.clccResponse(mDevice, clcc.mIndex, clcc.mDirection, clcc.mStatus,
                clcc.mMode, clcc.mMpty, clcc.mNumber, clcc.mType);
    }

    private void processSendVendorSpecificResultCode(HeadsetVendorSpecificResultCode resultCode) {
        String stringToSend = resultCode.mCommand + ": ";
        if (resultCode.mArg != null) {
            stringToSend += resultCode.mArg;
        }
        mNativeInterface.atResponseString(resultCode.mDevice, stringToSend);
    }

    private String getCurrentDeviceName() {
        String deviceName = mAdapterService.getRemoteName(mDevice);
        if (deviceName == null) {
            return "<unknown>";
        }
        return deviceName;
    }

    private void updateAgIndicatorEnableState(
            HeadsetAgIndicatorEnableState agIndicatorEnableState) {
        if (Objects.equals(mAgIndicatorEnableState, agIndicatorEnableState)) {
            Log.i(TAG, "updateAgIndicatorEnableState, no change in indicator state "
                    + mAgIndicatorEnableState);
            return;
        }
        mAgIndicatorEnableState = agIndicatorEnableState;
        int events = PhoneStateListener.LISTEN_NONE;
        if (mAgIndicatorEnableState != null && mAgIndicatorEnableState.service) {
            events |= PhoneStateListener.LISTEN_SERVICE_STATE;
        }
        if (mAgIndicatorEnableState != null && mAgIndicatorEnableState.signal) {
            events |= PhoneStateListener.LISTEN_SIGNAL_STRENGTHS;
        }
        mSystemInterface.getHeadsetPhoneState().listenForPhoneState(mDevice, events);
    }

    @Override
    protected void log(String msg) {
        if (DBG) {
            super.log(msg);
        }
    }

    @Override
    protected String getLogRecString(Message msg) {
        StringBuilder builder = new StringBuilder();
        builder.append(getMessageName(msg.what));
        builder.append(": ");
        builder.append("arg1=")
                .append(msg.arg1)
                .append(", arg2=")
                .append(msg.arg2)
                .append(", obj=");
        if (msg.obj instanceof HeadsetMessageObject) {
            HeadsetMessageObject object = (HeadsetMessageObject) msg.obj;
            object.buildString(builder);
        } else {
            builder.append(msg.obj);
        }
        return builder.toString();
    }

    private void handleAccessPermissionResult(Intent intent) {
        log("handleAccessPermissionResult");
        BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        if (!mPhonebook.getCheckingAccessPermission()) {
            return;
        }
        int atCommandResult = 0;
        int atCommandErrorCode = 0;
        // HeadsetBase headset = mHandsfree.getHeadset();
        // ASSERT: (headset != null) && headSet.isConnected()
        // REASON: mCheckingAccessPermission is true, otherwise resetAtState
        // has set mCheckingAccessPermission to false
        if (intent.getAction().equals(BluetoothDevice.ACTION_CONNECTION_ACCESS_REPLY)) {
            if (intent.getIntExtra(BluetoothDevice.EXTRA_CONNECTION_ACCESS_RESULT,
                    BluetoothDevice.CONNECTION_ACCESS_NO)
                    == BluetoothDevice.CONNECTION_ACCESS_YES) {
                if (intent.getBooleanExtra(BluetoothDevice.EXTRA_ALWAYS_ALLOWED, false)) {
                    mDevice.setPhonebookAccessPermission(BluetoothDevice.ACCESS_ALLOWED);
                }
                atCommandResult = mPhonebook.processCpbrCommand(device);
            } else {
                if (intent.getBooleanExtra(BluetoothDevice.EXTRA_ALWAYS_ALLOWED, false)) {
                    mDevice.setPhonebookAccessPermission(BluetoothDevice.ACCESS_REJECTED);
                }
            }
        }
        mPhonebook.setCpbrIndex(-1);
        mPhonebook.setCheckingAccessPermission(false);
        if (atCommandResult >= 0) {
            mNativeInterface.atResponseCode(device, atCommandResult, atCommandErrorCode);
        } else {
            log("handleAccessPermissionResult - RESULT_NONE");
        }
    }

    private static String getMessageName(int what) {
        switch (what) {
            case CONNECT:
                return "CONNECT";
            case DISCONNECT:
                return "DISCONNECT";
            case CONNECT_AUDIO:
                return "CONNECT_AUDIO";
            case DISCONNECT_AUDIO:
                return "DISCONNECT_AUDIO";
            case VOICE_RECOGNITION_START:
                return "VOICE_RECOGNITION_START";
            case VOICE_RECOGNITION_STOP:
                return "VOICE_RECOGNITION_STOP";
            case INTENT_SCO_VOLUME_CHANGED:
                return "INTENT_SCO_VOLUME_CHANGED";
            case INTENT_CONNECTION_ACCESS_REPLY:
                return "INTENT_CONNECTION_ACCESS_REPLY";
            case CALL_STATE_CHANGED:
                return "CALL_STATE_CHANGED";
            case DEVICE_STATE_CHANGED:
                return "DEVICE_STATE_CHANGED";
            case SEND_CCLC_RESPONSE:
                return "SEND_CCLC_RESPONSE";
            case SEND_VENDOR_SPECIFIC_RESULT_CODE:
                return "SEND_VENDOR_SPECIFIC_RESULT_CODE";
            case STACK_EVENT:
                return "STACK_EVENT";
            case VOICE_RECOGNITION_RESULT:
                return "VOICE_RECOGNITION_RESULT";
            case DIALING_OUT_RESULT:
                return "DIALING_OUT_RESULT";
            case CLCC_RSP_TIMEOUT:
                return "CLCC_RSP_TIMEOUT";
            case CONNECT_TIMEOUT:
                return "CONNECT_TIMEOUT";
            default:
                return "UNKNOWN(" + what + ")";
        }
    }
}
