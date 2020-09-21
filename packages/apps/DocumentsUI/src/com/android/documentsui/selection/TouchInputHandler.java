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

import static com.android.documentsui.base.SharedMinimal.DEBUG;

import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.MotionEvent;

import com.android.documentsui.selection.ItemDetailsLookup.ItemDetails;
import com.android.documentsui.selection.SelectionHelper.SelectionPredicate;

/**
 * A MotionInputHandler that provides the high-level glue for touch driven selection. This class
 * works with {@link RecyclerView}, {@link GestureRouter}, and {@link GestureSelectionHelper} to
 * provide robust user drive selection support.
 */
public final class TouchInputHandler extends MotionInputHandler {
    private static final String TAG = "TouchInputDelegate";

    private final SelectionPredicate mSelectionPredicate;
    private final Callbacks mCallbacks;
    private final Runnable mGestureKicker;

    public TouchInputHandler(
            SelectionHelper selectionHelper,
            ItemDetailsLookup detailsLookup,
            SelectionPredicate selectionPredicate,
            Runnable gestureKicker,
            Callbacks callbacks) {

        super(selectionHelper, detailsLookup, callbacks);

        mSelectionPredicate = selectionPredicate;
        mGestureKicker = gestureKicker;
        mCallbacks = callbacks;
    }

    public TouchInputHandler(
            SelectionHelper selectionHelper,
            ItemDetailsLookup detailsLookup,
            SelectionPredicate selectionPredicate,
            GestureSelectionHelper gestureHelper,
            Callbacks callbacks) {

        this(
                selectionHelper,
                detailsLookup,
                selectionPredicate,
                new Runnable() {
                    @Override
                    public void run() {
                        gestureHelper.start();
                    }
                },
                callbacks);
    }

    @Override
    public boolean onSingleTapUp(MotionEvent e) {
        if (!mDetailsLookup.overStableItem(e)) {
            if (DEBUG) Log.d(TAG, "Tap not associated w/ model item. Clearing selection.");
            mSelectionHelper.clearSelection();
            return false;
        }

        ItemDetails item = mDetailsLookup.getItemDetails(e);
        if (mSelectionHelper.hasSelection()) {
            if (isRangeExtension(e)) {
                extendSelectionRange(item);
            } else if (mSelectionHelper.isSelected(item.getStableId())) {
                mSelectionHelper.deselect(item.getStableId());
            } else {
                selectItem(item);
            }

            return true;
        }

        // Touch events select if they occur in the selection hotspot,
        // otherwise they activate.
        return item.inSelectionHotspot(e)
                ? selectItem(item)
                : mCallbacks.onItemActivated(item, e);
    }

    @Override
    public final void onLongPress(MotionEvent e) {
        if (!mDetailsLookup.overStableItem(e)) {
            if (DEBUG) Log.d(TAG, "Ignoring LongPress on non-model-backed item.");
            return;
        }

        ItemDetails item = mDetailsLookup.getItemDetails(e);
        boolean handled = false;

        if (isRangeExtension(e)) {
            extendSelectionRange(item);
            handled = true;
        } else {
            if (!mSelectionHelper.isSelected(item.getStableId())
                    && mSelectionPredicate.canSetStateForId(item.getStableId(), true)) {
                // If we cannot select it, we didn't apply anchoring - therefore should not
                // start gesture selection
                if (selectItem(item)) {
                    // And finally if the item was selected && we can select multiple
                    // we kick off gesture selection.
                    if (mSelectionPredicate.canSelectMultiple()) {
                        mGestureKicker.run();
                    }
                    handled = true;
                }
            } else {
                // We only initiate drag and drop on long press for touch to allow regular
                // touch-based scrolling
                // mTouchDragListener.accept(e);
                mCallbacks.onDragInitiated(e);
                handled = true;
            }
        }

        if (handled) {
            mCallbacks.onPerformHapticFeedback();
        }
    }

    public static abstract class Callbacks extends MotionInputHandler.Callbacks {
        public abstract boolean onItemActivated(ItemDetails item, MotionEvent e);
        public boolean onDragInitiated(MotionEvent e) {
            return false;
        }
    }
}
