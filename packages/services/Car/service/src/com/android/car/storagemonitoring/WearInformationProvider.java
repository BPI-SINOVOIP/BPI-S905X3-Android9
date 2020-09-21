/*
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
package com.android.car.storagemonitoring;

import android.annotation.Nullable;

public interface WearInformationProvider {
    @Nullable
    WearInformation load();

    default int convertLifetime(int lifetime) {
        if ((lifetime <= 0) || (lifetime > 11)) return WearInformation.UNKNOWN_LIFETIME_ESTIMATE;
        return 10 * (lifetime - 1);
    }

    default int adjustEol(int eol) {
        if ((eol <= 0) || (eol > 3)) return WearInformation.UNKNOWN_PRE_EOL_INFO;
        return eol;
    }

}
