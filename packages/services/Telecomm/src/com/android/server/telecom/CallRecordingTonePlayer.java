/*
 * Copyright (C) 2018 The Android Open Source Project
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

import android.content.Context;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.media.AudioRecordingConfiguration;
import android.media.MediaPlayer;
import android.os.Handler;
import android.os.Looper;
import android.telecom.Log;

import com.android.internal.annotations.VisibleForTesting;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.stream.Collectors;

/**
 * Plays a periodic, repeating tone to the remote party when an app on the device is recording
 * a call.  A call recording tone is played on the called party's audio if an app begins recording.
 * This ensures that the remote party is aware of the fact call recording is in progress.
 */
public class CallRecordingTonePlayer extends CallsManagerListenerBase {
    /**
     * Callback registered with {@link AudioManager} to track apps which are recording audio.
     * Registered when a SIM call is added and unregistered when it ends.
     */
    private AudioManager.AudioRecordingCallback mAudioRecordingCallback =
            new AudioManager.AudioRecordingCallback() {
                @Override
                public void onRecordingConfigChanged(List<AudioRecordingConfiguration> configs) {
                    synchronized (mLock) {
                        try {
                            Log.startSession("CRTP.oRCC");
                            handleRecordingConfigurationChange(configs);
                            maybeStartCallAudioTone();
                            maybeStopCallAudioTone();
                        } finally {
                            Log.endSession();
                        }
                    }
                }
    };

    private final AudioManager mAudioManager;
    private final Context mContext;
    private final TelecomSystem.SyncRoot mLock;
    private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());
    private boolean mIsRecording = false;
    private MediaPlayer mRecordingTonePlayer = null;
    private List<Call> mCalls = new ArrayList<>();

    public CallRecordingTonePlayer(Context context, AudioManager audioManager,
            TelecomSystem.SyncRoot lock) {
        mContext = context;
        mAudioManager = audioManager;
        mLock = lock;
    }

    @Override
    public void onCallAdded(Call call) {
        if (!shouldUseRecordingTone(call)) {
            return; // Ignore calls which don't use the recording tone.
        }

        addCall(call);
    }

    @Override
    public void onCallRemoved(Call call) {
        if (!shouldUseRecordingTone(call)) {
            return; // Ignore calls which don't use the recording tone.
        }

        removeCall(call);
    }

    @Override
    public void onCallStateChanged(Call call, int oldState, int newState) {
        if (!shouldUseRecordingTone(call)) {
            return; // Ignore calls which don't use the recording tone.
        }

        if (mIsRecording) {
            // Handle start and stop now; could be stopping if we held a call.
            maybeStartCallAudioTone();
            maybeStopCallAudioTone();
        }
    }

    /**
     * Handles addition of a new call by:
     * 1. Registering an audio manager listener to track changes to recording state.
     * 2. Checking if there is recording in progress.
     * 3. Potentially starting the call recording tone.
     *
     * @param toAdd The call to start tracking.
     */
    private void addCall(Call toAdd) {
        boolean isFirstCall = mCalls.isEmpty();

        mCalls.add(toAdd);
        if (isFirstCall) {
            // First call, so register the recording callback.  Also check for recordings which
            // started before we registered the callback (we don't receive a callback for those).
            handleRecordingConfigurationChange(mAudioManager.getActiveRecordingConfigurations());
            mAudioManager.registerAudioRecordingCallback(mAudioRecordingCallback,
                    mMainThreadHandler);
        }

        maybeStartCallAudioTone();
    }

    /**
     * Handles removal of tracked call by unregistering the audio recording callback and stopping
     * the recording tone if this is the last call.
     * @param toRemove The call to stop tracking.
     */
    private void removeCall(Call toRemove) {
        mCalls.remove(toRemove);
        boolean isLastCall = mCalls.isEmpty();

        if (isLastCall) {
            mAudioManager.unregisterAudioRecordingCallback(mAudioRecordingCallback);
            maybeStopCallAudioTone();
        }
    }

    /**
     * Determines whether a call is applicable for call recording tone generation.
     * Only top level sim calls are considered which have
     * {@link android.telecom.PhoneAccount#EXTRA_PLAY_CALL_RECORDING_TONE} set on their target
     * {@link android.telecom.PhoneAccount}.
     * @param call The call to check.
     * @return {@code true} if the call is should use the recording tone, {@code false} otherwise.
     */
    private boolean shouldUseRecordingTone(Call call) {
        return call.getParentCall() == null && !call.isExternalCall() &&
                !call.isEmergencyCall() && call.isUsingCallRecordingTone();
    }

    /**
     * Starts the call recording tone if recording has started and there are calls.
     */
    private void maybeStartCallAudioTone() {
        if (mIsRecording && hasActiveCall()) {
            startCallRecordingTone(mContext);
        }
    }

    /**
     * Stops the call recording tone if recording has stopped or there are no longer any calls.
     */
    private void maybeStopCallAudioTone() {
        if (!mIsRecording || !hasActiveCall()) {
            stopCallRecordingTone();
        }
    }

    /**
     * Determines if any of the calls tracked are active.
     * @return {@code true} if there is an active call, {@code false} otherwise.
     */
    private boolean hasActiveCall() {
        return !mCalls.isEmpty() && mCalls.stream()
                .filter(call -> call.isActive())
                .count() > 0;
    }

    /**
     * Handles changes to recording configuration changes.
     * @param configs the recording configurations.
     */
    private void handleRecordingConfigurationChange(List<AudioRecordingConfiguration> configs) {
        if (configs == null) {
            configs = Collections.emptyList();
        }
        boolean wasRecording = mIsRecording;
        boolean isRecording = isRecordingInProgress(configs);
        if (wasRecording != isRecording) {
            mIsRecording = isRecording;
            if (isRecording) {
                Log.i(this, "handleRecordingConfigurationChange: recording started");
            } else {
                Log.i(this, "handleRecordingConfigurationChange: recording stopped");
            }
        }
    }

    /**
     * Determines if call recording is potentially in progress.
     * Excludes from consideration any recordings from packages which have active calls themselves.
     * Presumably a call with an active recording session is doing so in order to capture the audio
     * for the purpose of making a call.  In practice Telephony calls don't show up in the
     * recording configurations, but it is reasonable to consider Connection Managers which are
     * using an over the top voip solution for calling.
     * @param configs the ongoing recording configurations.
     * @return {@code true} if there are active audio recordings for which we want to generate a
     * call recording tone, {@code false} otherwise.
     */
    private boolean isRecordingInProgress(List<AudioRecordingConfiguration> configs) {
        String recordingPackages = configs.stream()
                .map(config -> config.getClientPackageName())
                .collect(Collectors.joining(", "));
        Log.i(this, "isRecordingInProgress: recordingPackages=%s", recordingPackages);
        return configs.stream()
                .filter(config -> !hasCallForPackage(config.getClientPackageName()))
                .count() > 0;
    }

    /**
     * Begins playing the call recording tone to the remote end of the call.
     * The call recording tone is played via the telephony audio output device; this means that it
     * will only be audible to the remote end of the call, not the local side.
     *
     * @param context required for obtaining media player.
     */
    private void startCallRecordingTone(Context context) {
        if (mRecordingTonePlayer != null) {
            return;
        }
        AudioDeviceInfo telephonyDevice = getTelephonyDevice(mAudioManager);
        if (telephonyDevice != null) {
            Log.i(this ,"startCallRecordingTone: playing call recording tone to remote end.");
            mRecordingTonePlayer = MediaPlayer.create(context, R.raw.record);
            mRecordingTonePlayer.setLooping(true);
            mRecordingTonePlayer.setPreferredDevice(telephonyDevice);
            mRecordingTonePlayer.setVolume(0.1f);
            mRecordingTonePlayer.start();
        } else {
            Log.w(this ,"startCallRecordingTone: can't find telephony audio device.");
        }
    }

    /**
     * Attempts to stop the call recording tone if it is playing.
     */
    private void stopCallRecordingTone() {
        if (mRecordingTonePlayer != null) {
            Log.i(this ,"stopCallRecordingTone: stopping call recording tone.");
            mRecordingTonePlayer.stop();
            mRecordingTonePlayer = null;
        }
    }

    /**
     * Finds the the output device of type {@link AudioDeviceInfo#TYPE_TELEPHONY}.  This device is
     * the one on which outgoing audio for SIM calls is played.
     * @param audioManager the audio manage.
     * @return the {@link AudioDeviceInfo} corresponding to the telephony device, or {@code null}
     * if none can be found.
     */
    private AudioDeviceInfo getTelephonyDevice(AudioManager audioManager) {
        AudioDeviceInfo[] deviceList = audioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS);
        for (AudioDeviceInfo device: deviceList) {
            if (device.getType() == AudioDeviceInfo.TYPE_TELEPHONY) {
                return device;
            }
        }
        return null;
    }

    /**
     * Determines if any of the known calls belongs to a {@link android.telecom.PhoneAccount} with
     * the specified package name.
     * @param packageName The package name.
     * @return {@code true} if a call exists for this package, {@code false} otherwise.
     */
    private boolean hasCallForPackage(String packageName) {
        return mCalls.stream()
                .filter(call -> (call.getTargetPhoneAccount() != null &&
                        call.getTargetPhoneAccount()
                                .getComponentName().getPackageName().equals(packageName)) ||
                        (call.getConnectionManagerPhoneAccount() != null &&
                                call.getConnectionManagerPhoneAccount()
                                        .getComponentName().getPackageName().equals(packageName)))
                .count() >= 1;
    }

    @VisibleForTesting
    public boolean hasCalls() {
        return mCalls.size() > 0;
    }

    @VisibleForTesting
    public boolean isRecording() {
        return mIsRecording;
    }
}
