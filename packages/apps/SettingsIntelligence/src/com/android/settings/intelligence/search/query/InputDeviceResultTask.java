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

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ServiceInfo;
import android.icu.text.ListFormatter;
import android.support.annotation.NonNull;
import android.support.annotation.VisibleForTesting;
import android.util.Log;
import android.view.InputDevice;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputMethodSubtype;

import com.android.settings.intelligence.R;
import com.android.settings.intelligence.nano.SettingsIntelligenceLogProto;
import com.android.settings.intelligence.search.ResultPayload;
import com.android.settings.intelligence.search.SearchFeatureProvider;
import com.android.settings.intelligence.search.SearchResult;
import com.android.settings.intelligence.search.indexing.DatabaseIndexingUtils;
import com.android.settings.intelligence.search.sitemap.SiteMapManager;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

public class InputDeviceResultTask extends SearchQueryTask.QueryWorker {

    private static final String TAG = "InputResultFutureTask";

    public static final int QUERY_WORKER_ID =
            SettingsIntelligenceLogProto.SettingsIntelligenceEvent.SEARCH_QUERY_INPUT_DEVICES;

    @VisibleForTesting
    static final String PHYSICAL_KEYBOARD_FRAGMENT =
            "com.android.settings.inputmethod.PhysicalKeyboardFragment";
    @VisibleForTesting
    static final String VIRTUAL_KEYBOARD_FRAGMENT =
            "com.android.settings.inputmethod.AvailableVirtualKeyboardFragment";

    public static SearchQueryTask newTask(Context context, SiteMapManager manager,
            String query) {
        return new SearchQueryTask(new InputDeviceResultTask(context, manager, query));
    }


    private static final int NAME_NO_MATCH = -1;

    private final InputMethodManager mImm;
    private final PackageManager mPackageManager;

    private List<String> mPhysicalKeyboardBreadcrumb;
    private List<String> mVirtualKeyboardBreadcrumb;

    public InputDeviceResultTask(Context context, SiteMapManager manager, String query) {
        super(context, manager, query);

        mImm = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        mPackageManager = context.getPackageManager();
    }

    @Override
    protected int getQueryWorkerId() {
        return QUERY_WORKER_ID;
    }

    @Override
    protected List<? extends SearchResult> query() {
        long startTime = System.currentTimeMillis();
        final List<SearchResult> results = new ArrayList<>();
        results.addAll(buildPhysicalKeyboardSearchResults());
        results.addAll(buildVirtualKeyboardSearchResults());
        Collections.sort(results);
        if (SearchFeatureProvider.DEBUG) {
            Log.d(TAG, "Input search loading took:" + (System.currentTimeMillis() - startTime));
        }
        return results;
    }

    private Set<SearchResult> buildPhysicalKeyboardSearchResults() {
        final Set<SearchResult> results = new HashSet<>();
        final String screenTitle = mContext.getString(R.string.physical_keyboard_title);

        for (final InputDevice device : getPhysicalFullKeyboards()) {
            final String deviceName = device.getName();
            final int wordDiff = SearchQueryUtils.getWordDifference(deviceName, mQuery);
            if (wordDiff == NAME_NO_MATCH) {
                continue;
            }
            final Intent intent = DatabaseIndexingUtils.buildSearchTrampolineIntent(mContext,
                    PHYSICAL_KEYBOARD_FRAGMENT, deviceName, screenTitle);
            results.add(new SearchResult.Builder()
                    .setTitle(deviceName)
                    .setPayload(new ResultPayload(intent))
                    .setDataKey(deviceName)
                    .setRank(wordDiff)
                    .addBreadcrumbs(getPhysicalKeyboardBreadCrumb())
                    .build());
        }
        return results;
    }

    private Set<SearchResult> buildVirtualKeyboardSearchResults() {
        final Set<SearchResult> results = new HashSet<>();
        final String screenTitle = mContext.getString(R.string.add_virtual_keyboard);
        final List<InputMethodInfo> inputMethods = mImm.getInputMethodList();
        for (InputMethodInfo info : inputMethods) {
            final String title = info.loadLabel(mPackageManager).toString();
            final String summary = getSubtypeLocaleNameListAsSentence(
                    getAllSubtypesOf(info), mContext, info);
            int wordDiff = SearchQueryUtils.getWordDifference(title, mQuery);
            if (wordDiff == NAME_NO_MATCH) {
                wordDiff = SearchQueryUtils.getWordDifference(summary, mQuery);
            }
            if (wordDiff == NAME_NO_MATCH) {
                continue;
            }
            final ServiceInfo serviceInfo = info.getServiceInfo();
            final String key = new ComponentName(serviceInfo.packageName, serviceInfo.name)
                    .flattenToString();
            final Intent intent = DatabaseIndexingUtils.buildSearchTrampolineIntent(mContext,
                    VIRTUAL_KEYBOARD_FRAGMENT, key, screenTitle);
            results.add(new SearchResult.Builder()
                    .setTitle(title)
                    .setSummary(summary)
                    .setRank(wordDiff)
                    .setDataKey(key)
                    .addBreadcrumbs(getVirtualKeyboardBreadCrumb())
                    .setPayload(new ResultPayload(intent))
                    .build());
        }
        return results;
    }

    private List<String> getPhysicalKeyboardBreadCrumb() {
        if (mPhysicalKeyboardBreadcrumb == null || mPhysicalKeyboardBreadcrumb.isEmpty()) {
            mPhysicalKeyboardBreadcrumb = mSiteMapManager.buildBreadCrumb(
                    mContext, PHYSICAL_KEYBOARD_FRAGMENT,
                    mContext.getString(R.string.physical_keyboard_title));
        }
        return mPhysicalKeyboardBreadcrumb;
    }


    private List<String> getVirtualKeyboardBreadCrumb() {
        if (mVirtualKeyboardBreadcrumb == null || mVirtualKeyboardBreadcrumb.isEmpty()) {
            final Context context = mContext;
            mVirtualKeyboardBreadcrumb = mSiteMapManager.buildBreadCrumb(
                    context, VIRTUAL_KEYBOARD_FRAGMENT,
                    context.getString(R.string.add_virtual_keyboard));
        }
        return mVirtualKeyboardBreadcrumb;
    }

    private List<InputDevice> getPhysicalFullKeyboards() {
        final List<InputDevice> keyboards = new ArrayList<>();
        final int[] deviceIds = InputDevice.getDeviceIds();
        if (deviceIds != null) {
            for (int deviceId : deviceIds) {
                final InputDevice device = InputDevice.getDevice(deviceId);
                if (isFullPhysicalKeyboard(device)) {
                    keyboards.add(device);
                }
            }
        }
        return keyboards;
    }

    @NonNull
    private static String getSubtypeLocaleNameListAsSentence(
            @NonNull final List<InputMethodSubtype> subtypes, @NonNull final Context context,
            @NonNull final InputMethodInfo inputMethodInfo) {
        if (subtypes.isEmpty()) {
            return "";
        }
        final Locale locale = Locale.getDefault();
        final int subtypeCount = subtypes.size();
        final CharSequence[] subtypeNames = new CharSequence[subtypeCount];
        for (int i = 0; i < subtypeCount; i++) {
            subtypeNames[i] = subtypes.get(i).getDisplayName(context,
                    inputMethodInfo.getPackageName(), inputMethodInfo.getServiceInfo()
                            .applicationInfo);
        }
        return toSentenceCase(
                ListFormatter.getInstance(locale).format((Object[]) subtypeNames), locale);
    }

    private static String toSentenceCase(String str, Locale locale) {
        if (str.isEmpty()) {
            return str;
        }
        final int firstCodePointLen = str.offsetByCodePoints(0, 1);
        return str.substring(0, firstCodePointLen).toUpperCase(locale)
                + str.substring(firstCodePointLen);
    }

    private static boolean isFullPhysicalKeyboard(InputDevice device) {
        return device != null && !device.isVirtual() &&
                (device.getSources() & InputDevice.SOURCE_KEYBOARD)
                        == InputDevice.SOURCE_KEYBOARD
                && device.getKeyboardType() == InputDevice.KEYBOARD_TYPE_ALPHABETIC;
    }

    private static List<InputMethodSubtype> getAllSubtypesOf(final InputMethodInfo imi) {
        final int subtypeCount = imi.getSubtypeCount();
        final List<InputMethodSubtype> allSubtypes = new ArrayList<>(subtypeCount);
        for (int index = 0; index < subtypeCount; index++) {
            allSubtypes.add(imi.getSubtypeAt(index));
        }
        return allSubtypes;
    }
}
