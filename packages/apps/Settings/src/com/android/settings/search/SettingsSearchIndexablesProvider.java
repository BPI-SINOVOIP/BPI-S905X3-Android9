/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.settings.search;

import static android.provider.SearchIndexablesContract.COLUMN_INDEX_NON_INDEXABLE_KEYS_KEY_VALUE;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_CLASS_NAME;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_ENTRIES;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_ICON_RESID;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_INTENT_ACTION;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_INTENT_TARGET_CLASS;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_INTENT_TARGET_PACKAGE;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_KEY;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_KEYWORDS;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_SCREEN_TITLE;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_SUMMARY_OFF;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_SUMMARY_ON;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_TITLE;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_RAW_USER_ID;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_CLASS_NAME;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_ICON_RESID;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_INTENT_ACTION;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_INTENT_TARGET_CLASS;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_INTENT_TARGET_PACKAGE;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_RANK;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_RESID;
import static android.provider.SearchIndexablesContract.INDEXABLES_RAW_COLUMNS;
import static android.provider.SearchIndexablesContract.INDEXABLES_XML_RES_COLUMNS;
import static android.provider.SearchIndexablesContract.NON_INDEXABLES_KEYS_COLUMNS;
import static android.provider.SearchIndexablesContract.SITE_MAP_COLUMNS;

import static com.android.settings.dashboard.DashboardFragmentRegistry.CATEGORY_KEY_TO_PARENT_MAP;

import android.content.Context;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.provider.SearchIndexableResource;
import android.provider.SearchIndexablesContract;
import android.provider.SearchIndexablesProvider;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Log;

import com.android.settings.SettingsActivity;
import com.android.settings.overlay.FeatureFactory;
import com.android.settingslib.drawer.DashboardCategory;
import com.android.settingslib.drawer.Tile;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

public class SettingsSearchIndexablesProvider extends SearchIndexablesProvider {

    public static final boolean DEBUG = false;

    /**
     * Flag for a system property which checks if we should crash if there are issues in the
     * indexing pipeline.
     */
    public static final String SYSPROP_CRASH_ON_ERROR =
            "debug.com.android.settings.search.crash_on_error";

    private static final String TAG = "SettingsSearchProvider";

    private static final Collection<String> INVALID_KEYS;

    static {
        INVALID_KEYS = new ArraySet<>();
        INVALID_KEYS.add(null);
        INVALID_KEYS.add("");
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public Cursor queryXmlResources(String[] projection) {
        MatrixCursor cursor = new MatrixCursor(INDEXABLES_XML_RES_COLUMNS);
        final List<SearchIndexableResource> resources =
                getSearchIndexableResourcesFromProvider(getContext());
        for (SearchIndexableResource val : resources) {
            Object[] ref = new Object[INDEXABLES_XML_RES_COLUMNS.length];
            ref[COLUMN_INDEX_XML_RES_RANK] = val.rank;
            ref[COLUMN_INDEX_XML_RES_RESID] = val.xmlResId;
            ref[COLUMN_INDEX_XML_RES_CLASS_NAME] = val.className;
            ref[COLUMN_INDEX_XML_RES_ICON_RESID] = val.iconResId;
            ref[COLUMN_INDEX_XML_RES_INTENT_ACTION] = val.intentAction;
            ref[COLUMN_INDEX_XML_RES_INTENT_TARGET_PACKAGE] = val.intentTargetPackage;
            ref[COLUMN_INDEX_XML_RES_INTENT_TARGET_CLASS] = null; // intent target class
            cursor.addRow(ref);
        }

        return cursor;
    }

    @Override
    public Cursor queryRawData(String[] projection) {
        MatrixCursor cursor = new MatrixCursor(INDEXABLES_RAW_COLUMNS);
        final List<SearchIndexableRaw> raws = getSearchIndexableRawFromProvider(getContext());
        for (SearchIndexableRaw val : raws) {
            Object[] ref = new Object[INDEXABLES_RAW_COLUMNS.length];
            ref[COLUMN_INDEX_RAW_TITLE] = val.title;
            ref[COLUMN_INDEX_RAW_SUMMARY_ON] = val.summaryOn;
            ref[COLUMN_INDEX_RAW_SUMMARY_OFF] = val.summaryOff;
            ref[COLUMN_INDEX_RAW_ENTRIES] = val.entries;
            ref[COLUMN_INDEX_RAW_KEYWORDS] = val.keywords;
            ref[COLUMN_INDEX_RAW_SCREEN_TITLE] = val.screenTitle;
            ref[COLUMN_INDEX_RAW_CLASS_NAME] = val.className;
            ref[COLUMN_INDEX_RAW_ICON_RESID] = val.iconResId;
            ref[COLUMN_INDEX_RAW_INTENT_ACTION] = val.intentAction;
            ref[COLUMN_INDEX_RAW_INTENT_TARGET_PACKAGE] = val.intentTargetPackage;
            ref[COLUMN_INDEX_RAW_INTENT_TARGET_CLASS] = val.intentTargetClass;
            ref[COLUMN_INDEX_RAW_KEY] = val.key;
            ref[COLUMN_INDEX_RAW_USER_ID] = val.userId;
            cursor.addRow(ref);
        }

        return cursor;
    }

    /**
     * Gets a combined list non-indexable keys that come from providers inside of settings.
     * The non-indexable keys are used in Settings search at both index and update time to verify
     * the validity of results in the database.
     */
    @Override
    public Cursor queryNonIndexableKeys(String[] projection) {
        MatrixCursor cursor = new MatrixCursor(NON_INDEXABLES_KEYS_COLUMNS);
        final List<String> nonIndexableKeys = getNonIndexableKeysFromProvider(getContext());
        for (String nik : nonIndexableKeys) {
            final Object[] ref = new Object[NON_INDEXABLES_KEYS_COLUMNS.length];
            ref[COLUMN_INDEX_NON_INDEXABLE_KEYS_KEY_VALUE] = nik;
            cursor.addRow(ref);
        }

        return cursor;
    }

    @Override
    public Cursor querySiteMapPairs() {
        final MatrixCursor cursor = new MatrixCursor(SITE_MAP_COLUMNS);
        final Context context = getContext();
        // Loop through all IA categories and pages and build additional SiteMapPairs
        final List<DashboardCategory> categories = FeatureFactory.getFactory(context)
                .getDashboardFeatureProvider(context).getAllCategories();
        for (DashboardCategory category : categories) {
            // Use the category key to look up parent (which page hosts this key)
            final String parentClass = CATEGORY_KEY_TO_PARENT_MAP.get(category.key);
            if (parentClass == null) {
                continue;
            }
            // Build parent-child class pairs for all children listed under this key.
            for (Tile tile : category.getTiles()) {
                String childClass = null;
                if (tile.metaData != null) {
                    childClass = tile.metaData.getString(
                            SettingsActivity.META_DATA_KEY_FRAGMENT_CLASS);
                }
                if (childClass == null) {
                    continue;
                }
                cursor.newRow()
                        .add(SearchIndexablesContract.SiteMapColumns.PARENT_CLASS, parentClass)
                        .add(SearchIndexablesContract.SiteMapColumns.CHILD_CLASS, childClass);
            }
        }
        // Done.
        return cursor;
    }

    private List<String> getNonIndexableKeysFromProvider(Context context) {
        final Collection<Class> values = FeatureFactory.getFactory(context)
                .getSearchFeatureProvider().getSearchIndexableResources().getProviderValues();
        final List<String> nonIndexableKeys = new ArrayList<>();

        for (Class<?> clazz : values) {
            final long startTime = System.currentTimeMillis();
            Indexable.SearchIndexProvider provider = DatabaseIndexingUtils.getSearchIndexProvider(
                    clazz);

            List<String> providerNonIndexableKeys;
            try {
                providerNonIndexableKeys = provider.getNonIndexableKeys(context);
            } catch (Exception e) {
                // Catch a generic crash. In the absence of the catch, the background thread will
                // silently fail anyway, so we aren't losing information by catching the exception.
                // We crash when the system property exists so that we can test if crashes need to
                // be fixed.
                // The gain is that if there is a crash in a specific controller, we don't lose all
                // non-indexable keys, but we can still find specific crashes in development.
                if (System.getProperty(SYSPROP_CRASH_ON_ERROR) != null) {
                    throw new RuntimeException(e);
                }
                Log.e(TAG, "Error trying to get non-indexable keys from: " + clazz.getName() , e);
                continue;
            }

            if (providerNonIndexableKeys == null || providerNonIndexableKeys.isEmpty()) {
                if (DEBUG) {
                    final long totalTime = System.currentTimeMillis() - startTime;
                    Log.d(TAG, "No indexable, total time " + totalTime);
                }
                continue;
            }

            if (providerNonIndexableKeys.removeAll(INVALID_KEYS)) {
                Log.v(TAG, provider + " tried to add an empty non-indexable key");
            }

            if (DEBUG) {
                final long totalTime = System.currentTimeMillis() - startTime;
                Log.d(TAG, "Non-indexables " + providerNonIndexableKeys.size() + ", total time "
                        + totalTime);
            }

            nonIndexableKeys.addAll(providerNonIndexableKeys);
        }

        return nonIndexableKeys;
    }

    private List<SearchIndexableResource> getSearchIndexableResourcesFromProvider(Context context) {
        Collection<Class> values = FeatureFactory.getFactory(context)
                .getSearchFeatureProvider().getSearchIndexableResources().getProviderValues();
        List<SearchIndexableResource> resourceList = new ArrayList<>();

        for (Class<?> clazz : values) {
            Indexable.SearchIndexProvider provider = DatabaseIndexingUtils.getSearchIndexProvider(
                    clazz);

            final List<SearchIndexableResource> resList =
                    provider.getXmlResourcesToIndex(context, true);

            if (resList == null) {
                continue;
            }

            for (SearchIndexableResource item : resList) {
                item.className = TextUtils.isEmpty(item.className)
                        ? clazz.getName()
                        : item.className;
            }

            resourceList.addAll(resList);
        }

        return resourceList;
    }

    private List<SearchIndexableRaw> getSearchIndexableRawFromProvider(Context context) {
        final Collection<Class> values = FeatureFactory.getFactory(context)
                .getSearchFeatureProvider().getSearchIndexableResources().getProviderValues();
        final List<SearchIndexableRaw> rawList = new ArrayList<>();

        for (Class<?> clazz : values) {
            Indexable.SearchIndexProvider provider = DatabaseIndexingUtils.getSearchIndexProvider(
                    clazz);
            final List<SearchIndexableRaw> providerRaws = provider.getRawDataToIndex(context,
                    true /* enabled */);

            if (providerRaws == null) {
                continue;
            }

            for (SearchIndexableRaw raw : providerRaws) {
                // The classname and intent information comes from the PreIndexData
                // This will be more clear when provider conversion is done at PreIndex time.
                raw.className = clazz.getName();

            }
            rawList.addAll(providerRaws);
        }

        return rawList;
    }
}
