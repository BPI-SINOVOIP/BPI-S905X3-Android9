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

import android.annotation.WorkerThread;
import android.arch.lifecycle.LiveData;
import android.arch.lifecycle.Transformations;
import android.arch.persistence.room.Dao;
import android.arch.persistence.room.Database;
import android.arch.persistence.room.Insert;
import android.arch.persistence.room.OnConflictStrategy;
import android.arch.persistence.room.Query;
import android.arch.persistence.room.Room;
import android.arch.persistence.room.RoomDatabase;
import android.arch.persistence.room.TypeConverters;
import android.content.Context;
import android.hardware.radio.ProgramSelector;
import android.support.annotation.NonNull;

import com.android.car.broadcastradio.support.Program;

import java.util.List;
import java.util.stream.Collectors;

/**
 * Radio app database schema.
 *
 * This class should not be accessed directly.
 * Instead, {@link RadioStorage} interfaces directly with it.
 */
@Database(entities = {Favorite.class}, exportSchema = false, version = 1)
@TypeConverters({ProgramSelectorConverter.class})
abstract class RadioDatabase extends RoomDatabase {
    @Dao
    protected interface FavoriteDao {
        @Query("SELECT * FROM Favorite ORDER BY primaryId_type, primaryId_value")
        LiveData<List<Favorite>> loadAll();

        @Insert(onConflict = OnConflictStrategy.REPLACE)
        void insertAll(Favorite... favorites);

        @Query("DELETE FROM Favorite WHERE "
                + "primaryId_type = :primaryIdType AND primaryId_value = :primaryIdValue")
        void delete(int primaryIdType, long primaryIdValue);

        default void delete(@NonNull ProgramSelector.Identifier id) {
            delete(id.getType(), id.getValue());
        }
    }

    protected abstract FavoriteDao favoriteDao();

    public static RadioDatabase buildInstance(Context context) {
        return Room.databaseBuilder(context.getApplicationContext(),
                RadioDatabase.class, RadioDatabase.class.getSimpleName()).build();
    }

    /**
     * Returns a list of all user stored radio favorites sorted by primary identifier.
     */
    @WorkerThread
    @NonNull
    public LiveData<List<Program>> getAllFavorites() {
        return Transformations.map(favoriteDao().loadAll(), favorites ->
                favorites.stream().map(Favorite::toProgram).collect(Collectors.toList()));
    }

    /**
     * Saves a given {@link Program} as a favorite.
     *
     * The favorite will replace any existing entry for a given primary
     * identifier if there is a conflict.
     */
    @WorkerThread
    public void insertFavorite(@NonNull Program preset) {
        favoriteDao().insertAll(new Favorite(preset));
    }

    /**
     * Removes a favorite by primary id of its {@link ProgramSelector}.
     */
    @WorkerThread
    public void removeFavorite(@NonNull ProgramSelector sel) {
        favoriteDao().delete(sel.getPrimaryId());
    }
}
