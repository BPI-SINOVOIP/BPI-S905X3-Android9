/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.car.carlauncher;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Filter;
import android.widget.Filterable;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.regex.Pattern;

/**
 * The search result adapter that binds the filtered result to a list view.
 */

final class SearchResultAdapter
        extends RecyclerView.Adapter<SearchResultAdapter.ViewHolder> implements Filterable {
    private final Activity mContext;
    private List<AppMetaData> mAllApps;
    private List<AppMetaData> mSearchResults;
    private AppSearchFilter filter;
    private boolean mIsDistractionOptimizationRequired;

    public SearchResultAdapter(Activity context) {
        mContext = context;
        mSearchResults = null;
    }

    public void setIsDistractionOptimizationRequired(boolean isDistractionOptimizationRequired) {
        mIsDistractionOptimizationRequired = isDistractionOptimizationRequired;
        notifyDataSetChanged();
    }

    public void setAllApps(List<AppMetaData> apps) {
        mAllApps = apps;
        notifyDataSetChanged();
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        private View mAppItem;
        private ImageView mAppIconView;
        private TextView mAppNameView;

        public ViewHolder(View view) {
            super(view);
            mAppItem = view.findViewById(R.id.app_item);
            mAppIconView = mAppItem.findViewById(R.id.app_icon);
            mAppNameView = mAppItem.findViewById(R.id.app_name);
        }

        public void bind(AppMetaData app, View.OnClickListener listener) {
            if (app == null) {
                // Empty out the view
                mAppIconView.setImageDrawable(null);
                mAppNameView.setText(null);
                mAppItem.setClickable(false);
                mAppItem.setOnClickListener(null);
            } else {
                mAppIconView.setImageDrawable(app.getIcon());
                mAppNameView.setText(app.getDisplayName());
                mAppItem.setClickable(true);
                mAppItem.setOnClickListener(listener);
            }
        }
    }

    /**
     * This filter does a simple case insensitive text match based on the application
     * display name. e.g. "gm" -> "gmail".
     * It only matches apps starting with the search query. It does not do sub-string matching.
     */
    private class AppSearchFilter extends Filter {

        @Override
        protected FilterResults performFiltering(CharSequence constraint) {
            FilterResults results = new FilterResults();
            if (constraint != null && constraint.length() > 0) {
                List filterList = new ArrayList();
                for (int i = 0; i < mAllApps.size(); i++) {
                    AppMetaData app = mAllApps.get(i);
                    if (shouldShowApp(
                            app.getDisplayName(),
                            constraint.toString(),
                            app.getIsDistractionOptimized())) {
                        filterList.add(app);
                    }
                }
                Collections.sort(filterList, AppLauncherUtils.ALPHABETICAL_COMPARATOR);
                results.count = filterList.size();
                results.values = filterList;
            } else {
                results.count = 0;
                results.values = null;
            }

            return results;
        }

        private boolean shouldShowApp(
                String displayName, String constraint, boolean isDistractionOptimized) {
            if (!mIsDistractionOptimizationRequired
                    || (mIsDistractionOptimizationRequired && isDistractionOptimized)) {
                Pattern pattern = Pattern.compile(
                        "^" + constraint + ".*$", Pattern.CASE_INSENSITIVE | Pattern.UNICODE_CASE);
                return pattern.matcher(displayName).matches();
            }
            return false;
        }

        @Override
        protected void publishResults(CharSequence constraint, FilterResults results) {
            mSearchResults = (ArrayList) results.values;
            notifyDataSetChanged();
        }
    }

    @Override
    public Filter getFilter() {
        if (filter == null) {
            filter = new AppSearchFilter();
        }
        return filter;
    }

    @NonNull
    @Override
    public SearchResultAdapter.ViewHolder onCreateViewHolder(
            @NonNull ViewGroup parent, int viewType) {
        View view = mContext.getLayoutInflater().inflate(R.layout.app_search_result_item, null);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull SearchResultAdapter.ViewHolder holder, int position) {
        // Get the data item for this position
        AppMetaData app = mSearchResults.get(position);
        holder.bind(app, v -> AppLauncherUtils.launchApp(mContext, app));
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public int getItemCount() {
        if (mSearchResults != null) {
            return mSearchResults.size();
        } else {
            return 0;
        }
    }

    void clearResults() {
        mSearchResults.clear();
        notifyDataSetChanged();
    }
}
