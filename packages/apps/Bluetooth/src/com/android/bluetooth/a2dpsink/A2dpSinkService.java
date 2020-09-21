/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.bluetooth.a2dpsink;

import android.bluetooth.BluetoothAudioConfig;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.IBluetoothA2dpSink;
import android.content.Intent;
import android.provider.Settings;
import android.util.Log;

import com.android.bluetooth.Utils;
import com.android.bluetooth.a2dpsink.mbs.A2dpMediaBrowserService;
import com.android.bluetooth.avrcpcontroller.AvrcpControllerService;
import com.android.bluetooth.btservice.ProfileService;

import java.util.ArrayList;
import java.util.List;

/**
 * Provides Bluetooth A2DP Sink profile, as a service in the Bluetooth application.
 * @hide
 */
public class A2dpSinkService extends ProfileService {
    private static final boolean DBG = true;
    private static final String TAG = "A2dpSinkService";

    private A2dpSinkStateMachine mStateMachine;
    private static A2dpSinkService sA2dpSinkService;

    @Override
    protected IProfileServiceBinder initBinder() {
        return new BluetoothA2dpSinkBinder(this);
    }

    @Override
    protected boolean start() {
        if (DBG) {
            Log.d(TAG, "start()");
        }
        // Start the media browser service.
        Intent startIntent = new Intent(this, A2dpMediaBrowserService.class);
        startService(startIntent);
        mStateMachine = A2dpSinkStateMachine.make(this, this);
        setA2dpSinkService(this);
        return true;
    }

    @Override
    protected boolean stop() {
        if (DBG) {
            Log.d(TAG, "stop()");
        }
        setA2dpSinkService(null);
        if (mStateMachine != null) {
            mStateMachine.doQuit();
        }
        Intent stopIntent = new Intent(this, A2dpMediaBrowserService.class);
        stopService(stopIntent);
        return true;
    }

    @Override
    protected void cleanup() {
        if (mStateMachine != null) {
            mStateMachine.cleanup();
        }
    }

    //API Methods

    public static synchronized A2dpSinkService getA2dpSinkService() {
        if (sA2dpSinkService == null) {
            Log.w(TAG, "getA2dpSinkService(): service is null");
            return null;
        }
        if (!sA2dpSinkService.isAvailable()) {
            Log.w(TAG, "getA2dpSinkService(): service is not available ");
            return null;
        }
        return sA2dpSinkService;
    }

    private static synchronized void setA2dpSinkService(A2dpSinkService instance) {
        if (DBG) {
            Log.d(TAG, "setA2dpSinkService(): set to: " + instance);
        }
        sA2dpSinkService = instance;
    }

    public boolean connect(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");

        int connectionState = mStateMachine.getConnectionState(device);
        if (connectionState == BluetoothProfile.STATE_CONNECTED
                || connectionState == BluetoothProfile.STATE_CONNECTING) {
            return false;
        }

        if (getPriority(device) == BluetoothProfile.PRIORITY_OFF) {
            return false;
        }

        mStateMachine.sendMessage(A2dpSinkStateMachine.CONNECT, device);
        return true;
    }

    boolean disconnect(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH ADMIN permission");
        int connectionState = mStateMachine.getConnectionState(device);
        if (connectionState != BluetoothProfile.STATE_CONNECTED
                && connectionState != BluetoothProfile.STATE_CONNECTING) {
            return false;
        }

        mStateMachine.sendMessage(A2dpSinkStateMachine.DISCONNECT, device);
        return true;
    }

    public List<BluetoothDevice> getConnectedDevices() {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        return mStateMachine.getConnectedDevices();
    }

    List<BluetoothDevice> getDevicesMatchingConnectionStates(int[] states) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        return mStateMachine.getDevicesMatchingConnectionStates(states);
    }

    int getConnectionState(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        return mStateMachine.getConnectionState(device);
    }

    public boolean setPriority(BluetoothDevice device, int priority) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH_ADMIN permission");
        Settings.Global.putInt(getContentResolver(),
                Settings.Global.getBluetoothA2dpSrcPriorityKey(device.getAddress()), priority);
        if (DBG) {
            Log.d(TAG, "Saved priority " + device + " = " + priority);
        }
        return true;
    }

    public int getPriority(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM, "Need BLUETOOTH_ADMIN permission");
        int priority = Settings.Global.getInt(getContentResolver(),
                Settings.Global.getBluetoothA2dpSrcPriorityKey(device.getAddress()),
                BluetoothProfile.PRIORITY_UNDEFINED);
        return priority;
    }

    /**
     * Called by AVRCP controller to provide information about the last user intent on CT.
     *
     * If the user has pressed play in the last attempt then A2DP Sink component will grant focus to
     * any incoming sound from the phone (and also retain focus for a few seconds before
     * relinquishing. On the other hand if the user has pressed pause/stop then the A2DP sink
     * component will take the focus away but also notify the stack to throw away incoming data.
     */
    public void informAvrcpPassThroughCmd(BluetoothDevice device, int keyCode, int keyState) {
        if (mStateMachine != null) {
            if (keyCode == AvrcpControllerService.PASS_THRU_CMD_ID_PLAY
                    && keyState == AvrcpControllerService.KEY_STATE_RELEASED) {
                mStateMachine.sendMessage(A2dpSinkStateMachine.EVENT_AVRCP_CT_PLAY);
            } else if ((keyCode == AvrcpControllerService.PASS_THRU_CMD_ID_PAUSE
                    || keyCode == AvrcpControllerService.PASS_THRU_CMD_ID_STOP)
                    && keyState == AvrcpControllerService.KEY_STATE_RELEASED) {
                mStateMachine.sendMessage(A2dpSinkStateMachine.EVENT_AVRCP_CT_PAUSE);
            }
        }
    }

    /**
     * Called by AVRCP controller to provide information about the last user intent on TG.
     *
     * Tf the user has pressed pause on the TG then we can preempt streaming music. This is opposed
     * to when the streaming stops abruptly (jitter) in which case we will wait for sometime before
     * stopping playback.
     */
    public void informTGStatePlaying(BluetoothDevice device, boolean isPlaying) {
        if (mStateMachine != null) {
            if (!isPlaying) {
                mStateMachine.sendMessage(A2dpSinkStateMachine.EVENT_AVRCP_TG_PAUSE);
            } else {
                mStateMachine.sendMessage(A2dpSinkStateMachine.EVENT_AVRCP_TG_PLAY);
            }
        }
    }

    /**
     * Called by AVRCP controller to establish audio focus.
     *
     * In order to perform streaming the A2DP sink must have audio focus.  This interface allows the
     * associated MediaSession to inform the sink of intent to play and then allows streaming to be
     * started from either the source or the sink endpoint.
     */
    public void requestAudioFocus(BluetoothDevice device, boolean request) {
        if (mStateMachine != null) {
            mStateMachine.sendMessage(A2dpSinkStateMachine.EVENT_REQUEST_FOCUS);
        }
    }

    synchronized boolean isA2dpPlaying(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        if (DBG) {
            Log.d(TAG, "isA2dpPlaying(" + device + ")");
        }
        return mStateMachine.isPlaying(device);
    }

    BluetoothAudioConfig getAudioConfig(BluetoothDevice device) {
        enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
        return mStateMachine.getAudioConfig(device);
    }

    //Binder object: Must be static class or memory leak may occur
    private static class BluetoothA2dpSinkBinder extends IBluetoothA2dpSink.Stub
            implements IProfileServiceBinder {
        private A2dpSinkService mService;

        private A2dpSinkService getService() {
            if (!Utils.checkCaller()) {
                Log.w(TAG, "A2dp call not allowed for non-active user");
                return null;
            }

            if (mService != null && mService.isAvailable()) {
                return mService;
            }
            return null;
        }

        BluetoothA2dpSinkBinder(A2dpSinkService svc) {
            mService = svc;
        }

        @Override
        public void cleanup() {
            mService = null;
        }

        @Override
        public boolean connect(BluetoothDevice device) {
            A2dpSinkService service = getService();
            if (service == null) {
                return false;
            }
            return service.connect(device);
        }

        @Override
        public boolean disconnect(BluetoothDevice device) {
            A2dpSinkService service = getService();
            if (service == null) {
                return false;
            }
            return service.disconnect(device);
        }

        @Override
        public List<BluetoothDevice> getConnectedDevices() {
            A2dpSinkService service = getService();
            if (service == null) {
                return new ArrayList<BluetoothDevice>(0);
            }
            return service.getConnectedDevices();
        }

        @Override
        public List<BluetoothDevice> getDevicesMatchingConnectionStates(int[] states) {
            A2dpSinkService service = getService();
            if (service == null) {
                return new ArrayList<BluetoothDevice>(0);
            }
            return service.getDevicesMatchingConnectionStates(states);
        }

        @Override
        public int getConnectionState(BluetoothDevice device) {
            A2dpSinkService service = getService();
            if (service == null) {
                return BluetoothProfile.STATE_DISCONNECTED;
            }
            return service.getConnectionState(device);
        }

        @Override
        public boolean isA2dpPlaying(BluetoothDevice device) {
            A2dpSinkService service = getService();
            if (service == null) {
                return false;
            }
            return service.isA2dpPlaying(device);
        }

        @Override
        public boolean setPriority(BluetoothDevice device, int priority) {
            A2dpSinkService service = getService();
            if (service == null) {
                return false;
            }
            return service.setPriority(device, priority);
        }

        @Override
        public int getPriority(BluetoothDevice device) {
            A2dpSinkService service = getService();
            if (service == null) {
                return BluetoothProfile.PRIORITY_UNDEFINED;
            }
            return service.getPriority(device);
        }

        @Override
        public BluetoothAudioConfig getAudioConfig(BluetoothDevice device) {
            A2dpSinkService service = getService();
            if (service == null) {
                return null;
            }
            return service.getAudioConfig(device);
        }
    }

    ;

    @Override
    public void dump(StringBuilder sb) {
        super.dump(sb);
        if (mStateMachine != null) {
            mStateMachine.dump(sb);
        }
    }
}
