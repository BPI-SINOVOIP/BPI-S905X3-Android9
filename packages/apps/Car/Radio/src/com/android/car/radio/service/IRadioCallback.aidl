/**
 * Copyright (c) 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.car.radio.service;

import android.hardware.radio.RadioManager;

/**
 * Interface for applications to listen for changes in the current radio state.
 */
oneway interface IRadioCallback {
    /**
     * Called when the current program info has changed.
     *
     * This might happen either as a result of tune operation or just metadata change.
     *
     * @param info The current program info.
     */
    void onCurrentProgramInfoChanged(in RadioManager.ProgramInfo info);

    /**
     * Called when the mute state of the radio has changed.
     *
     * @param isMuted {@code true} if the radio is muted.
     */
    void onRadioMuteChanged(boolean isMuted);

    /**
     * Called when the radio has encountered an error.
     *
     * @param status One of the error states in {@link RadioManager}. For example,
     *               {@link RadioManager#ERROR_HARDWARE_FAILURE}.
     */
    void onError(int status);
}
