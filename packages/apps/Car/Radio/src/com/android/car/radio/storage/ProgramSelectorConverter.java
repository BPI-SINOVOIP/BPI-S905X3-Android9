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

package com.android.car.radio.storage;

import android.arch.persistence.room.TypeConverter;
import android.hardware.radio.ProgramSelector;
import android.net.Uri;
import android.support.annotation.NonNull;

import com.android.car.broadcastradio.support.platform.ProgramSelectorExt;

import java.util.Objects;

class ProgramSelectorConverter {
    @TypeConverter
    @NonNull
    public static ProgramSelector toSelector(@NonNull String uri) {
        return Objects.requireNonNull(ProgramSelectorExt.fromUri(Uri.parse(uri)));
    }

    @TypeConverter
    @NonNull
    public static String toString(@NonNull ProgramSelector sel) {
        return ProgramSelectorExt.toUri(sel).toString();
    }
}
