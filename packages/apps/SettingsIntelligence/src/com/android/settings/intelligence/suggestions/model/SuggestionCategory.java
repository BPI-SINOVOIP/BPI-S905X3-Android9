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

/**
 * A category for {@link android.service.settings.suggestions.Suggestion}.
 * <p/>
 * Each suggestion can sepcify it belongs to 1 or more categories. This metadata will be used to
 * help determine when to show a suggestion to user.
 */
public class SuggestionCategory {
    private final String mCategory;
    private final boolean mExclusive;
    private final long mExclusiveExpireDaysInMillis;

    private SuggestionCategory(Builder builder) {
        this.mCategory = builder.mCategory;
        this.mExclusive = builder.mExclusive;
        this.mExclusiveExpireDaysInMillis = builder.mExclusiveExpireDaysInMillis;
    }

    public String getCategory() {
        return mCategory;
    }

    public boolean isExclusive() {
        return mExclusive;
    }

    public long getExclusiveExpireDaysInMillis() {
        return mExclusiveExpireDaysInMillis;
    }

    public static class Builder {
        private String mCategory;
        private boolean mExclusive;
        private long mExclusiveExpireDaysInMillis;

        public Builder setCategory(String category) {
            mCategory = category;
            return this;
        }

        public Builder setExclusive(boolean exclusive) {
            mExclusive = exclusive;
            return this;
        }

        public Builder setExclusiveExpireDaysInMillis(long exclusiveExpireDaysInMillis) {
            mExclusiveExpireDaysInMillis = exclusiveExpireDaysInMillis;
            return this;
        }

        public SuggestionCategory build() {
            return new SuggestionCategory(this);
        }
    }
}
