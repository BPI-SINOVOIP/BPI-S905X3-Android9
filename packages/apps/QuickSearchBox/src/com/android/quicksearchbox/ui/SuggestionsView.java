/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.android.quicksearchbox.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ListAdapter;
import android.widget.ListView;

import com.android.quicksearchbox.SuggestionPosition;

/**
 * Holds a list of suggestions.
 */
public class SuggestionsView extends ListView implements SuggestionsListView<ListAdapter> {

    private static final boolean DBG = false;
    private static final String TAG = "QSB.SuggestionsView";

    private SuggestionsAdapter<ListAdapter> mSuggestionsAdapter;

    public SuggestionsView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void setSuggestionsAdapter(SuggestionsAdapter<ListAdapter> adapter) {
        super.setAdapter(adapter == null ? null : adapter.getListAdapter());
        mSuggestionsAdapter = adapter;
    }

    @Override
    public SuggestionsAdapter<ListAdapter> getSuggestionsAdapter() {
        return mSuggestionsAdapter;
    }

    @Override
    public void onFinishInflate() {
        super.onFinishInflate();
        setItemsCanFocus(true);
    }

    /**
     * Gets the position of the selected suggestion.
     *
     * @return A 0-based index, or {@code -1} if no suggestion is selected.
     */
    public int getSelectedPosition() {
        return getSelectedItemPosition();
    }

    /**
     * Gets the selected suggestion.
     *
     * @return {@code null} if no suggestion is selected.
     */
    public SuggestionPosition getSelectedSuggestion() {
        return (SuggestionPosition) getSelectedItem();
    }


}
