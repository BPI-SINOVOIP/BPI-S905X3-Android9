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
package com.android.documentsui.dirlist;

import static android.support.v4.util.Preconditions.checkArgument;

import com.android.documentsui.selection.SelectionHelper.StableIdProvider;

import java.util.List;

/**
 * Provides RecyclerView selection code access to stable ids backed
 * by DocumentsAdapter.
 */
public final class DocsStableIdProvider extends StableIdProvider {

    private final DocumentsAdapter mAdapter;

    public DocsStableIdProvider(DocumentsAdapter adapter) {
        checkArgument(adapter != null);
        mAdapter = adapter;
    }

    @Override
    public String getStableId(int position) {
        return mAdapter.getStableId(position);
    }

    @Override
    public int getPosition(String id) {
        return mAdapter.getPosition(id);
    }

    @Override
    public List<String> getStableIds() {
        return mAdapter.getStableIds();
    }
}
