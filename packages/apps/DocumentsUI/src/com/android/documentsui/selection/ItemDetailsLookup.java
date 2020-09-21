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
package com.android.documentsui.selection;

import android.support.v7.widget.RecyclerView;
import android.view.MotionEvent;

import javax.annotation.Nullable;

/**
 * Provides event handlers w/ access to details about documents details
 * view items Documents in the UI (RecyclerView).
 */
public abstract class ItemDetailsLookup {

    /** @return true if there is an item under the finger/cursor. */
    public abstract boolean overItem(MotionEvent e);

    /** @return true if there is an item w/ a stable ID under the finger/cursor. */
    public abstract boolean overStableItem(MotionEvent e);

    /**
     * @return true if the event is over an area that can be dragged via touch
     * or via mouse. List items have a white area that is not draggable.
     */
    public abstract boolean inItemDragRegion(MotionEvent e);

    /**
     * @return true if the event is in the "selection hot spot" region.
     * The hot spot region instantly selects in touch mode, vs launches.
     */
    public abstract boolean inItemSelectRegion(MotionEvent e);

    /**
     * @return the adapter position of the item under the finger/cursor.
     */
    public abstract int getItemPosition(MotionEvent e);

    /**
     * @return the DocumentDetails for the item under the event, or null.
     */
    public abstract @Nullable ItemDetails getItemDetails(MotionEvent e);

    /**
     * Abstract class providing helper classes with access to information about
     * RecyclerView item associated with a MotionEvent.
     */
    public static abstract class ItemDetails {

        public boolean hasPosition() {
            return getPosition() > RecyclerView.NO_POSITION;
        }

        public abstract int getPosition();

        public boolean hasStableId() {
            return getStableId() != null;
        }

        public abstract @Nullable String getStableId();

        /**
         * @return The view type of this ViewHolder.
         */
        public abstract int getItemViewType();

        /**
         * @return true if the event is in an area of the item that should be
         * directly interpreted as a user wishing to select the item. This
         * is useful for checkboxes and other UI affordances focused on enabling
         * selection.
         */
        public boolean inSelectionHotspot(MotionEvent e) {
            return false;
        }

        /**
         * Events in the drag region will not be processed as selection events. This
         * allows the client to implement custom handling for events related to drag
         * and drop.
         */
        public boolean inDragRegion(MotionEvent e) {
            return false;
        }

        @Override
        public boolean equals(Object obj) {
            if (obj instanceof ItemDetails) {
                return equals((ItemDetails) obj);
            }
            return false;
        }

        private boolean equals(ItemDetails other) {
            return this.getPosition() == other.getPosition()
                    && this.getStableId() == other.getStableId();
        }

        @Override
        public int hashCode() {
            return getPosition() >>> 8;
        }
    }
}
