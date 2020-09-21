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
 *
 */

package com.android.settings.intelligence.search;

import android.content.Context;
import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;
import android.view.View;

import com.android.settings.intelligence.search.indexing.DatabaseIndexingManager;
import com.android.settings.intelligence.search.indexing.IndexData;
import com.android.settings.intelligence.search.indexing.IndexingCallback;
import com.android.settings.intelligence.search.query.AccessibilityServiceResultTask;
import com.android.settings.intelligence.search.query.DatabaseResultTask;
import com.android.settings.intelligence.search.query.InputDeviceResultTask;
import com.android.settings.intelligence.search.query.InstalledAppResultTask;
import com.android.settings.intelligence.search.query.SearchQueryTask;
import com.android.settings.intelligence.search.savedqueries.SavedQueryLoader;
import com.android.settings.intelligence.search.sitemap.SiteMapManager;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.FutureTask;

/**
 * FeatureProvider for the refactored search code.
 */
public class SearchFeatureProviderImpl implements SearchFeatureProvider {

    private static final String TAG = "SearchFeatureProvider";

    private DatabaseIndexingManager mDatabaseIndexingManager;
    private ExecutorService mExecutorService;
    private SiteMapManager mSiteMapManager;

    @Override
    public SearchResultLoader getSearchResultLoader(Context context, String query) {
        return new SearchResultLoader(context, cleanQuery(query));
    }

    @Override
    public List<SearchQueryTask> getSearchQueryTasks(Context context, String query) {
        final List<SearchQueryTask> tasks = new ArrayList<>();
        final String cleanQuery = cleanQuery(query);
        tasks.add(DatabaseResultTask.newTask(context, getSiteMapManager(), cleanQuery));
        tasks.add(InstalledAppResultTask.newTask(context, getSiteMapManager(), cleanQuery));
        tasks.add(AccessibilityServiceResultTask.newTask(context, getSiteMapManager(), cleanQuery));
        tasks.add(InputDeviceResultTask.newTask(context, getSiteMapManager(), cleanQuery));
        return tasks;
    }

    @Override
    public SavedQueryLoader getSavedQueryLoader(Context context) {
        return new SavedQueryLoader(context);
    }

    @Override
    public DatabaseIndexingManager getIndexingManager(Context context) {
        if (mDatabaseIndexingManager == null) {
            mDatabaseIndexingManager = new DatabaseIndexingManager(context.getApplicationContext());
        }
        return mDatabaseIndexingManager;
    }

    @Override
    public SiteMapManager getSiteMapManager() {
        if (mSiteMapManager == null) {
            mSiteMapManager = new SiteMapManager();
        }
        return mSiteMapManager;
    }

    @Override
    public boolean isIndexingComplete(Context context) {
        return getIndexingManager(context).isIndexingComplete();
    }

    @Override
    public void initFeedbackButton() {
    }

    @Override
    public void showFeedbackButton(SearchFragment fragment, View root) {
    }

    @Override
    public void hideFeedbackButton(View root) {
    }

    @Override
    public void searchResultClicked(Context context, String query, SearchResult searchResult) {
    }

    @Override
    public boolean isSmartSearchRankingEnabled(Context context) {
        return false;
    }

    @Override
    public long smartSearchRankingTimeoutMs(Context context) {
        return 300L;
    }

    @Override
    public void searchRankingWarmup(Context context) {
    }

    @Override
    public FutureTask<List<Pair<String, Float>>> getRankerTask(Context context, String query) {
        return null;
    }

    @Override
    public void updateIndexAsync(Context context, IndexingCallback callback) {
        if (DEBUG) {
            Log.d(TAG, "updating index async");
        }
        getIndexingManager(context).indexDatabase(callback);
    }

    @Override
    public ExecutorService getExecutorService() {
        if (mExecutorService == null) {
            mExecutorService = Executors.newCachedThreadPool();
        }
        return mExecutorService;
    }

    /**
     * A generic method to make the query suitable for searching the database.
     *
     * @return the cleaned query string
     */
    @VisibleForTesting
    String cleanQuery(String query) {
        if (TextUtils.isEmpty(query)) {
            return null;
        }
        if (Locale.getDefault().equals(Locale.JAPAN)) {
            query = IndexData.normalizeJapaneseString(query);
        }
        return query.trim();
    }
}
