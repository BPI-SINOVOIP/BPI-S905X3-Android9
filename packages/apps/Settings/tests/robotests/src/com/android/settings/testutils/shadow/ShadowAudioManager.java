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
 * limitations under the License.
 */

package com.android.settings.testutils.shadow;

import static android.media.AudioManager.STREAM_ACCESSIBILITY;
import static android.media.AudioManager.STREAM_ALARM;
import static android.media.AudioManager.STREAM_MUSIC;
import static android.media.AudioManager.STREAM_NOTIFICATION;
import static android.media.AudioManager.STREAM_RING;
import static android.media.AudioManager.STREAM_SYSTEM;
import static android.media.AudioManager.STREAM_VOICE_CALL;
import static android.media.AudioManager.STREAM_DTMF;

import static org.robolectric.RuntimeEnvironment.application;

import android.media.AudioDeviceCallback;
import android.media.AudioManager;
import android.os.Handler;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.Resetter;
import org.robolectric.shadow.api.Shadow;

import java.util.ArrayList;

@Implements(value = AudioManager.class, inheritImplementationMethods = true)
public class ShadowAudioManager extends org.robolectric.shadows.ShadowAudioManager {
    private int mRingerMode;
    private int mDeviceCodes;
    private boolean mMusicActiveRemotely;
    private boolean mBluetoothScoOn;
    private ArrayList<AudioDeviceCallback> mDeviceCallbacks = new ArrayList();

    @Implementation
    private int getRingerModeInternal() {
        return mRingerMode;
    }

    public static ShadowAudioManager getShadow() {
        return Shadow.extract(application.getSystemService(AudioManager.class));
    }

    public void setRingerModeInternal(int mode) {
        mRingerMode = mode;
    }

    @Implementation
    public void registerAudioDeviceCallback(AudioDeviceCallback callback, Handler handler) {
        mDeviceCallbacks.add(callback);
    }

    @Implementation
    public void unregisterAudioDeviceCallback(AudioDeviceCallback callback) {
        if (mDeviceCallbacks.contains(callback)) {
            mDeviceCallbacks.remove(callback);
        }
    }

    public void setMusicActiveRemotely(boolean flag) {
        mMusicActiveRemotely = flag;
    }

    @Implementation
    public boolean isMusicActiveRemotely() {
        return mMusicActiveRemotely;
    }

    public void setOutputDevice(int deviceCodes) {
        mDeviceCodes = deviceCodes;
    }

    @Implementation
    public int getDevicesForStream(int streamType) {
        switch (streamType) {
            case STREAM_VOICE_CALL:
            case STREAM_SYSTEM:
            case STREAM_RING:
            case STREAM_MUSIC:
            case STREAM_ALARM:
            case STREAM_NOTIFICATION:
            case STREAM_DTMF:
            case STREAM_ACCESSIBILITY:
                return mDeviceCodes;
            default:
                return 0;
        }
    }

    @Resetter
    public void reset() {
        mDeviceCallbacks.clear();
    }

    public void setBluetoothScoOn(boolean bluetoothScoOn) {
        mBluetoothScoOn = bluetoothScoOn;
    }

    @Implementation
    public boolean isBluetoothScoOn() { return mBluetoothScoOn; }
}
