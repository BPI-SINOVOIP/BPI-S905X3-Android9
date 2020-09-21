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

import android.annotation.Nullable;
import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.List;

/**
 * View holder that contains a row of most recently used apps and a divider.
 */
public class RecentAppsRowViewHolder extends RecyclerView.ViewHolder {
    private final Context mContext;
    private final int mColumnNumber;
    private final LinearLayout mRecentAppsRow;

    public RecentAppsRowViewHolder(View view, Context context) {
        super(view);
        mContext = context;
        mRecentAppsRow = view.findViewById(R.id.recent_apps_row);
        mColumnNumber = context.getResources().getInteger(R.integer.car_app_selector_column_number);
    }

    /**
     * Binds the most recently used apps row view with the list of most recently used app meta data.
     *
     * @param apps Pass {@code null} will empty out the row.
     */
    public void bind(@Nullable List<AppMetaData> apps, boolean isDistractionOptimizationRequired) {
        // Empty out the views
        mRecentAppsRow.removeAllViews();
        mRecentAppsRow.setWeightSum(mColumnNumber);

        if (apps == null) {
            return;
        }

        int size = Math.min(mColumnNumber, apps.size());
        for (int i = 0; i < size; i++) {
            View view =
                    LayoutInflater.from(mContext).inflate(R.layout.app_item, mRecentAppsRow, false);
            ImageView iconView = view.findViewById(R.id.app_icon);
            TextView appNameView = view.findViewById(R.id.app_name);

            AppMetaData app = apps.get(i);

            if (isDistractionOptimizationRequired && !app.getIsDistractionOptimized()) {
                iconView.setImageDrawable(AppLauncherUtils.toGrayscale(app.getIcon()));
            } else {
                iconView.setImageDrawable(app.getIcon());
                view.setOnClickListener(v -> AppLauncherUtils.launchApp(mContext, app));
            }
            appNameView.setText(app.getDisplayName());

            LinearLayout.LayoutParams params =
                    (LinearLayout.LayoutParams) view.getLayoutParams();
            params.width = 0;
            params.height = ViewGroup.LayoutParams.WRAP_CONTENT;
            params.weight = 1;
            params.bottomMargin = 0;
            mRecentAppsRow.addView(view);
        }

        // Add empty views to fill out the entire first row
        for (int i = size; i < mColumnNumber; i++) {
            LinearLayout.LayoutParams params =
                    new LinearLayout.LayoutParams(
                            0, ViewGroup.LayoutParams.WRAP_CONTENT, /* weight= */ 1);
            mRecentAppsRow.addView(new View(mContext), params);
        }

    }
}

