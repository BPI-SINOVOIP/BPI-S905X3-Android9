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

package com.android.settings.intelligence.suggestions;

import android.content.Context;
import android.content.SharedPreferences;
import android.service.settings.suggestions.Suggestion;

import com.android.settings.intelligence.suggestions.ranking.SuggestionEventStore;
import com.android.settings.intelligence.suggestions.ranking.SuggestionFeaturizer;
import com.android.settings.intelligence.suggestions.ranking.SuggestionRanker;

import java.util.List;

public class SuggestionFeatureProvider {

    private static final String SHARED_PREF_FILENAME = "suggestions";
    private static final String IS_DISMISSED = "_is_dismissed";

    private SuggestionRanker mRanker;

    /**
     * Returns the {@link SharedPreferences} to store suggestion related user data.
     */
    public SharedPreferences getSharedPrefs(Context context) {
        return context.getApplicationContext()
                .getSharedPreferences(SHARED_PREF_FILENAME, Context.MODE_PRIVATE);
    }

    /**
     * Returns a list of suggestions that UI should display, sorted based on importance.
     */
    public List<Suggestion> getSuggestions(Context context) {
        final SuggestionParser parser = new SuggestionParser(context);
        final List<Suggestion> list = parser.getSuggestions();

        final List<Suggestion> rankedSuggestions = getRanker(context).rankRelevantSuggestions(list);

        final SuggestionEventStore eventStore = SuggestionEventStore.get(context);
        for (Suggestion suggestion : rankedSuggestions) {
            eventStore.writeEvent(suggestion.getId(), SuggestionEventStore.EVENT_SHOWN);
        }
        return rankedSuggestions;
    }

    /**
     * Mark a suggestion as dismissed
     */
    public void markSuggestionDismissed(Context context, String id) {
        getSharedPrefs(context)
                .edit()
                .putBoolean(getDismissKey(id), true)
                .apply();

        SuggestionEventStore.get(context).writeEvent(id, SuggestionEventStore.EVENT_DISMISSED);
    }

    /**
     * Mark a suggestion as not dismissed
     */
    public void markSuggestionNotDismissed(Context context, String id) {
        getSharedPrefs(context)
                .edit()
                .putBoolean(getDismissKey(id), false)
                .apply();
    }

    public void markSuggestionLaunched(Context context, String id) {
        SuggestionEventStore.get(context).writeEvent(id, SuggestionEventStore.EVENT_CLICKED);
    }

    /**
     * Whether or not a suggestion is dismissed
     */
    public boolean isSuggestionDismissed(Context context, String id) {
        return getSharedPrefs(context)
                .getBoolean(getDismissKey(id), false);
    }

    /**
     * Returns a manager that knows how to rank suggestions.
     */
    protected SuggestionRanker getRanker(Context context) {
        if (mRanker == null) {
            mRanker = new SuggestionRanker(context,
                    new SuggestionFeaturizer(context.getApplicationContext()));
        }
        return mRanker;
    }

    private static String getDismissKey(String id) {
        return id + IS_DISMISSED;
    }
}
