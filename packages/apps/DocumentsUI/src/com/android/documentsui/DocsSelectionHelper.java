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

package com.android.documentsui;

import android.support.annotation.VisibleForTesting;
import android.support.v7.widget.RecyclerView;

import com.android.documentsui.selection.DefaultSelectionHelper;
import com.android.documentsui.selection.DefaultSelectionHelper.SelectionMode;
import com.android.documentsui.selection.MutableSelection;
import com.android.documentsui.selection.Selection;
import com.android.documentsui.selection.SelectionHelper;

import java.util.Set;

import javax.annotation.Nullable;

/**
 * DocumentsUI SelectManager implementation that creates delegate instances
 * each time reset is called.
 */
public final class DocsSelectionHelper extends SelectionHelper {

    private final DelegateFactory mFactory;
    private final @SelectionMode int mSelectionMode;

    // initialize to a dummy object incase we get some input
    // event drive calls before we're properly initialized.
    // See: b/69306667.
    private SelectionHelper mDelegate = new DummySelectionHelper();

    @VisibleForTesting
    DocsSelectionHelper(DelegateFactory factory, @SelectionMode int mode) {
        mFactory = factory;
        mSelectionMode = mode;
    }

    public SelectionHelper reset(
            RecyclerView.Adapter<?> adapter,
            StableIdProvider stableIds,
            SelectionPredicate canSetState) {

        if (mDelegate != null) {
            mDelegate.clearSelection();
        }

        mDelegate = mFactory.create(mSelectionMode, adapter, stableIds, canSetState);
        return this;
    }

    @Override
    public void addObserver(SelectionObserver listener) {
        mDelegate.addObserver(listener);
    }

    @Override
    public boolean hasSelection() {
        return mDelegate.hasSelection();
    }

    @Override
    public Selection getSelection() {
        return mDelegate.getSelection();
    }

    @Override
    public void copySelection(Selection dest) {
        mDelegate.copySelection(dest);
    }

    @Override
    public boolean isSelected(String id) {
        return mDelegate.isSelected(id);
    }

    @VisibleForTesting
    public void replaceSelection(Iterable<String> ids) {
        mDelegate.clearSelection();
        mDelegate.setItemsSelected(ids, true);
    }

    @Override
    public void restoreSelection(Selection other) {
        mDelegate.restoreSelection(other);
    }

    @Override
    public boolean setItemsSelected(Iterable<String> ids, boolean selected) {
        return mDelegate.setItemsSelected(ids, selected);
    }

    @Override
    public void clearSelection() {
        mDelegate.clearSelection();
    }

    @Override
    public boolean select(String modelId) {
        return mDelegate.select(modelId);
    }

    @Override
    public boolean deselect(String modelId) {
        return mDelegate.deselect(modelId);
    }

    @Override
    public void startRange(int pos) {
        mDelegate.startRange(pos);
    }

    @Override
    public void extendRange(int pos) {
        mDelegate.extendRange(pos);
    }

    @Override
    public void extendProvisionalRange(int pos) {
        mDelegate.extendProvisionalRange(pos);
    }

    @Override
    public void clearProvisionalSelection() {
        mDelegate.clearProvisionalSelection();
    }

    @Override
    public void setProvisionalSelection(Set<String> newSelection) {
        mDelegate.setProvisionalSelection(newSelection);
    }

    @Override
    public void mergeProvisionalSelection() {
        mDelegate.mergeProvisionalSelection();
    }

    @Override
    public void endRange() {
        mDelegate.endRange();
    }

    @Override
    public boolean isRangeActive() {
        return mDelegate.isRangeActive();
    }

    @Override
    public void anchorRange(int position) {
        mDelegate.anchorRange(position);
    }

    public static DocsSelectionHelper createMultiSelect() {
        return new DocsSelectionHelper(
                DelegateFactory.INSTANCE,
                DefaultSelectionHelper.MODE_MULTIPLE);
    }

    public static DocsSelectionHelper createSingleSelect() {
        return new DocsSelectionHelper(
                DelegateFactory.INSTANCE,
                DefaultSelectionHelper.MODE_SINGLE);
    }

    /**
     * Use of a factory to create selection manager instances allows testable instances to
     * be inject from tests.
     */
    @VisibleForTesting
    static class DelegateFactory {
        static final DelegateFactory INSTANCE = new DelegateFactory();

        SelectionHelper create(
                @SelectionMode int mode,
                RecyclerView.Adapter<?> adapter,
                StableIdProvider stableIds,
                SelectionPredicate canSetState) {

            return new DefaultSelectionHelper(mode, adapter, stableIds, canSetState);
        }
    }

    /**
     * A dummy SelectHelper used by DocsSelectionHelper before a real
     * SelectionHelper has been initialized by DirectoryFragment.
     */
    private static final class DummySelectionHelper extends SelectionHelper {

        @Override
        public void addObserver(SelectionObserver listener) {
        }

        @Override
        public boolean hasSelection() {
            return false;
        }

        @Override
        public Selection getSelection() {
            return new MutableSelection();
        }

        @Override
        public void copySelection(Selection dest) {
        }

        @Override
        public boolean isSelected(String id) {
            return false;
        }

        @VisibleForTesting
        public void replaceSelection(Iterable<String> ids) {
        }

        @Override
        public void restoreSelection(Selection other) {
        }

        @Override
        public boolean setItemsSelected(Iterable<String> ids, boolean selected) {
            return false;
        }

        @Override
        public void clearSelection() {
        }

        @Override
        public boolean select(String modelId) {
            return false;
        }

        @Override
        public boolean deselect(String modelId) {
            return false;
        }

        @Override
        public void startRange(int pos) {
        }

        @Override
        public void extendRange(int pos) {
        }

        @Override
        public void extendProvisionalRange(int pos) {
        }

        @Override
        public void clearProvisionalSelection() {
        }

        @Override
        public void setProvisionalSelection(Set<String> newSelection) {
        }

        @Override
        public void mergeProvisionalSelection() {
        }

        @Override
        public void endRange() {
        }

        @Override
        public boolean isRangeActive() {
            return false;
        }

        @Override
        public void anchorRange(int position) {
        }
    }
}
