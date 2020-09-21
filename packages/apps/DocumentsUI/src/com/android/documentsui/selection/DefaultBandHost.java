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

import static com.android.internal.util.Preconditions.checkArgument;

import android.annotation.DrawableRes;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.View;

import com.android.documentsui.selection.BandSelectionHelper.BandHost;

/**
 * RecyclerView backed {@link BandHost}.
 */
public final class DefaultBandHost extends BandHost {

    private final RecyclerView mRecView;
    private final Drawable mBand;

    private boolean mIsOverlayShown;

    public DefaultBandHost(RecyclerView recView, @DrawableRes int bandOverlayId) {

        checkArgument(recView != null);

        mRecView = recView;
        mBand = mRecView.getContext().getTheme().getDrawable(bandOverlayId);

        checkArgument(mBand != null);
    }

    @Override
    public int getAdapterPositionAt(int index) {
        return mRecView.getChildAdapterPosition(mRecView.getChildAt(index));
    }

    @Override
    public void addOnScrollListener(RecyclerView.OnScrollListener listener) {
        mRecView.addOnScrollListener(listener);
    }

    @Override
    public void removeOnScrollListener(RecyclerView.OnScrollListener listener) {
        mRecView.removeOnScrollListener(listener);
    }

    @Override
    public Point createAbsolutePoint(Point relativePoint) {
        return new Point(relativePoint.x + mRecView.computeHorizontalScrollOffset(),
                relativePoint.y + mRecView.computeVerticalScrollOffset());
    }

    @Override
    public Rect getAbsoluteRectForChildViewAt(int index) {
        final View child = mRecView.getChildAt(index);
        final Rect childRect = new Rect();
        child.getHitRect(childRect);
        childRect.left += mRecView.computeHorizontalScrollOffset();
        childRect.right += mRecView.computeHorizontalScrollOffset();
        childRect.top += mRecView.computeVerticalScrollOffset();
        childRect.bottom += mRecView.computeVerticalScrollOffset();
        return childRect;
    }

    @Override
    public int getChildCount() {
        return mRecView.getAdapter().getItemCount();
    }

    @Override
    public int getVisibleChildCount() {
        return mRecView.getChildCount();
    }

    @Override
    public int getColumnCount() {
        RecyclerView.LayoutManager layoutManager = mRecView.getLayoutManager();
        if (layoutManager instanceof GridLayoutManager) {
            return ((GridLayoutManager) layoutManager).getSpanCount();
        }

        // Otherwise, it is a list with 1 column.
        return 1;
    }

    @Override
    public int getHeight() {
        return mRecView.getHeight();
    }

    @Override
    public void invalidateView() {
        mRecView.invalidate();
    }

    @Override
    public void runAtNextFrame(Runnable r) {
        mRecView.postOnAnimation(r);
    }

    @Override
    public void removeCallback(Runnable r) {
        mRecView.removeCallbacks(r);
    }

    @Override
    public void scrollBy(int dy) {
        mRecView.scrollBy(0, dy);
    }

    @Override
    public void showBand(Rect rect) {
        mBand.setBounds(rect);

        if (!mIsOverlayShown) {
            mRecView.getOverlay().add(mBand);
        }
    }

    @Override
    public void hideBand() {
        mRecView.getOverlay().remove(mBand);
    }

    @Override
    public boolean hasView(int pos) {
        return mRecView.findViewHolderForAdapterPosition(pos) != null;
    }
}