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
import static android.support.v7.widget.RecyclerView.NO_POSITION;
import static com.android.documentsui.selection.Shared.DEBUG;
import static com.android.documentsui.selection.Shared.TAG;

import android.util.Log;

import com.android.documentsui.selection.DefaultSelectionHelper.RangeType;

/**
 * Class providing support for managing range selections.
 */
final class Range {

    private final Callbacks mCallbacks;
    private final int mBegin;
    private int mEnd = NO_POSITION;

    public Range(Callbacks callbacks, int begin) {
        if (DEBUG) Log.d(TAG, "New Ranger created beginning @ " + begin);
        mCallbacks = callbacks;
        mBegin = begin;
    }

    void extendSelection(int position, @RangeType int type) {
        checkArgument(position != NO_POSITION, "Position cannot be NO_POSITION.");

        if (mEnd == NO_POSITION || mEnd == mBegin) {
            // Reset mEnd so it can be established in establishRange.
            mEnd = NO_POSITION;
            establishRange(position, type);
        } else {
            reviseRange(position, type);
        }
    }

    private void establishRange(int position, @RangeType int type) {
        checkArgument(mEnd == NO_POSITION, "End has already been set.");

        if (position == mBegin) {
            mEnd = position;
        }

        if (position > mBegin) {
            updateRange(mBegin + 1, position, true, type);
        } else if (position < mBegin) {
            updateRange(position, mBegin - 1, true, type);
        }

        mEnd = position;
    }

    private void reviseRange(int position, @RangeType int type) {
        checkArgument(mEnd != NO_POSITION, "End must already be set.");
        checkArgument(mBegin != mEnd, "Beging and end point to same position.");

        if (position == mEnd) {
            if (DEBUG) Log.v(TAG, "Ignoring no-op revision for range: " + this);
        }

        if (mEnd > mBegin) {
            reviseAscendingRange(position, type);
        } else if (mEnd < mBegin) {
            reviseDescendingRange(position, type);
        }
        // the "else" case is covered by checkState at beginning of method.

        mEnd = position;
    }

    /**
     * Updates an existing ascending selection.
     * @param position
     */
    private void reviseAscendingRange(int position, @RangeType int type) {
        // Reducing or reversing the range....
        if (position < mEnd) {
            if (position < mBegin) {
                updateRange(mBegin + 1, mEnd, false, type);
                updateRange(position, mBegin -1, true, type);
            } else {
                updateRange(position + 1, mEnd, false, type);
            }
        }

        // Extending the range...
        else if (position > mEnd) {
            updateRange(mEnd + 1, position, true, type);
        }
    }

    private void reviseDescendingRange(int position, @RangeType int type) {
        // Reducing or reversing the range....
        if (position > mEnd) {
            if (position > mBegin) {
                updateRange(mEnd, mBegin - 1, false, type);
                updateRange(mBegin + 1, position, true, type);
            } else {
                updateRange(mEnd, position - 1, false, type);
            }
        }

        // Extending the range...
        else if (position < mEnd) {
            updateRange(position, mEnd - 1, true, type);
        }
    }

    /**
     * Try to set selection state for all elements in range. Not that callbacks can cancel
     * selection of specific items, so some or even all items may not reflect the desired state
     * after the update is complete.
     *
     * @param begin Adapter position for range start (inclusive).
     * @param end Adapter position for range end (inclusive).
     * @param selected New selection state.
     */
    private void updateRange(int begin, int end, boolean selected, @RangeType int type) {
        mCallbacks.updateForRange(begin, end, selected, type);
    }

    @Override
    public String toString() {
        return "Range{begin=" + mBegin + ", end=" + mEnd + "}";
    }

    /*
     * @see {@link DefaultSelectionHelper#updateForRange(int, int , boolean, int)}.
     */
    static abstract class Callbacks {
        abstract void updateForRange(
                int begin, int end, boolean selected, @RangeType int type);
    }
}
