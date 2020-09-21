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
 * limitations under the License
 */

package com.android.car.list;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import com.android.car.list.R;

import java.util.List;

/**
 * Contains logic for a line item represents a spinner.
 */
public class SpinnerLineItem<T> extends TypedPagedListAdapter.LineItem<SpinnerLineItem.ViewHolder> {
    private final ArrayAdapter<T> mArrayAdapter;
    private final AdapterView.OnItemSelectedListener mOnItemSelectedListener;
    private final CharSequence mTitle;
    private final int mSelectedPosition;

    /**
     * Constructs a new SpinnerLineItem
     *
     * @param context Android context
     * @param listener Listener for when an item in spinner is selected
     * @param items The List of items in the spinner
     * @param title The title next to the spinner
     * @param selectedPosition The starting position of the spinner
     */
    public SpinnerLineItem(
            Context context,
            AdapterView.OnItemSelectedListener listener,
            List<?> items,
            CharSequence title,
            int selectedPosition) {
        mArrayAdapter = new ArrayAdapter(context, R.layout.spinner, items);
        mArrayAdapter.setDropDownViewResource(R.layout.spinner_drop_down);
        mOnItemSelectedListener = listener;
        mTitle = title;
        mSelectedPosition = selectedPosition;
    }

    @Override
    public int getType() {
        return SPINNER_TYPE;
    }

    @Override
    public void bindViewHolder(ViewHolder viewHolder) {
        super.bindViewHolder(viewHolder);
        viewHolder.spinner.setAdapter(mArrayAdapter);
        viewHolder.spinner.setSelection(mSelectedPosition);
        viewHolder.spinner.setOnItemSelectedListener(mOnItemSelectedListener);
        viewHolder.titleView.setText(mTitle);
    }

    /**
     * ViewHolder that contains the elements of a SpinnerLineItem,
     * including the spinner and title.
     */
    public static class ViewHolder extends RecyclerView.ViewHolder {
        public final Spinner spinner;
        public final TextView titleView;

        public ViewHolder(View view) {
            super(view);
            spinner = view.findViewById(R.id.spinner);
            titleView = view.findViewById(R.id.title);
        }
    }

    /**
     * Creates a ViewHolder that contains all the elements of a SpinnerLineItem
     */
    public static RecyclerView.ViewHolder createViewHolder(ViewGroup parent) {
        return new ViewHolder(LayoutInflater.from(parent.getContext())
                .inflate(R.layout.spinner_line_item, parent, false));
    }

    /**
     * Returns the item in the given position
     */
    public T getItem(int position) {
        return mArrayAdapter.getItem(position);
    }

    @Override
    public CharSequence getDesc() {
        return null;
    }

    @Override
    public boolean isExpandable() {
        return false;
    }

    @Override
    public boolean isClickable() {
        return true;
    }
}
