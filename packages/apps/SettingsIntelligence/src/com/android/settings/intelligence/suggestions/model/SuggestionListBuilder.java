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

package com.android.settings.intelligence.suggestions.model;

import android.service.settings.suggestions.Suggestion;
import android.util.ArraySet;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class SuggestionListBuilder {

    private Map<SuggestionCategory, List<Suggestion>> mSuggestions;
    private boolean isBuilt;

    public SuggestionListBuilder() {
        mSuggestions = new HashMap<>();
    }

    public void addSuggestions(SuggestionCategory category, List<Suggestion> suggestions) {
        if (isBuilt) {
            throw new IllegalStateException("Already built suggestion list, cannot add new ones");
        }
        mSuggestions.put(category, suggestions);
    }

    public List<Suggestion> build() {
        isBuilt = true;
        return dedupeSuggestions();
    }

    /**
     * Filter suggestions list so they are all unique.
     */
    private List<Suggestion> dedupeSuggestions() {
        final Set<String> ids = new ArraySet<>();
        final List<Suggestion> suggestions = new ArrayList<>();
        for (List<Suggestion> suggestionsInCategory : mSuggestions.values()) {
            suggestions.addAll(suggestionsInCategory);
        }
        for (int i = suggestions.size() - 1; i >= 0; i--) {
            final Suggestion suggestion = suggestions.get(i);
            final String id = suggestion.getId();
            if (ids.contains(id)) {
                suggestions.remove(i);
            } else {
                ids.add(id);
            }
        }
        return suggestions;
    }
}