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

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.net.Uri;
import android.support.annotation.Nullable;
import android.support.annotation.WorkerThread;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

/** Static utilities for {@link EpgInput}. */
public final class EpgInputs {
    // TODO create a fake EpgContentProvider for testing.

    /** Returns {@link EpgInput} for {@code inputId} or null if not found. */
    @WorkerThread
    @Nullable
    public static EpgInput queryEpgInput(ContentResolver contentResolver, String inputId) {

        for (EpgInput epgInput : queryEpgInputs(contentResolver)) {
            if (inputId.equals(epgInput.getInputId())) {
                return epgInput;
            }
        }
        return null;
    }

    /** Returns all {@link EpgInput}. */
    @WorkerThread
    public static Set<EpgInput> queryEpgInputs(ContentResolver contentResolver) {
        try (Cursor cursor =
                contentResolver.query(EpgContract.EpgInputs.CONTENT_URI, null, null, null, null)) {
            if (cursor == null) {
                return Collections.emptySet();
            }
            HashSet<EpgInput> result = new HashSet<>(cursor.getCount());
            while (cursor.moveToNext()) {
                ContentValues contentValues = new ContentValues();
                DatabaseUtils.cursorRowToContentValues(cursor, contentValues);
                result.add(EpgInput.createEpgChannel(contentValues));
            }
            return result;
        }
    }

    /** Insert an {@link EpgInput}. */
    @WorkerThread
    @Nullable
    public static Uri insert(ContentResolver contentResolver, EpgInput epgInput) {
        return contentResolver.insert(
                EpgContract.EpgInputs.CONTENT_URI, epgInput.toContentValues());
    }

    /** Update an {@link EpgInput}. */
    @WorkerThread
    public static int update(ContentResolver contentResolver, EpgInput epgInput) {
        return contentResolver.update(
                EpgContract.EpgInputs.buildUri(epgInput.getId()),
                epgInput.toContentValues(),
                null,
                null);
    }

    private EpgInputs() {}
}
