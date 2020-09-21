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

package com.android.settings.intelligence.search.query;

import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.IndexColumns;
import static com.android.settings.intelligence.search.indexing.IndexDatabaseHelper.Tables
        .TABLE_PREFS_INDEX;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.support.annotation.VisibleForTesting;
import android.util.Log;
import android.util.Pair;

import com.android.settings.intelligence.nano.SettingsIntelligenceLogProto;
import com.android.settings.intelligence.overlay.FeatureFactory;
import com.android.settings.intelligence.search.SearchFeatureProvider;
import com.android.settings.intelligence.search.SearchResult;
import com.android.settings.intelligence.search.indexing.IndexDatabaseHelper;
import com.android.settings.intelligence.search.sitemap.SiteMapManager;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * AsyncTask to retrieve Settings, first party app and any intent based results.
 */
public class DatabaseResultTask extends SearchQueryTask.QueryWorker {

    private static final String TAG = "DatabaseResultTask";

    public static final String[] SELECT_COLUMNS = {
            IndexColumns.DATA_TITLE,
            IndexColumns.DATA_SUMMARY_ON,
            IndexColumns.DATA_SUMMARY_OFF,
            IndexColumns.CLASS_NAME,
            IndexColumns.SCREEN_TITLE,
            IndexColumns.ICON,
            IndexColumns.INTENT_ACTION,
            IndexColumns.DATA_PACKAGE,
            IndexColumns.INTENT_TARGET_PACKAGE,
            IndexColumns.INTENT_TARGET_CLASS,
            IndexColumns.DATA_KEY_REF,
            IndexColumns.PAYLOAD_TYPE,
            IndexColumns.PAYLOAD
    };

    public static final String[] MATCH_COLUMNS_PRIMARY = {
            IndexColumns.DATA_TITLE,
            IndexColumns.DATA_TITLE_NORMALIZED,
    };

    public static final String[] MATCH_COLUMNS_SECONDARY = {
            IndexColumns.DATA_SUMMARY_ON,
            IndexColumns.DATA_SUMMARY_ON_NORMALIZED,
            IndexColumns.DATA_SUMMARY_OFF,
            IndexColumns.DATA_SUMMARY_OFF_NORMALIZED,
    };

    public static final int QUERY_WORKER_ID =
            SettingsIntelligenceLogProto.SettingsIntelligenceEvent.SEARCH_QUERY_DATABASE;

    /**
     * Base ranks defines the best possible rank based on what the query matches.
     * If the query matches the prefix of the first word in the title, the best rank it can be
     * is 1
     * If the query matches the prefix of the other words in the title, the best rank it can be
     * is 3
     * If the query only matches the summary, the best rank it can be is 7
     * If the query only matches keywords or entries, the best rank it can be is 9
     */
    static final int[] BASE_RANKS = {1, 3, 7, 9};

    public static SearchQueryTask newTask(Context context, SiteMapManager siteMapManager,
            String query) {
        return new SearchQueryTask(new DatabaseResultTask(context, siteMapManager, query));
    }

    public final String[] MATCH_COLUMNS_TERTIARY = {
            IndexColumns.DATA_KEYWORDS,
            IndexColumns.DATA_ENTRIES
    };

    private final CursorToSearchResultConverter mConverter;
    private final SearchFeatureProvider mFeatureProvider;

    public DatabaseResultTask(Context context, SiteMapManager siteMapManager, String queryText) {
        super(context, siteMapManager, queryText);
        mConverter = new CursorToSearchResultConverter(context);
        mFeatureProvider = FeatureFactory.get(context).searchFeatureProvider();
    }

    @Override
    protected int getQueryWorkerId() {
        return QUERY_WORKER_ID;
    }

    @Override
    protected List<? extends SearchResult> query() {
        if (mQuery == null || mQuery.isEmpty()) {
            return new ArrayList<>();
        }
        // Start a Future to get search result scores.
        FutureTask<List<Pair<String, Float>>> rankerTask = mFeatureProvider.getRankerTask(
                mContext, mQuery);

        if (rankerTask != null) {
            ExecutorService executorService = mFeatureProvider.getExecutorService();
            executorService.execute(rankerTask);
        }

        final Set<SearchResult> resultSet = new HashSet<>();

        resultSet.addAll(firstWordQuery(MATCH_COLUMNS_PRIMARY, BASE_RANKS[0]));
        resultSet.addAll(secondaryWordQuery(MATCH_COLUMNS_PRIMARY, BASE_RANKS[1]));
        resultSet.addAll(anyWordQuery(MATCH_COLUMNS_SECONDARY, BASE_RANKS[2]));
        resultSet.addAll(anyWordQuery(MATCH_COLUMNS_TERTIARY, BASE_RANKS[3]));

        // Try to retrieve the scores in time. Otherwise use static ranking.
        if (rankerTask != null) {
            try {
                final long timeoutMs = mFeatureProvider.smartSearchRankingTimeoutMs(mContext);
                List<Pair<String, Float>> searchRankScores = rankerTask.get(timeoutMs,
                        TimeUnit.MILLISECONDS);
                return getDynamicRankedResults(resultSet, searchRankScores);
            } catch (TimeoutException | InterruptedException | ExecutionException e) {
                Log.d(TAG, "Error waiting for result scores: " + e);
            }
        }

        List<SearchResult> resultList = new ArrayList<>(resultSet);
        Collections.sort(resultList);
        return resultList;
    }

    // TODO (b/33577327) Retrieve all search results with a single query.

    /**
     * Creates and executes the query which matches prefixes of the first word of the given
     * columns.
     *
     * @param matchColumns The columns to match on
     * @param baseRank     The highest rank achievable by these results
     * @return A set of the matching results.
     */
    private Set<SearchResult> firstWordQuery(String[] matchColumns, int baseRank) {
        final String whereClause = buildSingleWordWhereClause(matchColumns);
        final String query = mQuery + "%";
        final String[] selection = buildSingleWordSelection(query, matchColumns.length);

        return query(whereClause, selection, baseRank);
    }

    /**
     * Creates and executes the query which matches prefixes of the non-first words of the
     * given columns.
     *
     * @param matchColumns The columns to match on
     * @param baseRank     The highest rank achievable by these results
     * @return A set of the matching results.
     */
    private Set<SearchResult> secondaryWordQuery(String[] matchColumns, int baseRank) {
        final String whereClause = buildSingleWordWhereClause(matchColumns);
        final String query = "% " + mQuery + "%";
        final String[] selection = buildSingleWordSelection(query, matchColumns.length);

        return query(whereClause, selection, baseRank);
    }

    /**
     * Creates and executes the query which matches prefixes of the any word of the given
     * columns.
     *
     * @param matchColumns The columns to match on
     * @param baseRank     The highest rank achievable by these results
     * @return A set of the matching results.
     */
    private Set<SearchResult> anyWordQuery(String[] matchColumns, int baseRank) {
        final String whereClause = buildTwoWordWhereClause(matchColumns);
        final String[] selection = buildAnyWordSelection(matchColumns.length * 2);

        return query(whereClause, selection, baseRank);
    }

    /**
     * Generic method used by all of the query methods above to execute a query.
     *
     * @param whereClause Where clause for the SQL query which uses bindings.
     * @param selection   List of the transformed query to match each bind in the whereClause
     * @param baseRank    The highest rank achievable by these results.
     * @return A set of the matching results.
     */
    private Set<SearchResult> query(String whereClause, String[] selection, int baseRank) {
        final SQLiteDatabase database =
                IndexDatabaseHelper.getInstance(mContext).getReadableDatabase();
        try (Cursor resultCursor = database.query(TABLE_PREFS_INDEX, SELECT_COLUMNS,
                whereClause,
                selection, null, null, null)) {
            return mConverter.convertCursor(resultCursor, baseRank, mSiteMapManager);
        }
    }

    /**
     * Builds the SQLite WHERE clause that matches all matchColumns for a single query.
     *
     * @param matchColumns List of columns that will be used for matching.
     * @return The constructed WHERE clause.
     */
    private static String buildSingleWordWhereClause(String[] matchColumns) {
        StringBuilder sb = new StringBuilder(" (");
        final int count = matchColumns.length;
        for (int n = 0; n < count; n++) {
            sb.append(matchColumns[n]);
            sb.append(" like ? ");
            if (n < count - 1) {
                sb.append(" OR ");
            }
        }
        sb.append(") AND enabled = 1");
        return sb.toString();
    }

    /**
     * Builds the SQLite WHERE clause that matches all matchColumns to two different queries.
     *
     * @param matchColumns List of columns that will be used for matching.
     * @return The constructed WHERE clause.
     */
    private static String buildTwoWordWhereClause(String[] matchColumns) {
        StringBuilder sb = new StringBuilder(" (");
        final int count = matchColumns.length;
        for (int n = 0; n < count; n++) {
            sb.append(matchColumns[n]);
            sb.append(" like ? OR ");
            sb.append(matchColumns[n]);
            sb.append(" like ?");
            if (n < count - 1) {
                sb.append(" OR ");
            }
        }
        sb.append(") AND enabled = 1");
        return sb.toString();
    }

    /**
     * Fills out the selection array to match the query as the prefix of a single word.
     *
     * @param size is the number of columns to be matched.
     */
    private String[] buildSingleWordSelection(String query, int size) {
        String[] selection = new String[size];

        for (int i = 0; i < size; i++) {
            selection[i] = query;
        }
        return selection;
    }

    /**
     * Fills out the selection array to match the query as the prefix of a word.
     *
     * @param size is twice the number of columns to be matched. The first match is for the
     *             prefix
     *             of the first word in the column. The second match is for any subsequent word
     *             prefix match.
     */
    private String[] buildAnyWordSelection(int size) {
        String[] selection = new String[size];
        final String query = mQuery + "%";
        final String subStringQuery = "% " + mQuery + "%";

        for (int i = 0; i < (size - 1); i += 2) {
            selection[i] = query;
            selection[i + 1] = subStringQuery;
        }
        return selection;
    }

    private List<SearchResult> getDynamicRankedResults(Set<SearchResult> unsortedSet,
            final List<Pair<String, Float>> searchRankScores) {
        final TreeSet<SearchResult> dbResultsSortedByScores = new TreeSet<>(
                new Comparator<SearchResult>() {
                    @Override
                    public int compare(SearchResult o1, SearchResult o2) {
                        final float score1 = getRankingScoreByKey(searchRankScores, o1.dataKey);
                        final float score2 = getRankingScoreByKey(searchRankScores, o2.dataKey);
                        if (score1 > score2) {
                            return -1;
                        } else {
                            return 1;
                        }
                    }
                });
        dbResultsSortedByScores.addAll(unsortedSet);

        return new ArrayList<>(dbResultsSortedByScores);
    }

    /**
     * Looks up ranking score by key.
     *
     * @param key key for a single search result.
     * @return the ranking score corresponding to the given stableId. If there is no score
     * available for this stableId, -Float.MAX_VALUE is returned.
     */
    @VisibleForTesting
    Float getRankingScoreByKey(List<Pair<String, Float>> searchRankScores, String key) {
        for (Pair<String, Float> rankingScore : searchRankScores) {
            if (key.compareTo(rankingScore.first) == 0) {
                return rankingScore.second;
            }
        }
        // If key not found in the list, we assign the minimum score so it will appear at
        // the end of the list.
        Log.w(TAG, key + " was not in the ranking scores.");
        return -Float.MAX_VALUE;
    }
}