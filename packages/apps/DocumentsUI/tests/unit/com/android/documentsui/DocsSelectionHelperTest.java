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

package com.android.documentsui;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.support.v7.widget.RecyclerView.Adapter;

import com.android.documentsui.DocsSelectionHelper.DelegateFactory;
import com.android.documentsui.selection.DefaultSelectionHelper;
import com.android.documentsui.selection.Selection;
import com.android.documentsui.selection.SelectionHelper;
import com.android.documentsui.selection.SelectionHelper.SelectionPredicate;
import com.android.documentsui.selection.SelectionHelper.StableIdProvider;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Tests for the specialized behaviors provided by DocsSelectionManager.
 */
@RunWith(AndroidJUnit4.class)
@SmallTest
public class DocsSelectionHelperTest {

    private DocsSelectionHelper mSelectionMgr;
    private List<TestSelectionManager> mCreated;
    private DelegateFactory mFactory;

    @Before
    public void setup() {
        mCreated = new ArrayList<>();
        mFactory = new DelegateFactory() {
            @Override
            TestSelectionManager create(
                    int mode,
                    Adapter<?> adapter,
                    StableIdProvider stableIds,
                    SelectionPredicate canSetState) {

                TestSelectionManager mgr = new TestSelectionManager();
                mCreated.add(mgr);
                return mgr;
            }
        };

        mSelectionMgr = new DocsSelectionHelper(mFactory, DefaultSelectionHelper.MODE_MULTIPLE);
    }

    @Test
    public void testCallableBeforeReset() {
        mSelectionMgr.hasSelection();
        assertNotNull(mSelectionMgr.getSelection());
        assertFalse(mSelectionMgr.isSelected("poodle"));
    }

    @Test
    public void testReset_CreatesNewInstances() {
        mSelectionMgr.reset(null, null, null);  // nulls are passed to factory. We ignore.
        mSelectionMgr.reset(null, null, null);  // nulls are passed to factory. We ignore.

        assertCreated(2);
    }

    @Test
    public void testReset_ClearsPreviousSelection() {
        mSelectionMgr.reset(null, null, null);  // nulls are passed to factory. We ignore.
        mSelectionMgr.reset(null, null, null);  // nulls are passed to factory. We ignore.

        mCreated.get(0).assertCleared(true);
        mCreated.get(1).assertCleared(false);
    }

    @Test
    public void testReplaceSelection() {
        mSelectionMgr.reset(null, null, null);  // nulls are passed to factory. We ignore.

        List<String> ids = new ArrayList<>();
        ids.add("poodles");
        ids.add("hammy");
        mSelectionMgr.replaceSelection(ids);
        mCreated.get(0).assertCleared(true);
        mCreated.get(0).assertSelected("poodles", "hammy");
    }

    void assertCreated(int count) {
        assertEquals(count, mCreated.size());
    }

    private static final class TestSelectionManager extends SelectionHelper {

        private boolean mCleared;
        private Map<String, Boolean> mSelected = new HashMap<>();

        void assertCleared(boolean expected) {
            assertEquals(expected, mCleared);
        }

        void assertSelected(String... expected) {
            for (String id : expected) {
                assertTrue(mSelected.containsKey(id));
                assertTrue(mSelected.get(id));
            }
            assertEquals(expected.length, mSelected.size());
        }

        @Override
        public void addObserver(SelectionObserver listener) {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean hasSelection() {
            throw new UnsupportedOperationException();
        }

        @Override
        public Selection getSelection() {
            throw new UnsupportedOperationException();
        }

        @Override
        public void copySelection(Selection dest) {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean isSelected(String id) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void restoreSelection(Selection other) {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean setItemsSelected(Iterable<String> ids, boolean selected) {
            for (String id : ids) {
                mSelected.put(id, selected);
            }
            return true;
        }

        @Override
        public void clearSelection() {
            mCleared = true;
        }

        @Override
        public boolean select(String itemId) {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean deselect(String itemId) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void startRange(int pos) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void extendRange(int pos) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void endRange() {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean isRangeActive() {
            throw new UnsupportedOperationException();
        }

        @Override
        public void anchorRange(int position) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void extendProvisionalRange(int pos) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void clearProvisionalSelection() {
            throw new UnsupportedOperationException();
        }

        @Override
        public void mergeProvisionalSelection() {
            throw new UnsupportedOperationException();
        }

        @Override
        public void setProvisionalSelection(Set<String> newSelection) {
            throw new UnsupportedOperationException();
        }
    }
}
