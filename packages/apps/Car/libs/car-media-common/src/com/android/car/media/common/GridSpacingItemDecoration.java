/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.car.media.common;

import android.graphics.Rect;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.View;

/**
 * An {@link RecyclerView.ItemDecoration} that adds spacing between cells in a
 * {@link GridLayoutManager}.
 */
public class GridSpacingItemDecoration extends RecyclerView.ItemDecoration {
    private final int mSpacing;
    private final int mStartMargin;
    private final int mEndMargin;

    /**
     * Creates a {@link GridSpacingItemDecoration}.
     *
     * @param spacing space to add between grid cells, both vertically and horizontally.
     * @param startMargin margin on the start side of the grid.
     * @param endMargin margin on the end side of the grid
     */
    public GridSpacingItemDecoration(int spacing, int startMargin, int endMargin) {
        this.mSpacing = spacing;
        this.mStartMargin = startMargin;
        this.mEndMargin = endMargin;
    }

    @Override
    public void getItemOffsets(Rect outRect, View view, RecyclerView parent,
            RecyclerView.State state) {
        GridLayoutManager.LayoutParams lp = (GridLayoutManager.LayoutParams) view.getLayoutParams();
        GridLayoutManager layoutManager = (GridLayoutManager) parent.getLayoutManager();
        int column = lp.getSpanIndex();
        int spanCount = layoutManager.getSpanCount();

        outRect.left = mStartMargin + column * mSpacing / spanCount
            - (column + 1) * mStartMargin / spanCount;
        outRect.right = mSpacing - (column + 1) * mSpacing / spanCount
            + (column + 1) * mEndMargin / spanCount;
        outRect.top = mSpacing;
    }
}
