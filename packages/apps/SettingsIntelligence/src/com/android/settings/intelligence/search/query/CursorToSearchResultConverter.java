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

import static com.android.settings.intelligence.search.query.DatabaseResultTask.BASE_RANKS;
import static com.android.settings.intelligence.search.SearchResult.TOP_RANK;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.os.BadParcelableException;
import android.text.TextUtils;
import android.util.Log;

import com.android.settings.intelligence.search.ResultPayload;
import com.android.settings.intelligence.search.ResultPayloadUtils;
import com.android.settings.intelligence.search.SearchResult;
import com.android.settings.intelligence.search.indexing.IndexDatabaseHelper;
import com.android.settings.intelligence.search.sitemap.SiteMapManager;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Controller to Build search results from {@link Cursor} Objects.
 *
 * Each converted {@link Cursor} has the following fields:
 * - String Title
 * - String Summary
 * - int rank
 * - {@link Drawable} icon
 * - {@link ResultPayload} payload
 */
public class CursorToSearchResultConverter {

    private static final String TAG = "CursorConverter";

    private final Context mContext;

    private final int LONG_TITLE_LENGTH = 20;

    private static final String[] whiteList = {
            "main_toggle_wifi",
            "main_toggle_bluetooth",
            "main_toggle_bluetooth_obsolete",
            "toggle_airplane",
            "tether_settings",
            "battery_saver",
            "toggle_nfc",
            "restrict_background",
            "data_usage_enable",
            "button_roaming_key",
    };
    private static final Set<String> prioritySettings = new HashSet(Arrays.asList(whiteList));


    public CursorToSearchResultConverter(Context context) {
        mContext = context;
    }

    public Set<SearchResult> convertCursor(Cursor cursorResults, int baseRank,
            SiteMapManager siteMapManager) {
        if (cursorResults == null) {
            return null;
        }
        final Map<String, Context> contextMap = new HashMap<>();
        final Set<SearchResult> results = new HashSet<>();

        while (cursorResults.moveToNext()) {
            SearchResult result = buildSingleSearchResultFromCursor(siteMapManager,
                    contextMap, cursorResults, baseRank);
            if (result != null) {
                results.add(result);
            }
        }
        return results;
    }

    public static ResultPayload getUnmarshalledPayload(byte[] marshalledPayload,
            int payloadType) {
        try {
            switch (payloadType) {
                case ResultPayload.PayloadType.INTENT:
                    return ResultPayloadUtils.unmarshall(marshalledPayload,
                            ResultPayload.CREATOR);
                // TODO REFACTOR (b/62807132) Re-add inline
//                case ResultPayload.PayloadType.INLINE_SWITCH:
//                    return ResultPayloadUtils.unmarshall(marshalledPayload,
//                            InlineSwitchPayload.CREATOR);
//                case ResultPayload.PayloadType.INLINE_LIST:
//                    return ResultPayloadUtils.unmarshall(marshalledPayload,
//                            InlineListPayload.CREATOR);
            }
        } catch (BadParcelableException e) {
            Log.w(TAG, "Error creating parcelable: " + e);
        }
        return null;
    }

    private SearchResult buildSingleSearchResultFromCursor(SiteMapManager siteMapManager,
            Map<String, Context> contextMap, Cursor cursor, int baseRank) {
        final String pkgName = cursor.getString(cursor.getColumnIndexOrThrow(
                IndexDatabaseHelper.IndexColumns.DATA_PACKAGE));
        final String title = cursor.getString(cursor.getColumnIndexOrThrow(
                IndexDatabaseHelper.IndexColumns.DATA_TITLE));
        final String summaryOn = cursor.getString(cursor.getColumnIndex(
                IndexDatabaseHelper.IndexColumns.DATA_SUMMARY_ON));
        final String key = cursor.getString(cursor.getColumnIndexOrThrow(
                IndexDatabaseHelper.IndexColumns.DATA_KEY_REF));
        final String iconResStr = cursor.getString(cursor.getColumnIndexOrThrow(
                IndexDatabaseHelper.IndexColumns.ICON));
        final int payloadType = cursor.getInt(cursor.getColumnIndexOrThrow(
                IndexDatabaseHelper.IndexColumns.PAYLOAD_TYPE));
        final byte[] marshalledPayload = cursor.getBlob(cursor.getColumnIndexOrThrow(
                IndexDatabaseHelper.IndexColumns.PAYLOAD));
        final ResultPayload payload = getUnmarshalledPayload(marshalledPayload, payloadType);

        final List<String> breadcrumbs = getBreadcrumbs(siteMapManager, cursor);
        final int rank = getRank(title, baseRank, key);
        final Drawable icon = getIconForPackage(contextMap, pkgName, iconResStr);

        final SearchResult.Builder builder = new SearchResult.Builder()
                .setDataKey(key)
                .setTitle(title)
                .setSummary(summaryOn)
                .addBreadcrumbs(breadcrumbs)
                .setRank(rank)
                .setIcon(icon)
                .setPayload(payload);
        return builder.build();
    }

    private Drawable getIconForPackage(Map<String, Context> contextMap, String pkgName,
            String iconResStr) {
        if (TextUtils.isEmpty(pkgName)) {
            return null;
        }
        final int iconId = TextUtils.isEmpty(iconResStr) ? 0 : Integer.parseInt(iconResStr);
        if (iconId == 0) {
            return null;
        }
        Context packageContext = contextMap.get(pkgName);
        if (packageContext == null) {
            try {
                packageContext = mContext.createPackageContext(pkgName, 0);
                contextMap.put(pkgName, packageContext);
            } catch (PackageManager.NameNotFoundException e) {
                Log.e(TAG, "Cannot create Context for package: " + pkgName);
                return null;
            }
        }
        try {
            final Drawable drawable = packageContext.getDrawable(iconId);
            Log.d(TAG, "Returning icon, id :" + iconId);
            return drawable;
        } catch (Resources.NotFoundException nfe) {
            Log.w(TAG, "Cannot get icon, pkg/id :" + pkgName + "/" + iconId);
            return null;
        }
    }

    private List<String> getBreadcrumbs(SiteMapManager siteMapManager, Cursor cursor) {
        final String screenTitle = cursor.getString(cursor.getColumnIndexOrThrow(
                IndexDatabaseHelper.IndexColumns.SCREEN_TITLE));
        final String screenClass = cursor.getString(cursor.getColumnIndexOrThrow(
                IndexDatabaseHelper.IndexColumns.CLASS_NAME));
        return siteMapManager == null ? null : siteMapManager.buildBreadCrumb(mContext,
                screenClass, screenTitle);
    }

    /**
     * Uses the breadcrumbs to determine the offset to the base rank.
     * There are three checks
     * A) If the result is prioritized and the highest base level
     * B) If the query matches the highest level menu title
     * C) If the query is longer than 20
     *
     * If the query matches A, set it to TOP_RANK
     * If the query matches B, the offset is 0.
     * If the query matches C, the offset is 1
     *
     * @param title    of the result.
     * @param baseRank of the result. Lower if it's a better result.
     */
    private int getRank(String title, int baseRank, String key) {
        // The result can only be prioritized if it is a top ranked result.
        if (prioritySettings.contains(key) && baseRank < BASE_RANKS[1]) {
            return TOP_RANK;
        }
        if (title.length() > LONG_TITLE_LENGTH) {
            return baseRank + 1;
        }
        return baseRank;
    }
}