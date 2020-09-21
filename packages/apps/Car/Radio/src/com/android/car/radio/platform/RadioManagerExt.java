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
import android.content.Context;
import android.graphics.Bitmap;
import android.hardware.radio.RadioManager;
import android.hardware.radio.RadioManager.BandDescriptor;
import android.hardware.radio.RadioTuner;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import com.android.car.broadcastradio.support.platform.RadioMetadataExt;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.stream.Collectors;

/**
 * Proposed extensions to android.hardware.radio.RadioManager.
 *
 * They might eventually get pushed to the framework.
 */
public class RadioManagerExt {
    private static final String TAG = "BcRadioApp.mgrext";

    // For now, we open first radio module only.
    private static final int HARDCODED_MODULE_INDEX = 0;

    private final Object mLock = new Object();

    private final HandlerThread mCallbackHandlerThread = new HandlerThread("BcRadioApp.cbhandler");

    private final @NonNull RadioManager mRadioManager;
    private final RadioTunerExt mRadioTunerExt;
    private List<RadioManager.ModuleProperties> mModules;
    private @Nullable List<BandDescriptor> mAmFmRegionConfig;

    public RadioManagerExt(@NonNull Context ctx) {
        mRadioManager = (RadioManager)ctx.getSystemService(Context.RADIO_SERVICE);
        Objects.requireNonNull(mRadioManager, "RadioManager could not be loaded");
        mRadioTunerExt = new RadioTunerExt(ctx);
        mCallbackHandlerThread.start();
    }

    public RadioTunerExt getRadioTunerExt() {
        return mRadioTunerExt;
    }

    /* Select only one region. HAL 2.x moves region selection responsibility from the app to the
     * Broadcast Radio service, so we won't implement region selection based on bands in the app.
     */
    private @Nullable List<BandDescriptor> reduceAmFmBands(@Nullable BandDescriptor[] bands) {
        if (bands == null || bands.length == 0) return null;
        int region = bands[0].getRegion();
        Log.d(TAG, "Auto-selecting region " + region);

        return Arrays.stream(bands).filter(band -> band.getRegion() == region).
                collect(Collectors.toList());
    }

    private void initModules() {
        synchronized (mLock) {
            if (mModules != null) return;

            mModules = new ArrayList<>();
            int status = mRadioManager.listModules(mModules);
            if (status != RadioManager.STATUS_OK) {
                Log.w(TAG, "Couldn't get radio module list: " + status);
                return;
            }

            if (mModules.size() == 0) {
                Log.i(TAG, "No radio modules on this device");
                return;
            }

            RadioManager.ModuleProperties module = mModules.get(HARDCODED_MODULE_INDEX);
            mAmFmRegionConfig = reduceAmFmBands(module.getBands());
        }
    }

    public @Nullable RadioTuner openSession(RadioTuner.Callback callback, Handler handler) {
        Log.i(TAG, "Opening broadcast radio session...");

        initModules();
        if (mModules.size() == 0) return null;

        /* We won't need custom default wrapper when we push these proposed extensions to the
         * framework; this is solely to avoid deadlock on onConfigurationChanged callback versus
         * waitForInitialization.
         */
        Handler hwHandler = new Handler(mCallbackHandlerThread.getLooper());

        RadioManager.ModuleProperties module = mModules.get(HARDCODED_MODULE_INDEX);
        TunerCallbackAdapterExt cbExt = new TunerCallbackAdapterExt(callback, handler);

        RadioTuner tuner = mRadioManager.openTuner(
                module.getId(),
                null,  // BandConfig - let the service automatically select one.
                true,  // withAudio
                cbExt, hwHandler);
        mSessions.put(module.getId(), tuner);
        if (tuner == null) return null;
        RadioMetadataExt.setModuleId(module.getId());

        if (module.isInitializationRequired()) {
            if (!cbExt.waitForInitialization()) {
                Log.w(TAG, "Timed out waiting for tuner initialization");
                tuner.close();
                return null;
            }
        }

        return tuner;
    }

    public @Nullable List<BandDescriptor> getAmFmRegionConfig() {
        initModules();
        return mAmFmRegionConfig;
    }

    /* This won't be necessary when we push this code to the framework,
     * as we really need only module references. */
    private static Map<Integer, RadioTuner> mSessions = new HashMap<>();

    public @Nullable Bitmap getMetadataImage(long globalId) {
        if (globalId == 0) return null;

        int moduleId = (int)(globalId >>> 32);
        int localId = (int)(globalId & 0xFFFFFFFF);

        RadioTuner tuner = mSessions.get(moduleId);
        if (tuner == null) return null;

        return tuner.getMetadataImage(localId);
    }
}
