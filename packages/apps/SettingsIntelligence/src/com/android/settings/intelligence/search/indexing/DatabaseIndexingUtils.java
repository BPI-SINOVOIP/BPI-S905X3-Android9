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

package com.android.settings.intelligence.search.indexing;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;

/**
 * Utility class for {@like DatabaseIndexingManager} to handle the mapping between Payloads
 * and Preference controllers, and managing indexable classes.
 */
public class DatabaseIndexingUtils {

    private static final String TAG = "DatabaseIndexingUtils";

    // frameworks/base/proto/src/metrics_constants.proto#DASHBOARD_SEARCH_RESULTS
    // Have to hardcode value here because we can't depend on internal framework constants.
    public static final int DASHBOARD_SEARCH_RESULTS = 34;

    /**
     * Below are internal contract between Settings/SettingsIntelligence to launch a search result
     * page.
     */
    public static final String EXTRA_SHOW_FRAGMENT = ":settings:show_fragment";
    public static final String EXTRA_SOURCE_METRICS_CATEGORY = ":settings:source_metrics";
    public static final String EXTRA_FRAGMENT_ARG_KEY = ":settings:fragment_args_key";
    public static final String EXTRA_SHOW_FRAGMENT_TITLE = ":settings:show_fragment_title";
    public static final String SEARCH_RESULT_TRAMPOLINE_ACTION =
            "com.android.settings.SEARCH_RESULT_TRAMPOLINE";

    /**
     * Builds intent that launches the search destination as a sub-setting.
     */
    public static Intent buildSearchTrampolineIntent(Context context, String className, String key,
            String screenTitle) {
        final Intent intent = new Intent(SEARCH_RESULT_TRAMPOLINE_ACTION);
        intent.putExtra(EXTRA_SHOW_FRAGMENT, className)
                .putExtra(EXTRA_SHOW_FRAGMENT_TITLE, screenTitle)
                .putExtra(EXTRA_SOURCE_METRICS_CATEGORY, DASHBOARD_SEARCH_RESULTS)
                .putExtra(EXTRA_FRAGMENT_ARG_KEY, key);
        return intent;
    }

    public static Intent buildDirectSearchResultIntent(String action, String targetPackage,
            String targetClass, String key) {
        final Intent intent = new Intent(action).putExtra(EXTRA_FRAGMENT_ARG_KEY, key);
        if (!TextUtils.isEmpty(targetPackage) && !TextUtils.isEmpty(targetClass)) {
            final ComponentName component = new ComponentName(targetPackage, targetClass);
            intent.setComponent(component);
        }
        return intent;
    }

    // TODO REFACTOR (b/62807132) Add inline support with proper intents.
//    /**
//     * @param className which wil provide the map between from {@link Uri}s to
//     *                  {@link PreferenceControllerMixin}
//     * @return A map between {@link Uri}s and {@link PreferenceControllerMixin}s to get the
// payload
//     * types for Settings.
//     */
//    public static Map<String, PreferenceControllerMixin> getPreferenceControllerUriMap(
//            String className, Context context) {
//        if (context == null) {
//            return null;
//        }
//
//        final Class<?> clazz = getIndexableClass(className);
//
//        if (clazz == null) {
//            Log.d(TAG, "SearchIndexableResource '" + className +
//                    "' should implement the " + Indexable.class.getName() + " interface!");
//            return null;
//        }
//
//        // Will be non null only for a Local provider implementing a
//        // SEARCH_INDEX_DATA_PROVIDER field
//        final Indexable.SearchIndexProvider provider = getSearchIndexProvider(clazz);
//
//        List<AbstractPreferenceController> controllers =
//                provider.getPreferenceControllers(context);
//
//        if (controllers == null) {
//            return null;
//        }
//
//        ArrayMap<String, PreferenceControllerMixin> map = new ArrayMap<>();
//
//        for (AbstractPreferenceController controller : controllers) {
//            if (controller instanceof PreferenceControllerMixin) {
//                map.put(controller.getPreferenceKey(), (PreferenceControllerMixin) controller);
//            } else {
//                throw new IllegalStateException(controller.getClass().getName()
//                        + " must implement " + PreferenceControllerMixin.class.getName());
//            }
//        }
//
//        return map;
//    }
//
//    /**
//     * @param uriMap Map between the {@link PreferenceControllerMixin} keys
//     *               and the controllers themselves.
//     * @param key    The look-up key
//     * @return The Payload from the {@link PreferenceControllerMixin} specified by the key,
//     * if it exists. Otherwise null.
//     */
//    public static ResultPayload getPayloadFromUriMap(Map<String, PreferenceControllerMixin>
// uriMap,
//            String key) {
//        if (uriMap == null) {
//            return null;
//        }
//
//        PreferenceControllerMixin controller = uriMap.get(key);
//        if (controller == null) {
//            return null;
//        }
//
//        return controller.getResultPayload();
//    }

}
