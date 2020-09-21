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

package com.android.car.broadcastradio.support.platform;

import android.annotation.NonNull;
import android.hardware.radio.RadioMetadata;

/**
 * Proposed extensions to android.hardware.radio.RadioMetadata.
 *
 * They might eventually get pushed to the framework.
 */
public class RadioMetadataExt {
    private static int sModuleId;

    /**
     * A hack to inject module ID for getGlobalBitmapId. When pushed to the framework,
     * it will be set with RadioMetadata object creation or just separate int field.
     * @hide
     */
    public static void setModuleId(int id) {
        sModuleId = id;
    }

    /**
     * Proposed redefinition of {@link RadioMetadata#getBitmapId}.
     *
     * {@link RadioMetadata#getBitmapId} isn't part of the system API yet, so we can skip
     * deprecation here and jump straight to the correct solution.
     */
    public static long getGlobalBitmapId(@NonNull RadioMetadata meta, @NonNull String key) {
        int localId = meta.getBitmapId(key);
        if (localId == 0) return 0;

        /* When generating global bitmap ID, we want them to remain stable between sessions
         * (radio app might cache images to disk between sessions).
         *
         * Local IDs are already stable, but module ID is not guaranteed to be stable (i.e. some
         * module might be not available at each boot, due to HW failure).
         *
         * When we push this to the framework, we will need persistence mechanism at the radio
         * service to permanently match modules to their IDs.
         */
        return ((long) sModuleId << 32) | localId;
    }
}
