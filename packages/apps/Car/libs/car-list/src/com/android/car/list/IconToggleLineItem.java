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

import android.annotation.DrawableRes;
import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.Switch;
import android.widget.TextView;

import com.android.car.list.R;

/**
 * Contains logic for a line item represents title text, description text and a toggle widget.
 */
public abstract class IconToggleLineItem
        extends TypedPagedListAdapter.LineItem<IconToggleLineItem.ViewHolder> {
    protected final Context mContext;
    private final CharSequence mTitle;
    protected IconUpdateListener mIconUpdateListener;
    protected SwitchStateUpdateListener mSwitchStateUpdateListener;

    /**
     * Interface for enabling icon to be changed by the LineItem.
     */
    public interface IconUpdateListener {
        /**
         * Change the current icon.
         *
         * @param iconRes The new icon
         */
        void onUpdateIcon(@DrawableRes int iconRes);
    }

    /**
     * Interface for changing the state of the switch toggle.
     */
    public interface SwitchStateUpdateListener {

        void onToggleChanged(boolean checked);
    }

    private final Switch.OnTouchListener mOnTouchListener =
            (view, event) -> onToggleTouched((Switch)view, event);

    /**
     * Constructor for IconToggleLineItem
     *
     * @param title Title text
     * @param context Android context
     */
    public IconToggleLineItem(CharSequence title, Context context) {
        mTitle = title;
        mContext = context;
    }

    @Override
    @LineItemType
    public int getType() {
        return ICON_TOGGLE_TYPE;
    }

    @Override
    public void bindViewHolder(ViewHolder viewHolder) {
        super.bindViewHolder(viewHolder);
        viewHolder.title.setText(mTitle);
        CharSequence desc = getDesc();
        if (TextUtils.isEmpty(desc)) {
            viewHolder.summary.setVisibility(View.GONE);
        } else {
            viewHolder.summary.setVisibility(View.VISIBLE);
            viewHolder.summary.setText(desc);
        }
        viewHolder.toggle.setEnabled(true);
        viewHolder.toggle.setChecked(isChecked());
        viewHolder.onUpdateIcon(getIcon());
        viewHolder.toggle.setOnTouchListener(mOnTouchListener);
        mIconUpdateListener = viewHolder;
        mSwitchStateUpdateListener = viewHolder;
    }

    /**
     * ViewHolder class that holds all of the elements of an IconToggleLineItem with
     * the ability to be called to change the icon.
     */
    static class ViewHolder extends RecyclerView.ViewHolder
            implements IconUpdateListener, SwitchStateUpdateListener {
        public final ImageView icon;
        public final TextView title;
        public final TextView summary;
        public final Switch toggle;

        public ViewHolder(View itemView) {
            super(itemView);
            icon = (ImageView) itemView.findViewById(R.id.icon);
            title = (TextView) itemView.findViewById(R.id.title);
            summary = (TextView) itemView.findViewById(R.id.desc);
            toggle = (Switch) itemView.findViewById(R.id.toggle_switch);
        }

        @Override
        public void onUpdateIcon(@DrawableRes int iconRes) {
            icon.setImageResource(iconRes);
        }

        @Override
        public void onToggleChanged(boolean checked) {
            toggle.setChecked(checked);
        }
    }

    public static RecyclerView.ViewHolder createViewHolder(ViewGroup parent) {
        return new ViewHolder(LayoutInflater.from(parent.getContext())
                .inflate(R.layout.icon_toggle_line_item, parent, false));
    }

    /**
     * Called when toggle widget is touched.
     * @return True if the listener has consumed the event, false otherwise.
     */
    public abstract boolean onToggleTouched(Switch toggleSwitch, MotionEvent event);

    /**
     * Return true if checked, false if not.
     */
    public abstract boolean isChecked();

    /**
     * Return the resource id of what should be the current icon, can be used to
     * update the icon if it should be changing.
     */
    @DrawableRes
    public abstract int getIcon();

    @Override
    public boolean isClickable() {
        return true;
    }
}
