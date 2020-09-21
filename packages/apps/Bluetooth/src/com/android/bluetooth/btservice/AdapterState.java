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

package com.android.bluetooth.btservice;

import android.bluetooth.BluetoothAdapter;
import android.os.Message;
import android.util.Log;

import com.android.internal.util.State;
import com.android.internal.util.StateMachine;

/**
 * This state machine handles Bluetooth Adapter State.
 * Stable States:
 *      {@link OffState}: Initial State
 *      {@link BleOnState} : Bluetooth Low Energy, Including GATT, is on
 *      {@link OnState} : Bluetooth is on (All supported profiles)
 *
 * Transition States:
 *      {@link TurningBleOnState} : OffState to BleOnState
 *      {@link TurningBleOffState} : BleOnState to OffState
 *      {@link TurningOnState} : BleOnState to OnState
 *      {@link TurningOffState} : OnState to BleOnState
 *
 *        +------   Off  <-----+
 *        |                    |
 *        v                    |
 * TurningBleOn   TO--->   TurningBleOff
 *        |                  ^ ^
 *        |                  | |
 *        +----->        ----+ |
 *                 BleOn       |
 *        +------        <---+ O
 *        v                  | T
 *    TurningOn  TO---->  TurningOff
 *        |                    ^
 *        |                    |
 *        +----->   On   ------+
 *
 */

final class AdapterState extends StateMachine {
    private static final boolean DBG = true;
    private static final String TAG = AdapterState.class.getSimpleName();

    static final int USER_TURN_ON = 1;
    static final int USER_TURN_OFF = 2;
    static final int BLE_TURN_ON = 3;
    static final int BLE_TURN_OFF = 4;
    static final int BREDR_STARTED = 5;
    static final int BREDR_STOPPED = 6;
    static final int BLE_STARTED = 7;
    static final int BLE_STOPPED = 8;
    static final int BREDR_START_TIMEOUT = 9;
    static final int BREDR_STOP_TIMEOUT = 10;
    static final int BLE_STOP_TIMEOUT = 11;
    static final int BLE_START_TIMEOUT = 12;

    static final int BLE_START_TIMEOUT_DELAY = 4000;
    static final int BLE_STOP_TIMEOUT_DELAY = 1000;
    static final int BREDR_START_TIMEOUT_DELAY = 4000;
    static final int BREDR_STOP_TIMEOUT_DELAY = 4000;

    private AdapterService mAdapterService;
    private TurningOnState mTurningOnState = new TurningOnState();
    private TurningBleOnState mTurningBleOnState = new TurningBleOnState();
    private TurningOffState mTurningOffState = new TurningOffState();
    private TurningBleOffState mTurningBleOffState = new TurningBleOffState();
    private OnState mOnState = new OnState();
    private OffState mOffState = new OffState();
    private BleOnState mBleOnState = new BleOnState();

    private int mPrevState = BluetoothAdapter.STATE_OFF;

    private AdapterState(AdapterService service) {
        super(TAG);
        addState(mOnState);
        addState(mBleOnState);
        addState(mOffState);
        addState(mTurningOnState);
        addState(mTurningOffState);
        addState(mTurningBleOnState);
        addState(mTurningBleOffState);
        mAdapterService = service;
        setInitialState(mOffState);
    }

    private String messageString(int message) {
        switch (message) {
            case BLE_TURN_ON: return "BLE_TURN_ON";
            case USER_TURN_ON: return "USER_TURN_ON";
            case BREDR_STARTED: return "BREDR_STARTED";
            case BLE_STARTED: return "BLE_STARTED";
            case USER_TURN_OFF: return "USER_TURN_OFF";
            case BLE_TURN_OFF: return "BLE_TURN_OFF";
            case BLE_STOPPED: return "BLE_STOPPED";
            case BREDR_STOPPED: return "BREDR_STOPPED";
            case BLE_START_TIMEOUT: return "BLE_START_TIMEOUT";
            case BLE_STOP_TIMEOUT: return "BLE_STOP_TIMEOUT";
            case BREDR_START_TIMEOUT: return "BREDR_START_TIMEOUT";
            case BREDR_STOP_TIMEOUT: return "BREDR_STOP_TIMEOUT";
            default: return "Unknown message (" + message + ")";
        }
    }

    public static AdapterState make(AdapterService service) {
        Log.d(TAG, "make() - Creating AdapterState");
        AdapterState as = new AdapterState(service);
        as.start();
        return as;
    }

    public void doQuit() {
        quitNow();
    }

    private void cleanup() {
        if (mAdapterService != null) {
            mAdapterService = null;
        }
    }

    @Override
    protected void onQuitting() {
        cleanup();
    }

    @Override
    protected String getLogRecString(Message msg) {
        return messageString(msg.what);
    }

    private abstract class BaseAdapterState extends State {

        abstract int getStateValue();

        @Override
        public void enter() {
            int currState = getStateValue();
            infoLog("entered ");
            mAdapterService.updateAdapterState(mPrevState, currState);
            mPrevState = currState;
        }

        void infoLog(String msg) {
            if (DBG) {
                Log.i(TAG, BluetoothAdapter.nameForState(getStateValue()) + " : " + msg);
            }
        }

        void errorLog(String msg) {
            Log.e(TAG, BluetoothAdapter.nameForState(getStateValue()) + " : " + msg);
        }
    }

    private class OffState extends BaseAdapterState {

        @Override
        int getStateValue() {
            return BluetoothAdapter.STATE_OFF;
        }

        @Override
        public boolean processMessage(Message msg) {
            switch (msg.what) {
                case BLE_TURN_ON:
                    transitionTo(mTurningBleOnState);
                    break;

                default:
                    infoLog("Unhandled message - " + messageString(msg.what));
                    return false;
            }
            return true;
        }
    }

    private class BleOnState extends BaseAdapterState {

        @Override
        int getStateValue() {
            return BluetoothAdapter.STATE_BLE_ON;
        }

        @Override
        public boolean processMessage(Message msg) {
            switch (msg.what) {
                case USER_TURN_ON:
                    transitionTo(mTurningOnState);
                    break;

                case BLE_TURN_OFF:
                    transitionTo(mTurningBleOffState);
                    break;

                default:
                    infoLog("Unhandled message - " + messageString(msg.what));
                    return false;
            }
            return true;
        }
    }

    private class OnState extends BaseAdapterState {

        @Override
        int getStateValue() {
            return BluetoothAdapter.STATE_ON;
        }

        @Override
        public boolean processMessage(Message msg) {
            switch (msg.what) {
                case USER_TURN_OFF:
                    transitionTo(mTurningOffState);
                    break;

                default:
                    infoLog("Unhandled message - " + messageString(msg.what));
                    return false;
            }
            return true;
        }
    }

    private class TurningBleOnState extends BaseAdapterState {

        @Override
        int getStateValue() {
            return BluetoothAdapter.STATE_BLE_TURNING_ON;
        }

        @Override
        public void enter() {
            super.enter();
            sendMessageDelayed(BLE_START_TIMEOUT, BLE_START_TIMEOUT_DELAY);
            mAdapterService.bringUpBle();
        }

        @Override
        public void exit() {
            removeMessages(BLE_START_TIMEOUT);
            super.exit();
        }

        @Override
        public boolean processMessage(Message msg) {
            switch (msg.what) {
                case BLE_STARTED:
                    transitionTo(mBleOnState);
                    break;

                case BLE_START_TIMEOUT:
                    errorLog(messageString(msg.what));
                    transitionTo(mTurningBleOffState);
                    break;

                default:
                    infoLog("Unhandled message - " + messageString(msg.what));
                    return false;
            }
            return true;
        }
    }

    private class TurningOnState extends BaseAdapterState {

        @Override
        int getStateValue() {
            return BluetoothAdapter.STATE_TURNING_ON;
        }

        @Override
        public void enter() {
            super.enter();
            sendMessageDelayed(BREDR_START_TIMEOUT, BREDR_START_TIMEOUT_DELAY);
            mAdapterService.startProfileServices();
        }

        @Override
        public void exit() {
            removeMessages(BREDR_START_TIMEOUT);
            super.exit();
        }

        @Override
        public boolean processMessage(Message msg) {
            switch (msg.what) {
                case BREDR_STARTED:
                    transitionTo(mOnState);
                    break;

                case BREDR_START_TIMEOUT:
                    errorLog(messageString(msg.what));
                    transitionTo(mTurningOffState);
                    break;

                default:
                    infoLog("Unhandled message - " + messageString(msg.what));
                    return false;
            }
            return true;
        }
    }

    private class TurningOffState extends BaseAdapterState {

        @Override
        int getStateValue() {
            return BluetoothAdapter.STATE_TURNING_OFF;
        }

        @Override
        public void enter() {
            super.enter();
            sendMessageDelayed(BREDR_STOP_TIMEOUT, BREDR_STOP_TIMEOUT_DELAY);
            mAdapterService.stopProfileServices();
        }

        @Override
        public void exit() {
            removeMessages(BREDR_STOP_TIMEOUT);
            super.exit();
        }

        @Override
        public boolean processMessage(Message msg) {
            switch (msg.what) {
                case BREDR_STOPPED:
                    transitionTo(mBleOnState);
                    break;

                case BREDR_STOP_TIMEOUT:
                    errorLog(messageString(msg.what));
                    transitionTo(mTurningBleOffState);
                    break;

                default:
                    infoLog("Unhandled message - " + messageString(msg.what));
                    return false;
            }
            return true;
        }
    }

    private class TurningBleOffState extends BaseAdapterState {

        @Override
        int getStateValue() {
            return BluetoothAdapter.STATE_BLE_TURNING_OFF;
        }

        @Override
        public void enter() {
            super.enter();
            sendMessageDelayed(BLE_STOP_TIMEOUT, BLE_STOP_TIMEOUT_DELAY);
            mAdapterService.bringDownBle();
        }

        @Override
        public void exit() {
            removeMessages(BLE_STOP_TIMEOUT);
            super.exit();
        }

        @Override
        public boolean processMessage(Message msg) {
            switch (msg.what) {
                case BLE_STOPPED:
                    transitionTo(mOffState);
                    break;

                case BLE_STOP_TIMEOUT:
                    errorLog(messageString(msg.what));
                    transitionTo(mOffState);
                    break;

                default:
                    infoLog("Unhandled message - " + messageString(msg.what));
                    return false;
            }
            return true;
        }
    }
}
