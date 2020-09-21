/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tv.search;

import android.app.SearchManager;
import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import android.util.Log;
import com.android.tv.TvSingletons;
import com.android.tv.common.CommonConstants;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.util.CommonUtils;
import com.android.tv.common.util.PermissionUtils;
import com.android.tv.perf.EventNames;
import com.android.tv.perf.PerformanceMonitor;
import com.android.tv.perf.TimerEvent;
import com.android.tv.util.TvUriMatcher;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class LocalSearchProvider extends ContentProvider {
    private static final String TAG = "LocalSearchProvider";
    private static final boolean DEBUG = false;

    /** The authority for LocalSearchProvider. */
    public static final String AUTHORITY = CommonConstants.BASE_PACKAGE + ".search";

    public static final int PROGRESS_PERCENTAGE_HIDE = -1;

    // TODO: Remove this once added to the SearchManager.
    private static final String SUGGEST_COLUMN_PROGRESS_BAR_PERCENTAGE = "progress_bar_percentage";

    private static final String[] SEARCHABLE_COLUMNS =
            new String[] {
                SearchManager.SUGGEST_COLUMN_TEXT_1,
                SearchManager.SUGGEST_COLUMN_TEXT_2,
                SearchManager.SUGGEST_COLUMN_RESULT_CARD_IMAGE,
                SearchManager.SUGGEST_COLUMN_INTENT_ACTION,
                SearchManager.SUGGEST_COLUMN_INTENT_DATA,
                SearchManager.SUGGEST_COLUMN_INTENT_EXTRA_DATA,
                SearchManager.SUGGEST_COLUMN_CONTENT_TYPE,
                SearchManager.SUGGEST_COLUMN_IS_LIVE,
                SearchManager.SUGGEST_COLUMN_VIDEO_WIDTH,
                SearchManager.SUGGEST_COLUMN_VIDEO_HEIGHT,
                SearchManager.SUGGEST_COLUMN_DURATION,
                SUGGEST_COLUMN_PROGRESS_BAR_PERCENTAGE
            };

    private static final String EXPECTED_PATH_PREFIX = "/" + SearchManager.SUGGEST_URI_PATH_QUERY;
    static final String SUGGEST_PARAMETER_ACTION = "action";
    // The launcher passes 10 as a 'limit' parameter by default.
    @VisibleForTesting static final int DEFAULT_SEARCH_LIMIT = 10;

    @VisibleForTesting
    static final int DEFAULT_SEARCH_ACTION = SearchInterface.ACTION_TYPE_AMBIGUOUS;

    private static final String NO_LIVE_CONTENTS = "0";
    private static final String LIVE_CONTENTS = "1";

    private PerformanceMonitor mPerformanceMonitor;

    /** Used only for testing */
    private SearchInterface mSearchInterface;

    @Override
    public boolean onCreate() {
        mPerformanceMonitor = TvSingletons.getSingletons(getContext()).getPerformanceMonitor();
        return true;
    }

    @VisibleForTesting
    void setSearchInterface(SearchInterface searchInterface) {
        SoftPreconditions.checkState(CommonUtils.isRunningInTest());
        mSearchInterface = searchInterface;
    }

    @Override
    public Cursor query(
            @NonNull Uri uri,
            String[] projection,
            String selection,
            String[] selectionArgs,
            String sortOrder) {
        if (TvUriMatcher.match(uri) != TvUriMatcher.MATCH_ON_DEVICE_SEARCH) {
            throw new IllegalArgumentException("Unknown URI: " + uri);
        }
        TimerEvent queryTimer = mPerformanceMonitor.startTimer();
        if (DEBUG) {
            Log.d(
                    TAG,
                    "query("
                            + uri
                            + ", "
                            + Arrays.toString(projection)
                            + ", "
                            + selection
                            + ", "
                            + Arrays.toString(selectionArgs)
                            + ", "
                            + sortOrder
                            + ")");
        }
        long time = SystemClock.elapsedRealtime();
        SearchInterface search = mSearchInterface;
        if (search == null) {
            if (PermissionUtils.hasAccessAllEpg(getContext())) {
                if (DEBUG) Log.d(TAG, "Performing TV Provider search.");
                search = new TvProviderSearch(getContext());
            } else {
                if (DEBUG) Log.d(TAG, "Performing Data Manager search.");
                search = new DataManagerSearch(getContext());
            }
        }
        String query = uri.getLastPathSegment();
        int limit =
                getQueryParamater(uri, SearchManager.SUGGEST_PARAMETER_LIMIT, DEFAULT_SEARCH_LIMIT);
        if (limit <= 0) {
            limit = DEFAULT_SEARCH_LIMIT;
        }
        int action = getQueryParamater(uri, SUGGEST_PARAMETER_ACTION, DEFAULT_SEARCH_ACTION);
        if (action < SearchInterface.ACTION_TYPE_START
                || action > SearchInterface.ACTION_TYPE_END) {
            action = DEFAULT_SEARCH_ACTION;
        }
        List<SearchResult> results = new ArrayList<>();
        if (!TextUtils.isEmpty(query)) {
            results.addAll(search.search(query, limit, action));
        }
        Cursor c = createSuggestionsCursor(results);
        if (DEBUG) {
            Log.d(
                    TAG,
                    "Elapsed time(count="
                            + c.getCount()
                            + "): "
                            + (SystemClock.elapsedRealtime() - time)
                            + "(msec)");
        }
        mPerformanceMonitor.stopTimer(queryTimer, EventNames.ON_DEVICE_SEARCH);
        return c;
    }

    private int getQueryParamater(Uri uri, String key, int defaultValue) {
        try {
            return Integer.parseInt(uri.getQueryParameter(key));
        } catch (NumberFormatException | UnsupportedOperationException e) {
            // Ignore the exceptions
        }
        return defaultValue;
    }

    private Cursor createSuggestionsCursor(List<SearchResult> results) {
        MatrixCursor cursor = new MatrixCursor(SEARCHABLE_COLUMNS, results.size());
        List<String> row = new ArrayList<>(SEARCHABLE_COLUMNS.length);

        int index = 0;
        for (SearchResult result : results) {
            row.clear();
            row.add(result.getTitle());
            row.add(result.getDescription());
            row.add(result.getImageUri());
            row.add(result.getIntentAction());
            row.add(result.getIntentData());
            row.add(result.getIntentExtraData());
            row.add(result.getContentType());
            row.add(result.getIsLive() ? LIVE_CONTENTS : NO_LIVE_CONTENTS);
            row.add(result.getVideoWidth() == 0 ? null : String.valueOf(result.getVideoWidth()));
            row.add(result.getVideoHeight() == 0 ? null : String.valueOf(result.getVideoHeight()));
            row.add(result.getDuration() == 0 ? null : String.valueOf(result.getDuration()));
            row.add(String.valueOf(result.getProgressPercentage()));
            cursor.addRow(row);
            if (DEBUG) Log.d(TAG, "Result[" + (++index) + "]: " + result);
        }
        return cursor;
    }

    @Override
    public String getType(Uri uri) {
        if (!checkUriCorrect(uri)) return null;
        return SearchManager.SUGGEST_MIME_TYPE;
    }

    private static boolean checkUriCorrect(Uri uri) {
        return uri != null && uri.getPath().startsWith(EXPECTED_PATH_PREFIX);
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    /** A placeholder to a search result. */
    // TODO(b/72052568): Get autovalue to work in aosp master
    public abstract static class SearchResult {
        public static Builder builder() {
            // primitive fields cannot be nullable. Set to default;
            return new AutoValue_LocalSearchProvider_SearchResult.Builder()
                    .setChannelId(0)
                    .setIsLive(false)
                    .setVideoWidth(0)
                    .setVideoHeight(0)
                    .setDuration(0)
                    .setProgressPercentage(0);
        }

        // TODO(b/72052568): Get autovalue to work in aosp master
        abstract static class Builder {
            abstract Builder setChannelId(long value);

            abstract Builder setChannelNumber(String value);

            abstract Builder setTitle(String value);

            abstract Builder setDescription(String value);

            abstract Builder setImageUri(String value);

            abstract Builder setIntentAction(String value);

            abstract Builder setIntentData(String value);

            abstract Builder setIntentExtraData(String value);

            abstract Builder setContentType(String value);

            abstract Builder setIsLive(boolean value);

            abstract Builder setVideoWidth(int value);

            abstract Builder setVideoHeight(int value);

            abstract Builder setDuration(long value);

            abstract Builder setProgressPercentage(int value);

            abstract SearchResult build();
        }

        abstract long getChannelId();

        @Nullable
        abstract String getChannelNumber();

        @Nullable
        abstract String getTitle();

        @Nullable
        abstract String getDescription();

        @Nullable
        abstract String getImageUri();

        @Nullable
        abstract String getIntentAction();

        @Nullable
        abstract String getIntentData();

        @Nullable
        abstract String getIntentExtraData();

        @Nullable
        abstract String getContentType();

        abstract boolean getIsLive();

        abstract int getVideoWidth();

        abstract int getVideoHeight();

        abstract long getDuration();

        abstract int getProgressPercentage();
    }
}
