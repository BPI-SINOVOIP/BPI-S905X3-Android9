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

import android.content.Context;
import android.support.annotation.NonNull;

import com.android.settings.intelligence.overlay.FeatureFactory;
import com.android.settings.intelligence.search.SearchResult;
import com.android.settings.intelligence.search.sitemap.SiteMapManager;

import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.FutureTask;

public class SearchQueryTask extends FutureTask<List<? extends SearchResult>> {

    private final int mId;

    public SearchQueryTask(@NonNull QueryWorker queryWorker) {
        super(queryWorker);
        mId = queryWorker.getQueryWorkerId();
    }

    public int getTaskId() {
        return mId;
    }

    public static abstract class QueryWorker implements Callable<List<? extends SearchResult>> {

        protected final Context mContext;
        protected final SiteMapManager mSiteMapManager;
        protected final String mQuery;

        public QueryWorker(Context context, SiteMapManager siteMapManager, String query) {
            mContext = context;
            mSiteMapManager = siteMapManager;
            mQuery = query;
        }

        @Override
        public List<? extends SearchResult> call() throws Exception {
            final long startTime = System.currentTimeMillis();
            try {
                return query();
            } finally {
                final long endTime = System.currentTimeMillis();
                FeatureFactory.get(mContext).metricsFeatureProvider(mContext)
                        .logEvent(getQueryWorkerId(), endTime - startTime);
            }
        }

        protected abstract int getQueryWorkerId();

        protected abstract List<? extends SearchResult> query();
    }
}
