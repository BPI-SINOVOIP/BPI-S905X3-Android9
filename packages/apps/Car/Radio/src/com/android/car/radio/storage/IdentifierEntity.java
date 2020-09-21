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

import android.annotation.Nullable;
import android.arch.persistence.room.Entity;
import android.hardware.radio.ProgramSelector;
import android.hardware.radio.ProgramSelector.IdentifierType;
import android.support.annotation.NonNull;

@Entity
class IdentifierEntity {
    @IdentifierType
    public final int type;

    public final long value;

    IdentifierEntity(@IdentifierType int type, long value) {
        this.type = type;
        this.value = value;
    }

    IdentifierEntity(@NonNull ProgramSelector.Identifier id) {
        type = id.getType();
        value = id.getValue();
    }

    public boolean sameAs(@Nullable ProgramSelector.Identifier id) {
        if (id == null) return false;
        return id.getType() == type && id.getValue() == value;
    }
}
