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

package com.android.documentsui.selection;

import static android.support.v4.util.Preconditions.checkArgument;

import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.OnItemTouchListener;
import android.view.GestureDetector;
import android.view.MotionEvent;

/**
 * A class responsible for routing MotionEvents to tool-type specific handlers,
 * and if not handled by a handler, on to a {@link GestureDetector} for further
 * processing.
 *
 * <p>TouchEventRouter takes its name from
 * {@link RecyclerView#addOnItemTouchListener(OnItemTouchListener)}. Despite "Touch"
 * being in the name, it receives MotionEvents for all types of tools.
 */
public final class TouchEventRouter implements OnItemTouchListener {

    private final GestureDetector mDetector;
    private final ToolHandlerRegistry<OnItemTouchListener> mDelegates;

    public TouchEventRouter(GestureDetector detector, OnItemTouchListener defaultDelegate) {
        checkArgument(detector != null);
        checkArgument(defaultDelegate != null);

        mDetector = detector;
        mDelegates = new ToolHandlerRegistry<>(defaultDelegate);
    }

    public TouchEventRouter(GestureDetector detector) {
        this(
                detector,
                // Default listener does nothing.
                new OnItemTouchListener() {
                    @Override
                    public boolean onInterceptTouchEvent(RecyclerView rv, MotionEvent e) {
                        return false;
                    }

                    @Override
                    public void onTouchEvent(RecyclerView rv, MotionEvent e) {
                    }

                    @Override
                    public void onRequestDisallowInterceptTouchEvent(boolean disallowIntercept) {
                    }
                });
    }

    /**
     * @param toolType See MotionEvent for details on available types.
     * @param delegate An {@link OnItemTouchListener} to receive events
     *     of {@code toolType}.
     */
    public void register(int toolType, OnItemTouchListener delegate) {
        checkArgument(delegate != null);
        mDelegates.set(toolType, delegate);
    }

    @Override
    public boolean onInterceptTouchEvent(RecyclerView rv, MotionEvent e) {
        boolean handled = mDelegates.get(e).onInterceptTouchEvent(rv, e);

        // Forward all events to UserInputHandler.
        // This is necessary since UserInputHandler needs to always see the first DOWN event. Or
        // else all future UP events will be tossed.
        handled |= mDetector.onTouchEvent(e);

        return handled;
    }

    @Override
    public void onTouchEvent(RecyclerView rv, MotionEvent e) {
        mDelegates.get(e).onTouchEvent(rv, e);

        // Note: even though this event is being handled as part of gestures such as drag and band,
        // continue forwarding to the GestureDetector. The detector needs to see the entire cluster
        // of events in order to properly interpret other gestures, such as long press.
        mDetector.onTouchEvent(e);
    }

    @Override
    public void onRequestDisallowInterceptTouchEvent(boolean disallowIntercept) {}
}
