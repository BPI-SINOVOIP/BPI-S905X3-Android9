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

import static android.support.v4.util.Preconditions.checkArgument;

import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.MotionEvent;

import com.android.documentsui.selection.ItemDetailsLookup.ItemDetails;

/**
 * Base class for handlers that can be registered w/ {@link GestureRouter}.
 */
public abstract class MotionInputHandler extends SimpleOnGestureListener {

    protected final SelectionHelper mSelectionHelper;
    protected final ItemDetailsLookup mDetailsLookup;

    private final Callbacks mCallbacks;

    public MotionInputHandler(
            SelectionHelper selectionHelper,
            ItemDetailsLookup detailsLookup,
            Callbacks callbacks) {

        checkArgument(selectionHelper != null);
        checkArgument(detailsLookup != null);
        checkArgument(callbacks != null);

        mSelectionHelper = selectionHelper;
        mDetailsLookup = detailsLookup;
        mCallbacks = callbacks;
    }

    protected final boolean selectItem(ItemDetails details) {
        checkArgument(details != null);
        checkArgument(details.hasPosition());
        checkArgument(details.hasStableId());

        if (mSelectionHelper.select(details.getStableId())) {
            mSelectionHelper.anchorRange(details.getPosition());
        }

        // we set the focus on this doc so it will be the origin for keyboard events or shift+clicks
        // if there is only a single item selected, otherwise clear focus
        if (mSelectionHelper.getSelection().size() == 1) {
            mCallbacks.focusItem(details);
        } else {
            mCallbacks.clearFocus();
        }
        return true;
    }

    protected final boolean focusItem(ItemDetails details) {
        checkArgument(details != null);
        checkArgument(details.hasStableId());

        mSelectionHelper.clearSelection();
        mCallbacks.focusItem(details);
        return true;
    }

    protected final void extendSelectionRange(ItemDetails details) {
        checkArgument(details.hasPosition());
        checkArgument(details.hasStableId());

        mSelectionHelper.extendRange(details.getPosition());
        mCallbacks.focusItem(details);
    }

    protected final boolean isRangeExtension(MotionEvent e) {
        return MotionEvents.isShiftKeyPressed(e) && mSelectionHelper.isRangeActive();
    }

    protected boolean shouldClearSelection(MotionEvent e, ItemDetails item) {
        return !MotionEvents.isCtrlKeyPressed(e)
                && !item.inSelectionHotspot(e)
                && !mSelectionHelper.isSelected(item.getStableId());
    }

    public static abstract class Callbacks {
        public abstract void onPerformHapticFeedback();
        public void focusItem(ItemDetails item) {}
        public void clearFocus() {}
    }
}
