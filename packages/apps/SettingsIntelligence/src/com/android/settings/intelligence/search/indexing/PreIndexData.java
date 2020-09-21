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

package com.android.settings.intelligence.search.indexing;

import android.provider.SearchIndexableData;
import android.util.Pair;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;


/**
 * Holds Data sources for indexable data.
 */
public class PreIndexData {

    private final List<SearchIndexableData> mDataToUpdate;
    private final Map<String, Set<String>> mNonIndexableKeys;
    private final List<Pair<String, String>> mSiteMapPairs;

    public PreIndexData() {
        mDataToUpdate = new ArrayList<>();
        mNonIndexableKeys = new HashMap<>();
        mSiteMapPairs = new ArrayList<>();
    }

    public Map<String, Set<String>> getNonIndexableKeys() {
        return mNonIndexableKeys;
    }

    public List<SearchIndexableData> getDataToUpdate() {
        return mDataToUpdate;
    }

    public List<Pair<String, String>> getSiteMapPairs() {
        return mSiteMapPairs;
    }

    public void addNonIndexableKeysForAuthority(String authority, Set<String> keys) {
        mNonIndexableKeys.put(authority, keys);
    }

    public void addDataToUpdate(List<? extends SearchIndexableData> data) {
        mDataToUpdate.addAll(data);
    }

    public void addSiteMapPairs(List<Pair<String, String>> siteMapPairs) {
        if (siteMapPairs == null) {
            return;
        }
        mSiteMapPairs.addAll(siteMapPairs);
    }

}
