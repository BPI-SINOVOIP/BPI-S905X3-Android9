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

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.IBluetoothHeadsetPhone;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.media.AudioManager;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.RemoteException;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;

/**
 * Defines system calls that is used by state machine/service to either send or receive
 * messages from the Android System.
 */
@VisibleForTesting
public class HeadsetSystemInterface {
    private static final String TAG = HeadsetSystemInterface.class.getSimpleName();
    private static final boolean DBG = false;

    private final HeadsetService mHeadsetService;
    private final AudioManager mAudioManager;
    private final HeadsetPhoneState mHeadsetPhoneState;
    private PowerManager.WakeLock mVoiceRecognitionWakeLock;
    private volatile IBluetoothHeadsetPhone mPhoneProxy;
    private final ServiceConnection mPhoneProxyConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            if (DBG) {
                Log.d(TAG, "Proxy object connected");
            }
            synchronized (HeadsetSystemInterface.this) {
                mPhoneProxy = IBluetoothHeadsetPhone.Stub.asInterface(service);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            if (DBG) {
                Log.d(TAG, "Proxy object disconnected");
            }
            synchronized (HeadsetSystemInterface.this) {
                mPhoneProxy = null;
            }
        }
    };

    HeadsetSystemInterface(HeadsetService headsetService) {
        if (headsetService == null) {
            Log.wtfStack(TAG, "HeadsetService parameter is null");
        }
        mHeadsetService = headsetService;
        mAudioManager = (AudioManager) mHeadsetService.getSystemService(Context.AUDIO_SERVICE);
        PowerManager powerManager =
                (PowerManager) mHeadsetService.getSystemService(Context.POWER_SERVICE);
        mVoiceRecognitionWakeLock =
                powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG + ":VoiceRecognition");
        mVoiceRecognitionWakeLock.setReferenceCounted(false);
        mHeadsetPhoneState = new HeadsetPhoneState(mHeadsetService);
    }

    /**
     * Initialize this system interface
     */
    public synchronized void init() {
        // Bind to Telecom phone proxy service
        Intent intent = new Intent(IBluetoothHeadsetPhone.class.getName());
        intent.setComponent(intent.resolveSystemService(mHeadsetService.getPackageManager(), 0));
        if (intent.getComponent() == null || !mHeadsetService.bindService(intent,
                mPhoneProxyConnection, 0)) {
            // Crash the stack if cannot bind to Telecom
            Log.wtfStack(TAG, "Could not bind to IBluetoothHeadsetPhone Service, intent=" + intent);
        }
    }

    /**
     * Stop this system interface
     */
    public synchronized void stop() {
        if (mPhoneProxy != null) {
            if (DBG) {
                Log.d(TAG, "Unbinding phone proxy");
            }
            mPhoneProxy = null;
            // Synchronization should make sure unbind can be successful
            mHeadsetService.unbindService(mPhoneProxyConnection);
        }
        mHeadsetPhoneState.cleanup();
    }

    /**
     * Get audio manager. Most audio manager oprations are pass through and therefore are not
     * individually managed by this class
     *
     * @return audio manager for setting audio parameters
     */
    @VisibleForTesting
    public AudioManager getAudioManager() {
        return mAudioManager;
    }

    /**
     * Get wake lock for voice recognition
     *
     * @return wake lock for voice recognition
     */
    @VisibleForTesting
    public PowerManager.WakeLock getVoiceRecognitionWakeLock() {
        return mVoiceRecognitionWakeLock;
    }

    /**
     * Get HeadsetPhoneState instance to interact with Telephony service
     *
     * @return HeadsetPhoneState interface to interact with Telephony service
     */
    @VisibleForTesting
    public HeadsetPhoneState getHeadsetPhoneState() {
        return mHeadsetPhoneState;
    }

    /**
     * Answer the current incoming call in Telecom service
     *
     * @param device the Bluetooth device used for answering this call
     */
    @VisibleForTesting
    public void answerCall(BluetoothDevice device) {
        if (device == null) {
            Log.w(TAG, "answerCall device is null");
            return;
        }

        if (mPhoneProxy != null) {
            try {
                mHeadsetService.setActiveDevice(device);
                mPhoneProxy.answerCall();
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(new Throwable()));
            }
        } else {
            Log.e(TAG, "Handsfree phone proxy null for answering call");
        }
    }

    /**
     * Hangup the current call, could either be Telecom call or virtual call
     *
     * @param device the Bluetooth device used for hanging up this call
     */
    @VisibleForTesting
    public void hangupCall(BluetoothDevice device) {
        if (device == null) {
            Log.w(TAG, "hangupCall device is null");
            return;
        }
        // Close the virtual call if active. Virtual call should be
        // terminated for CHUP callback event
        if (mHeadsetService.isVirtualCallStarted()) {
            mHeadsetService.stopScoUsingVirtualVoiceCall();
        } else {
            if (mPhoneProxy != null) {
                try {
                    mPhoneProxy.hangupCall();
                } catch (RemoteException e) {
                    Log.e(TAG, Log.getStackTraceString(new Throwable()));
                }
            } else {
                Log.e(TAG, "Handsfree phone proxy null for hanging up call");
            }
        }
    }

    /**
     * Instructs Telecom to play the specified DTMF tone for the current foreground call
     *
     * @param dtmf dtmf code
     * @param device the Bluetooth device that sent this code
     */
    @VisibleForTesting
    public boolean sendDtmf(int dtmf, BluetoothDevice device) {
        if (device == null) {
            Log.w(TAG, "sendDtmf device is null");
            return false;
        }
        if (mPhoneProxy != null) {
            try {
                return mPhoneProxy.sendDtmf(dtmf);
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(new Throwable()));
            }
        } else {
            Log.e(TAG, "Handsfree phone proxy null for sending DTMF");
        }
        return false;
    }

    /**
     * Instructs Telecom hold an incoming call
     *
     * @param chld index of the call to hold
     */
    @VisibleForTesting
    public boolean processChld(int chld) {
        if (mPhoneProxy != null) {
            try {
                return mPhoneProxy.processChld(chld);
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(new Throwable()));
            }
        } else {
            Log.e(TAG, "Handsfree phone proxy null for sending DTMF");
        }
        return false;
    }

    /**
     * Get the the alphabetic name of current registered operator.
     *
     * @return null on error, empty string if not available
     */
    @VisibleForTesting
    public String getNetworkOperator() {
        final IBluetoothHeadsetPhone phoneProxy = mPhoneProxy;
        if (phoneProxy == null) {
            Log.e(TAG, "getNetworkOperator() failed: mPhoneProxy is null");
            return null;
        }
        try {
            // Should never return null
            return mPhoneProxy.getNetworkOperator();
        } catch (RemoteException exception) {
            Log.e(TAG, "getNetworkOperator() failed: " + exception.getMessage());
            exception.printStackTrace();
            return null;
        }
    }

    /**
     * Get the phone number of this device
     *
     * @return null if unavailable
     */
    @VisibleForTesting
    public String getSubscriberNumber() {
        final IBluetoothHeadsetPhone phoneProxy = mPhoneProxy;
        if (phoneProxy == null) {
            Log.e(TAG, "getSubscriberNumber() failed: mPhoneProxy is null");
            return null;
        }
        try {
            return mPhoneProxy.getSubscriberNumber();
        } catch (RemoteException exception) {
            Log.e(TAG, "getSubscriberNumber() failed: " + exception.getMessage());
            exception.printStackTrace();
            return null;
        }
    }


    /**
     * Ask the Telecomm service to list current list of calls through CLCC response
     * {@link BluetoothHeadset#clccResponse(int, int, int, int, boolean, String, int)}
     *
     * @return
     */
    @VisibleForTesting
    public boolean listCurrentCalls() {
        final IBluetoothHeadsetPhone phoneProxy = mPhoneProxy;
        if (phoneProxy == null) {
            Log.e(TAG, "listCurrentCalls() failed: mPhoneProxy is null");
            return false;
        }
        try {
            return mPhoneProxy.listCurrentCalls();
        } catch (RemoteException exception) {
            Log.e(TAG, "listCurrentCalls() failed: " + exception.getMessage());
            exception.printStackTrace();
            return false;
        }
    }

    /**
     * Request Telecom service to send an update of the current call state to the headset service
     * through {@link BluetoothHeadset#phoneStateChanged(int, int, int, String, int)}
     */
    @VisibleForTesting
    public void queryPhoneState() {
        final IBluetoothHeadsetPhone phoneProxy = mPhoneProxy;
        if (phoneProxy != null) {
            try {
                mPhoneProxy.queryPhoneState();
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(new Throwable()));
            }
        } else {
            Log.e(TAG, "Handsfree phone proxy null for query phone state");
        }
    }

    /**
     * Check if we are currently in a phone call
     *
     * @return True iff we are in a phone call
     */
    @VisibleForTesting
    public boolean isInCall() {
        return ((mHeadsetPhoneState.getNumActiveCall() > 0) || (mHeadsetPhoneState.getNumHeldCall()
                > 0) || ((mHeadsetPhoneState.getCallState() != HeadsetHalConstants.CALL_STATE_IDLE)
                && (mHeadsetPhoneState.getCallState() != HeadsetHalConstants.CALL_STATE_INCOMING)));
    }

    /**
     * Check if there is currently an incoming call
     *
     * @return True iff there is an incoming call
     */
    @VisibleForTesting
    public boolean isRinging() {
        return mHeadsetPhoneState.getCallState() == HeadsetHalConstants.CALL_STATE_INCOMING;
    }

    /**
     * Check if call status is idle
     *
     * @return true if call state is neither ringing nor in call
     */
    @VisibleForTesting
    public boolean isCallIdle() {
        return !isInCall() && !isRinging();
    }

    /**
     * Activate voice recognition on Android system
     *
     * @return true if activation succeeds, caller should wait for
     * {@link BluetoothHeadset#startVoiceRecognition(BluetoothDevice)} callback that will then
     * trigger {@link HeadsetService#startVoiceRecognition(BluetoothDevice)}, false if failed to
     * activate
     */
    @VisibleForTesting
    public boolean activateVoiceRecognition() {
        Intent intent = new Intent(Intent.ACTION_VOICE_COMMAND);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try {
            mHeadsetService.startActivity(intent);
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "activateVoiceRecognition, failed due to activity not found for " + intent);
            return false;
        }
        return true;
    }

    /**
     * Deactivate voice recognition on Android system
     *
     * @return true if activation succeeds, caller should wait for
     * {@link BluetoothHeadset#stopVoiceRecognition(BluetoothDevice)} callback that will then
     * trigger {@link HeadsetService#stopVoiceRecognition(BluetoothDevice)}, false if failed to
     * activate
     */
    @VisibleForTesting
    public boolean deactivateVoiceRecognition() {
        // TODO: need a method to deactivate voice recognition on Android
        return true;
    }

}
