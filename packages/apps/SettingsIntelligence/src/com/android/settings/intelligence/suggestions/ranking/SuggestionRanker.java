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

package com.android.settings.intelligence.suggestions.ranking;

import android.content.Context;
import android.service.settings.suggestions.Suggestion;
import android.support.annotation.VisibleForTesting;

import com.android.settings.intelligence.overlay.FeatureFactory;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Copied from packages/apps/Settings/src/.../dashboard/suggestions/SuggestionRanker
 */
public class SuggestionRanker {

    private static final String TAG = "SuggestionRanker";

    // The following coefficients form a linear model, which mixes the features to obtain a
    // relevance metric for ranking the suggestion items. This model is learned with off-line data
    // by training a binary classifier to detect the clicked items. The higher the obtained
    // relevance metric, the higher chance of getting clicked.
    private static final Map<String, Double> WEIGHTS = new HashMap<String, Double>() {{
        put(SuggestionFeaturizer.FEATURE_IS_SHOWN, 5.05140842519);
        put(SuggestionFeaturizer.FEATURE_IS_DISMISSED, 2.29641455171);
        put(SuggestionFeaturizer.FEATURE_IS_CLICKED, -2.98812233623);
        put(SuggestionFeaturizer.FEATURE_TIME_FROM_LAST_SHOWN, 5.02807250202);
        put(SuggestionFeaturizer.FEATURE_TIME_FROM_LAST_DISMISSED, 2.49589700842);
        put(SuggestionFeaturizer.FEATURE_TIME_FROM_LAST_CLICKED, -4.3377039948);
        put(SuggestionFeaturizer.FEATURE_SHOWN_COUNT, -2.35993512546);
    }};

    private final long mMaxSuggestionsDisplayCount;
    private final SuggestionFeaturizer mSuggestionFeaturizer;
    private final Map<Suggestion, Double> mRelevanceMetrics;

    Comparator<Suggestion> suggestionComparator = new Comparator<Suggestion>() {
        @Override
        public int compare(Suggestion suggestion1, Suggestion suggestion2) {
            return mRelevanceMetrics.get(suggestion1) < mRelevanceMetrics.get(suggestion2) ? 1 : -1;
        }
    };

    public SuggestionRanker(Context context, SuggestionFeaturizer suggestionFeaturizer) {
        mSuggestionFeaturizer = suggestionFeaturizer;
        mRelevanceMetrics = new HashMap<>();
        mMaxSuggestionsDisplayCount = FeatureFactory.get(context).experimentFeatureProvider()
                .getMaxSuggestionDisplayCount(context);
    }

    /**
     * Filter out suggestions that are not relevant at the moment, and rank the rest.
     *
     * @return a list of suggestion ranked by relevance.
     */
    public List<Suggestion> rankRelevantSuggestions(List<Suggestion> suggestions) {
        mRelevanceMetrics.clear();
        Map<String, Map<String, Double>> features = mSuggestionFeaturizer.featurize(suggestions);
        for (Suggestion suggestion : suggestions) {
            mRelevanceMetrics.put(suggestion, getRelevanceMetric(features.get(suggestion.getId())));
        }
        final List<Suggestion> rankedSuggestions = new ArrayList<>();
        rankedSuggestions.addAll(suggestions);
        Collections.sort(rankedSuggestions, suggestionComparator);

        if (rankedSuggestions.size() < mMaxSuggestionsDisplayCount) {
            return rankedSuggestions;
        } else {
            final List<Suggestion> relevantSuggestions = new ArrayList<>();
            for (int i = 0; i < mMaxSuggestionsDisplayCount; i++) {
                relevantSuggestions.add(rankedSuggestions.get(i));
            }
            return relevantSuggestions;
        }
    }

    @VisibleForTesting
    double getRelevanceMetric(Map<String, Double> features) {
        double sum = 0;
        if (features == null) {
            return sum;
        }
        for (String feature : WEIGHTS.keySet()) {
            sum += WEIGHTS.get(feature) * features.get(feature);
        }
        return sum;
    }
}