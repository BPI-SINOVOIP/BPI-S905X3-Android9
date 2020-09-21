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

import android.view.MotionEvent;

import com.android.documentsui.selection.ItemDetailsLookup.ItemDetails;
import com.android.internal.widget.RecyclerView;

public final class TestItemDetails extends ItemDetails {

    // DocumentsAdapter.ITEM_TYPE_DOCUMENT
    private int mViewType = -1;
    private int mPosition;
    private String mStableId;
    private boolean mInDragRegion;
    private boolean mInSelectionHotspot;

    public TestItemDetails() {
       mPosition = RecyclerView.NO_POSITION;
    }

    public TestItemDetails(TestItemDetails source) {
        mPosition = source.mPosition;
        mStableId = source.mStableId;
        mViewType = source.mViewType;
        mInDragRegion = source.mInDragRegion;
        mInSelectionHotspot = source.mInSelectionHotspot;
    }

    public void setViewType(int viewType) {
        mViewType = viewType;
    }

    public void at(int position) {
        mPosition = position;  // this is both "adapter position" and "item position".
        mStableId = (position == RecyclerView.NO_POSITION)
                ? null
                : String.valueOf(position);
    }

    public void setInItemDragRegion(boolean inHotspot) {
        mInDragRegion = inHotspot;
    }

    public void setInItemSelectRegion(boolean over) {
        mInSelectionHotspot = over;
    }

    @Override
    public int getItemViewType() {
        return mViewType;
    }

    @Override
    public boolean inDragRegion(MotionEvent event) {
        return mInDragRegion;
    }

    @Override
    public int hashCode() {
        return mPosition;
    }

    @Override
    public boolean equals(Object o) {
      if (this == o) {
          return true;
      }

      if (!(o instanceof TestItemDetails)) {
          return false;
      }

      TestItemDetails other = (TestItemDetails) o;
      return mPosition == other.mPosition
              && mStableId == other.mStableId
              && mViewType == other.mViewType;
    }

    @Override
    public int getPosition() {
        return mPosition;
    }

    @Override
    public String getStableId() {
        return mStableId;
    }

    @Override
    public boolean inSelectionHotspot(MotionEvent e) {
        return mInSelectionHotspot;
    }
}
