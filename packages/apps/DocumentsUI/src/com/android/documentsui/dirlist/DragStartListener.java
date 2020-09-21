/*
 * Copyright (C) 2016 The Android Open Source Project
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

import static com.android.documentsui.base.SharedMinimal.DEBUG;
import static com.android.internal.util.Preconditions.checkArgument;

import android.net.Uri;
import android.support.annotation.VisibleForTesting;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import com.android.documentsui.DragAndDropManager;
import com.android.documentsui.MenuManager.SelectionDetails;
import com.android.documentsui.Model;
import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.base.Events;
import com.android.documentsui.base.State;
import com.android.documentsui.selection.ItemDetailsLookup;
import com.android.documentsui.selection.MutableSelection;
import com.android.documentsui.selection.Selection;
import com.android.documentsui.selection.SelectionHelper;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Function;

import javax.annotation.Nullable;

/**
 * Listens for potential "drag-like" events and kick-start dragging as needed. Also allows external
 * direct call to {@code #startDrag(RecyclerView, View)} if explicit start is needed, such as long-
 * pressing on an item via touch. (e.g. InputEventDispatcher#onLongPress(MotionEvent)} via touch.
 */
interface DragStartListener {

    static final DragStartListener DUMMY = new DragStartListener() {
        @Override
        public boolean onMouseDragEvent(MotionEvent event) {
            return false;
        }
        @Override
        public boolean onTouchDragEvent(MotionEvent event) {
            return false;
        }
    };

    boolean onMouseDragEvent(MotionEvent event);
    boolean onTouchDragEvent(MotionEvent event);

    @VisibleForTesting
    class RuntimeDragStartListener implements DragStartListener {

        private static String TAG = "DragStartListener";

        private final IconHelper mIconHelper;
        private final State mState;
        private final ItemDetailsLookup mDetailsLookup;
        private final SelectionHelper mSelectionMgr;
        private final SelectionDetails mSelectionDetails;
        private final ViewFinder mViewFinder;
        private final Function<View, String> mIdFinder;
        private final Function<Selection, List<DocumentInfo>> mDocsConverter;
        private final DragAndDropManager mDragAndDropManager;


        // use DragStartListener.create
        @VisibleForTesting
        public RuntimeDragStartListener(
                IconHelper iconHelper,
                State state,
                ItemDetailsLookup detailsLookup,
                SelectionHelper selectionMgr,
                SelectionDetails selectionDetails,
                ViewFinder viewFinder,
                Function<View, String> idFinder,
                Function<Selection, List<DocumentInfo>> docsConverter,
                DragAndDropManager dragAndDropManager) {

            mIconHelper = iconHelper;
            mState = state;
            mDetailsLookup = detailsLookup;
            mSelectionMgr = selectionMgr;
            mSelectionDetails = selectionDetails;
            mViewFinder = viewFinder;
            mIdFinder = idFinder;
            mDocsConverter = docsConverter;
            mDragAndDropManager = dragAndDropManager;
        }

        @Override
        public final boolean onMouseDragEvent(MotionEvent event) {
            checkArgument(Events.isMouseDragEvent(event));
            checkArgument(mDetailsLookup.inItemDragRegion(event));

            return startDrag(mViewFinder.findView(event.getX(), event.getY()), event);
        }

        @Override
        public final boolean onTouchDragEvent(MotionEvent event) {
            return startDrag(mViewFinder.findView(event.getX(), event.getY()), event);
        }

        /**
         * May be called externally when drag is initiated from other event handling code.
         */
        private boolean startDrag(@Nullable View view, MotionEvent event) {

            if (view == null) {
                if (DEBUG) Log.d(TAG, "Ignoring drag event, null view.");
                return false;
            }

            @Nullable String modelId = mIdFinder.apply(view);
            if (modelId == null) {
                if (DEBUG) Log.d(TAG, "Ignoring drag on view not represented in model.");
                return false;
            }

            Selection selection = getSelectionToBeCopied(modelId, event);

            final List<DocumentInfo> srcs = mDocsConverter.apply(selection);

            final List<Uri> invalidDest = new ArrayList<>(srcs.size() + 1);
            for (DocumentInfo doc : srcs) {
                invalidDest.add(doc.derivedUri);
            }

            final DocumentInfo parent = mState.stack.peek();
            // parent is null when we're in Recents
            if (parent != null) {
                invalidDest.add(parent.derivedUri);
            }

            mDragAndDropManager.startDrag(view, srcs, mState.stack.getRoot(), invalidDest,
                    mSelectionDetails, mIconHelper, parent);

            return true;
        }

        /**
         * Given the MotionEvent (for CTRL case) and modelId of the view associated with the
         * coordinates of the event, return a valid selection for drag and drop operation
         */
        @VisibleForTesting
        MutableSelection getSelectionToBeCopied(String modelId, MotionEvent event) {
            MutableSelection selection = new MutableSelection();
            // If CTRL-key is held down and there's other existing selection, add item to
            // selection (if not already selected)
            if (Events.isCtrlKeyPressed(event)
                    && mSelectionMgr.hasSelection()
                    && !mSelectionMgr.isSelected(modelId)) {
                mSelectionMgr.select(modelId);
            }

            if (mSelectionMgr.isSelected(modelId)) {
                mSelectionMgr.copySelection(selection);
            } else {
                selection.add(modelId);
                mSelectionMgr.clearSelection();
            }
            return selection;
        }
    }

    static DragStartListener create(
            IconHelper iconHelper,
            Model model,
            SelectionHelper selectionMgr,
            SelectionDetails selectionDetails,
            State state,
            ItemDetailsLookup detailsLookup,
            Function<View, String> idFinder,
            ViewFinder viewFinder,
            DragAndDropManager dragAndDropManager) {

        return new RuntimeDragStartListener(
                iconHelper,
                state,
                detailsLookup,
                selectionMgr,
                selectionDetails,
                viewFinder,
                idFinder,
                model::getDocuments,
                dragAndDropManager);
    }

    @FunctionalInterface
    interface ViewFinder {
        @Nullable View findView(float x, float y);
    }
}
