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

package com.android.settings.intelligence.search.indexing;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.os.Build;
import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import android.util.Log;

import java.util.List;
import java.util.Locale;

public class IndexDatabaseHelper extends SQLiteOpenHelper {

    private static final String TAG = "IndexDatabaseHelper";

    private static final String DATABASE_NAME = "search_index.db";
    private static final int DATABASE_VERSION = 119;

    @VisibleForTesting
    static final String SHARED_PREFS_TAG = "indexing_manager";

    private static final String PREF_KEY_INDEXED_PROVIDERS = "indexed_providers";

    public interface Tables {
        String TABLE_PREFS_INDEX = "prefs_index";
        String TABLE_SITE_MAP = "site_map";
        String TABLE_META_INDEX = "meta_index";
        String TABLE_SAVED_QUERIES = "saved_queries";
    }

    public interface IndexColumns {
        String DATA_TITLE = "data_title";
        String DATA_TITLE_NORMALIZED = "data_title_normalized";
        String DATA_SUMMARY_ON = "data_summary_on";
        String DATA_SUMMARY_ON_NORMALIZED = "data_summary_on_normalized";
        String DATA_SUMMARY_OFF = "data_summary_off";
        String DATA_SUMMARY_OFF_NORMALIZED = "data_summary_off_normalized";
        String DATA_ENTRIES = "data_entries";
        String DATA_KEYWORDS = "data_keywords";
        String DATA_PACKAGE = "package";
        String CLASS_NAME = "class_name";
        String SCREEN_TITLE = "screen_title";
        String INTENT_ACTION = "intent_action";
        String INTENT_TARGET_PACKAGE = "intent_target_package";
        String INTENT_TARGET_CLASS = "intent_target_class";
        String ICON = "icon";
        String ENABLED = "enabled";
        String DATA_KEY_REF = "data_key_reference";
        String PAYLOAD_TYPE = "payload_type";
        String PAYLOAD = "payload";
    }

    public interface MetaColumns {
        String BUILD = "build";
    }

    public interface SavedQueriesColumns {
        String QUERY = "query";
        String TIME_STAMP = "timestamp";
    }

    public interface SiteMapColumns {
        String DOCID = "docid";
        String PARENT_CLASS = "parent_class";
        String CHILD_CLASS = "child_class";
        String PARENT_TITLE = "parent_title";
        String CHILD_TITLE = "child_title";
    }

    private static final String CREATE_INDEX_TABLE =
            "CREATE VIRTUAL TABLE " + Tables.TABLE_PREFS_INDEX + " USING fts4" +
                    "(" +
                    IndexColumns.DATA_TITLE +
                    ", " +
                    IndexColumns.DATA_TITLE_NORMALIZED +
                    ", " +
                    IndexColumns.DATA_SUMMARY_ON +
                    ", " +
                    IndexColumns.DATA_SUMMARY_ON_NORMALIZED +
                    ", " +
                    IndexColumns.DATA_SUMMARY_OFF +
                    ", " +
                    IndexColumns.DATA_SUMMARY_OFF_NORMALIZED +
                    ", " +
                    IndexColumns.DATA_ENTRIES +
                    ", " +
                    IndexColumns.DATA_KEYWORDS +
                    ", " +
                    IndexColumns.DATA_PACKAGE +
                    ", " +
                    IndexColumns.SCREEN_TITLE +
                    ", " +
                    IndexColumns.CLASS_NAME +
                    ", " +
                    IndexColumns.ICON +
                    ", " +
                    IndexColumns.INTENT_ACTION +
                    ", " +
                    IndexColumns.INTENT_TARGET_PACKAGE +
                    ", " +
                    IndexColumns.INTENT_TARGET_CLASS +
                    ", " +
                    IndexColumns.ENABLED +
                    ", " +
                    IndexColumns.DATA_KEY_REF +
                    ", " +
                    IndexColumns.PAYLOAD_TYPE +
                    ", " +
                    IndexColumns.PAYLOAD +
                    ");";

    private static final String CREATE_META_TABLE =
            "CREATE TABLE " + Tables.TABLE_META_INDEX +
                    "(" +
                    MetaColumns.BUILD + " VARCHAR(32) NOT NULL" +
                    ")";

    private static final String CREATE_SAVED_QUERIES_TABLE =
            "CREATE TABLE " + Tables.TABLE_SAVED_QUERIES +
                    "(" +
                    SavedQueriesColumns.QUERY + " VARCHAR(64) NOT NULL" +
                    ", " +
                    SavedQueriesColumns.TIME_STAMP + " INTEGER" +
                    ")";

    private static final String CREATE_SITE_MAP_TABLE =
            "CREATE VIRTUAL TABLE " + Tables.TABLE_SITE_MAP + " USING fts4" +
                    "(" +
                    SiteMapColumns.PARENT_CLASS +
                    ", " +
                    SiteMapColumns.CHILD_CLASS +
                    ", " +
                    SiteMapColumns.PARENT_TITLE +
                    ", " +
                    SiteMapColumns.CHILD_TITLE +
                    ")";
    private static final String INSERT_BUILD_VERSION =
            "INSERT INTO " + Tables.TABLE_META_INDEX +
                    " VALUES ('" + Build.VERSION.INCREMENTAL + "');";

    private static final String SELECT_BUILD_VERSION =
            "SELECT " + MetaColumns.BUILD + " FROM " + Tables.TABLE_META_INDEX + " LIMIT 1;";

    private static IndexDatabaseHelper sSingleton;

    private final Context mContext;

    public static synchronized IndexDatabaseHelper getInstance(Context context) {
        if (sSingleton == null) {
            sSingleton = new IndexDatabaseHelper(context);
        }
        return sSingleton;
    }

    public IndexDatabaseHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
        mContext = context.getApplicationContext();
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        bootstrapDB(db);
    }

    private void bootstrapDB(SQLiteDatabase db) {
        db.execSQL(CREATE_INDEX_TABLE);
        db.execSQL(CREATE_META_TABLE);
        db.execSQL(CREATE_SAVED_QUERIES_TABLE);
        db.execSQL(CREATE_SITE_MAP_TABLE);
        db.execSQL(INSERT_BUILD_VERSION);
        Log.i(TAG, "Bootstrapped database");
    }

    @Override
    public void onOpen(SQLiteDatabase db) {
        super.onOpen(db);

        Log.i(TAG, "Using schema version: " + db.getVersion());

        if (!Build.VERSION.INCREMENTAL.equals(getBuildVersion(db))) {
            Log.w(TAG, "Index needs to be rebuilt as build-version is not the same");
            // We need to drop the tables and recreate them
            reconstruct(db);
        } else {
            Log.i(TAG, "Index is fine");
        }
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
        mContext.getSharedPreferences(SHARED_PREFS_TAG, Context.MODE_PRIVATE)
                .edit()
                .clear()
                .commit();
        dropTables(db);
        bootstrapDB(db);
    }

    private String getBuildVersion(SQLiteDatabase db) {
        String version = null;
        Cursor cursor = null;
        try {
            cursor = db.rawQuery(SELECT_BUILD_VERSION, null);
            if (cursor.moveToFirst()) {
                version = cursor.getString(0);
            }
        } catch (Exception e) {
            Log.e(TAG, "Cannot get build version from Index metadata");
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return version;
    }

    @VisibleForTesting
    static String buildProviderVersionedNames(Context context, List<ResolveInfo> providers) {
        // TODO Refactor update test to reflect version code change.
        try {
            StringBuilder sb = new StringBuilder();
            for (ResolveInfo info : providers) {
                String packageName = info.providerInfo.packageName;
                PackageInfo packageInfo = context.getPackageManager().getPackageInfo(packageName,
                        0 /* flags */);
                sb.append(packageName)
                        .append(':')
                        .append(packageInfo.versionCode)
                        .append(',');
            }
            // add SettingsIntelligence version as well.
            sb.append(context.getPackageName())
                    .append(':')
                    .append(context.getPackageManager()
                            .getPackageInfo(context.getPackageName(), 0 /* flags */).versionCode);
            return sb.toString();
        } catch (PackageManager.NameNotFoundException e) {
            Log.d(TAG, "Could not find package name in provider", e);
        }
        return "";
    }

    /**
     * Set a flag that indicates the search database is fully indexed.
     */
    static void setIndexed(Context context, List<ResolveInfo> providers) {
        final String localeStr = Locale.getDefault().toString();
        final String fingerprint = Build.FINGERPRINT;
        final String providerVersionedNames =
                IndexDatabaseHelper.buildProviderVersionedNames(context, providers);
        context.getSharedPreferences(SHARED_PREFS_TAG, Context.MODE_PRIVATE)
                .edit()
                .putBoolean(localeStr, true)
                .putBoolean(fingerprint, true)
                .putString(PREF_KEY_INDEXED_PROVIDERS, providerVersionedNames)
                .apply();
    }

    /**
     * Checks if the indexed data requires full index. The index data is out of date when:
     * - Device language has changed
     * - Device has taken an OTA.
     * In both cases, the device requires a full index.
     *
     * @return true if a full index should be preformed.
     */
    static boolean isFullIndex(Context context, List<ResolveInfo> providers) {
        final String localeStr = Locale.getDefault().toString();
        final String fingerprint = Build.FINGERPRINT;
        final String providerVersionedNames =
                IndexDatabaseHelper.buildProviderVersionedNames(context, providers);
        final SharedPreferences prefs = context
                .getSharedPreferences(SHARED_PREFS_TAG, Context.MODE_PRIVATE);

        final boolean isIndexed = prefs.getBoolean(fingerprint, false)
                && prefs.getBoolean(localeStr, false)
                && TextUtils.equals(
                prefs.getString(PREF_KEY_INDEXED_PROVIDERS, null), providerVersionedNames);
        return !isIndexed;
    }

    private void dropTables(SQLiteDatabase db) {
        db.execSQL("DROP TABLE IF EXISTS " + Tables.TABLE_META_INDEX);
        db.execSQL("DROP TABLE IF EXISTS " + Tables.TABLE_PREFS_INDEX);
        db.execSQL("DROP TABLE IF EXISTS " + Tables.TABLE_SAVED_QUERIES);
        db.execSQL("DROP TABLE IF EXISTS " + Tables.TABLE_SITE_MAP);
    }
}
