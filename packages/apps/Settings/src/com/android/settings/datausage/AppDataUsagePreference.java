/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.settings.datausage;

import android.content.Context;
import android.support.v7.preference.PreferenceViewHolder;
import android.view.View;
import android.widget.ProgressBar;

import com.android.settings.widget.AppPreference;
import com.android.settingslib.AppItem;
import com.android.settingslib.net.UidDetail;
import com.android.settingslib.net.UidDetailProvider;
import com.android.settingslib.utils.ThreadUtils;

public class AppDataUsagePreference extends AppPreference {

    private final AppItem mItem;
    private final int mPercent;
    private UidDetail mDetail;

    public AppDataUsagePreference(Context context, AppItem item, int percent,
            UidDetailProvider provider) {
        super(context);
        mItem = item;
        mPercent = percent;

        if (item.restricted && item.total <= 0) {
            setSummary(com.android.settings.R.string.data_usage_app_restricted);
        } else {
            setSummary(DataUsageUtils.formatDataUsage(context, item.total));
        }
        mDetail = provider.getUidDetail(item.key, false /* blocking */);
        if (mDetail != null) {
            setAppInfo();
        } else {
            ThreadUtils.postOnBackgroundThread(() -> {
                mDetail = provider.getUidDetail(mItem.key, true /* blocking */);
                ThreadUtils.postOnMainThread(() -> setAppInfo());
            });
        }
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        final ProgressBar progress = (ProgressBar) holder.findViewById(
                android.R.id.progress);

        if (mItem.restricted && mItem.total <= 0) {
            progress.setVisibility(View.GONE);
        } else {
            progress.setVisibility(View.VISIBLE);
        }
        progress.setProgress(mPercent);
    }

    private void setAppInfo() {
        if (mDetail != null) {
            setIcon(mDetail.icon);
            setTitle(mDetail.label);
        } else {
            setIcon(null);
            setTitle(null);
        }
    }

    public AppItem getItem() {
        return mItem;
    }
}
