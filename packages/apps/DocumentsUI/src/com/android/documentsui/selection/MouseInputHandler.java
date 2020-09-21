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

import static com.android.documentsui.selection.Shared.DEBUG;
import static com.android.documentsui.selection.Shared.VERBOSE;

import android.util.Log;
import android.view.MotionEvent;

import com.android.documentsui.selection.ItemDetailsLookup.ItemDetails;
import com.android.internal.widget.RecyclerView;

import javax.annotation.Nullable;

/**
 * A MotionInputHandler that provides the high-level glue for mouse/stylus driven selection. This
 * class works with {@link RecyclerView}, {@link GestureRouter}, and {@link GestureSelectionHelper}
 * to provide robust user drive selection support.
 */
public final class MouseInputHandler extends MotionInputHandler {

        private static final String TAG = "MouseInputDelegate";

        private final Callbacks mCallbacks;

        // The event has been handled in onSingleTapUp
        private boolean mHandledTapUp;
        // true when the previous event has consumed a right click motion event
        private boolean mHandledOnDown;

        public MouseInputHandler(
                SelectionHelper selectionHelper,
                ItemDetailsLookup detailsLookup,
                Callbacks callbacks) {

            super(selectionHelper, detailsLookup, callbacks);

            mCallbacks = callbacks;
        }

        @Override
        public boolean onDown(MotionEvent e) {
            if (VERBOSE) Log.v(TAG, "Delegated onDown event.");
            if ((MotionEvents.isAltKeyPressed(e) && MotionEvents.isPrimaryButtonPressed(e))
                    || MotionEvents.isSecondaryButtonPressed(e)) {
                mHandledOnDown = true;
                return onRightClick(e);
            }

            return false;
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            // Don't scroll content window in response to mouse drag
            // If it's two-finger trackpad scrolling, we want to scroll
            return !MotionEvents.isTouchpadScroll(e2);
        }

        @Override
        public boolean onSingleTapUp(MotionEvent e) {
            // See b/27377794. Since we don't get a button state back from UP events, we have to
            // explicitly save this state to know whether something was previously handled by
            // DOWN events or not.
            if (mHandledOnDown) {
                if (VERBOSE) Log.v(TAG, "Ignoring onSingleTapUp, previously handled in onDown.");
                mHandledOnDown = false;
                return false;
            }

            if (!mDetailsLookup.overStableItem(e)) {
                if (DEBUG) Log.d(TAG, "Tap not associated w/ model item. Clearing selection.");
                mSelectionHelper.clearSelection();
                mCallbacks.clearFocus();
                return false;
            }

            if (MotionEvents.isTertiaryButtonPressed(e)) {
                if (DEBUG) Log.d(TAG, "Ignoring middle click");
                return false;
            }

            ItemDetails item = mDetailsLookup.getItemDetails(e);
            if (mSelectionHelper.hasSelection()) {
                if (isRangeExtension(e)) {
                    extendSelectionRange(item);
                } else {
                    if (shouldClearSelection(e, item)) {
                        mSelectionHelper.clearSelection();
                    }
                    if (mSelectionHelper.isSelected(item.getStableId())) {
                        if (mSelectionHelper.deselect(item.getStableId())) {
                            mCallbacks.clearFocus();
                        }
                    } else {
                        selectOrFocusItem(item, e);
                    }
                }
                mHandledTapUp = true;
                return true;
            }

            return false;
        }

        @Override
        public boolean onSingleTapConfirmed(MotionEvent e) {
            if (mHandledTapUp) {
                if (VERBOSE) Log.v(TAG,
                        "Ignoring onSingleTapConfirmed, previously handled in onSingleTapUp.");
                mHandledTapUp = false;
                return false;
            }

            if (mSelectionHelper.hasSelection()) {
                return false;  // should have been handled by onSingleTapUp.
            }

            if (!mDetailsLookup.overItem(e)) {
                if (DEBUG) Log.d(TAG, "Ignoring Confirmed Tap on non-item.");
                return false;
            }

            if (MotionEvents.isTertiaryButtonPressed(e)) {
                if (DEBUG) Log.d(TAG, "Ignoring middle click");
                return false;
            }

            @Nullable ItemDetails item = mDetailsLookup.getItemDetails(e);
            if (item == null || !item.hasStableId()) {
                Log.w(TAG, "Ignoring Confirmed Tap. No document details associated w/ event.");
                return false;
            }

            if (mCallbacks.hasFocusedItem() && MotionEvents.isShiftKeyPressed(e)) {
                mSelectionHelper.startRange(mCallbacks.getFocusedPosition());
                mSelectionHelper.extendRange(item.getPosition());
            } else {
                selectOrFocusItem(item, e);
            }
            return true;
        }

        @Override
        public boolean onDoubleTap(MotionEvent e) {
            mHandledTapUp = false;

            if (!mDetailsLookup.overStableItem(e)) {
                if (DEBUG) Log.d(TAG, "Ignoring DoubleTap on non-model-backed item.");
                return false;
            }

            if (MotionEvents.isTertiaryButtonPressed(e)) {
                if (DEBUG) Log.d(TAG, "Ignoring middle click");
                return false;
            }

            @Nullable ItemDetails item = mDetailsLookup.getItemDetails(e);
            if (item != null) {
                return mCallbacks.onItemActivated(item, e);
            }

            return false;
        }

        private boolean onRightClick(MotionEvent e) {
            if (mDetailsLookup.overStableItem(e)) {
                @Nullable ItemDetails item = mDetailsLookup.getItemDetails(e);
                if (item != null && !mSelectionHelper.isSelected(item.getStableId())) {
                    mSelectionHelper.clearSelection();
                    selectItem(item);
                }
            }

            // We always delegate final handling of the event,
            // since the handler might want to show a context menu
            // in an empty area or some other weirdo view.
            return mCallbacks.onContextClick(e);
        }

        private void selectOrFocusItem(ItemDetails item, MotionEvent e) {
            if (item.inSelectionHotspot(e) || MotionEvents.isCtrlKeyPressed(e)) {
                selectItem(item);
            } else {
                focusItem(item);
            }
        }

        public static abstract class Callbacks extends MotionInputHandler.Callbacks {
            public abstract boolean onItemActivated(ItemDetails item, MotionEvent e);
            public boolean onContextClick(MotionEvent e) {
                return false;
            }
            public boolean hasFocusedItem() {
                return false;
            }
            public int getFocusedPosition() {
                return RecyclerView.NO_POSITION;
            }
        }
    }