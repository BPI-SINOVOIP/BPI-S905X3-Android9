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

package com.android.settings.intelligence.search.sitemap;

import android.content.ContentValues;
import android.text.TextUtils;

import com.android.settings.intelligence.search.indexing.IndexDatabaseHelper;

import java.util.Objects;

/**
 * Data model for a parent-child page pair.
 * <p/>
 * A list of {@link SiteMapPair} can represent the breadcrumb for a search result from settings.
 */
public class SiteMapPair {
    private final String mParentClass;
    private final String mParentTitle;
    private final String mChildClass;
    private final String mChildTitle;

    public SiteMapPair(String parentClass, String parentTitle, String childClass,
            String childTitle) {
        mParentClass = parentClass;
        mParentTitle = parentTitle;
        mChildClass = childClass;
        mChildTitle = childTitle;
    }

    @Override
    public int hashCode() {
        return Objects.hash(mParentClass, mChildClass);
    }

    @Override
    public boolean equals(Object other) {
        if (other == null || !(other instanceof SiteMapPair)) {
            return false;
        }
        return TextUtils.equals(mParentClass, ((SiteMapPair) other).mParentClass)
                && TextUtils.equals(mChildClass, ((SiteMapPair) other).mChildClass);
    }

    public String getParentClass() {
        return mParentClass;
    }

    public String getParentTitle() {
        return mParentTitle;
    }

    public String getChildClass() {
        return mChildClass;
    }

    public String getChildTitle() {
        return mChildTitle;
    }

    /**
     * Converts this object into {@link ContentValues}. The content follows schema in
     * {@link IndexDatabaseHelper.SiteMapColumns}.
     */
    public ContentValues toContentValue() {
        final ContentValues values = new ContentValues();
        values.put(IndexDatabaseHelper.SiteMapColumns.DOCID, hashCode());
        values.put(IndexDatabaseHelper.SiteMapColumns.PARENT_CLASS, mParentClass);
        values.put(IndexDatabaseHelper.SiteMapColumns.PARENT_TITLE, mParentTitle);
        values.put(IndexDatabaseHelper.SiteMapColumns.CHILD_CLASS, mChildClass);
        values.put(IndexDatabaseHelper.SiteMapColumns.CHILD_TITLE, mChildTitle);
        return values;
    }
}
