/**
 * Copyright (C) 2017 The Android Open Source Project
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
import android.hardware.radio.ProgramSelector;
import android.hardware.radio.RadioManager;
import android.hardware.radio.RadioMetadata;
import android.hardware.radio.RadioTuner;
import android.os.Handler;
import android.os.Looper;

import java.util.Map;
import java.util.Objects;

/**
 * Proposed extensions to android.hardware.radio.TunerCallbackAdapter.
 *
 * This class is not compatible with the original at all (because they operate
 * on different callback types), it just represents a proposed feature
 * extensions.
 *
 * They might eventually get pushed to the framework.
 */
class TunerCallbackAdapterExt extends RadioTuner.Callback {
    private static final int INIT_TIMEOUT_MS = 10000;  // 10s

    private final Object mInitLock = new Object();
    private boolean mIsInitialized = false;

    private final RadioTuner.Callback mCallback;
    private final Handler mHandler;

    TunerCallbackAdapterExt(@NonNull RadioTuner.Callback callback, @Nullable Handler handler) {
        mCallback = Objects.requireNonNull(callback);
        if (handler == null) {
            mHandler = new Handler(Looper.getMainLooper());
        } else {
            mHandler = handler;
        }
    }

    public boolean waitForInitialization() {
        synchronized (mInitLock) {
            if (mIsInitialized) return true;
            try {
                mInitLock.wait(INIT_TIMEOUT_MS);
            } catch (InterruptedException ex) {
                // ignore the exception, as we check mIsInitialized anyway
            }
            return mIsInitialized;
        }
    }

    @Override
    public void onError(int status) {
        mHandler.post(() -> mCallback.onError(status));
    }

    @Override
    public void onTuneFailed(int result, @Nullable ProgramSelector selector) {
        mHandler.post(() -> mCallback.onTuneFailed(result, selector));
    }

    @Override
    public void onConfigurationChanged(RadioManager.BandConfig config) {
        mHandler.post(() -> mCallback.onConfigurationChanged(config));
        if (mIsInitialized) return;
        synchronized (mInitLock) {
            mIsInitialized = true;
            mInitLock.notifyAll();
        }
    }

    public void onProgramInfoChanged(RadioManager.ProgramInfo info) {
        mHandler.post(() -> mCallback.onProgramInfoChanged(info));
    }

    public void onMetadataChanged(RadioMetadata metadata) {
        mHandler.post(() -> mCallback.onMetadataChanged(metadata));
    }

    public void onTrafficAnnouncement(boolean active) {
        mHandler.post(() -> mCallback.onTrafficAnnouncement(active));
    }

    public void onEmergencyAnnouncement(boolean active) {
        mHandler.post(() -> mCallback.onEmergencyAnnouncement(active));
    }

    public void onAntennaState(boolean connected) {
        mHandler.post(() -> mCallback.onAntennaState(connected));
    }

    public void onControlChanged(boolean control) {
        mHandler.post(() -> mCallback.onControlChanged(control));
    }

    public void onBackgroundScanAvailabilityChange(boolean isAvailable) {
        mHandler.post(() -> mCallback.onBackgroundScanAvailabilityChange(isAvailable));
    }

    public void onBackgroundScanComplete() {
        mHandler.post(() -> mCallback.onBackgroundScanComplete());
    }

    public void onProgramListChanged() {
        mHandler.post(() -> mCallback.onProgramListChanged());
    }

    public void onParametersUpdated(@NonNull Map<String, String> parameters) {
        mHandler.post(() -> mCallback.onParametersUpdated(parameters));
    }
}
