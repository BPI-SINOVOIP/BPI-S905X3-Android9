/*
 * Copyright (C) 2015 The Android Open Source Project
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
import android.graphics.Rect;
import android.os.Build;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.OnItemTouchListener;
import android.support.v7.widget.RecyclerView.OnScrollListener;
import android.util.Log;
import android.view.MotionEvent;

import com.android.documentsui.selection.SelectionHelper.SelectionPredicate;
import com.android.documentsui.selection.SelectionHelper.StableIdProvider;
import com.android.documentsui.selection.ViewAutoScroller.ScrollHost;
import com.android.documentsui.selection.ViewAutoScroller.ScrollerCallbacks;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * Provides mouse driven band-selection support when used in conjunction with a {@link RecyclerView}
 * instance. This class is responsible for rendering a band overlay and manipulating selection
 * status of the items it intersects with.
 *
 * <p> Given the recycling nature of RecyclerView items that have scrolled off-screen would not
 * be selectable with a band that itself was partially rendered off-screen. To address this,
 * BandSelectionController builds a model of the list/grid information presented by RecyclerView as
 * the user interacts with items using their pointer (and the band). Selectable items that intersect
 * with the band, both on and off screen, are selected on pointer up.
 */
public class BandSelectionHelper implements OnItemTouchListener {

    static final boolean DEBUG = false;
    static final String TAG = "BandController";

    private final BandHost mHost;
    private final StableIdProvider mStableIds;
    private final RecyclerView.Adapter<?> mAdapter;
    private final SelectionHelper mSelectionHelper;
    private final SelectionPredicate mSelectionPredicate;
    private final BandPredicate mBandPredicate;
    private final ContentLock mLock;
    private final Runnable mViewScroller;
    private final GridModel.SelectionObserver mGridObserver;
    private final List<Runnable> mBandStartedListeners = new ArrayList<>();

    @Nullable private Rect mBounds;
    @Nullable private Point mCurrentPosition;
    @Nullable private Point mOrigin;
    @Nullable private GridModel mModel;

    public BandSelectionHelper(
            BandHost host,
            RecyclerView.Adapter<?> adapter,
            StableIdProvider stableIds,
            SelectionHelper selectionHelper,
            SelectionPredicate selectionPredicate,
            BandPredicate bandPredicate,
            ContentLock lock) {

        checkArgument(host != null);
        checkArgument(adapter != null);
        checkArgument(stableIds != null);
        checkArgument(selectionHelper != null);
        checkArgument(selectionPredicate != null);
        checkArgument(bandPredicate != null);
        checkArgument(lock != null);

        mHost = host;
        mStableIds = stableIds;
        mAdapter = adapter;
        mSelectionHelper = selectionHelper;
        mSelectionPredicate = selectionPredicate;
        mBandPredicate = bandPredicate;
        mLock = lock;

        mHost.addOnScrollListener(
                new OnScrollListener() {
                    @Override
                    public void onScrolled(RecyclerView recyclerView, int dx, int dy) {
                        BandSelectionHelper.this.onScrolled(recyclerView, dx, dy);
                    }
                });

        mViewScroller = new ViewAutoScroller(
                new ScrollHost() {
                    @Override
                    public Point getCurrentPosition() {
                        return mCurrentPosition;
                    }

                    @Override
                    public int getViewHeight() {
                        return mHost.getHeight();
                    }

                    @Override
                    public boolean isActive() {
                        return BandSelectionHelper.this.isActive();
                    }
                },
                host);

        mAdapter.registerAdapterDataObserver(
                new RecyclerView.AdapterDataObserver() {
                    @Override
                    public void onChanged() {
                        if (isActive()) {
                            endBandSelect();
                        }
                    }

                    @Override
                    public void onItemRangeChanged(
                            int startPosition, int itemCount, Object payload) {
                        // No change in position. Ignoring.
                    }

                    @Override
                    public void onItemRangeInserted(int startPosition, int itemCount) {
                        if (isActive()) {
                            endBandSelect();
                        }
                    }

                    @Override
                    public void onItemRangeRemoved(int startPosition, int itemCount) {
                        assert(startPosition >= 0);
                        assert(itemCount > 0);

                        // TODO: Should update grid model.
                    }

                    @Override
                    public void onItemRangeMoved(int fromPosition, int toPosition, int itemCount) {
                        throw new UnsupportedOperationException();
                    }
                });

        mGridObserver = new GridModel.SelectionObserver() {
                @Override
                public void onSelectionChanged(Set<String> updatedSelection) {
                    mSelectionHelper.setProvisionalSelection(updatedSelection);
                }
            };
    }

    @VisibleForTesting
    boolean isActive() {
        boolean active = mModel != null;
        if (Build.IS_DEBUGGABLE && active) {
            mLock.checkLocked();
        }
        return active;
    }

    /**
     * Adds a new listener to be notified when band is created.
     */
    public void addOnBandStartedListener(Runnable listener) {
        checkArgument(listener != null);

        mBandStartedListeners.add(listener);
    }

    /**
     * Removes listener. No-op if listener was not previously installed.
     */
    public void removeOnBandStartedListener(Runnable listener) {
        mBandStartedListeners.remove(listener);
    }

    /**
     * Clients must call reset when there are any material changes to the layout of items
     * in RecyclerView.
     */
    public void reset() {
        if (!isActive()) {
            return;
        }

        mHost.hideBand();
        mModel.stopCapturing();
        mModel.onDestroy();
        mModel = null;
        mOrigin = null;
        mLock.unblock();
    }

    boolean shouldStart(MotionEvent e) {
        // Don't start, or extend bands on non-left clicks.
        if (!MotionEvents.isPrimaryButtonPressed(e)) {
            return false;
        }

        // TODO: Refactor to NOT have side-effects on this "should" method.
        // Weird things happen if we keep up band select
        // when touch events happen.
        if (isActive() && !MotionEvents.isMouseEvent(e)) {
            endBandSelect();
            return false;
        }

        // b/30146357 && b/23793622. onInterceptTouchEvent does not dispatch events to onTouchEvent
        // unless the event is != ACTION_DOWN. Thus, we need to actually start band selection when
        // mouse moves, or else starting band selection on mouse down can cause problems as events
        // don't get routed correctly to onTouchEvent.
        return !isActive()
                && MotionEvents.isActionMove(e)
                // the initial button move via mouse-touch (ie. down press)
                // The adapter inserts items for UI layout purposes that aren't
                // associated with files. Checking against actual modelIds count
                // effectively ignores those UI layout items.
                && !mStableIds.getStableIds().isEmpty()
                && mBandPredicate.canInitiate(e);
    }

    public boolean shouldStop(MotionEvent e) {
        return isActive()
                && MotionEvents.isMouseEvent(e)
                && (MotionEvents.isActionUp(e)
                        || MotionEvents.isActionPointerUp(e)
                        || MotionEvents.isActionCancel(e));
    }

    @Override
    public boolean onInterceptTouchEvent(RecyclerView unused, MotionEvent e) {
        if (shouldStart(e)) {
            if (!MotionEvents.isCtrlKeyPressed(e)) {
                mSelectionHelper.clearSelection();
            }

            startBandSelect(MotionEvents.getOrigin(e));
            return isActive();
        }

        if (shouldStop(e)) {
            endBandSelect();
            checkState(mModel == null);
            // fall through to return false, because the band eeess done!
        }

        return false;
    }

    /**
     * Processes a MotionEvent by starting, ending, or resizing the band select overlay.
     * @param input
     */
    @Override
    public void onTouchEvent(RecyclerView unused, MotionEvent e) {
        if (shouldStop(e)) {
            endBandSelect();
            return;
        }

        // We shouldn't get any events in this method when band select is not active,
        // but it turns some guests show up late to the party.
        // Probably happening when a re-layout is happening to the ReyclerView (ie. Pull-To-Refresh)
        if (!isActive()) {
            return;
        }

        assert MotionEvents.isActionMove(e);

        mCurrentPosition = MotionEvents.getOrigin(e);
        mModel.resizeSelection(mCurrentPosition);

        scrollViewIfNecessary();
        resizeBand();
    }

    @Override
    public void onRequestDisallowInterceptTouchEvent(boolean disallowIntercept) {}

    /**
     * Starts band select by adding the drawable to the RecyclerView's overlay.
     */
    private void startBandSelect(Point origin) {
        if (DEBUG) Log.d(TAG, "Starting band select @ " + origin);

        reset();
        mModel = new GridModel(mHost, mStableIds, mSelectionPredicate);
        mModel.addOnSelectionChangedListener(mGridObserver);

        mLock.block();
        notifyBandStarted();
        mOrigin = origin;
        mModel.startCapturing(mOrigin);
    }

    private void notifyBandStarted() {
        for (Runnable listener : mBandStartedListeners) {
            listener.run();
        }
    }

    private void scrollViewIfNecessary() {
        mHost.removeCallback(mViewScroller);
        mViewScroller.run();
        mHost.invalidateView();
    }

    /**
     * Resizes the band select rectangle by using the origin and the current pointer position as
     * two opposite corners of the selection.
     */
    private void resizeBand() {
        mBounds = new Rect(Math.min(mOrigin.x, mCurrentPosition.x),
                Math.min(mOrigin.y, mCurrentPosition.y),
                Math.max(mOrigin.x, mCurrentPosition.x),
                Math.max(mOrigin.y, mCurrentPosition.y));

        mHost.showBand(mBounds);
    }

    /**
     * Ends band select by removing the overlay.
     */
    private void endBandSelect() {
        if (DEBUG) Log.d(TAG, "Ending band select.");

        // TODO: Currently when a band select operation ends outside
        // of an item (e.g. in the empty area between items),
        // getPositionNearestOrigin may return an unselected item.
        // Since the point of this code is to establish the
        // anchor point for subsequent range operations (SHIFT+CLICK)
        // we really want to do a better job figuring out the last
        // item selected (and nearest to the cursor).
        int firstSelected = mModel.getPositionNearestOrigin();
        if (firstSelected != GridModel.NOT_SET
                && mSelectionHelper.isSelected(mStableIds.getStableId(firstSelected))) {
            // Establish the band selection point as range anchor. This
            // allows touch and keyboard based selection activities
            // to be based on the band selection anchor point.
            mSelectionHelper.anchorRange(firstSelected);
        }

        mSelectionHelper.mergeProvisionalSelection();
        reset();
    }

    /**
     * @see RecyclerView.OnScrollListener
     */
    private void onScrolled(RecyclerView recyclerView, int dx, int dy) {
        if (!isActive()) {
            return;
        }

        // Adjust the y-coordinate of the origin the opposite number of pixels so that the
        // origin remains in the same place relative to the view's items.
        mOrigin.y -= dy;
        resizeBand();
    }

    /**
     * Provides functionality for BandController. Exists primarily to tests that are
     * fully isolated from RecyclerView.
     */
    public static abstract class BandHost extends ScrollerCallbacks {
        public abstract void showBand(Rect rect);
        public abstract void hideBand();
        public abstract void addOnScrollListener(RecyclerView.OnScrollListener listener);
        public abstract void removeOnScrollListener(RecyclerView.OnScrollListener listener);
        public abstract int getHeight();
        public abstract void invalidateView();
        public abstract Point createAbsolutePoint(Point relativePoint);
        public abstract Rect getAbsoluteRectForChildViewAt(int index);
        public abstract int getAdapterPositionAt(int index);
        public abstract int getColumnCount();
        public abstract int getChildCount();
        public abstract int getVisibleChildCount();
        /**
         * @return true if the item at adapter position is attached to a view.
         */
        public abstract boolean hasView(int adapterPosition);
    }
}
