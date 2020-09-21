/**
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

package com.android.car.radio.platform;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.car.Car;
import android.car.CarNotConnectedException;
import android.car.media.CarAudioManager;
import android.car.media.CarAudioPatchHandle;
import android.content.ComponentName;
import android.content.Context;
import android.content.ServiceConnection;
import android.media.AudioAttributes;
import android.os.IBinder;
import android.util.Log;

import java.util.stream.Stream;

/**
 * Proposed extensions to android.hardware.radio.RadioTuner.
 *
 * They might eventually get pushed to the framework.
 */
public class RadioTunerExt {
    private static final String TAG = "BcRadioApp.tunerext";

    // for now, we only support a single tuner with hardcoded address
    private static final String HARDCODED_TUNER_ADDRESS = "tuner0";

    private final Object mLock = new Object();
    private final Car mCar;
    @Nullable private CarAudioManager mCarAudioManager;

    @Nullable private CarAudioPatchHandle mAudioPatch;
    @Nullable private Boolean mPendingMuteOperation;

    RadioTunerExt(Context context) {
        mCar = Car.createCar(context, mCarServiceConnection);
        mCar.connect();
    }

    private final ServiceConnection mCarServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            synchronized (mLock) {
                try {
                    mCarAudioManager = (CarAudioManager)mCar.getCarManager(Car.AUDIO_SERVICE);
                    if (mPendingMuteOperation != null) {
                        boolean mute = mPendingMuteOperation;
                        mPendingMuteOperation = null;
                        Log.i(TAG, "Car connected, executing postponed operation: "
                                + (mute ? "mute" : "unmute"));
                        setMuted(mute);
                    }
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Car is not connected", e);
                }
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            synchronized (mLock) {
                mCarAudioManager = null;
                mAudioPatch = null;
            }
        }
    };

    private boolean isSourceAvailableLocked(@NonNull String address)
            throws CarNotConnectedException {
        String[] sources = mCarAudioManager.getExternalSources();
        return Stream.of(sources).anyMatch(source -> address.equals(source));
    }

    public boolean setMuted(boolean muted) {
        synchronized (mLock) {
            if (mCarAudioManager == null) {
                Log.i(TAG, "Car not connected yet, postponing operation: "
                        + (muted ? "mute" : "unmute"));
                mPendingMuteOperation = muted;
                return true;
            }

            // if it's already (not) muted - no need to (un)mute again
            if ((mAudioPatch == null) == muted) return true;

            try {
                if (!muted) {
                    if (!isSourceAvailableLocked(HARDCODED_TUNER_ADDRESS)) {
                        Log.e(TAG, "Tuner source \"" + HARDCODED_TUNER_ADDRESS
                                + "\" is not available");
                        return false;
                    }
                    Log.d(TAG, "Creating audio patch for " + HARDCODED_TUNER_ADDRESS);
                    mAudioPatch = mCarAudioManager.createAudioPatch(HARDCODED_TUNER_ADDRESS,
                            AudioAttributes.USAGE_MEDIA, 0);
                } else {
                    Log.d(TAG, "Releasing audio patch");
                    mCarAudioManager.releaseAudioPatch(mAudioPatch);
                    mAudioPatch = null;
                }
                return true;
            } catch (CarNotConnectedException e) {
                Log.e(TAG, "Can't (un)mute - car is not connected", e);
                return false;
            }
        }
    }

    public boolean isMuted() {
        synchronized (mLock) {
            if (mPendingMuteOperation != null) return mPendingMuteOperation;
            return mAudioPatch == null;
        }
    }

    public void close() {
        mCar.disconnect();
    }
}
