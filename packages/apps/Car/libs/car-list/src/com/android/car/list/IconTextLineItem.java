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
import android.widget.ImageView;
import android.widget.TextView;

import com.android.car.list.R;

/**
 * Contains logic for a line item represents icon and texts of a title and a description.
 */
public abstract class IconTextLineItem
        extends TypedPagedListAdapter.LineItem<IconTextLineItem.ViewHolder> {
    private final CharSequence mTitle;

    /**
     * Constructs an IconTextLIneItem with just the title set.
     */
    public IconTextLineItem(CharSequence title) {
        mTitle = title;
    }

    @Override
    public int getType() {
        return ICON_TEXT_TYPE;
    }

    @Override
    public void bindViewHolder(ViewHolder viewHolder) {
        super.bindViewHolder(viewHolder);
        viewHolder.titleView.setText(mTitle);
        setIcon(viewHolder.iconView);
        CharSequence desc = getDesc();
        if (TextUtils.isEmpty(desc)) {
            viewHolder.descView.setVisibility(View.GONE);
        } else {
            viewHolder.descView.setVisibility(View.VISIBLE);
            viewHolder.descView.setText(desc);
        }
        viewHolder.rightArrow.setVisibility(
                isExpandable() ? View.VISIBLE : View.INVISIBLE);
        viewHolder.dividerLine.setVisibility(
                isClickable() && isEnabled() ? View.VISIBLE : View.INVISIBLE);
    }

    /**
     * ViewHolder that contains the elements that make up an IconTextLineItem,
     * including the title, description, icon, arrow, and divider.
     */
    public static class ViewHolder extends RecyclerView.ViewHolder {
        public final TextView titleView;
        final TextView descView;
        final ImageView iconView;
        final ImageView rightArrow;
        public final View dividerLine;

        public ViewHolder(View view) {
            super(view);
            iconView = (ImageView) view.findViewById(R.id.icon);
            titleView = (TextView) view.findViewById(R.id.title);
            descView = (TextView) view.findViewById(R.id.desc);
            rightArrow = (ImageView) view.findViewById(R.id.right_chevron);
            dividerLine = view.findViewById(R.id.line_item_divider);
        }
    }

    /**
     * Creates a ViewHolder with the elements of an IconTextLineItem
     */
    public static RecyclerView.ViewHolder createViewHolder(ViewGroup parent) {
        return new ViewHolder(LayoutInflater.from(parent.getContext())
                .inflate(R.layout.icon_text_line_item, parent, false));
    }

    /**
     * Method to be overwritten to set the icon
     */
    public abstract void setIcon(ImageView iconView);
}
