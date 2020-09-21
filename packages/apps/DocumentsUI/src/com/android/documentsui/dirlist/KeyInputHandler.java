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
package com.android.documentsui.dirlist;

import android.view.KeyEvent;

import com.android.documentsui.base.Events;
import com.android.documentsui.selection.ItemDetailsLookup;
import com.android.documentsui.selection.MotionInputHandler;
import com.android.documentsui.selection.SelectionHelper;
import com.android.documentsui.selection.ItemDetailsLookup.ItemDetails;
import com.android.documentsui.selection.MotionInputHandler.Callbacks;
import com.android.documentsui.selection.SelectionHelper.SelectionPredicate;

import javax.annotation.Nullable;

/**
 * Class that handles keyboard events on RecyclerView items. The input handler
 * must be attached directly to a RecyclerView item since, unlike DOM, events
 * don't appear bubble up.
 */
public final class KeyInputHandler extends KeyboardEventListener {

    private final SelectionHelper mSelectionHelper;
    private final SelectionPredicate mSelectionPredicate;
    private final Callbacks mCallbacks;

    public KeyInputHandler(
            SelectionHelper selectionHelper,
            SelectionPredicate selectionPredicate,
            Callbacks callbacks) {

        mSelectionHelper = selectionHelper;
        mSelectionPredicate = selectionPredicate;
        mCallbacks = callbacks;
    }

    @Override
    public boolean onKey(@Nullable ItemDetails details, int keyCode, KeyEvent event) {
        // Only handle key-down events. This is simpler, consistent with most other UIs, and
        // enables the handling of repeated key events from holding down a key.
        if (event.getAction() != KeyEvent.ACTION_DOWN) {
            return false;
        }

        // Ignore tab key events.  Those should be handled by the top-level key handler.
        if (keyCode == KeyEvent.KEYCODE_TAB) {
            return false;
        }

        // Ignore events sent to Addon Holders.
        if (details != null) {
            int itemType = details.getItemViewType();
            if (itemType == DocumentsAdapter.ITEM_TYPE_HEADER_MESSAGE
                    || itemType == DocumentsAdapter.ITEM_TYPE_INFLATED_MESSAGE
                    || itemType == DocumentsAdapter.ITEM_TYPE_SECTION_BREAK) {
                return false;
            }
        }

        if (mCallbacks.onFocusItem(details, keyCode, event)) {
            // Handle range selection adjustments. Extending the selection will adjust the
            // bounds of the in-progress range selection. Each time an unshifted navigation
            // event is received, the range selection is restarted.
            if (shouldExtendSelection(details, event)) {
                if (!mSelectionHelper.isRangeActive()) {
                    // Start a range selection if one isn't active
                    mSelectionHelper.startRange(details.getPosition());
                }
                mSelectionHelper.extendRange(details.getPosition());
            } else {
                mSelectionHelper.endRange();
                mSelectionHelper.clearSelection();
            }
            return true;
        }

        // we don't yet have a mechanism to handle opening/previewing multiple documents at once
        if (mSelectionHelper.getSelection().size() > 1) {
            return false;
        }

        return mCallbacks.onItemActivated(details, event);
    }

    private boolean shouldExtendSelection(ItemDetails item, KeyEvent event) {
        if (!Events.isNavigationKeyCode(event.getKeyCode()) || !event.isShiftPressed()) {
            return false;
        }

        return mSelectionPredicate.canSetStateForId(item.getStableId(), true);
    }

    public static abstract class Callbacks extends MotionInputHandler.Callbacks {
        public abstract boolean isInteractiveItem(ItemDetails item, KeyEvent e);
        public abstract boolean onItemActivated(ItemDetails item, KeyEvent e);
        public boolean onFocusItem(ItemDetails details, int keyCode, KeyEvent event) {
            return true;
        }
    }
}