/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tv.tuner;

public class FakeTunerHal extends TunerHal {

    private boolean mDeviceOpened;

    public FakeTunerHal() {
        super(null);
    }

    @Override
    protected boolean openFirstAvailable() {
        mDeviceOpened = true;
        getDeliverySystemTypeFromDevice();
        return true;
    }

    @Override
    protected boolean isDeviceOpen() {
        return mDeviceOpened;
    }

    @Override
    protected long getDeviceId() {
        return 0;
    }

    @Override
    public void close() throws Exception {
        mDeviceOpened = false;
    }

    @Override
    protected void nativeFinalize(long deviceId) {}

    @Override
    protected boolean nativeTune(
            long deviceId, int frequency, @ModulationType String modulation, int timeoutMs) {
        return true;
    }

    @Override
    protected void nativeAddPidFilter(long deviceId, int pid, @FilterType int filterType) {}

    @Override
    protected void nativeCloseAllPidFilters(long deviceId) {}

    @Override
    protected void nativeStopTune(long deviceId) {}

    @Override
    protected int nativeWriteInBuffer(long deviceId, byte[] javaBuffer, int javaBufferSize) {
        return 0;
    }

    @Override
    protected void nativeSetHasPendingTune(long deviceId, boolean hasPendingTune) {}

    @Override
    protected int nativeGetDeliverySystemType(long deviceId) {
        return DELIVERY_SYSTEM_ATSC;
    }
}
