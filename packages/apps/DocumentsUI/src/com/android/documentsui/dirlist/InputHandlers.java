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

import static android.support.v4.util.Preconditions.checkArgument;

import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.HapticFeedbackConstants;
import android.view.KeyEvent;
import android.view.MotionEvent;

import com.android.documentsui.ActionHandler;
import com.android.documentsui.base.EventHandler;
import com.android.documentsui.base.State;
import com.android.documentsui.selection.GestureSelectionHelper;
import com.android.documentsui.selection.ItemDetailsLookup;
import com.android.documentsui.selection.MouseInputHandler;
import com.android.documentsui.selection.SelectionHelper;
import com.android.documentsui.selection.TouchInputHandler;
import com.android.documentsui.selection.ItemDetailsLookup.ItemDetails;
import com.android.documentsui.selection.SelectionHelper.SelectionPredicate;

/**
 * Helper class dedicated to building gesture input handlers. The construction
 * of the various input handlers is non trivial. To keep logic clear,
 * code flexible, and DirectoryFragment small(er), the construction has been
 * isolated here in a separate class.
 */
final class InputHandlers {

    private ActionHandler mActions;
    private SelectionHelper mSelectionHelper;
    private SelectionPredicate mSelectionPredicate;
    private ItemDetailsLookup mDetailsLookup;
    private FocusHandler mFocusHandler;
    private RecyclerView mRecView;
    private State mState;

    InputHandlers(
            ActionHandler actions,
            SelectionHelper selectionHelper,
            SelectionPredicate selectionPredicate,
            ItemDetailsLookup detailsLookup,
            FocusHandler focusHandler,
            RecyclerView recView,
            State state) {

        checkArgument(actions != null);
        checkArgument(selectionHelper != null);
        checkArgument(selectionPredicate != null);
        checkArgument(detailsLookup != null);
        checkArgument(focusHandler != null);
        checkArgument(recView != null);
        checkArgument(state != null);

        mActions = actions;
        mSelectionHelper = selectionHelper;
        mSelectionPredicate = selectionPredicate;
        mDetailsLookup = detailsLookup;
        mFocusHandler = focusHandler;
        mRecView = recView;
        mState = state;
    }

    KeyInputHandler createKeyHandler() {
        KeyInputHandler.Callbacks callbacks = new KeyInputHandler.Callbacks() {
            @Override
            public boolean isInteractiveItem(ItemDetails item, KeyEvent e) {
                switch (item.getItemViewType()) {
                    case DocumentsAdapter.ITEM_TYPE_HEADER_MESSAGE:
                    case DocumentsAdapter.ITEM_TYPE_INFLATED_MESSAGE:
                    case DocumentsAdapter.ITEM_TYPE_SECTION_BREAK:
                        return false;
                    case DocumentsAdapter.ITEM_TYPE_DOCUMENT:
                    case DocumentsAdapter.ITEM_TYPE_DIRECTORY:
                        return true;
                    default:
                        throw new RuntimeException(
                                "Unsupported item type: " + item.getItemViewType());
                }
            }

            @Override
            public boolean onItemActivated(ItemDetails item, KeyEvent e) {
                // Handle enter key events
                switch (e.getKeyCode()) {
                    case KeyEvent.KEYCODE_ENTER:
                    case KeyEvent.KEYCODE_DPAD_CENTER:
                    case KeyEvent.KEYCODE_BUTTON_A:
                        return mActions.openItem(
                                item,
                                ActionHandler.VIEW_TYPE_REGULAR,
                                ActionHandler.VIEW_TYPE_PREVIEW);
                    case KeyEvent.KEYCODE_SPACE:
                        return mActions.openItem(
                                item,
                                ActionHandler.VIEW_TYPE_PREVIEW,
                                ActionHandler.VIEW_TYPE_NONE);
                }

                return false;
            }

            @Override
            public boolean onFocusItem(ItemDetails details, int keyCode, KeyEvent event) {
                ViewHolder holder =
                        mRecView.findViewHolderForAdapterPosition(details.getPosition());
                if (holder instanceof DocumentHolder) {
                    return mFocusHandler.handleKey((DocumentHolder) holder, keyCode, event);
                }
                return false;
            }

            @Override
            public void onPerformHapticFeedback() {
                mRecView.performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
            }
        };

        return new KeyInputHandler(mSelectionHelper, mSelectionPredicate, callbacks);
    }

    MouseInputHandler createMouseHandler(
            EventHandler<MotionEvent> showContextMenuCallback) {

        checkArgument(showContextMenuCallback != null);

        MouseInputHandler.Callbacks callbacks = new MouseInputHandler.Callbacks() {
            @Override
            public boolean onItemActivated(ItemDetails item, MotionEvent e) {
                return mActions.openItem(
                        item,
                        ActionHandler.VIEW_TYPE_REGULAR,
                        ActionHandler.VIEW_TYPE_PREVIEW);
            }

            @Override
            public boolean onContextClick(MotionEvent e) {
                return showContextMenuCallback.accept(e);
            }

            @Override
            public void onPerformHapticFeedback() {
                mRecView.performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
            }

            @Override
            public void focusItem(ItemDetails item) {
                mFocusHandler.focusDocument(item.getStableId());
            }

            @Override
            public void clearFocus() {
                mFocusHandler.clearFocus();
            }

            @Override
            public boolean hasFocusedItem() {
                return mFocusHandler.hasFocusedItem();
            }

            @Override
            public int getFocusedPosition() {
                return mFocusHandler.getFocusPosition();
            }
        };

        return new MouseInputHandler(mSelectionHelper, mDetailsLookup, callbacks);
    }

    /**
     * Factory method for input touch delegate. Exists to reduce complexity in the
     * calling scope.
     * @param gestureHelper
     */
    TouchInputHandler createTouchHandler(
            GestureSelectionHelper gestureHelper, DragStartListener dragStartListener) {
        checkArgument(dragStartListener != null);

        TouchInputHandler.Callbacks callbacks = new TouchInputHandler.Callbacks() {
            @Override
            public boolean onItemActivated(ItemDetails item, MotionEvent e) {
                return mActions.openItem(
                        item,
                        ActionHandler.VIEW_TYPE_PREVIEW,
                        ActionHandler.VIEW_TYPE_REGULAR);
            }

            @Override
            public boolean onDragInitiated(MotionEvent e) {
                return dragStartListener.onTouchDragEvent(e);
            }

            @Override
            public void onPerformHapticFeedback() {
                mRecView.performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
            }

            @Override
            public void focusItem(ItemDetails item) {
                mFocusHandler.focusDocument(item.getStableId());
            }

            @Override
            public void clearFocus() {
                mFocusHandler.clearFocus();
            }
        };

        return new TouchInputHandler(
                mSelectionHelper, mDetailsLookup, mSelectionPredicate, gestureHelper, callbacks);
    }
}
