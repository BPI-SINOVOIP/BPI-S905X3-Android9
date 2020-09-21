/*
 * Copyright (C) 2015 The Android Open Source Project
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

import static org.junit.Assert.assertTrue;

import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.AdapterDataObserver;
import android.view.ViewGroup;

import com.android.documentsui.Model.Update;
import com.android.documentsui.base.EventListener;
import com.android.documentsui.selection.SelectionHelper;
import com.android.documentsui.testing.TestEventListener;

import java.util.ArrayList;
import java.util.List;

/**
 * A skeletal {@link DocumentsAdapter} test double.
 */
public class TestDocumentsAdapter extends DocumentsAdapter {

    final TestEventListener<Update> mModelListener = new TestEventListener<>();
    List<String> mModelIds = new ArrayList<>();
    private final AdapterDataObserver mAdapterObserver;
    private final List<Integer> mSelectionChanged = new ArrayList<>();

    public TestDocumentsAdapter(List<String> modelIds) {
        mModelIds = modelIds;

        mAdapterObserver = new RecyclerView.AdapterDataObserver() {

            @Override
            public void onChanged() {
            }

            @Override
            public void onItemRangeChanged(int startPosition, int itemCount, Object payload) {
                if (SelectionHelper.SELECTION_CHANGED_MARKER.equals(payload)) {
                    int last = startPosition + itemCount;
                    for (int i = startPosition; i < last; i++) {
                        mSelectionChanged.add(i);
                    }
                }
            }

            @Override
            public void onItemRangeInserted(int startPosition, int itemCount) {
            }

            @Override
            public void onItemRangeRemoved(int startPosition, int itemCount) {
            }

            @Override
            public void onItemRangeMoved(int fromPosition, int toPosition, int itemCount) {
                throw new UnsupportedOperationException();
            }
        };

        registerAdapterDataObserver(mAdapterObserver);
    }

    public void resetSelectionChanged() {
        mSelectionChanged.clear();
    }

    public void assertSelectionChanged(int position) {
        assertTrue(mSelectionChanged.contains(position));
    }

    @Override
    EventListener<Update> getModelUpdateListener() {
        return mModelListener;
    }

    @Override
    public List<String> getStableIds() {
        return mModelIds;
    }

    @Override
    public int getPosition(String id) {
        return mModelIds.indexOf(id);
    }

    @Override
    public String getStableId(int position) {
        return mModelIds.get(position);
    }

    @Override
    public DocumentHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void onBindViewHolder(DocumentHolder holder, int position) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getAdapterPosition(String docId) {
        return mModelIds.indexOf(docId);
    }

    @Override
    public int getItemCount() {
        return mModelIds.size();
    }

    public void updateTestModelIds(List<String> modelIds) {
        mModelIds = modelIds;

        notifyDataSetChanged();
    }
}
