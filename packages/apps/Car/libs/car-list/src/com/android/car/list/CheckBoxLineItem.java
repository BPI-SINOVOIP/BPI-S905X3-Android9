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
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.TextView;

import com.android.car.list.R;

/**
 * Contains logic for a line item represents title text and a checkbox.
 */
public abstract class CheckBoxLineItem
        extends TypedPagedListAdapter.LineItem<CheckBoxLineItem.CheckboxLineItemViewHolder> {
    private final CharSequence mTitle;

    /**
     * Constructs CheckBoxLineItem
     *
     * @param title The title text displayed with the checkbox
     */
    public CheckBoxLineItem(CharSequence title) {
        mTitle = title;
    }

    @Override
    @LineItemType
    public int getType() {
        return CHECKBOX_TYPE;
    }

    @Override
    public void bindViewHolder(CheckboxLineItemViewHolder viewHolder) {
        super.bindViewHolder(viewHolder);
        viewHolder.titleView.setText(mTitle);
        viewHolder.checkbox.setChecked(isChecked());
    }

    /**
     * Contains logic for a ViewHolder that contains the elements of a title
     * and checkbox
     */
    public static class CheckboxLineItemViewHolder extends RecyclerView.ViewHolder {
        final TextView titleView;
        public final CheckBox checkbox;

        /**
         * Constructs CheckboxLineItemViewHolder
         *
         * @param view The view that contains the items of the CheckBoxLineItem
         */
        public CheckboxLineItemViewHolder(View view) {
            super(view);
            titleView = view.findViewById(R.id.title);
            checkbox = view.findViewById(R.id.checkbox);
        }
    }

    /**
     *  Static factory method that creates the elements of a CheckBoxLineItem and
     *  attaches them to a ViewHolder
     *
     * @param parent The parent ViewGroup
     * @return A ViewHolder with the specified contents
     */
    public static RecyclerView.ViewHolder createViewHolder(ViewGroup parent) {
        return new CheckboxLineItemViewHolder(LayoutInflater.from(parent.getContext())
                .inflate(R.layout.checkbox_line_item, parent, false));
    }

    /**
     * @return whether the CheckBox is checked
     */
    public abstract boolean isChecked();

    @Override
    public boolean isClickable() {
        return true;
    }

    @Override
    public CharSequence getDesc() {
        return null;
    }
}
