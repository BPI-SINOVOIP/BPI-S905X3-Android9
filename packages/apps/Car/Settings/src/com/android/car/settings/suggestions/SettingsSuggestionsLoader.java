/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car.settings.suggestions;

import android.content.Context;
import android.service.settings.suggestions.Suggestion;

import com.android.car.settingslib.loader.AsyncLoader;
import com.android.car.settings.common.Logger;
import com.android.settingslib.suggestions.SuggestionController;

import java.util.List;

/**
 * Loads suggestions for car settings. Taken from
 * {@link com.android.settingslib.suggestions.SuggestionLoader}, only change is to extend from car
 * settings {@link AsyncLoader} which extends from support library {@link AsyncTaskLoader}.
 */
public class SettingsSuggestionsLoader extends AsyncLoader<List<Suggestion>> {
    private static final Logger LOG = new Logger(SettingsSuggestionsLoader.class);

    /**
     * ID used for loader that loads the settings suggestions. ID value is an arbitrary value.
     */
    public static final int LOADER_ID_SUGGESTIONS = 42;

    private final SuggestionController mSuggestionController;

    public SettingsSuggestionsLoader(Context context, SuggestionController controller) {
        super(context);
        mSuggestionController = controller;
    }

    @Override
    public List<Suggestion> loadInBackground() {
        final List<Suggestion> data = mSuggestionController.getSuggestions();
        if (data == null) {
            LOG.d("data is null");
        } else {
            LOG.d("data size " + data.size());
        }
        return data;
    }
}