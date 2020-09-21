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

import android.support.annotation.VisibleForTesting;
import android.text.format.DateUtils;

import java.util.ArrayList;
import java.util.List;

public class SuggestionCategoryRegistry {

    @VisibleForTesting
    static final String CATEGORY_KEY_DEFERRED_SETUP =
            "com.android.settings.suggested.category.DEFERRED_SETUP";
    @VisibleForTesting
    static final String CATEGORY_KEY_HIGH_PRIORITY =
        "com.android.settings.suggested.category.HIGH_PRIORITY";
    @VisibleForTesting
    static final String CATEGORY_KEY_FIRST_IMPRESSION =
            "com.android.settings.suggested.category.FIRST_IMPRESSION";

    public static final List<SuggestionCategory> CATEGORIES;

    private static final long NEVER_EXPIRE = -1L;

    // This is equivalent to packages/apps/settings/res/values/suggestion_ordering.xml
    static {
        CATEGORIES = new ArrayList<>();
        CATEGORIES.add(buildCategory(CATEGORY_KEY_DEFERRED_SETUP,
                true /* exclusive */, 14 * DateUtils.DAY_IN_MILLIS));
        CATEGORIES.add(buildCategory(CATEGORY_KEY_HIGH_PRIORITY,
                true /* exclusive */, 3 * DateUtils.DAY_IN_MILLIS));
        CATEGORIES.add(buildCategory(CATEGORY_KEY_FIRST_IMPRESSION,
                true /* exclusive */, 14 * DateUtils.DAY_IN_MILLIS));
        CATEGORIES.add(buildCategory("com.android.settings.suggested.category.LOCK_SCREEN",
                false /* exclusive */, NEVER_EXPIRE));
        CATEGORIES.add(buildCategory("com.android.settings.suggested.category.TRUST_AGENT",
                false /* exclusive */, NEVER_EXPIRE));
        CATEGORIES.add(buildCategory("com.android.settings.suggested.category.EMAIL",
                false /* exclusive */, NEVER_EXPIRE));
        CATEGORIES.add(buildCategory("com.android.settings.suggested.category.PARTNER_ACCOUNT",
                false /* exclusive */, NEVER_EXPIRE));
        CATEGORIES.add(buildCategory("com.android.settings.suggested.category.GESTURE",
                false /* exclusive */, NEVER_EXPIRE));
        CATEGORIES.add(buildCategory("com.android.settings.suggested.category.HOTWORD",
                false /* exclusive */, NEVER_EXPIRE));
        CATEGORIES.add(buildCategory("com.android.settings.suggested.category.DEFAULT",
                false /* exclusive */, NEVER_EXPIRE));
        CATEGORIES.add(buildCategory("com.android.settings.suggested.category.SETTINGS_ONLY",
                false /* exclusive */, NEVER_EXPIRE));
    }

    private static SuggestionCategory buildCategory(String categoryName, boolean exclusive,
            long expireDaysInMillis) {
        return new SuggestionCategory.Builder()
                .setCategory(categoryName)
                .setExclusive(exclusive)
                .setExclusiveExpireDaysInMillis(expireDaysInMillis)
                .build();
    }

}
