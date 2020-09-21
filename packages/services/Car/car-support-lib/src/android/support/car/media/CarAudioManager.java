/*
 * Copyright (C) 2015 The Android Open Source Project
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
package android.support.car.media;

import android.Manifest;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.support.car.CarManagerBase;
import android.support.car.CarNotConnectedException;

import androidx.annotation.RequiresPermission;

/**
 * APIs for handling car-specific audio use cases. Provides a set of CAR_AUDIO_USAGE_* constants
 * that can be used to route audio by use case to the car. Important beyond the normal
 * {@link AudioManager} class methods because it handles multi channel audio. Includes use cases
 * such as routing call audio only to the driver and not through all speakers.
 */
public abstract class CarAudioManager implements CarManagerBase {

    /**
     * Get {@link AudioFormat} for audio record.
     * @return {@link AudioFormat} for audio record.
     */
    public abstract AudioFormat getAudioRecordAudioFormat() throws CarNotConnectedException;

    public abstract boolean isAudioRecordSupported() throws CarNotConnectedException;

    /**
     * Get minimum buffer size for {@link CarAudioRecord}.
     *
     * @return Buffer size in bytes.
     */
    public abstract int getAudioRecordMinBufferSize()
            throws CarNotConnectedException;

    /**
     * Get maximum buffer size for {@link CarAudioRecord}.
     *
     * @return Buffer size in bytes.
     */
    public abstract int getAudioRecordMaxBufferSize()
            throws CarNotConnectedException;

    /**
     * Create a {@link CarAudioRecord} for the current {@link CarAudioManager}. There can be
     * multiple instances of {@link CarAudioRecord}. Requires {@link
     * android.Manifest.permission#RECORD_AUDIO} permission.
     *
     * @param bufferSize Should be a multiple of minimum buffer size acquired from {@link
     * #getAudioRecordMinBufferSize()}. Cannot exceed {@link #getAudioRecordMaxBufferSize()}.
     *
     * @return {@link CarAudioRecord} instance for the given stream.
     * @throws IllegalArgumentException if passed parameter (such as bufferSize) is wrong.
     * @throws SecurityException if client does not have
     * {@link android.Manifest.permission#RECORD_AUDIO} permission.
     */
    @RequiresPermission(Manifest.permission.RECORD_AUDIO)
    public abstract CarAudioRecord createCarAudioRecord(int bufferSize)
            throws SecurityException, CarNotConnectedException;
}
