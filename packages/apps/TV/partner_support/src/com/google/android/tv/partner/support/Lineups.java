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
import java.util.HashSet;
import java.util.Set;

/** Static utilities for {@link Lineup}. */
public final class Lineups {

    public static Set<Lineup> query(ContentResolver contentResolver, String postalCode) {
        Set<Lineup> result = new HashSet<>();
        try (Cursor cursor =
                contentResolver.query(
                        EpgContract.Lineups.uriForPostalCode(postalCode), null, null, null, null)) {
            ContentValues contentValues = new ContentValues();
            while (cursor.moveToNext()) {
                contentValues.clear();
                DatabaseUtils.cursorRowToContentValues(cursor, contentValues);
                result.add(Lineup.createLineup(contentValues));
            }
            return result;
        }
    }

    private Lineups() {}
}
