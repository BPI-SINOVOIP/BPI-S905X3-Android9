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

import android.arch.persistence.room.Embedded;
import android.arch.persistence.room.Entity;
import android.arch.persistence.room.PrimaryKey;
import android.hardware.radio.ProgramSelector;
import android.support.annotation.NonNull;

import com.android.car.broadcastradio.support.Program;

import java.util.Objects;

@Entity
class Favorite {
    @PrimaryKey
    @NonNull
    @Embedded(prefix = "primaryId_")
    public final IdentifierEntity primaryId;

    @NonNull
    public final ProgramSelector selector;

    @NonNull
    public final String name;

    Favorite(@NonNull IdentifierEntity primaryId, @NonNull ProgramSelector selector,
            @NonNull String name) {
        if (!primaryId.sameAs(selector.getPrimaryId())) {
            throw new IllegalArgumentException(
                    "Can't set different primary ID than program selector's");
        }

        this.primaryId = primaryId;
        this.selector = selector;
        this.name = Objects.requireNonNull(name);
    }

    Favorite(@NonNull Program program) {
        this(new IdentifierEntity(program.getSelector().getPrimaryId()),
                program.getSelector(), program.getName());
    }

    @NonNull
    public Program toProgram() {
        return new Program(selector, name);
    }
}
