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

package com.android.car.radio.utils;

import android.annotation.NonNull;
import android.hardware.radio.ProgramSelector;
import android.hardware.radio.RadioManager;

import com.android.car.broadcastradio.support.platform.ProgramSelectorExt;
import com.android.car.radio.storage.RadioStorage;

/**
 * Helper methods for android.hardware.radio.ProgramSelector.
 *
 * Most probably, this code will end up refactored out, less likely pushed to the platform.
 */
public class ProgramSelectorUtils {
    /**
     * Returns AM/FM band for a given ProgramSelector.
     *
     * @param sel ProgramSelector to check.
     * @return {@link RadioManager#BAND_FM} if sel is FM program,
     *         {@link RadioManager#BAND_AM} if sel is AM program,
     *         {@link RadioStorage#INVALID_RADIO_BAND} otherwise.
     */
    public static int getRadioBand(@NonNull ProgramSelector sel) {
        if (!ProgramSelectorExt.isAmFmProgram(sel)) return RadioStorage.INVALID_RADIO_BAND;
        if (!ProgramSelectorExt.hasId(sel, ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY)) {
            return RadioStorage.INVALID_RADIO_BAND;
        }

        long freq = sel.getFirstId(ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY);
        if (ProgramSelectorExt.isAmFrequency(freq)) return RadioManager.BAND_AM;
        if (ProgramSelectorExt.isFmFrequency(freq)) return RadioManager.BAND_FM;

        return RadioStorage.INVALID_RADIO_BAND;
    }
}
