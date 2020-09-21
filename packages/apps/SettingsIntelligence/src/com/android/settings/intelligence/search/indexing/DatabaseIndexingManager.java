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

import static com.android.settings.intelligence.search.query.DatabaseResultTask.SELECT_COLUMNS;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.DATA_PACKAGE;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.CLASS_NAME;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.DATA_ENTRIES;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.DATA_KEYWORDS;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.DATA_KEY_REF;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.DATA_SUMMARY_ON;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.DATA_SUMMARY_ON_NORMALIZED;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.DATA_TITLE;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.DATA_TITLE_NORMALIZED;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.ENABLED;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.ICON;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.INTENT_ACTION;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.INTENT_TARGET_CLASS;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.INTENT_TARGET_PACKAGE;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.PAYLOAD;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.PAYLOAD_TYPE;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns.SCREEN_TITLE;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.Tables.TABLE_PREFS_INDEX;
import static com.android.settings.intelligence.search.SearchFeatureProvider.DEBUG;

import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.os.AsyncTask;
import android.provider.SearchIndexablesContract;
import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;

import com.android.settings.intelligence.nano.SettingsIntelligenceLogProto;
import com.android.settings.intelligence.overlay.FeatureFactory;
import com.android.settings.intelligence.search.sitemap.SiteMapPair;

import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Consumes the SearchIndexableProvider content providers.
 * Updates the Resource, Raw Data and non-indexable data for Search.
 *
 * TODO(b/33577327) this class needs to be refactored by moving most of its methods into controllers
 */
public class DatabaseIndexingManager {

    private static final String TAG = "DatabaseIndexingManager";

    @VisibleForTesting
    final AtomicBoolean mIsIndexingComplete = new AtomicBoolean(false);

    private PreIndexDataCollector mCollector;
    private IndexDataConverter mConverter;

    private Context mContext;

    public DatabaseIndexingManager(Context context) {
        mContext = context;
    }

    public boolean isIndexingComplete() {
        return mIsIndexingComplete.get();
    }

    public void indexDatabase(IndexingCallback callback) {
        IndexingTask task = new IndexingTask(callback);
        task.execute();
    }

    /**
     * Accumulate all data and non-indexable keys from each of the content-providers.
     * Only the first indexing for the default language gets static search results - subsequent
     * calls will only gather non-indexable keys.
     */
    public void performIndexing() {
        final Intent intent = new Intent(SearchIndexablesContract.PROVIDER_INTERFACE);
        final List<ResolveInfo> providers =
                mContext.getPackageManager().queryIntentContentProviders(intent, 0);

        final boolean isFullIndex = IndexDatabaseHelper.isFullIndex(mContext, providers);

        if (isFullIndex) {
            rebuildDatabase();
        }

        PreIndexData indexData = getIndexDataFromProviders(providers, isFullIndex);

        final long updateDatabaseStartTime = System.currentTimeMillis();
        updateDatabase(indexData, isFullIndex);
        IndexDatabaseHelper.setIndexed(mContext, providers);
        if (DEBUG) {
            final long updateDatabaseTime = System.currentTimeMillis() - updateDatabaseStartTime;
            Log.d(TAG, "performIndexing updateDatabase took time: " + updateDatabaseTime);
        }
    }

    @VisibleForTesting
    PreIndexData getIndexDataFromProviders(List<ResolveInfo> providers, boolean isFullIndex) {
        if (mCollector == null) {
            mCollector = new PreIndexDataCollector(mContext);
        }
        return mCollector.collectIndexableData(providers, isFullIndex);
    }

    /**
     * Drop the currently stored database, and clear the flags which mark the database as indexed.
     */
    private void rebuildDatabase() {
        // Drop the database when the locale or build has changed. This eliminates rows which are
        // dynamically inserted in the old language, or deprecated settings.
        final SQLiteDatabase db = getWritableDatabase();
        IndexDatabaseHelper.getInstance(mContext).reconstruct(db);
    }

    /**
     * Adds new data to the database and verifies the correctness of the ENABLED column.
     * First, the data to be updated and all non-indexable keys are copied locally.
     * Then all new data to be added is inserted.
     * Then search results are verified to have the correct value of enabled.
     * Finally, we record that the locale has been indexed.
     *
     * @param isFullIndex true the database needs to be rebuilt.
     */
    @VisibleForTesting
    void updateDatabase(PreIndexData preIndexData, boolean isFullIndex) {
        final Map<String, Set<String>> nonIndexableKeys = preIndexData.getNonIndexableKeys();

        final SQLiteDatabase database = getWritableDatabase();
        if (database == null) {
            Log.w(TAG, "Cannot indexDatabase Index as I cannot get a writable database");
            return;
        }

        try {
            database.beginTransaction();

            // Convert all Pre-index data to Index data and and insert to db.
            final List<IndexData> indexData = getIndexData(preIndexData);
            insertIndexData(database, indexData);
            insertSiteMapData(database, getSiteMapPairs(indexData, preIndexData.getSiteMapPairs()));

            // Only check for non-indexable key updates after initial index.
            // Enabled state with non-indexable keys is checked when items are first inserted.
            if (!isFullIndex) {
                updateDataInDatabase(database, nonIndexableKeys);
            }

            database.setTransactionSuccessful();
        } finally {
            database.endTransaction();
        }
    }

    private List<IndexData> getIndexData(PreIndexData data) {
        if (mConverter == null) {
            mConverter = new IndexDataConverter(mContext);
        }
        return mConverter.convertPreIndexDataToIndexData(data);
    }

    private List<SiteMapPair> getSiteMapPairs(List<IndexData> indexData,
            List<Pair<String, String>> siteMapClassNames) {
        if (mConverter == null) {
            mConverter = new IndexDataConverter(mContext);
        }
        return mConverter.convertSiteMapPairs(indexData, siteMapClassNames);
    }

    private void insertSiteMapData(SQLiteDatabase database, List<SiteMapPair> siteMapPairs) {
        if (siteMapPairs == null) {
            return;
        }
        for (SiteMapPair pair : siteMapPairs) {
            database.replaceOrThrow(IndexDatabaseHelper.Tables.TABLE_SITE_MAP,
                    null /* nullColumnHack */, pair.toContentValue());
        }
    }

    /**
     * Inserts all of the entries in {@param indexData} into the {@param database}
     * as Search Data and as part of the Information Hierarchy.
     */
    private void insertIndexData(SQLiteDatabase database, List<IndexData> indexData) {
        ContentValues values;

        for (IndexData dataRow : indexData) {
            if (TextUtils.isEmpty(dataRow.normalizedTitle)) {
                continue;
            }

            values = new ContentValues();
            values.put(DATA_TITLE, dataRow.updatedTitle);
            values.put(DATA_TITLE_NORMALIZED, dataRow.normalizedTitle);
            values.put(DATA_SUMMARY_ON, dataRow.updatedSummaryOn);
            values.put(DATA_SUMMARY_ON_NORMALIZED, dataRow.normalizedSummaryOn);
            values.put(DATA_ENTRIES, dataRow.entries);
            values.put(DATA_KEYWORDS, dataRow.spaceDelimitedKeywords);
            values.put(DATA_PACKAGE, dataRow.packageName);
            values.put(CLASS_NAME, dataRow.className);
            values.put(SCREEN_TITLE, dataRow.screenTitle);
            values.put(INTENT_ACTION, dataRow.intentAction);
            values.put(INTENT_TARGET_PACKAGE, dataRow.intentTargetPackage);
            values.put(INTENT_TARGET_CLASS, dataRow.intentTargetClass);
            values.put(ICON, dataRow.iconResId);
            values.put(ENABLED, dataRow.enabled);
            values.put(DATA_KEY_REF, dataRow.key);
            values.put(PAYLOAD_TYPE, dataRow.payloadType);
            values.put(PAYLOAD, dataRow.payload);

            database.replaceOrThrow(TABLE_PREFS_INDEX, null, values);
        }
    }

    /**
     * Upholds the validity of enabled data for the user.
     * All rows which are enabled but are now flagged with non-indexable keys will become disabled.
     * All rows which are disabled but no longer a non-indexable key will become enabled.
     *
     * @param database         The database to validate.
     * @param nonIndexableKeys A map between package name and the set of non-indexable keys for it.
     */
    @VisibleForTesting
    void updateDataInDatabase(SQLiteDatabase database,
            Map<String, Set<String>> nonIndexableKeys) {
        final String whereEnabled = ENABLED + " = 1";
        final String whereDisabled = ENABLED + " = 0";

        final Cursor enabledResults = database.query(TABLE_PREFS_INDEX, SELECT_COLUMNS,
                whereEnabled, null, null, null, null);

        final ContentValues enabledToDisabledValue = new ContentValues();
        enabledToDisabledValue.put(ENABLED, 0);

        String packageName;
        // TODO Refactor: Move these two loops into one method.
        while (enabledResults.moveToNext()) {
            packageName = enabledResults.getString(enabledResults.getColumnIndexOrThrow(
                    IndexDatabaseHelper.IndexColumns.DATA_PACKAGE));
            final String key = enabledResults.getString(enabledResults.getColumnIndexOrThrow(
                    IndexDatabaseHelper.IndexColumns.DATA_KEY_REF));
            final Set<String> packageKeys = nonIndexableKeys.get(packageName);

            // The indexed item is set to Enabled but is now non-indexable
            if (packageKeys != null && packageKeys.contains(key)) {
                final String whereClause = getKeyWhereClause(key);
                database.update(TABLE_PREFS_INDEX, enabledToDisabledValue, whereClause, null);
            }
        }
        enabledResults.close();

        final Cursor disabledResults = database.query(TABLE_PREFS_INDEX, SELECT_COLUMNS,
                whereDisabled, null, null, null, null);

        final ContentValues disabledToEnabledValue = new ContentValues();
        disabledToEnabledValue.put(ENABLED, 1);

        while (disabledResults.moveToNext()) {
            packageName = disabledResults.getString(disabledResults.getColumnIndexOrThrow(
                    IndexDatabaseHelper.IndexColumns.DATA_PACKAGE));

            final String key = disabledResults.getString(disabledResults.getColumnIndexOrThrow(
                    IndexDatabaseHelper.IndexColumns.DATA_KEY_REF));
            final Set<String> packageKeys = nonIndexableKeys.get(packageName);

            // The indexed item is set to Disabled but is no longer non-indexable.
            // We do not enable keys when packageKeys is null because it means the keys came
            // from an unrecognized package and therefore should not be surfaced as results.
            if (packageKeys != null && !packageKeys.contains(key)) {
                final String whereClause = getKeyWhereClause(key);
                database.update(TABLE_PREFS_INDEX, disabledToEnabledValue, whereClause, null);
            }
        }
        disabledResults.close();
    }

    private String getKeyWhereClause(String key) {
        return IndexDatabaseHelper.IndexColumns.DATA_KEY_REF + " = \"" + key + "\"";
    }

    private SQLiteDatabase getWritableDatabase() {
        try {
            return IndexDatabaseHelper.getInstance(mContext).getWritableDatabase();
        } catch (SQLiteException e) {
            Log.e(TAG, "Cannot open writable database", e);
            return null;
        }
    }

    public class IndexingTask extends AsyncTask<Void, Void, Void> {

        @VisibleForTesting
        IndexingCallback mCallback;
        private long mIndexStartTime;

        public IndexingTask(IndexingCallback callback) {
            mCallback = callback;
        }

        @Override
        protected void onPreExecute() {
            mIndexStartTime = System.currentTimeMillis();
            mIsIndexingComplete.set(false);
        }

        @Override
        protected Void doInBackground(Void... voids) {
            performIndexing();
            return null;
        }

        @Override
        protected void onPostExecute(Void aVoid) {
            int indexingTime = (int) (System.currentTimeMillis() - mIndexStartTime);
            FeatureFactory.get(mContext).metricsFeatureProvider(mContext).logEvent(
                    SettingsIntelligenceLogProto.SettingsIntelligenceEvent.INDEX_SEARCH,
                    indexingTime);

            mIsIndexingComplete.set(true);
            if (mCallback != null) {
                mCallback.onIndexingFinished();
            }
        }
    }
}