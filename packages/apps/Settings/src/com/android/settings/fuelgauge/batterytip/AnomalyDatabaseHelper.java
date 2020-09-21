/*
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

package com.android.settings.fuelgauge.batterytip;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.support.annotation.IntDef;
import android.util.Log;

import com.android.settings.fuelgauge.anomaly.Anomaly;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Database controls the anomaly logging(e.g. packageName, anomalyType and time)
 */
public class AnomalyDatabaseHelper extends SQLiteOpenHelper {
    private static final String TAG = "BatteryDatabaseHelper";

    private static final String DATABASE_NAME = "battery_settings.db";
    private static final int DATABASE_VERSION = 4;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({State.NEW,
            State.HANDLED,
            State.AUTO_HANDLED})
    public @interface State {
        int NEW = 0;
        int HANDLED = 1;
        int AUTO_HANDLED = 2;
    }

    public interface Tables {
        String TABLE_ANOMALY = "anomaly";
    }

    public interface AnomalyColumns {
        /**
         * The package name of the anomaly app
         */
        String PACKAGE_NAME = "package_name";
        /**
         * The uid of the anomaly app
         */
        String UID = "uid";
        /**
         * The type of the anomaly app
         * @see Anomaly.AnomalyType
         */
        String ANOMALY_TYPE = "anomaly_type";
        /**
         * The state of the anomaly app
         * @see State
         */
        String ANOMALY_STATE = "anomaly_state";
        /**
         * The time when anomaly happens
         */
        String TIME_STAMP_MS = "time_stamp_ms";
    }

    private static final String CREATE_ANOMALY_TABLE =
            "CREATE TABLE " + Tables.TABLE_ANOMALY +
                    "(" +
                    AnomalyColumns.UID +
                    " INTEGER NOT NULL, " +
                    AnomalyColumns.PACKAGE_NAME +
                    " TEXT, " +
                    AnomalyColumns.ANOMALY_TYPE +
                    " INTEGER NOT NULL, " +
                    AnomalyColumns.ANOMALY_STATE +
                    " INTEGER NOT NULL, " +
                    AnomalyColumns.TIME_STAMP_MS +
                    " INTEGER NOT NULL, " +
                    " PRIMARY KEY (" + AnomalyColumns.UID + "," + AnomalyColumns.ANOMALY_TYPE + ","
                    + AnomalyColumns.ANOMALY_STATE + "," + AnomalyColumns.TIME_STAMP_MS + ")"
                    + ")";

    private static AnomalyDatabaseHelper sSingleton;

    public static synchronized AnomalyDatabaseHelper getInstance(Context context) {
        if (sSingleton == null) {
            sSingleton = new AnomalyDatabaseHelper(context.getApplicationContext());
        }
        return sSingleton;
    }

    private AnomalyDatabaseHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        bootstrapDB(db);
    }

    private void bootstrapDB(SQLiteDatabase db) {
        db.execSQL(CREATE_ANOMALY_TABLE);
        Log.i(TAG, "Bootstrapped database");
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        if (oldVersion < DATABASE_VERSION) {
            Log.w(TAG, "Detected schema version '" + oldVersion + "'. " +
                    "Index needs to be rebuilt for schema version '" + newVersion + "'.");
            // We need to drop the tables and recreate them
            reconstruct(db);
        }
    }

    @Override
    public void onDowngrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        Log.w(TAG, "Detected schema version '" + oldVersion + "'. " +
                "Index needs to be rebuilt for schema version '" + newVersion + "'.");
        // We need to drop the tables and recreate them
        reconstruct(db);
    }

    public void reconstruct(SQLiteDatabase db) {
        dropTables(db);
        bootstrapDB(db);
    }

    private void dropTables(SQLiteDatabase db) {
        db.execSQL("DROP TABLE IF EXISTS " + Tables.TABLE_ANOMALY);
    }
}
