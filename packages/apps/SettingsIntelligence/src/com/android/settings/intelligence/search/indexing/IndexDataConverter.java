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

import android.content.Context;
import android.content.res.Resources;
import android.content.res.XmlResourceParser;
import android.os.AsyncTask;
import android.provider.SearchIndexableData;
import android.provider.SearchIndexableResource;
import android.support.annotation.DrawableRes;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Pair;
import android.util.Xml;

import com.android.settings.intelligence.search.ResultPayload;
import com.android.settings.intelligence.search.SearchFeatureProvider;
import com.android.settings.intelligence.search.SearchIndexableRaw;
import com.android.settings.intelligence.search.sitemap.SiteMapPair;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

/**
 * Helper class to convert {@link PreIndexData} to {@link IndexData}.
 */
public class IndexDataConverter {

    private static final String TAG = "IndexDataConverter";

    private static final String NODE_NAME_PREFERENCE_SCREEN = "PreferenceScreen";
    private static final String NODE_NAME_CHECK_BOX_PREFERENCE = "CheckBoxPreference";
    private static final String NODE_NAME_LIST_PREFERENCE = "ListPreference";
    private static final List<String> SKIP_NODES = Arrays.asList("intent", "extra");

    private final Context mContext;

    public IndexDataConverter(Context context) {
        mContext = context;
    }

    /**
     * Return the collection of {@param preIndexData} converted into {@link IndexData}.
     *
     * @param preIndexData a collection of {@link SearchIndexableResource},
     *                     {@link SearchIndexableRaw} and non-indexable keys.
     */
    public List<IndexData> convertPreIndexDataToIndexData(PreIndexData preIndexData) {
        final long startConversion = System.currentTimeMillis();
        final List<SearchIndexableData> indexableData = preIndexData.getDataToUpdate();
        final Map<String, Set<String>> nonIndexableKeys = preIndexData.getNonIndexableKeys();
        final List<IndexData> indexData = new ArrayList<>();

        for (SearchIndexableData data : indexableData) {
            if (data instanceof SearchIndexableRaw) {
                final SearchIndexableRaw rawData = (SearchIndexableRaw) data;
                final Set<String> rawNonIndexableKeys = nonIndexableKeys.get(
                        rawData.intentTargetPackage);
                final IndexData convertedRaw = convertRaw(mContext, rawData, rawNonIndexableKeys);
                if (convertedRaw != null) {
                    indexData.add(convertedRaw);
                }
            } else if (data instanceof SearchIndexableResource) {
                final SearchIndexableResource sir = (SearchIndexableResource) data;
                final Set<String> resourceNonIndexableKeys =
                        getNonIndexableKeysForResource(nonIndexableKeys, sir.packageName);
                final List<IndexData> resourceData = convertResource(sir, resourceNonIndexableKeys);
                indexData.addAll(resourceData);
            }
        }

        final long endConversion = System.currentTimeMillis();
        Log.d(TAG, "Converting pre-index data to index data took: "
                + (endConversion - startConversion));

        return indexData;
    }

    /**
     * Returns a full list of site map pairs based on metadata from all data sources.
     *
     * The content schema follows {@link IndexDatabaseHelper.Tables#TABLE_SITE_MAP}
     */
    public List<SiteMapPair> convertSiteMapPairs(List<IndexData> indexData,
            List<Pair<String, String>> siteMapClassNames) {
        final List<SiteMapPair> pairs = new ArrayList<>();
        if (indexData == null) {
            return pairs;
        }
        // Step 1: loop indexData and build all static site map pairs.
        final Map<String, String> classToTitleMap = new TreeMap<>();
        for (IndexData row : indexData) {
            if (TextUtils.isEmpty(row.className)) {
                continue;
            }
            // Build a map of [class, title] for the next step.
            classToTitleMap.put(row.className, row.screenTitle);
            if (!TextUtils.isEmpty(row.childClassName)) {
                pairs.add(new SiteMapPair(row.className, row.screenTitle,
                        row.childClassName, row.updatedTitle));
            }
        }
        // Step 2: Extend the sitemap pairs by adding dynamic pairs provided by
        // SearchIndexableProvider. The provider only tells us class name so we need to finish
        // the mapping by looking up display title for each class.
        for (Pair<String, String> pair : siteMapClassNames) {
            final String parentName = classToTitleMap.get(pair.first);
            final String childName = classToTitleMap.get(pair.second);
            if (TextUtils.isEmpty(parentName) || TextUtils.isEmpty(childName)) {
                Log.w(TAG, "Cannot build sitemap pair for incomplete names "
                        + pair + parentName + childName);
            } else {
                pairs.add(new SiteMapPair(pair.first, parentName, pair.second, childName));
            }
        }
        // Done
        return pairs;
    }

    /**
     * Return the conversion of {@link SearchIndexableRaw} to {@link IndexData}.
     * The fields of {@link SearchIndexableRaw} are a subset of {@link IndexData},
     * and there is some data sanitization in the conversion.
     */
    @Nullable
    private IndexData convertRaw(Context context, SearchIndexableRaw raw,
            Set<String> nonIndexableKeys) {
        if (TextUtils.isEmpty(raw.key)) {
            Log.w(TAG, "Skipping null key for raw indexable " + raw.packageName + "/" + raw.title);
            return null;
        }
        // A row is enabled if it does not show up as an nonIndexableKey
        boolean enabled = !(nonIndexableKeys != null && nonIndexableKeys.contains(raw.key));

        final IndexData.Builder builder = new IndexData.Builder();
        builder.setTitle(raw.title)
                .setSummaryOn(raw.summaryOn)
                .setEntries(raw.entries)
                .setKeywords(raw.keywords)
                .setClassName(raw.className)
                .setScreenTitle(raw.screenTitle)
                .setIconResId(raw.iconResId)
                .setIntentAction(raw.intentAction)
                .setIntentTargetPackage(raw.intentTargetPackage)
                .setIntentTargetClass(raw.intentTargetClass)
                .setEnabled(enabled)
                .setPackageName(raw.packageName)
                .setKey(raw.key);

        return builder.build(context);
    }

    /**
     * Return the conversion of the {@link SearchIndexableResource} to {@link IndexData}.
     * Each of the elements in the xml layout attribute of {@param sir} is a candidate to be
     * converted (including the header element).
     *
     * TODO (b/33577327) simplify this method.
     */
    private List<IndexData> convertResource(SearchIndexableResource sir,
            Set<String> nonIndexableKeys) {
        final Context context = sir.context;
        XmlResourceParser parser = null;

        List<IndexData> resourceIndexData = new ArrayList<>();
        try {
            parser = context.getResources().getXml(sir.xmlResId);

            int type;
            while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                    && type != XmlPullParser.START_TAG) {
                // Parse next until start tag is found
            }

            String nodeName = parser.getName();
            if (!NODE_NAME_PREFERENCE_SCREEN.equals(nodeName)) {
                throw new RuntimeException(
                        "XML document must start with <PreferenceScreen> tag; found"
                                + nodeName + " at " + parser.getPositionDescription());
            }

            final int outerDepth = parser.getDepth();
            final AttributeSet attrs = Xml.asAttributeSet(parser);

            final String screenTitle = XmlParserUtils.getDataTitle(context, attrs);
            final String headerKey = XmlParserUtils.getDataKey(context, attrs);

            String title;
            String key;
            String headerTitle;
            String summary;
            String headerSummary;
            String keywords;
            String headerKeywords;
            String childFragment;
            @DrawableRes int iconResId;
            ResultPayload payload;
            boolean enabled;

            // TODO REFACTOR (b/62807132) Add proper inline support
//            Map<String, PreferenceControllerMixin> controllerUriMap = null;
//
//            if (fragmentName != null) {
//                controllerUriMap = DatabaseIndexingUtils
//                        .getPreferenceControllerUriMap(fragmentName, context);
//            }

            headerTitle = XmlParserUtils.getDataTitle(context, attrs);
            headerSummary = XmlParserUtils.getDataSummary(context, attrs);
            headerKeywords = XmlParserUtils.getDataKeywords(context, attrs);
            enabled = !nonIndexableKeys.contains(headerKey);
            // TODO: Set payload type for header results
            IndexData.Builder headerBuilder = new IndexData.Builder();
            headerBuilder.setTitle(headerTitle)
                    .setSummaryOn(headerSummary)
                    .setScreenTitle(screenTitle)
                    .setKeywords(headerKeywords)
                    .setClassName(sir.className)
                    .setPackageName(sir.packageName)
                    .setIntentAction(sir.intentAction)
                    .setIntentTargetPackage(sir.intentTargetPackage)
                    .setIntentTargetClass(sir.intentTargetClass)
                    .setEnabled(enabled)
                    .setKey(headerKey);

            // Flag for XML headers which a child element's title.
            boolean isHeaderUnique = true;
            IndexData.Builder builder;

            while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                    && (type != XmlPullParser.END_TAG || parser.getDepth() > outerDepth)) {
                if (type == XmlPullParser.END_TAG || type == XmlPullParser.TEXT) {
                    continue;
                }

                nodeName = parser.getName();
                if (SKIP_NODES.contains(nodeName)) {
                    if (SearchFeatureProvider.DEBUG) {
                        Log.d(TAG, nodeName + " is not a valid entity to index, skip.");
                    }
                    continue;
                }

                title = XmlParserUtils.getDataTitle(context, attrs);
                key = XmlParserUtils.getDataKey(context, attrs);
                enabled = !nonIndexableKeys.contains(key);
                keywords = XmlParserUtils.getDataKeywords(context, attrs);
                iconResId = XmlParserUtils.getDataIcon(context, attrs);

                if (isHeaderUnique && TextUtils.equals(headerTitle, title)) {
                    isHeaderUnique = false;
                }

                builder = new IndexData.Builder();
                builder.setTitle(title)
                        .setKeywords(keywords)
                        .setClassName(sir.className)
                        .setScreenTitle(screenTitle)
                        .setIconResId(iconResId)
                        .setPackageName(sir.packageName)
                        .setIntentAction(sir.intentAction)
                        .setIntentTargetPackage(sir.intentTargetPackage)
                        .setIntentTargetClass(sir.intentTargetClass)
                        .setEnabled(enabled)
                        .setKey(key);

                if (!nodeName.equals(NODE_NAME_CHECK_BOX_PREFERENCE)) {
                    summary = XmlParserUtils.getDataSummary(context, attrs);

                    String entries = null;

                    if (nodeName.endsWith(NODE_NAME_LIST_PREFERENCE)) {
                        entries = XmlParserUtils.getDataEntries(context, attrs);
                    }

                    // TODO (b/62254931) index primitives instead of payload
                    // TODO (b/62807132) Add proper inline support
                    //payload = DatabaseIndexingUtils.getPayloadFromUriMap(controllerUriMap, key);
                    childFragment = XmlParserUtils.getDataChildFragment(context, attrs);

                    builder.setSummaryOn(summary)
                            .setEntries(entries)
                            .setChildClassName(childFragment);
                    tryAddIndexDataToList(resourceIndexData, builder);
                } else {
                    // TODO (b/33577327) We removed summary off here. We should check if we can
                    // merge this 'else' section with the one above. Put a break point to
                    // investigate.
                    String summaryOn = XmlParserUtils.getDataSummaryOn(context, attrs);

                    if (TextUtils.isEmpty(summaryOn)) {
                        summaryOn = XmlParserUtils.getDataSummary(context, attrs);
                    }

                    builder.setSummaryOn(summaryOn);

                    tryAddIndexDataToList(resourceIndexData, builder);
                }
            }

            // The xml header's title does not match the title of one of the child settings.
            if (isHeaderUnique) {
                tryAddIndexDataToList(resourceIndexData, headerBuilder);
            }
        } catch (XmlPullParserException e) {
            Log.w(TAG, "XML Error parsing PreferenceScreen: " + sir.className, e);
        } catch (IOException e) {
            Log.w(TAG, "IO Error parsing PreferenceScreen: " + sir.className, e);
        } catch (Resources.NotFoundException e) {
            Log.w(TAG, "Resoucre not found error parsing PreferenceScreen: " + sir.className, e);
        } finally {
            if (parser != null) {
                parser.close();
            }
        }
        return resourceIndexData;
    }

    private void tryAddIndexDataToList(List<IndexData> list, IndexData.Builder data) {
        if (!TextUtils.isEmpty(data.getKey())) {
            list.add(data.build(mContext));
        } else {
            Log.w(TAG, "Skipping index for null-key item " + data);
        }
    }

    private Set<String> getNonIndexableKeysForResource(Map<String, Set<String>> nonIndexableKeys,
            String packageName) {
        return nonIndexableKeys.containsKey(packageName)
                ? nonIndexableKeys.get(packageName)
                : new HashSet<String>();
    }
}
