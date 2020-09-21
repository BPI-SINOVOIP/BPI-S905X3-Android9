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

import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.OnItemTouchListener;
import android.view.MotionEvent;

import com.android.documentsui.base.EventHandler;
import com.android.documentsui.base.Events;
import com.android.documentsui.selection.BandSelectionHelper;
import com.android.documentsui.selection.ItemDetailsLookup;
import com.android.documentsui.selection.TouchEventRouter;

/**
 * OnItemTouchListener that helps enable support for drag/drop functionality
 * in DirectoryFragment.
 *
 * <p>This class takes an OnItemTouchListener that is presumably the
 * one provided by {@link BandSelectionHelper#getListener()}, and
 * is used in conjunction with the {@link TouchEventRouter}.
 *
 * <p>See DirectoryFragment for more details on how this is glued
 * into DocumetnsUI event handling to make the magic happen.
 */
class MouseDragEventInterceptor implements OnItemTouchListener {

    private final ItemDetailsLookup mEventDetailsLookup;
    private final EventHandler<MotionEvent> mMouseDragListener;
    private final @Nullable OnItemTouchListener mDelegate;

    public MouseDragEventInterceptor(
            ItemDetailsLookup eventDetailsLookup,
            EventHandler<MotionEvent> mouseDragListener,
            @Nullable OnItemTouchListener delegate) {

        checkArgument(eventDetailsLookup != null);
        checkArgument(mouseDragListener != null);

        mEventDetailsLookup = eventDetailsLookup;
        mMouseDragListener = mouseDragListener;
        mDelegate = delegate;
    }

    @Override
    public boolean onInterceptTouchEvent(RecyclerView rv, MotionEvent e) {
        if (Events.isMouseDragEvent(e) && mEventDetailsLookup.inItemDragRegion(e)) {
            return mMouseDragListener.accept(e);
        } else if (mDelegate != null) {
            return mDelegate.onInterceptTouchEvent(rv, e);
        }
        return false;
    }

    @Override
    public void onTouchEvent(RecyclerView rv, MotionEvent e) {
        if (mDelegate != null) {
            mDelegate.onTouchEvent(rv, e);
        }
    }

    @Override
    public void onRequestDisallowInterceptTouchEvent(boolean disallowIntercept) {}
}
