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

import android.accessibilityservice.AccessibilityServiceInfo;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.graphics.drawable.Drawable;
import android.view.accessibility.AccessibilityManager;

import com.android.settings.intelligence.R;
import com.android.settings.intelligence.nano.SettingsIntelligenceLogProto;
import com.android.settings.intelligence.search.ResultPayload;
import com.android.settings.intelligence.search.SearchResult;
import com.android.settings.intelligence.search.indexing.DatabaseIndexingUtils;
import com.android.settings.intelligence.search.sitemap.SiteMapManager;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class AccessibilityServiceResultTask extends SearchQueryTask.QueryWorker {

    public static final int QUERY_WORKER_ID = SettingsIntelligenceLogProto.SettingsIntelligenceEvent
            .SEARCH_QUERY_ACCESSIBILITY_SERVICES;

    private static final String ACCESSIBILITY_SETTINGS_CLASSNAME =
            "com.android.settings.accessibility.AccessibilitySettings";
    private static final int NAME_NO_MATCH = -1;

    private final AccessibilityManager mAccessibilityManager;
    private final PackageManager mPackageManager;

    private List<String> mBreadcrumb;

    public static SearchQueryTask newTask(Context context, SiteMapManager manager,
            String query) {
        return new SearchQueryTask(new AccessibilityServiceResultTask(context, manager, query));
    }

    public AccessibilityServiceResultTask(Context context, SiteMapManager mapManager,
            String query) {
        super(context, mapManager, query);
        mPackageManager = context.getPackageManager();
        mAccessibilityManager =
                (AccessibilityManager) context.getSystemService(Context.ACCESSIBILITY_SERVICE);
    }

    @Override
    protected List<? extends SearchResult> query() {
        final List<SearchResult> results = new ArrayList<>();
        final List<AccessibilityServiceInfo> services = mAccessibilityManager
                .getInstalledAccessibilityServiceList();
        final String screenTitle = mContext.getString(R.string.accessibility_settings);
        for (AccessibilityServiceInfo service : services) {
            if (service == null) {
                continue;
            }
            final ResolveInfo resolveInfo = service.getResolveInfo();
            if (service.getResolveInfo() == null) {
                continue;
            }
            final ServiceInfo serviceInfo = resolveInfo.serviceInfo;
            final CharSequence title = resolveInfo.loadLabel(mPackageManager);
            final int wordDiff = SearchQueryUtils.getWordDifference(title.toString(), mQuery);
            if (wordDiff == NAME_NO_MATCH) {
                continue;
            }
            final Drawable icon = serviceInfo.loadIcon(mPackageManager);
            final String componentName = new ComponentName(serviceInfo.packageName,
                    serviceInfo.name).flattenToString();
            final Intent intent = DatabaseIndexingUtils.buildSearchTrampolineIntent(mContext,
                    ACCESSIBILITY_SETTINGS_CLASSNAME, componentName, screenTitle);

            results.add(new SearchResult.Builder()
                    .setTitle(title)
                    .addBreadcrumbs(getBreadCrumb())
                    .setPayload(new ResultPayload(intent))
                    .setRank(wordDiff)
                    .setIcon(icon)
                    .setDataKey(componentName)
                    .build());
        }
        Collections.sort(results);
        return results;
    }

    @Override
    protected int getQueryWorkerId() {
        return QUERY_WORKER_ID;
    }

    private List<String> getBreadCrumb() {
        if (mBreadcrumb == null || mBreadcrumb.isEmpty()) {
            mBreadcrumb = mSiteMapManager.buildBreadCrumb(
                    mContext, ACCESSIBILITY_SETTINGS_CLASSNAME,
                    mContext.getString(R.string.accessibility_settings));
        }
        return mBreadcrumb;
    }
}
