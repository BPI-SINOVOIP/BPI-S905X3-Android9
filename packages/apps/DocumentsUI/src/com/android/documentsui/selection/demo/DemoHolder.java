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

import android.graphics.Rect;
import android.support.v7.widget.RecyclerView;
import android.view.MotionEvent;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.android.documentsui.R;
import com.android.documentsui.selection.ItemDetailsLookup.ItemDetails;

public final class DemoHolder extends RecyclerView.ViewHolder {

        private final LinearLayout mContainer;
        public final TextView mSelector;
        public final TextView mLabel;
        private final Details mDetails;
        private DemoItem mItem;

        DemoHolder(LinearLayout layout) {
            super(layout);
            mContainer = layout.findViewById(R.id.container);
            mSelector = layout.findViewById(R.id.selector);
            mLabel = layout.findViewById(R.id.label);
            mDetails = new Details(this);
        }

        public void update(DemoItem demoItem) {
            mItem = demoItem;
            mLabel.setText(mItem.getName());
        }

        void setSelected(boolean selected) {
            mContainer.setActivated(selected);
            mSelector.setActivated(selected);
        }

        public String getStableId() {
            return mItem != null ? mItem.getId() : null;
        }

        public boolean inDragRegion(MotionEvent e) {
            // If itemView is activated = selected, then whole region is interactive
            if (mContainer.isActivated()) {
                return true;
            }

            if (inSelectRegion(e)) {
                return true;
            }

            Rect rect = new Rect();
            mLabel.getPaint().getTextBounds(
                    mLabel.getText().toString(), 0, mLabel.getText().length(), rect);

            // If the tap occurred inside the text, these are interactive spots.
            return rect.contains((int) e.getRawX(), (int) e.getRawY());
        }

        public boolean inSelectRegion(MotionEvent e) {
            Rect iconRect = new Rect();
            mSelector.getGlobalVisibleRect(iconRect);
            return iconRect.contains((int) e.getRawX(), (int) e.getRawY());
        }

        Details getItemDetails() {
            return mDetails;
        }

        private static final class Details extends ItemDetails {

            private final DemoHolder mHolder;

            Details(DemoHolder holder) {
                mHolder = holder;
            }

            @Override
            public int getPosition() {
                return mHolder.getAdapterPosition();
            }

            @Override
            public String getStableId() {
                return mHolder.getStableId();
            }

            @Override
            public int getItemViewType() {
                return mHolder.getItemViewType();
            }

            @Override
            public boolean inDragRegion(MotionEvent e) {
                return mHolder.inDragRegion(e);
            }

            @Override
            public boolean inSelectionHotspot(MotionEvent e) {
                return mHolder.inSelectRegion(e);
            }
        }
    }