/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car.list;

import android.graphics.drawable.Drawable;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

/**
 * A line item which presents an action button at the end of the line.
 */
public abstract class ActionIconButtonLineItem
        extends TypedPagedListAdapter.LineItem<ActionIconButtonLineItem.ViewHolder> {

    private final CharSequence mTitle;
    private final CharSequence mPrimaryAction;
    private final CharSequence mSecondaryAction;
    private final Drawable mIconDrawable;

    /**
     * Constructs an ActionIconButtonLineItem with just the title set.
     */
    public ActionIconButtonLineItem(
            CharSequence title,
            CharSequence primaryAction,
            CharSequence secondaryAction,
            Drawable iconDrawable) {
        mTitle = title;
        mPrimaryAction = primaryAction;
        mSecondaryAction = secondaryAction;
        mIconDrawable = iconDrawable;
    }

    @Override
    public int getType() {
        return ACTION_BUTTON_TYPE;
    }

    @Override
    public void bindViewHolder(ActionIconButtonLineItem.ViewHolder viewHolder) {
        super.bindViewHolder(viewHolder);
        viewHolder.mActionButton1.setText(mPrimaryAction);
        viewHolder.mActionButton2.setText(mSecondaryAction);
        viewHolder.mTitleView.setText(mTitle);
        viewHolder.mEndIconView.setImageDrawable(mIconDrawable);
        viewHolder.mActionButton1.setText(mPrimaryAction);
        viewHolder.mActionButton2.setText(mSecondaryAction);
        CharSequence desc = getDesc();
        if (TextUtils.isEmpty(desc)) {
            viewHolder.mDescView.setVisibility(View.GONE);
        } else {
            viewHolder.mDescView.setVisibility(View.VISIBLE);
            viewHolder.mDescView.setText(desc);
        }
        viewHolder.mActionButton2.setOnClickListener(
                v -> onSecondaryActionButtonClick(viewHolder.getAdapterPosition()));

        viewHolder.mActionButton1.setOnClickListener(
                v -> onPrimaryActionButtonClick(viewHolder.mView));
    }

    /**
     * ViewHolder that contains the elements that make up an ActionIconButtonLineItem,
     * including the title, description, icon, end action button, and divider.
     */
    public static class ViewHolder extends RecyclerView.ViewHolder {
        final TextView mTitleView;
        final TextView mDescView;
        final ImageView mEndIconView;
        final Button mActionButton1;
        final Button mActionButton2;
        final View mView;

        public ViewHolder(View view) {
            super(view);
            mView = view;
            mEndIconView = (ImageView) mView.findViewById(R.id.end_icon);
            mTitleView = (TextView) mView.findViewById(R.id.title);
            mDescView = (TextView) mView.findViewById(R.id.desc);
            mActionButton1 = (Button) mView.findViewById(R.id.action_button_1);
            mActionButton2 = (Button) mView.findViewById(R.id.action_button_2);
        }
    }

    /**
     * Creates a ViewHolder with the elements of an ActionIconButtonLineItem
     */
    public static RecyclerView.ViewHolder createViewHolder(ViewGroup parent) {
        return new ActionIconButtonLineItem.ViewHolder(LayoutInflater.from(parent.getContext())
                .inflate(R.layout.action_icon_button_line_item, parent, false));
    }

    /**
     * Invoked when the secondary action button's onClickListener is invoked.
     */
    public abstract void onSecondaryActionButtonClick(int adapterPosition);

    /**
     * Invoked when the primary action button's onClickListener is invoked.
     */
    public abstract void onPrimaryActionButtonClick(View view);
}
