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

import static com.android.settings.intelligence.nano.SettingsIntelligenceLogProto.SettingsIntelligenceEvent;

import android.app.Activity;
import android.app.Fragment;
import android.app.LoaderManager;
import android.content.Context;
import android.content.Loader;
import android.os.Bundle;
import android.support.annotation.VisibleForTesting;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.util.EventLog;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.LinearLayout;
import android.widget.SearchView;
import android.widget.Toolbar;

import com.android.settings.intelligence.R;
import com.android.settings.intelligence.instrumentation.MetricsFeatureProvider;
import com.android.settings.intelligence.overlay.FeatureFactory;
import com.android.settings.intelligence.search.indexing.IndexingCallback;
import com.android.settings.intelligence.search.savedqueries.SavedQueryController;
import com.android.settings.intelligence.search.savedqueries.SavedQueryViewHolder;

import java.util.List;

/**
 * This fragment manages the lifecycle of indexing and searching.
 *
 * In onCreate, the indexing process is initiated in DatabaseIndexingManager.
 * While the indexing is happening, loaders are blocked from accessing the database, but the user
 * is free to start typing their query.
 *
 * When the indexing is complete, the fragment gets a callback to initialize the loaders and search
 * the query if the user has entered text.
 */
public class SearchFragment extends Fragment implements SearchView.OnQueryTextListener,
        LoaderManager.LoaderCallbacks<List<? extends SearchResult>>, IndexingCallback {
    private static final String TAG = "SearchFragment";

    // State values
    private static final String STATE_QUERY = "state_query";
    private static final String STATE_SHOWING_SAVED_QUERY = "state_showing_saved_query";
    private static final String STATE_NEVER_ENTERED_QUERY = "state_never_entered_query";

    public static final class SearchLoaderId {
        // Search Query IDs
        public static final int SEARCH_RESULT = 1;

        // Saved Query IDs
        public static final int SAVE_QUERY_TASK = 2;
        public static final int REMOVE_QUERY_TASK = 3;
        public static final int SAVED_QUERIES = 4;
    }

    @VisibleForTesting
    String mQuery;

    private boolean mNeverEnteredQuery = true;
    private long mEnterQueryTimestampMs;

    @VisibleForTesting
    boolean mShowingSavedQuery;
    private MetricsFeatureProvider mMetricsFeatureProvider;
    @VisibleForTesting
    SavedQueryController mSavedQueryController;

    @VisibleForTesting
    SearchFeatureProvider mSearchFeatureProvider;

    @VisibleForTesting
    SearchResultsAdapter mSearchAdapter;

    @VisibleForTesting
    RecyclerView mResultsRecyclerView;
    @VisibleForTesting
    SearchView mSearchView;
    @VisibleForTesting
    LinearLayout mNoResultsView;

    @VisibleForTesting
    final RecyclerView.OnScrollListener mScrollListener = new RecyclerView.OnScrollListener() {
        @Override
        public void onScrolled(RecyclerView recyclerView, int dx, int dy) {
            if (dy != 0) {
                hideKeyboard();
            }
        }
    };

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mSearchFeatureProvider = FeatureFactory.get(context).searchFeatureProvider();
        mMetricsFeatureProvider = FeatureFactory.get(context).metricsFeatureProvider(context);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        long startTime = System.currentTimeMillis();
        setHasOptionsMenu(true);

        final LoaderManager loaderManager = getLoaderManager();
        mSearchAdapter = new SearchResultsAdapter(this /* fragment */);
        mSavedQueryController = new SavedQueryController(
                getContext(), loaderManager, mSearchAdapter);
        mSearchFeatureProvider.initFeedbackButton();

        if (savedInstanceState != null) {
            mQuery = savedInstanceState.getString(STATE_QUERY);
            mNeverEnteredQuery = savedInstanceState.getBoolean(STATE_NEVER_ENTERED_QUERY);
            mShowingSavedQuery = savedInstanceState.getBoolean(STATE_SHOWING_SAVED_QUERY);
        } else {
            mShowingSavedQuery = true;
        }
        mSearchFeatureProvider.updateIndexAsync(getContext(), this /* indexingCallback */);
        if (SearchFeatureProvider.DEBUG) {
            Log.d(TAG, "onCreate spent " + (System.currentTimeMillis() - startTime) + " ms");
        }
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);
        mSavedQueryController.buildMenuItem(menu);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        final Activity activity = getActivity();
        final View view = inflater.inflate(R.layout.search_panel, container, false);
        mResultsRecyclerView = view.findViewById(R.id.list_results);
        mResultsRecyclerView.setAdapter(mSearchAdapter);
        mResultsRecyclerView.setLayoutManager(new LinearLayoutManager(activity));
        mResultsRecyclerView.addOnScrollListener(mScrollListener);

        mNoResultsView = view.findViewById(R.id.no_results_layout);

        final Toolbar toolbar = view.findViewById(R.id.search_toolbar);
        activity.setActionBar(toolbar);
        activity.getActionBar().setDisplayHomeAsUpEnabled(true);

        mSearchView = toolbar.findViewById(R.id.search_view);
        mSearchView.setQuery(mQuery, false /* submitQuery */);
        mSearchView.setOnQueryTextListener(this);
        mSearchView.requestFocus();

        return view;
    }

    @Override
    public void onStart() {
        super.onStart();
        mMetricsFeatureProvider.logEvent(SettingsIntelligenceEvent.OPEN_SEARCH_PAGE);
    }

    @Override
    public void onResume() {
        super.onResume();
        Context appContext = getContext().getApplicationContext();
        if (mSearchFeatureProvider.isSmartSearchRankingEnabled(appContext)) {
            mSearchFeatureProvider.searchRankingWarmup(appContext);
        }
        requery();
    }

    @Override
    public void onStop() {
        super.onStop();
        mMetricsFeatureProvider.logEvent(SettingsIntelligenceEvent.LEAVE_SEARCH_PAGE);
        final Activity activity = getActivity();
        if (activity != null && activity.isFinishing()) {
            if (mNeverEnteredQuery) {
                mMetricsFeatureProvider.logEvent(
                        SettingsIntelligenceEvent.LEAVE_SEARCH_WITHOUT_QUERY);
            }
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putString(STATE_QUERY, mQuery);
        outState.putBoolean(STATE_NEVER_ENTERED_QUERY, mNeverEnteredQuery);
        outState.putBoolean(STATE_SHOWING_SAVED_QUERY, mShowingSavedQuery);
    }

    @Override
    public boolean onQueryTextChange(String query) {
        if (TextUtils.equals(query, mQuery)) {
            return true;
        }
        mEnterQueryTimestampMs = System.currentTimeMillis();
        final boolean isEmptyQuery = TextUtils.isEmpty(query);

        // Hide no-results-view when the new query is not a super-string of the previous
        if (mQuery != null
                && mNoResultsView.getVisibility() == View.VISIBLE
                && query.length() < mQuery.length()) {
            mNoResultsView.setVisibility(View.GONE);
        }

        mNeverEnteredQuery = false;
        mQuery = query;

        // If indexing is not finished, register the query text, but don't search.
        if (!mSearchFeatureProvider.isIndexingComplete(getActivity())) {
            return true;
        }

        if (isEmptyQuery) {
            final LoaderManager loaderManager = getLoaderManager();
            loaderManager.destroyLoader(SearchLoaderId.SEARCH_RESULT);
            mShowingSavedQuery = true;
            mSavedQueryController.loadSavedQueries();
            mSearchFeatureProvider.hideFeedbackButton(getView());
        } else {
            mMetricsFeatureProvider.logEvent(SettingsIntelligenceEvent.PERFORM_SEARCH);
            restartLoaders();
        }

        return true;
    }

    @Override
    public boolean onQueryTextSubmit(String query) {
        // Save submitted query.
        mSavedQueryController.saveQuery(mQuery);
        hideKeyboard();
        return true;
    }

    @Override
    public Loader<List<? extends SearchResult>> onCreateLoader(int id, Bundle args) {
        final Activity activity = getActivity();

        switch (id) {
            case SearchLoaderId.SEARCH_RESULT:
                return mSearchFeatureProvider.getSearchResultLoader(activity, mQuery);
            default:
                return null;
        }
    }

    @Override
    public void onLoadFinished(Loader<List<? extends SearchResult>> loader,
            List<? extends SearchResult> data) {
        mSearchAdapter.postSearchResults(data);
    }

    @Override
    public void onLoaderReset(Loader<List<? extends SearchResult>> loader) {
    }

    /**
     * Gets called when Indexing is completed.
     */
    @Override
    public void onIndexingFinished() {
        if (getActivity() == null) {
            return;
        }
        if (mShowingSavedQuery) {
            mSavedQueryController.loadSavedQueries();
        } else {
            final LoaderManager loaderManager = getLoaderManager();
            loaderManager.initLoader(SearchLoaderId.SEARCH_RESULT, null /* args */,
                    this /* callback */);
        }

        requery();
    }

    public List<SearchResult> getSearchResults() {
        return mSearchAdapter.getSearchResults();
    }

    public void onSearchResultClicked(SearchViewHolder resultViewHolder, SearchResult result) {
        logSearchResultClicked(resultViewHolder, result);
        mSearchFeatureProvider.searchResultClicked(getContext(), mQuery, result);
        mSavedQueryController.saveQuery(mQuery);
    }

    public void onSearchResultsDisplayed(int resultCount) {
        final long queryToResultLatencyMs = mEnterQueryTimestampMs > 0
                ? System.currentTimeMillis() - mEnterQueryTimestampMs
                : 0;
        if (resultCount == 0) {
            mNoResultsView.setVisibility(View.VISIBLE);
            mMetricsFeatureProvider.logEvent(SettingsIntelligenceEvent.SHOW_SEARCH_NO_RESULT,
                    queryToResultLatencyMs);
            EventLog.writeEvent(90204 /* settings_latency*/, 1 /* query_to_result_latency */,
                    (int) queryToResultLatencyMs);
        } else {
            mNoResultsView.setVisibility(View.GONE);
            mResultsRecyclerView.scrollToPosition(0);
            mMetricsFeatureProvider.logEvent(SettingsIntelligenceEvent.SHOW_SEARCH_RESULT,
                    queryToResultLatencyMs);
        }
        mSearchFeatureProvider.showFeedbackButton(this, getView());
    }

    public void onSavedQueryClicked(SavedQueryViewHolder vh, CharSequence query) {
        final String queryString = query.toString();
        mMetricsFeatureProvider.logEvent(vh.getClickActionMetricName());
        mSearchView.setQuery(queryString, false /* submit */);
        onQueryTextChange(queryString);
    }

    private void restartLoaders() {
        mShowingSavedQuery = false;
        final LoaderManager loaderManager = getLoaderManager();
        loaderManager.restartLoader(
                SearchLoaderId.SEARCH_RESULT, null /* args */, this /* callback */);
    }

    public String getQuery() {
        return mQuery;
    }

    private void requery() {
        if (TextUtils.isEmpty(mQuery)) {
            return;
        }
        final String query = mQuery;
        mQuery = "";
        onQueryTextChange(query);
    }

    private void hideKeyboard() {
        final Activity activity = getActivity();
        if (activity != null) {
            View view = activity.getCurrentFocus();
            InputMethodManager imm = (InputMethodManager)
                    activity.getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }

        if (mResultsRecyclerView != null) {
            mResultsRecyclerView.requestFocus();
        }
    }

    private void logSearchResultClicked(SearchViewHolder resultViewHolder, SearchResult result) {
        final int resultType = resultViewHolder.getClickActionMetricName();
        final int resultCount = mSearchAdapter.getItemCount();
        final int resultRank = resultViewHolder.getAdapterPosition();
        mMetricsFeatureProvider.logSearchResultClick(result, mQuery, resultType, resultCount,
                resultRank);
    }
}
