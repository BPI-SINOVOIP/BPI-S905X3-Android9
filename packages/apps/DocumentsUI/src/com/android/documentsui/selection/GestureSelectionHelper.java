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
import static android.support.v4.util.Preconditions.checkState;

import android.graphics.Point;
import android.os.Build;
import android.support.annotation.VisibleForTesting;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.OnItemTouchListener;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import com.android.documentsui.selection.ViewAutoScroller.ScrollHost;
import com.android.documentsui.selection.ViewAutoScroller.ScrollerCallbacks;

/**
 * GestureSelectionHelper provides logic that interprets a combination
 * of motions and gestures in order to provide gesture driven selection support
 * when used in conjunction with RecyclerView and other classes in the ReyclerView
 * selection support package.
 */
public final class GestureSelectionHelper extends ScrollHost implements OnItemTouchListener {

    private static final String TAG = "GestureSelectionHelper";

    private final SelectionHelper mSelectionMgr;
    private final Runnable mScroller;
    private final ViewDelegate mView;
    private final ContentLock mLock;
    private final ItemDetailsLookup mItemLookup;

    private int mLastTouchedItemPosition = -1;
    private boolean mStarted = false;
    private Point mLastInterceptedPoint;

    /**
     * See {@link #create(SelectionHelper, RecyclerView, ContentLock)} for convenience
     * method.
     */
    @VisibleForTesting
    GestureSelectionHelper(
            SelectionHelper selectionHelper,
            ViewDelegate view,
            ContentLock lock,
            ItemDetailsLookup itemLookup) {

        checkArgument(selectionHelper != null);
        checkArgument(view != null);
        checkArgument(lock != null);
        checkArgument(itemLookup != null);

        mSelectionMgr = selectionHelper;
        mView = view;
        mLock = lock;
        mItemLookup = itemLookup;

        mScroller = new ViewAutoScroller(this, mView);
    }

    /**
     * Explicitly kicks off a gesture multi-select.
     *
     * @return true if started.
     */
    public void start() {
        checkState(!mStarted);
        // See: b/70518185. It appears start() is being called via onLongPress
        // even though we never received an intial handleInterceptedDownEvent
        // where we would usually initialize mLastStartedItemPos.
        if (mLastTouchedItemPosition < 0){
          Log.w(TAG, "Illegal state. Can't start without valid mLastStartedItemPos.");
          return;
        }

        // Partner code in MotionInputHandler ensures items
        // are selected and range established prior to
        // start being called.
        // Verify the truth of that statement here
        // to make the implicit coupling less of a time bomb.
        checkState(mSelectionMgr.isRangeActive());

        mLock.checkUnlocked();

        mStarted = true;
        mLock.block();
    }

    @Override
    public boolean onInterceptTouchEvent(RecyclerView unused, MotionEvent e) {
        if (MotionEvents.isMouseEvent(e)) {
            if (Shared.DEBUG) Log.w(TAG, "Unexpected Mouse event. Check configuration.");
        }

        switch (e.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                // NOTE: Unlike events with other actions, RecyclerView eats
                // "DOWN" events. So even if we return true here we'll
                // never see an event w/ ACTION_DOWN passed to onTouchEvent.
                return handleInterceptedDownEvent(e);
            case MotionEvent.ACTION_MOVE:
                return mStarted;
        }

        return false;
    }

    @Override
    public void onTouchEvent(RecyclerView unused, MotionEvent e) {
        // Note: There were a couple times I as this check firing
        // after combinations of mouse + touch + rotation.
        // But after further investigation I couldn't repro.
        // For that reason we guard this check (for now) w/ IS_DEBUGGABLE.
        if (Build.IS_DEBUGGABLE) checkState(mStarted);

        switch (e.getActionMasked()) {
            case MotionEvent.ACTION_MOVE:
                handleMoveEvent(e);
                break;
            case MotionEvent.ACTION_UP:
                handleUpEvent(e);
                break;
            case MotionEvent.ACTION_CANCEL:
                handleCancelEvent(e);
                break;
        }
    }

    @Override
    public void onRequestDisallowInterceptTouchEvent(boolean disallowIntercept) {}

    // Called when an ACTION_DOWN event is intercepted. See onInterceptTouchEvent
    // for additional notes.
    // If down event happens on an item, we mark that item's position as last started.
    private boolean handleInterceptedDownEvent(MotionEvent e) {
        // Ignore events where details provider doesn't return details.
        // These objects don't participate in selection.
        if (mItemLookup.getItemDetails(e) == null) {
            return false;
        }
        mLastTouchedItemPosition = mView.getItemUnder(e);
        return mLastTouchedItemPosition != RecyclerView.NO_POSITION;
    }

    // Called when ACTION_UP event is to be handled.
    // Essentially, since this means all gesture movement is over, reset everything and apply
    // provisional selection.
    private void handleUpEvent(MotionEvent e) {
        mSelectionMgr.mergeProvisionalSelection();
        endSelection();
        if (mLastTouchedItemPosition > -1) {
            mSelectionMgr.startRange(mLastTouchedItemPosition);
        }
    }

    // Called when ACTION_CANCEL event is to be handled.
    // This means this gesture selection is aborted, so reset everything and abandon provisional
    // selection.
    private void handleCancelEvent(MotionEvent unused) {
        mSelectionMgr.clearProvisionalSelection();
        endSelection();
    }

    private void endSelection() {
        checkState(mStarted);

        mLastTouchedItemPosition = -1;
        mStarted = false;
        mLock.unblock();
    }

    // Call when an intercepted ACTION_MOVE event is passed down.
    // At this point, we are sure user wants to gesture multi-select.
    private void handleMoveEvent(MotionEvent e) {
        mLastInterceptedPoint = MotionEvents.getOrigin(e);

        int lastGlidedItemPos = mView.getLastGlidedItemPosition(e);
        if (lastGlidedItemPos != RecyclerView.NO_POSITION) {
            doGestureMultiSelect(lastGlidedItemPos);
        }
        scrollIfNecessary();
    }

    // It's possible for events to go over the top/bottom of the RecyclerView.
    // We want to get a Y-coordinate within the RecyclerView so we can find the childView underneath
    // correctly.
    private static float getInboundY(float max, float y) {
        if (y < 0f) {
            return 0f;
        } else if (y > max) {
            return max;
        }
        return y;
    }

    /* Given the end position, select everything in-between.
     * @param endPos  The adapter position of the end item.
     */
    private void doGestureMultiSelect(int endPos) {
        mSelectionMgr.extendProvisionalRange(endPos);
    }

    private void scrollIfNecessary() {
        mScroller.run();
    }

    @Override
    public Point getCurrentPosition() {
        return mLastInterceptedPoint;
    }

    @Override
    public int getViewHeight() {
        return mView.getHeight();
    }

    @Override
    public boolean isActive() {
        return mStarted && mSelectionMgr.hasSelection();
    }

    /**
     * Returns a new instance of GestureSelectionHelper, wrapping the
     * RecyclerView in a test friendly wrapper.
     */
    public static GestureSelectionHelper create(
            SelectionHelper selectionMgr,
            RecyclerView recycler,
            ContentLock lock,
            ItemDetailsLookup itemLookup) {

        return new GestureSelectionHelper(
                selectionMgr, new RecyclerViewDelegate(recycler), lock, itemLookup);
    }

    @VisibleForTesting
    static abstract class ViewDelegate extends ScrollerCallbacks {
        abstract int getHeight();
        abstract int getItemUnder(MotionEvent e);
        abstract int getLastGlidedItemPosition(MotionEvent e);
    }

    @VisibleForTesting
    static final class RecyclerViewDelegate extends ViewDelegate {

        private final RecyclerView mView;

        RecyclerViewDelegate(RecyclerView view) {
            checkArgument(view != null);
            mView = view;
        }

        @Override
        int getHeight() {
            return mView.getHeight();
        }

        @Override
        int getItemUnder(MotionEvent e) {
            View child = mView.findChildViewUnder(e.getX(), e.getY());
            return child != null
                    ? mView.getChildAdapterPosition(child)
                    : RecyclerView.NO_POSITION;
        }

        @Override
        int getLastGlidedItemPosition(MotionEvent e) {
            // If user has moved his pointer to the bottom-right empty pane (ie. to the right of the
            // last item of the recycler view), we would want to set that as the currentItemPos
            View lastItem = mView.getLayoutManager()
                    .getChildAt(mView.getLayoutManager().getChildCount() - 1);
            int direction =
                    mView.getContext().getResources().getConfiguration().getLayoutDirection();
            final boolean pastLastItem = isPastLastItem(lastItem.getTop(),
                    lastItem.getLeft(),
                    lastItem.getRight(),
                    e,
                    direction);

            // Since views get attached & detached from RecyclerView,
            // {@link LayoutManager#getChildCount} can return a different number from the actual
            // number
            // of items in the adapter. Using the adapter is the for sure way to get the actual last
            // item position.
            final float inboundY = getInboundY(mView.getHeight(), e.getY());
            return (pastLastItem) ? mView.getAdapter().getItemCount() - 1
                    : mView.getChildAdapterPosition(mView.findChildViewUnder(e.getX(), inboundY));
        }

        /*
         * Check to see if MotionEvent if past a particular item, i.e. to the right or to the bottom
         * of the item.
         * For RTL, it would to be to the left or to the bottom of the item.
         */
        @VisibleForTesting
        static boolean isPastLastItem(int top, int left, int right, MotionEvent e, int direction) {
            if (direction == View.LAYOUT_DIRECTION_LTR) {
                return e.getX() > right && e.getY() > top;
            } else {
                return e.getX() < left && e.getY() > top;
            }
        }

        @Override
        public void scrollBy(int dy) {
            mView.scrollBy(0, dy);
        }

        @Override
        public void runAtNextFrame(Runnable r) {
            mView.postOnAnimation(r);
        }

        @Override
        public void removeCallback(Runnable r) {
            mView.removeCallbacks(r);
        }
    }
}
