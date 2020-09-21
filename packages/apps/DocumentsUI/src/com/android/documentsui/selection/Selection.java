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

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.VisibleForTesting;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

/**
 * Object representing the current selection and provisional selection. Provides read only public
 * access, and private write access.
 * <p>
 * This class tracks selected items by managing two sets:
 *
 * <li>primary selection
 *
 * Primary selection consists of items tapped by a user or by lassoed by band select operation.
 *
 * <li>provisional selection
 *
 * Provisional selections are selections which have been temporarily created
 * by an in-progress band select or gesture selection. Once the user releases the mouse button
 * or lifts their finger the corresponding provisional selection should be converted into
 * primary selection.
 *
 * <p>The total selection is the combination of
 * both the core selection and the provisional selection. Tracking both separately is necessary to
 * ensure that items in the core selection are not "erased" from the core selection when they
 * are temporarily included in a secondary selection (like band selection).
 */
public class Selection implements Iterable<String>, Parcelable {

    final Set<String> mSelection;
    final Set<String> mProvisionalSelection;

    public Selection() {
        mSelection = new HashSet<>();
        mProvisionalSelection = new HashSet<>();
    }

    /**
     * Used by CREATOR.
     */
    private Selection(Set<String> selection) {
        mSelection = selection;
        mProvisionalSelection = new HashSet<>();
    }

    /**
     * @param id
     * @return true if the position is currently selected.
     */
    public boolean contains(@Nullable String id) {
        return mSelection.contains(id) || mProvisionalSelection.contains(id);
    }

    /**
     * Returns an {@link Iterator} that iterators over the selection, *excluding*
     * any provisional selection.
     *
     * {@inheritDoc}
     */
    @Override
    public Iterator<String> iterator() {
        return mSelection.iterator();
    }

    /**
     * @return size of the selection including both final and provisional selected items.
     */
    public int size() {
        return mSelection.size() + mProvisionalSelection.size();
    }

    /**
     * @return true if the selection is empty.
     */
    public boolean isEmpty() {
        return mSelection.isEmpty() && mProvisionalSelection.isEmpty();
    }

    /**
     * Sets the provisional selection, which is a temporary selection that can be saved,
     * canceled, or adjusted at a later time. When a new provision selection is applied, the old
     * one (if it exists) is abandoned.
     * @return Map of ids added or removed. Added ids have a value of true, removed are false.
     */
    Map<String, Boolean> setProvisionalSelection(Set<String> newSelection) {
        Map<String, Boolean> delta = new HashMap<>();

        for (String id: mProvisionalSelection) {
            // Mark each item that used to be in the provisional selection
            // but is not in the new provisional selection.
            if (!newSelection.contains(id) && !mSelection.contains(id)) {
                delta.put(id, false);
            }
        }

        for (String id: mSelection) {
            // Mark each item that used to be in the selection but is unsaved and not in the new
            // provisional selection.
            if (!newSelection.contains(id)) {
                delta.put(id, false);
            }
        }

        for (String id: newSelection) {
            // Mark each item that was not previously in the selection but is in the new
            // provisional selection.
            if (!mSelection.contains(id) && !mProvisionalSelection.contains(id)) {
                delta.put(id, true);
            }
        }

        // Now, iterate through the changes and actually add/remove them to/from the current
        // selection. This could not be done in the previous loops because changing the size of
        // the selection mid-iteration changes iteration order erroneously.
        for (Map.Entry<String, Boolean> entry: delta.entrySet()) {
            String id = entry.getKey();
            if (entry.getValue()) {
                mProvisionalSelection.add(id);
            } else {
                mProvisionalSelection.remove(id);
            }
        }

        return delta;
    }

    /**
     * Saves the existing provisional selection. Once the provisional selection is saved,
     * subsequent provisional selections which are different from this existing one cannot
     * cause items in this existing provisional selection to become deselected.
     */
    @VisibleForTesting
    protected void mergeProvisionalSelection() {
        mSelection.addAll(mProvisionalSelection);
        mProvisionalSelection.clear();
    }

    /**
     * Abandons the existing provisional selection so that all items provisionally selected are
     * now deselected.
     */
    @VisibleForTesting
    void clearProvisionalSelection() {
        mProvisionalSelection.clear();
    }

    /**
     * Adds a new item to the primary selection.
     *
     * @return true if the operation resulted in a modification to the selection.
     */
    boolean add(String id) {
        if (mSelection.contains(id)) {
            return false;
        }

        mSelection.add(id);
        return true;
    }

    /**
     * Removes an item from the primary selection.
     *
     * @return true if the operation resulted in a modification to the selection.
     */
    boolean remove(String id) {
        if (!mSelection.contains(id)) {
            return false;
        }

        mSelection.remove(id);
        return true;
    }

    /**
     * Clears the primary selection. The provisional selection, if any, is unaffected.
     */
    void clear() {
        mSelection.clear();
    }

    /**
     * Trims this selection to be the intersection of itself and {@code ids}.
     */
    void intersect(Collection<String> ids) {
        checkArgument(ids != null);

        mSelection.retainAll(ids);
        mProvisionalSelection.retainAll(ids);
    }

    /**
     * Clones primary and provisional selection from supplied {@link Selection}.
     * Does not copy active range data.
     */
    @VisibleForTesting
    void copyFrom(Selection source) {
        mSelection.clear();
        mSelection.addAll(source.mSelection);

        mProvisionalSelection.clear();
        mProvisionalSelection.addAll(source.mProvisionalSelection);
    }

    @Override
    public String toString() {
        if (size() <= 0) {
            return "size=0, items=[]";
        }

        StringBuilder buffer = new StringBuilder(size() * 28);
        buffer.append("Selection{")
            .append("primary{size=" + mSelection.size())
            .append(", entries=" + mSelection)
            .append("}, provisional{size=" + mProvisionalSelection.size())
            .append(", entries=" + mProvisionalSelection)
            .append("}}");
        return buffer.toString();
    }

    @Override
    public int hashCode() {
        return mSelection.hashCode() ^ mProvisionalSelection.hashCode();
    }

    @Override
    public boolean equals(Object other) {
        if (this == other) {
            return true;
        }

        return other instanceof Selection
            ? equals((Selection) other)
            : false;
    }

    private boolean equals(Selection other) {
        return mSelection.equals(((Selection) other).mSelection) &&
                mProvisionalSelection.equals(((Selection) other).mProvisionalSelection);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeStringList(new ArrayList<>(mSelection));
        // We don't include provisional selection since it is
        // typically coupled to some other runtime state (like a band).
    }

    public static final ClassLoaderCreator<Selection> CREATOR =
            new ClassLoaderCreator<Selection>() {
        @Override
        public Selection createFromParcel(Parcel in) {
            return createFromParcel(in, null);
        }

        @Override
        public Selection createFromParcel(Parcel in, ClassLoader loader) {
            ArrayList<String> selected = new ArrayList<>();
            in.readStringList(selected);

            return new Selection(new HashSet<>(selected));
        }

        @Override
        public Selection[] newArray(int size) {
            return new Selection[size];
        }
    };
}