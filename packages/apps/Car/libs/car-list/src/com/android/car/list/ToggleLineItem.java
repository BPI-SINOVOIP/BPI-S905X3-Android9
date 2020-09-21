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

import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Switch;
import android.widget.TextView;

import com.android.car.list.R;

/**
 * Contains logic for a line item represents title text, description text and a checkbox widget.
 */
public abstract class ToggleLineItem
        extends TypedPagedListAdapter.LineItem<ToggleLineItem.ToggleLineItemViewHolder> {
    private final CharSequence mTitle;

    /**
     * Constructs a new ToggleLineItem with the given title
     */
    public ToggleLineItem(CharSequence title) {
        mTitle = title;
    }

    @Override
    @LineItemType
    public int getType() {
        return TOGGLE_TYPE;
    }

    @Override
    public void bindViewHolder(ToggleLineItemViewHolder viewHolder) {
        super.bindViewHolder(viewHolder);
        viewHolder.titleView.setText(mTitle);
        CharSequence desc = getDesc();
        if (TextUtils.isEmpty(desc)) {
            viewHolder.descView.setVisibility(View.GONE);
        } else {
            viewHolder.descView.setVisibility(View.VISIBLE);
            viewHolder.descView.setText(desc);
        }
        viewHolder.toggle.setChecked(isChecked());
    }

    /**
     * ViewHolder that contains the elements of a ToggleLineItem,
     * including the title, toggle, and description.
     */
    public static class ToggleLineItemViewHolder extends RecyclerView.ViewHolder {
        public final TextView titleView;
        public final TextView descView;
        public final Switch toggle;

        public ToggleLineItemViewHolder(View view) {
            super(view);
            titleView = (TextView) view.findViewById(R.id.title);
            descView = (TextView) view.findViewById(R.id.desc);
            toggle = (Switch) view.findViewById(R.id.toggle_switch);
        }
    }

    /**
     * Creates a ViewHolder that holds the elements of a ToggleLineItem
     */
    public static RecyclerView.ViewHolder createViewHolder(ViewGroup parent) {
        return new ToggleLineItemViewHolder(LayoutInflater.from(parent.getContext())
                .inflate(R.layout.toggle_line_item, parent, false));
    }

    /**
     * Return true if the toggle should be checked, can be used to set the actual
     * state of the toggle.
     */
    public abstract boolean isChecked();

    @Override
    public boolean isClickable() {
        return true;
    }
}
