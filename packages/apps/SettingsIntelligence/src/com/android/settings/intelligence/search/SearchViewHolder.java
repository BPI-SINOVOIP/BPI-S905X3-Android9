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
package com.android.settings.intelligence.search;

import android.content.Context;
import android.content.pm.PackageManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.settings.intelligence.R;
import com.android.settings.intelligence.overlay.FeatureFactory;

/**
 * The ViewHolder for the Search RecyclerView.
 * There are multiple search result types in the same Recycler view with different UI requirements.
 * Some examples include Intent results, Inline results, and Help articles.
 */
public abstract class SearchViewHolder extends RecyclerView.ViewHolder {

    private final String DYNAMIC_PLACEHOLDER = "%s";

    private final String mPlaceholderSummary;

    public final TextView titleView;
    public final TextView summaryView;
    public final TextView breadcrumbView;
    public final ImageView iconView;

    protected final SearchFeatureProvider mSearchFeatureProvider;

    public SearchViewHolder(View view) {
        super(view);
        final FeatureFactory featureFactory = FeatureFactory
                .get(view.getContext().getApplicationContext());
        mSearchFeatureProvider = featureFactory.searchFeatureProvider();
        titleView = view.findViewById(android.R.id.title);
        summaryView = view.findViewById(android.R.id.summary);
        iconView = view.findViewById(android.R.id.icon);
        breadcrumbView = view.findViewById(R.id.breadcrumb);

        mPlaceholderSummary = view.getContext().getString(R.string.summary_placeholder);
    }

    public abstract int getClickActionMetricName();

    public void onBind(SearchFragment fragment, SearchResult result) {
        titleView.setText(result.title);
        // TODO (b/36101902) remove check for DYNAMIC_PLACEHOLDER
        if (TextUtils.isEmpty(result.summary)
                || TextUtils.equals(result.summary, mPlaceholderSummary)
                || TextUtils.equals(result.summary, DYNAMIC_PLACEHOLDER)) {
            summaryView.setVisibility(View.GONE);
        } else {
            summaryView.setText(result.summary);
            summaryView.setVisibility(View.VISIBLE);
        }

        if (result instanceof AppSearchResult) {
            AppSearchResult appResult = (AppSearchResult) result;
            PackageManager pm = fragment.getActivity().getPackageManager();
            iconView.setImageDrawable(appResult.info.loadIcon(pm));
        } else {
            // Valid even when result.icon is null.
            iconView.setImageDrawable(result.icon);
        }

        bindBreadcrumbView(result);
    }

    private void bindBreadcrumbView(SearchResult result) {
        if (result.breadcrumbs == null || result.breadcrumbs.isEmpty()) {
            breadcrumbView.setVisibility(View.GONE);
            return;
        }
        final Context context = breadcrumbView.getContext();
        String breadcrumb = result.breadcrumbs.get(0);
        final int count = result.breadcrumbs.size();
        for (int i = 1; i < count; i++) {
            breadcrumb = context.getString(R.string.search_breadcrumb_connector,
                    breadcrumb, result.breadcrumbs.get(i));
        }
        breadcrumbView.setText(breadcrumb);
        breadcrumbView.setVisibility(View.VISIBLE);
    }
}