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
 * limitations under the License
 */

package com.google.android.tv.partner.support;

import android.content.ContentValues;

/**
 * Value class representing a TV Input that uses Live TV EPG.
 *
 * @see {@link EpgContract.EpgInputs}
 */
// TODO(b/72052568): Get autovalue to work in aosp master
public abstract class EpgInput {

    public static EpgInput createEpgChannel(long id, String inputId, String lineupId) {
        return new AutoValue_EpgInput(id, inputId, lineupId);
    }

    public static EpgInput createEpgChannel(ContentValues contentValues) {
        long id = contentValues.getAsLong(EpgContract.EpgInputs.COLUMN_ID);
        String inputId = contentValues.getAsString(EpgContract.EpgInputs.COLUMN_INPUT_ID);
        String lineupId = contentValues.getAsString(EpgContract.EpgInputs.COLUMN_LINEUP_ID);
        return createEpgChannel(id, inputId, lineupId);
    }

    /** The unique ID for a row. */
    public abstract long getId();

    /**
     * The ID of the TV input service that provides this TV channel.
     *
     * <p>Use {@link android.media.tv.TvContract#buildInputId} to build the ID.
     */
    public abstract String getInputId();

    /**
     * The line up id.
     *
     * <p>This is a opaque string that should not be parsed.
     */
    public abstract String getLineupId();

    public final ContentValues toContentValues() {
        ContentValues contentValues = new ContentValues();
        contentValues.put(EpgContract.EpgInputs.COLUMN_ID, getId());
        contentValues.put(EpgContract.EpgInputs.COLUMN_INPUT_ID, getInputId());
        contentValues.put(EpgContract.EpgInputs.COLUMN_LINEUP_ID, getLineupId());
        return contentValues;
    }
}
