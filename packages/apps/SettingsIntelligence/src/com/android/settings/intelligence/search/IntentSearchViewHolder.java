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

import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.support.annotation.VisibleForTesting;
import android.util.Log;
import android.view.View;

import com.android.settings.intelligence.nano.SettingsIntelligenceLogProto;

import java.util.List;

/**
 * ViewHolder for intent based search results.
 * The DatabaseResultTask is the primary use case for this ViewHolder.
 */
public class IntentSearchViewHolder extends SearchViewHolder {

    private static final String TAG = "IntentSearchViewHolder";
    @VisibleForTesting
    static final int REQUEST_CODE_NO_OP = 0;

    public IntentSearchViewHolder(View view) {
        super(view);
    }

    @Override
    public int getClickActionMetricName() {
        return SettingsIntelligenceLogProto.SettingsIntelligenceEvent.CLICK_SEARCH_RESULT;
    }

    @Override
    public void onBind(final SearchFragment fragment, final SearchResult result) {
        super.onBind(fragment, result);
        final SearchViewHolder viewHolder = this;

        // TODO (b/64935342) add dynamic api
        itemView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                fragment.onSearchResultClicked(viewHolder, result);
                final Intent intent = result.payload.getIntent();
                // Use app user id to support work profile use case.
                if (result instanceof AppSearchResult) {
                    AppSearchResult appResult = (AppSearchResult) result;
                    fragment.getActivity().startActivity(intent);
                } else {
                    final PackageManager pm = fragment.getActivity().getPackageManager();
                    final List<ResolveInfo> info = pm.queryIntentActivities(intent, 0 /* flags */);
                    if (info != null && !info.isEmpty()) {
                        fragment.startActivityForResult(intent, REQUEST_CODE_NO_OP);
                    } else {
                        Log.e(TAG, "Cannot launch search result, title: "
                                + result.title + ", " + intent);
                    }
                }
            }
        });
    }
}
