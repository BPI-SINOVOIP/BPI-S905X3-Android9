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
package com.android.documentsui.selection.testing;

import static org.junit.Assert.assertTrue;

import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.Adapter;
import android.support.v7.widget.RecyclerView.AdapterDataObserver;
import android.view.ViewGroup;

import com.android.documentsui.selection.SelectionHelper;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class TestAdapter extends Adapter<TestHolder> {

    private final List<String> mItems = new ArrayList<>();
    private final List<Integer> mNotifiedOfSelection = new ArrayList<>();
    private final AdapterDataObserver mAdapterObserver;

    public TestAdapter() {
        this(Collections.EMPTY_LIST);
    }

    public TestAdapter(List<String> items) {
        mItems.addAll(items);
        mAdapterObserver = new RecyclerView.AdapterDataObserver() {

            @Override
            public void onChanged() {
            }

            @Override
            public void onItemRangeChanged(int startPosition, int itemCount, Object payload) {
                if (SelectionHelper.SELECTION_CHANGED_MARKER.equals(payload)) {
                    int last = startPosition + itemCount;
                    for (int i = startPosition; i < last; i++) {
                        mNotifiedOfSelection.add(i);
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

    @Override
    public TestHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        return new TestHolder(parent);
    }

    @Override
    public void onBindViewHolder(TestHolder holder, int position) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getItemCount() {
        return mItems.size();
    }

    public void updateTestModelIds(List<String> items) {
        mItems.clear();
        mItems.addAll(items);

        notifyDataSetChanged();
    }

    public List<String> getStableIds() {
        return mItems;
    }

    public int getPosition(String id) {
        return mItems.indexOf(id);
    }

    public String getStableId(int position) {
        return mItems.get(position);
    }


    public void resetSelectionNotifications() {
        mNotifiedOfSelection.clear();
    }

    public void assertNotifiedOfSelectionChange(int position) {
        assertTrue(mNotifiedOfSelection.contains(position));
    }

    public static List<String> createItemList(int num) {
        List<String> items = new ArrayList<>(num);
        for (int i = 0; i < num; ++i) {
            items.add(Integer.toString(i));
        }
        return items;
    }
}
