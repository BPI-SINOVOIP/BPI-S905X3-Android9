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
package com.android.documentsui.selection.demo;

import android.content.Context;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import com.android.documentsui.R;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

final class SelectionDemoAdapter extends RecyclerView.Adapter<DemoHolder> {

    private static final Map<String, DemoItem> sDemoData = new HashMap<>();
    static {
        for (int i = 0; i < 1000; i++) {
            String id = createId(i);
            sDemoData.put(id, new DemoItem(id, "item" + i));
        }
    }

    private final Context mContext;
    private @Nullable OnBindCallback mBindCallback;

    SelectionDemoAdapter(Context context) {
        mContext = context;
    }

    void addOnBindCallback(OnBindCallback bindCallback) {
        mBindCallback = bindCallback;
    }

    void loadData() {
        onDataReady();
    }

    private void onDataReady() {
        notifyDataSetChanged();
    }

    @Override
    public int getItemCount() {
        return sDemoData.size();
    }

    @Override
    public void onBindViewHolder(DemoHolder holder, int position) {
        String id = createId(position);
        holder.update(sDemoData.get(id));
        if (mBindCallback != null) {
            mBindCallback.onBound(holder, position);
        }
    }

    private static String createId(int position) {
        return "id" + position;
    }

    @Override
    public DemoHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        LinearLayout layout = inflateLayout(mContext, parent, R.layout.selection_demo_list_item);
        return new DemoHolder(layout);
    }

    String getStableId(int position) {
        return createId(position);
    }

    int getPosition(String id) {
        return Integer.parseInt(id.substring(2));
    }

    List<String> getStableIds() {
        return new ArrayList<>(sDemoData.keySet());
    }

    @SuppressWarnings("TypeParameterUnusedInFormals")  // Convenience to avoid clumsy cast.
    private static <V extends View> V inflateLayout(
            Context context, ViewGroup parent, int layout) {

        return (V) LayoutInflater.from(context).inflate(layout, parent, false);
    }

    static abstract class OnBindCallback {
        abstract void onBound(DemoHolder holder, int position);
    }
}
