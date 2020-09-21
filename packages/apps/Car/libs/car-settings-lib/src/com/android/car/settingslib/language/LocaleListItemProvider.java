/*
 * Copyright (C) 2018 Google Inc.
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

package com.android.car.settingslib.language;

import android.content.Context;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.app.LocaleStore;
import com.android.internal.app.SuggestedLocaleAdapter;

import com.android.car.settingslib.R;

import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.TextListItem;

/**
 * ListItemProvider that wraps {@link SuggestedLocaleAdapter}.
 */
public class LocaleListItemProvider extends ListItemProvider {
    private static final String TAG = "LocaleListItemProvider";
    /**
     * Header types are copied from {@link SuggestedLocaleAdapter} in order to be able to
     * determine the header rows.
     */
    private static final int TYPE_HEADER_SUGGESTED = 0;
    private static final int TYPE_HEADER_ALL_OTHERS = 1;
    private static final int TYPE_LOCALE = 2;

    private final Context mContext;

    private final HashSet<String> mIgnorableTags;
    private final LocaleSelectionListener mLocaleSelectionListener;

    private SuggestedLocaleAdapter mSuggestedLocaleAdapter;
    private boolean mIsChildLocale;

    public LocaleListItemProvider(
            Context context,
            Set<LocaleStore.LocaleInfo> localeInfoSet,
            LocaleSelectionListener localeSelectionListener,
            HashSet<String> ignorableTags) {
        mContext = context;
        mLocaleSelectionListener = localeSelectionListener;
        mIgnorableTags = ignorableTags;
        mSuggestedLocaleAdapter =
                LanguagePickerUtils.createSuggestedLocaleAdapter(context, localeInfoSet, null);
    }

    @Override
    public ListItem get(int position) {
        TextListItem item = new TextListItem(mContext);
        int type = mSuggestedLocaleAdapter.getItemViewType(position);
        switch (type) {
            case TYPE_HEADER_SUGGESTED:
            case TYPE_HEADER_ALL_OTHERS:
                item.addViewBinder(viewHolder ->
                        viewHolder.getTitle().setTextAppearance(
                                R.style.TextAppearance_Car_Settings_ListHeader));
                item.setTitle(mContext.getString(type == TYPE_HEADER_SUGGESTED
                        ? R.string.language_picker_list_suggested_header
                        : R.string.language_picker_list_all_header));
                break;

            case TYPE_LOCALE:
                LocaleStore.LocaleInfo info =
                        (LocaleStore.LocaleInfo) mSuggestedLocaleAdapter.getItem(position);
                item.setTitle(info.getFullNameNative());
                final Locale[] originalLocale = {null};
                item.addViewBinder(
                        viewHolder -> {
                            originalLocale[0] = viewHolder.getTitle().getTextLocale();
                            viewHolder.getTitle().setTextLocale(info.getLocale());
                        },
                        // Restore TextLocale when ViewHolder is recycled.
                        viewHolder -> viewHolder.getTitle().setTextLocale(originalLocale[0]));
                item.setOnClickListener(view -> {
                    if (info.getParent() == null) {
                        // getParent() == null means we should check if there are > 1 children
                        // locales. If there are, then show those as a new list
                        Set<LocaleStore.LocaleInfo> localeInfoSet =
                                LocaleStore.getLevelLocales(
                                        mContext, mIgnorableTags, info, true);
                        if (localeInfoSet.size() > 1) {
                            updateSuggestedLocaleAdapter(
                                    LanguagePickerUtils.createSuggestedLocaleAdapter(
                                            mContext,
                                            localeInfoSet,
                                            info),
                                    true);
                            mLocaleSelectionListener.onParentWithChildrenLocaleSelected(info);
                        } else {
                            mLocaleSelectionListener.onLocaleSelected(info);
                        }
                    } else {
                        mLocaleSelectionListener.onLocaleSelected(info);
                    }
                });
                break;

            default:
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Log.d(TAG, "Attempting to get unknown type: " + type);
                }
                break;
        }
        return item;
    }

    @Override
    public int size() {
        return mSuggestedLocaleAdapter.getCount();
    }

    public boolean isChildLocale() {
        return mIsChildLocale;
    }

    public void updateSuggestedLocaleAdapter(
            SuggestedLocaleAdapter suggestedLocaleAdapter,
            boolean isChildLocale) {
        mIsChildLocale = isChildLocale;
        mSuggestedLocaleAdapter = suggestedLocaleAdapter;
    }

    @VisibleForTesting
    SuggestedLocaleAdapter getSuggestedLocaleAdapter() {
        return mSuggestedLocaleAdapter;
    }
}
