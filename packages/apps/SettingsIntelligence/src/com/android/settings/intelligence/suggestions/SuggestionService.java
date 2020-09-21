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

import android.service.settings.suggestions.Suggestion;
import android.util.Log;

import com.android.settings.intelligence.overlay.FeatureFactory;

import java.util.ArrayList;
import java.util.List;

public class SuggestionService extends android.service.settings.suggestions.SuggestionService {

    private static final String TAG = "SuggestionService";

    @Override
    public List<Suggestion> onGetSuggestions() {
        final long startTime = System.currentTimeMillis();
        final List<Suggestion> list = FeatureFactory.get(this)
                .suggestionFeatureProvider()
                .getSuggestions(this);

        final List<String> ids = new ArrayList<>(list.size());
        for (Suggestion suggestion : list) {
            ids.add(suggestion.getId());
        }
        final long endTime = System.currentTimeMillis();
        FeatureFactory.get(this)
                .metricsFeatureProvider(this)
                .logGetSuggestion(ids, endTime - startTime);
        return list;
    }

    @Override
    public void onSuggestionDismissed(Suggestion suggestion) {
        final long startTime = System.currentTimeMillis();
        final String id = suggestion.getId();
        Log.d(TAG, "dismissing suggestion " + id);
        final long endTime = System.currentTimeMillis();
        FeatureFactory.get(this)
                .suggestionFeatureProvider()
                .markSuggestionDismissed(this /* context */, id);
        FeatureFactory.get(this)
                .metricsFeatureProvider(this)
                .logDismissSuggestion(id, endTime - startTime);
    }

    @Override
    public void onSuggestionLaunched(Suggestion suggestion) {
        final long startTime = System.currentTimeMillis();
        final String id = suggestion.getId();
        Log.d(TAG, "Suggestion is launched" + id);
        final long endTime = System.currentTimeMillis();
        FeatureFactory.get(this)
                .suggestionFeatureProvider()
                .markSuggestionLaunched(this, id);
        FeatureFactory.get(this)
                .metricsFeatureProvider(this)
                .logLaunchSuggestion(id, endTime - startTime);
    }
}
