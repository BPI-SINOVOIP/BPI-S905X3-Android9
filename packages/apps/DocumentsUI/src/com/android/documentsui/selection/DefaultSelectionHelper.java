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
import static com.android.documentsui.selection.Shared.DEBUG;
import static com.android.documentsui.selection.Shared.TAG;

import android.support.annotation.IntDef;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.Adapter;
import android.util.Log;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

/**
 * {@link SelectionHelper} providing support traditional multi-item selection on top
 * of {@link RecyclerView}.
 *
 * <p>The class supports running in a single-select mode, which can be enabled
 * by passing {@colde #MODE_SINGLE} to the constructor.
 */
public final class DefaultSelectionHelper extends SelectionHelper {

    public static final int MODE_MULTIPLE = 0;
    public static final int MODE_SINGLE = 1;

    @IntDef(flag = true, value = {
            MODE_MULTIPLE,
            MODE_SINGLE
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface SelectionMode {}

    private static final int RANGE_REGULAR = 0;

    /**
     * "Provisional" selection represents a overlay on the primary selection. A provisional
     * selection maybe be eventually added to the primary selection, or it may be abandoned.
     *
     * <p>E.g. BandController creates a provisional selection while a user is actively selecting
     * items with the band. Provisionally selected items are considered to be selected in
     * {@link Selection#contains(String)} and related methods. A provisional may be abandoned or
     * applied by selection components (like
     * {@link com.android.documentsui.selection.BandSelectionHelper}).
     *
     * <p>A provisional selection may intersect the primary selection, however clearing the
     * provisional selection will not affect the primary selection where the two may intersect.
     */
    private static final int RANGE_PROVISIONAL = 1;
    @IntDef({
        RANGE_REGULAR,
        RANGE_PROVISIONAL
    })
    @Retention(RetentionPolicy.SOURCE)
    @interface RangeType {}

    private final Selection mSelection = new Selection();
    private final List<SelectionObserver> mObservers = new ArrayList<>(1);
    private final RecyclerView.Adapter<?> mAdapter;
    private final StableIdProvider mStableIds;
    private final SelectionPredicate mSelectionPredicate;
    private final RecyclerView.AdapterDataObserver mAdapterObserver;
    private final RangeCallbacks mRangeCallbacks;
    private final boolean mSingleSelect;

    private @Nullable Range mRange;

    /**
     * Creates a new instance.
     *
     * @param mode single or multiple selection mode. In single selection mode
     *     users can only select a single item.
     * @param adapter {@link Adapter} for the RecyclerView this instance is coupled with.
     * @param stableIds client supplied class providing access to stable ids.
     * @param selectionPredicate A predicate allowing the client to disallow selection
     *     of individual elements.
     */
    public DefaultSelectionHelper(
            @SelectionMode int mode,
            RecyclerView.Adapter<?> adapter,
            StableIdProvider stableIds,
            SelectionPredicate selectionPredicate) {

        checkArgument(mode == MODE_SINGLE || mode == MODE_MULTIPLE);
        checkArgument(adapter != null);
        checkArgument(stableIds != null);
        checkArgument(selectionPredicate != null);

        mAdapter = adapter;
        mStableIds = stableIds;
        mSelectionPredicate = selectionPredicate;
        mAdapterObserver = new AdapterObserver();
        mRangeCallbacks = new RangeCallbacks();

        mSingleSelect = mode == MODE_SINGLE;

        mAdapter.registerAdapterDataObserver(mAdapterObserver);
    }

    @Override
    public void addObserver(SelectionObserver callback) {
        checkArgument(callback != null);
        mObservers.add(callback);
    }

    @Override
    public boolean hasSelection() {
        return !mSelection.isEmpty();
    }

    @Override
    public Selection getSelection() {
        return mSelection;
    }

    @Override
    public void copySelection(Selection dest) {
        dest.copyFrom(mSelection);
    }

    @Override
    public boolean isSelected(String id) {
        return mSelection.contains(id);
    }

    @Override
    public void restoreSelection(Selection other) {
        setItemsSelectedQuietly(other.mSelection, true);
        // NOTE: We intentionally don't restore provisional selection. It's provisional.
        notifySelectionRestored();
    }

    @Override
    public boolean setItemsSelected(Iterable<String> ids, boolean selected) {
        boolean changed = setItemsSelectedQuietly(ids, selected);
        notifySelectionChanged();
        return changed;
    }

    private boolean setItemsSelectedQuietly(Iterable<String> ids, boolean selected) {
        boolean changed = false;
        for (String id: ids) {
            boolean itemChanged = selected
                    ? canSetState(id, true) && mSelection.add(id)
                    : canSetState(id, false) && mSelection.remove(id);
            if (itemChanged) {
                notifyItemStateChanged(id, selected);
            }
            changed |= itemChanged;
        }
        return changed;
    }

    @Override
    public void clearSelection() {
        if (!hasSelection()) {
            return;
        }

        Selection prev = clearSelectionQuietly();
        notifySelectionCleared(prev);
        notifySelectionChanged();
    }

    /**
     * Clears the selection, without notifying selection listeners.
     * Returns items in previous selection. Callers are responsible for notifying
     * listeners about changes.
     */
    private Selection clearSelectionQuietly() {
        mRange = null;

        Selection prevSelection = new Selection();
        if (hasSelection()) {
            copySelection(prevSelection);
            mSelection.clear();
        }

        return prevSelection;
    }

    @Override
    public boolean select(String id) {
        checkArgument(id != null);

        if (!mSelection.contains(id)) {
            if (!canSetState(id, true)) {
                if (DEBUG) Log.d(TAG, "Select cancelled by selection predicate test.");
                return false;
            }

            // Enforce single selection policy.
            if (mSingleSelect && hasSelection()) {
                Selection prev = clearSelectionQuietly();
                notifySelectionCleared(prev);
            }

            mSelection.add(id);
            notifyItemStateChanged(id, true);
            notifySelectionChanged();

            return true;
        }

        return false;
    }

    @Override
    public boolean deselect(String id) {
        checkArgument(id != null);

        if (mSelection.contains(id)) {
            if (!canSetState(id, false)) {
                if (DEBUG) Log.d(TAG, "Deselect cancelled by selection predicate test.");
                return false;
            }
            mSelection.remove(id);
            notifyItemStateChanged(id, false);
            notifySelectionChanged();
            if (mSelection.isEmpty() && isRangeActive()) {
                // if there's nothing in the selection and there is an active ranger it results
                // in unexpected behavior when the user tries to start range selection: the item
                // which the ranger 'thinks' is the already selected anchor becomes unselectable
                endRange();
            }
            return true;
        }

        return false;
    }

    @Override
    public void startRange(int pos) {
        select(mStableIds.getStableId(pos));
        anchorRange(pos);
    }

    @Override
    public void extendRange(int pos) {
        extendRange(pos, RANGE_REGULAR);
    }

    @Override
    public void endRange() {
        mRange = null;
        // Clean up in case there was any leftover provisional selection
        clearProvisionalSelection();
    }

    @Override
    public void anchorRange(int position) {
        checkArgument(position != RecyclerView.NO_POSITION);

        // TODO: I'm not a fan of silently ignoring calls.
        // Determine if there are any cases where method can be called
        // w/o item already being selected. Else, tighten up the ship
        // and make this conditional guard into a proper precondition check.
        if (mSelection.contains(mStableIds.getStableId(position))) {
            mRange = new Range(mRangeCallbacks, position);
        }
    }

    @Override
    public void extendProvisionalRange(int pos) {
        extendRange(pos, RANGE_PROVISIONAL);
    }

    /**
     * Sets the end point for the current range selection, started by a call to
     * {@link #startRange(int)}. This function should only be called when a range selection
     * is active (see {@link #isRangeActive()}. Items in the range [anchor, end] will be
     * selected or in provisional select, depending on the type supplied. Note that if the type is
     * provisional selection, one should do {@link #mergeProvisionalSelection()} at some
     * point before calling on {@link #endRange()}.
     *
     * @param pos The new end position for the selection range.
     * @param type The type of selection the range should utilize.
     */
    private void extendRange(int pos, @RangeType int type) {
        checkState(isRangeActive(), "Range start point not set.");

        mRange.extendSelection(pos, type);

        // We're being lazy here notifying even when something might not have changed.
        // To make this more correct, we'd need to update the Ranger class to return
        // information about what has changed.
        notifySelectionChanged();
    }

    @Override
    public void setProvisionalSelection(Set<String> newSelection) {
        Map<String, Boolean> delta = mSelection.setProvisionalSelection(newSelection);
        for (Map.Entry<String, Boolean> entry: delta.entrySet()) {
            notifyItemStateChanged(entry.getKey(), entry.getValue());
        }

        notifySelectionChanged();
    }

    @Override
    public void mergeProvisionalSelection() {
        mSelection.mergeProvisionalSelection();
    }

    @Override
    public void clearProvisionalSelection() {
        for (String id : mSelection.mProvisionalSelection) {
            notifyItemStateChanged(id, false);
        }
        mSelection.clearProvisionalSelection();
    }

    @Override
    public boolean isRangeActive() {
        return mRange != null;
    }

    private boolean canSetState(String id, boolean nextState) {
        return mSelectionPredicate.canSetStateForId(id, nextState);
    }

    private void onDataSetChanged() {
        // Update the selection to remove any disappeared IDs.
        mSelection.clearProvisionalSelection();
        mSelection.intersect(mStableIds.getStableIds());
        notifySelectionReset();

        for (String id : mSelection) {
            // If the underlying data set has changed, before restoring
            // selection we must re-verify that it can be selected.
            // Why? Because if the dataset has changed, then maybe the
            // selectability of an item has changed.
            if (!canSetState(id, true)) {
                deselect(id);
            } else {
                int lastListener = mObservers.size() - 1;
                for (int i = lastListener; i >= 0; i--) {
                    mObservers.get(i).onItemStateChanged(id, true);
                }
            }
        }
        notifySelectionChanged();
    }

    private void onDataSetItemRangeInserted(int startPosition, int itemCount) {
        mSelection.clearProvisionalSelection();
    }

    private void onDataSetItemRangeRemoved(int startPosition, int itemCount) {
        checkArgument(startPosition >= 0);
        checkArgument(itemCount > 0);

        mSelection.clearProvisionalSelection();

        // Remove any disappeared IDs from the selection.
        //
        // Ideally there could be a cheaper approach, checking
        // each position individually, but since the source of
        // truth for stable ids (StableIdProvider) probably
        // it-self no-longer knows about the positions in question
        // we fall back to the sledge hammer approach.
        mSelection.intersect(mStableIds.getStableIds());
    }

    /**
     * Notifies registered listeners when the selection status of a single item
     * (identified by {@code position}) changes.
     */
    private void notifyItemStateChanged(String id, boolean selected) {
        checkArgument(id != null);

        int lastListenerIndex = mObservers.size() - 1;
        for (int i = lastListenerIndex; i >= 0; i--) {
            mObservers.get(i).onItemStateChanged(id, selected);
        }

        int position = mStableIds.getPosition(id);
        if (DEBUG) Log.d(TAG, "ITEM " + id + " CHANGED at pos: " + position);

        if (position >= 0) {
            mAdapter.notifyItemChanged(position, SelectionHelper.SELECTION_CHANGED_MARKER);
        } else {
            Log.w(TAG, "Item change notification received for unknown item: " + id);
        }
    }

    private void notifySelectionCleared(Selection selection) {
        for (String id: selection.mSelection) {
            notifyItemStateChanged(id, false);
        }
        for (String id: selection.mProvisionalSelection) {
            notifyItemStateChanged(id, false);
        }
    }

    /**
     * Notifies registered listeners when the selection has changed. This
     * notification should be sent only once a full series of changes
     * is complete, e.g. clearingSelection, or updating the single
     * selection from one item to another.
     */
    private void notifySelectionChanged() {
        int lastListenerIndex = mObservers.size() - 1;
        for (int i = lastListenerIndex; i >= 0; i--) {
            mObservers.get(i).onSelectionChanged();
        }
    }

    private void notifySelectionRestored() {
        int lastListenerIndex = mObservers.size() - 1;
        for (int i = lastListenerIndex; i >= 0; i--) {
            mObservers.get(i).onSelectionRestored();
        }
    }

    private void notifySelectionReset() {
        int lastListenerIndex = mObservers.size() - 1;
        for (int i = lastListenerIndex; i >= 0; i--) {
            mObservers.get(i).onSelectionReset();
        }
    }

    private void updateForRange(int begin, int end, boolean selected, @RangeType int type) {
        switch (type) {
            case RANGE_REGULAR:
                updateForRegularRange(begin, end, selected);
                break;
            case RANGE_PROVISIONAL:
                updateForProvisionalRange(begin, end, selected);
                break;
            default:
                throw new IllegalArgumentException("Invalid range type: " + type);
        }
    }

    private void updateForRegularRange(int begin, int end, boolean selected) {
        checkArgument(end >= begin);

        for (int i = begin; i <= end; i++) {
            String id = mStableIds.getStableId(i);
            if (id == null) {
                continue;
            }

            if (selected) {
                select(id);
            } else {
                deselect(id);
            }
        }
    }

    private void updateForProvisionalRange(int begin, int end, boolean selected) {
        checkArgument(end >= begin);

        for (int i = begin; i <= end; i++) {
            String id = mStableIds.getStableId(i);
            if (id == null) {
                continue;
            }

            boolean changedState = false;
            if (selected) {
                boolean canSelect = canSetState(id, true);
                if (canSelect && !mSelection.mSelection.contains(id)) {
                    mSelection.mProvisionalSelection.add(id);
                    changedState = true;
                }
            } else {
                mSelection.mProvisionalSelection.remove(id);
                changedState = true;
            }

            // Only notify item callbacks when something's state is actually changed in provisional
            // selection.
            if (changedState) {
                notifyItemStateChanged(id, selected);
            }
        }

        notifySelectionChanged();
    }

    private final class AdapterObserver extends RecyclerView.AdapterDataObserver {
        @Override
        public void onChanged() {
            onDataSetChanged();
        }

        @Override
        public void onItemRangeChanged(
                int startPosition, int itemCount, Object payload) {
            // No change in position. Ignore, since we assume
            // selection is a user driven activity. So changes
            // in properties of items shouldn't result in a
            // change of selection.
        }

        @Override
        public void onItemRangeInserted(int startPosition, int itemCount) {
            onDataSetItemRangeInserted(startPosition, itemCount);
        }

        @Override
        public void onItemRangeRemoved(int startPosition, int itemCount) {
            onDataSetItemRangeRemoved(startPosition, itemCount);
        }

        @Override
        public void onItemRangeMoved(int fromPosition, int toPosition, int itemCount) {
            throw new UnsupportedOperationException();
        }
    }

    private final class RangeCallbacks extends Range.Callbacks {
        @Override
        void updateForRange(int begin, int end, boolean selected, int type) {
            switch (type) {
                case RANGE_REGULAR:
                    updateForRegularRange(begin, end, selected);
                    break;
                case RANGE_PROVISIONAL:
                    updateForProvisionalRange(begin, end, selected);
                    break;
                default:
                    throw new IllegalArgumentException(
                            "Invalid range type: " + type);
            }
        }
    }
}
