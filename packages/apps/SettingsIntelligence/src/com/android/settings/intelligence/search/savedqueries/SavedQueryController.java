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

package com.android.settings.intelligence.search.savedqueries;

import android.app.LoaderManager;
import android.content.Context;
import android.content.Loader;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

import com.android.settings.intelligence.R;
import com.android.settings.intelligence.overlay.FeatureFactory;
import com.android.settings.intelligence.search.SearchFeatureProvider;
import com.android.settings.intelligence.search.SearchFragment;
import com.android.settings.intelligence.search.SearchResult;
import com.android.settings.intelligence.search.SearchResultsAdapter;

import java.util.List;

public class SavedQueryController implements LoaderManager.LoaderCallbacks,
        MenuItem.OnMenuItemClickListener {

    // TODO: make a generic background task manager to handle one-off tasks like this one.
    private static final String ARG_QUERY = "remove_query";
    private static final String TAG = "SearchSavedQueryCtrl";

    private static final int MENU_SEARCH_HISTORY = 1000;

    private final Context mContext;
    private final LoaderManager mLoaderManager;
    private final SearchFeatureProvider mSearchFeatureProvider;
    private final SearchResultsAdapter mResultAdapter;

    public SavedQueryController(Context context, LoaderManager loaderManager,
            SearchResultsAdapter resultsAdapter) {
        mContext = context;
        mLoaderManager = loaderManager;
        mResultAdapter = resultsAdapter;
        mSearchFeatureProvider = FeatureFactory.get(context)
                .searchFeatureProvider();
    }

    @Override
    public Loader onCreateLoader(int id, Bundle args) {
        switch (id) {
            case SearchFragment.SearchLoaderId.SAVE_QUERY_TASK:
                return new SavedQueryRecorder(mContext, args.getString(ARG_QUERY));
            case SearchFragment.SearchLoaderId.REMOVE_QUERY_TASK:
                return new SavedQueryRemover(mContext);
            case SearchFragment.SearchLoaderId.SAVED_QUERIES:
                return mSearchFeatureProvider.getSavedQueryLoader(mContext);
        }
        return null;
    }

    @Override
    public void onLoadFinished(Loader loader, Object data) {
        switch (loader.getId()) {
            case SearchFragment.SearchLoaderId.REMOVE_QUERY_TASK:
                mLoaderManager.restartLoader(SearchFragment.SearchLoaderId.SAVED_QUERIES,
                        null /* args */, this /* callback */);
                break;
            case SearchFragment.SearchLoaderId.SAVED_QUERIES:
                if (SearchFeatureProvider.DEBUG) {
                    Log.d(TAG, "Saved queries loaded");
                }
                mResultAdapter.displaySavedQuery((List<SearchResult>) data);
                break;
        }
    }

    @Override
    public void onLoaderReset(Loader loader) {
    }

    @Override
    public boolean onMenuItemClick(MenuItem item) {
        if (item.getItemId() != MENU_SEARCH_HISTORY) {
            return false;
        }
        removeQueries();
        return true;
    }

    public void buildMenuItem(Menu menu) {
        final MenuItem item =
                menu.add(Menu.NONE, MENU_SEARCH_HISTORY, Menu.NONE, R.string.search_clear_history);
        item.setOnMenuItemClickListener(this);
    }

    public void saveQuery(String query) {
        final Bundle args = new Bundle();
        args.putString(ARG_QUERY, query);
        mLoaderManager.restartLoader(SearchFragment.SearchLoaderId.SAVE_QUERY_TASK, args,
                this /* callback */);
    }

    /**
     * Remove all saved queries from DB
     */
    public void removeQueries() {
        final Bundle args = new Bundle();
        mLoaderManager.restartLoader(SearchFragment.SearchLoaderId.REMOVE_QUERY_TASK, args,
                this /* callback */);
    }

    public void loadSavedQueries() {
        if (SearchFeatureProvider.DEBUG) {
            Log.d(TAG, "loading saved queries");
        }
        mLoaderManager.restartLoader(SearchFragment.SearchLoaderId.SAVED_QUERIES, null /* args */,
                this /* callback */);
    }
}
