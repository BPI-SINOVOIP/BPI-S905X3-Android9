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

package com.android.settings.slices;

import android.content.ContentValues;
import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import com.android.settings.dashboard.DashboardFragment;

import com.android.settings.core.BasePreferenceController;
import com.android.settings.overlay.FeatureFactory;
import com.android.settings.slices.SlicesDatabaseHelper.IndexColumns;
import com.android.settings.slices.SlicesDatabaseHelper.Tables;

import java.util.List;

/**
 * Manages the conversion of {@link DashboardFragment} and {@link BasePreferenceController} to
 * indexable data {@link SliceData} to be stored for Slices.
 */
class SlicesIndexer implements Runnable {

    private static final String TAG = "SlicesIndexer";

    private Context mContext;

    private SlicesDatabaseHelper mHelper;

    public SlicesIndexer(Context context) {
        mContext = context;
        mHelper = SlicesDatabaseHelper.getInstance(mContext);
    }

    /**
     * Asynchronously index slice data from {@link #indexSliceData()}.
     */
    @Override
    public void run() {
        indexSliceData();
    }

    /**
     * Synchronously takes data obtained from {@link SliceDataConverter} and indexes it into a
     * SQLite database
     */
    protected void indexSliceData() {
        if (mHelper.isSliceDataIndexed()) {
            Log.d(TAG, "Slices already indexed - returning.");
            return;
        }

        SQLiteDatabase database = mHelper.getWritableDatabase();

        try {
            long startTime = System.currentTimeMillis();
            database.beginTransaction();

            mHelper.reconstruct(mHelper.getWritableDatabase());
            List<SliceData> indexData = getSliceData();
            insertSliceData(database, indexData);

            mHelper.setIndexedState();

            // TODO (b/71503044) Log indexing time.
            Log.d(TAG,
                    "Indexing slices database took: " + (System.currentTimeMillis() - startTime));
            database.setTransactionSuccessful();
        } finally {
            database.endTransaction();
        }
    }

    @VisibleForTesting
    List<SliceData> getSliceData() {
        return FeatureFactory.getFactory(mContext)
                .getSlicesFeatureProvider()
                .getSliceDataConverter(mContext)
                .getSliceData();
    }

    @VisibleForTesting
    void insertSliceData(SQLiteDatabase database, List<SliceData> indexData) {
        ContentValues values;

        for (SliceData dataRow : indexData) {
            values = new ContentValues();
            values.put(IndexColumns.KEY, dataRow.getKey());
            values.put(IndexColumns.TITLE, dataRow.getTitle());
            values.put(IndexColumns.SUMMARY, dataRow.getSummary());
            values.put(IndexColumns.SCREENTITLE, dataRow.getScreenTitle().toString());
            values.put(IndexColumns.KEYWORDS, dataRow.getKeywords());
            values.put(IndexColumns.ICON_RESOURCE, dataRow.getIconResource());
            values.put(IndexColumns.FRAGMENT, dataRow.getFragmentClassName());
            values.put(IndexColumns.CONTROLLER, dataRow.getPreferenceController());
            values.put(IndexColumns.PLATFORM_SLICE, dataRow.isPlatformDefined());
            values.put(IndexColumns.SLICE_TYPE, dataRow.getSliceType());

            database.replaceOrThrow(Tables.TABLE_SLICES_INDEX, null /* nullColumnHack */,
                    values);
        }
    }
}